/*
 *  lde/recover.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: recover.c,v 1.33 2002/01/11 18:30:41 scottheavner Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <string.h>

#ifdef   HAVE_UNAME
#include <sys/utsname.h>
#endif

#include "lde.h"
#include "recover.h"
#include "tty_lde.h"
#include "allfs.h"

/* This takes care of the mapping from a char pointer to unsigned
 * long/short, depending on the file system */
unsigned long block_pointer(unsigned char *ind, 
			    unsigned long blknr, int zone_entry_size)
{
  unsigned long  *lind;
  unsigned short *sind;

  if (zone_entry_size == 2) {
    sind = (unsigned short *) ind;
    return sind[blknr];
  } else {
    lind = (unsigned long *) ind;
    return lind[blknr];
  }
}

#ifdef BETA_CODE
/* Try to work around Linux <= 2.0.33 bug */ 
/* This is used to pull the correct block from the inode block table which
 * should have been copied into zone_index */
static int hacked_map_block_helper(unsigned long zone_index[],
				   unsigned long blknr,
				   unsigned long *mapped_block)
{
  long last_direct = 0, i, offset = 0;

  *mapped_block = 0UL;
  offset = blknr;

  if (fsc->N_DIRECT) {
    /* Direct blocks: Same as map_block() */
    if (blknr<fsc->N_DIRECT) {
      *mapped_block = zone_index[blknr];
      if ( zone_index[blknr] < sb->nzones ) {
	return (EMB_NO_ERROR);
      } else {
	return (EMB_DIRECT_RANGE);
      }
    }
    offset -= fsc->N_DIRECT;
    blknr  -= (fsc->N_DIRECT-1); /* This is the only place I subtract from 
				  * blknr: want to reference from last used
				  * direct block */
  } else {
    /* If no direct block we're hosed */
    return (EMB_WAY_OUT_OF_RANGE);
  }    
  
  /* Need to find last used direct block */
  for (i=0; i<fsc->N_DIRECT; i++)
    if (zone_index[i])
      last_direct = zone_index[i];
    
  /* If no used directs, we're really screwed */
  if (last_direct) {
    if (fsc->INDIRECT) {
      blknr++;       /* Skip data for indirect block */   
      if (offset<ZONES_PER_BLOCK) {
	/* Skip indirect block contents */
	*mapped_block = last_direct + blknr;
	return EMB_NO_ERROR;
      }
      offset -= ZONES_PER_BLOCK;
    }
    if (fsc->X2_INDIRECT) {
      blknr++;       /* Skip data for 2x indirect block */
      if (offset<(ZONES_PER_BLOCK*ZONES_PER_BLOCK)) {
	/* Already skipped 2x, but need to adjust for any 1x's we're past, 
	 * offset starts at 0, so always add 1 to offset/ZONES_PER_BLOCK */
	*mapped_block = last_direct + blknr + (offset/ZONES_PER_BLOCK+1);
	return EMB_NO_ERROR;
      }
      offset -= ZONES_PER_BLOCK*ZONES_PER_BLOCK;
      blknr  += ZONES_PER_BLOCK;   /* There were ZONES_PER_BLOCK indirect index blocks we skipped over */ 
    }
    if (fsc->X3_INDIRECT) {
      blknr++;       /* Skip data for 3x indirect block */
      if (offset<(ZONES_PER_BLOCK*ZONES_PER_BLOCK*ZONES_PER_BLOCK)) {
	/* Need to multiple 2x indirect and any 1x's we're passed */
	*mapped_block = last_direct + blknr +
	  (offset/ZONES_PER_BLOCK/ZONES_PER_BLOCK+1) + /* num 2x's */
	  (offset/ZONES_PER_BLOCK+1);                 /* num 1x's */
	return EMB_NO_ERROR;
      }
      offset -= ZONES_PER_BLOCK*ZONES_PER_BLOCK*ZONES_PER_BLOCK; 
      blknr  += ZONES_PER_BLOCK*ZONES_PER_BLOCK + ZONES_PER_BLOCK; 
    }
  }

  /* Nothing */
  return (EMB_WAY_OUT_OF_RANGE);
}

static int hacked_map_block(unsigned long zone_index[],
			    unsigned long blknr,
			    unsigned long *mapped_block, 
			    unsigned long *skipped_block)
{
  int result;

  /* Lookup block, return any error */
  result = hacked_map_block_helper(zone_index,blknr,mapped_block);
  if (result!=EMB_NO_ERROR)
    return result;

  *mapped_block += *skipped_block;

  /* Check for system/used blocks and skip 
   * (system blocks will be marked used, duh!
   *  FS_cmd.is_system_block seems like a waste) */
  while ( ((!lde_flags.search_all)&&FS_cmd.zone_in_use(*mapped_block)) ||
	  FS_cmd.is_system_block(*mapped_block) ) {
    (*skipped_block)++;
    if ( ++(*mapped_block) > sb->nzones)
      return (EMB_WAY_OUT_OF_RANGE);
  }

  return EMB_NO_ERROR;
}
#endif

/* This is used to pull the correct block from the inode block table which
 * should have been copied into zone_index */
int map_block(unsigned long zone_index[], unsigned long blknr,
	      unsigned long *mapped_block)
{
  unsigned char *ind = NULL;
  unsigned long block;

  /* Direct blocks */
  if (fsc->N_DIRECT) {
    if (blknr<fsc->N_DIRECT) {
      *mapped_block = zone_index[blknr];
      if ( zone_index[blknr] < sb->nzones ) {
	return (EMB_NO_ERROR);
      } else {
	*mapped_block = 0UL;
	return (EMB_DIRECT_RANGE);
      }
    }

    blknr -= fsc->N_DIRECT;
  }

  /* Ok, it wasn't a direct block.  Move on to indirect block(s) */
  if (fsc->INDIRECT) {
    if (blknr<ZONES_PER_BLOCK) {
      /* Check value for indirect block */
      block = zone_index[fsc->INDIRECT];
      if (block==0UL) {
	*mapped_block = ZONES_PER_BLOCK + fsc->N_DIRECT;
	return (EMB_IND_ZERO);
      } else if (block<sb->nzones) {
	/* Indirect block is in fs range, now read it in */
	ind = cache_read_block(block,NULL,CACHEABLE);
	*mapped_block = block_pointer(ind,blknr,fsc->ZONE_ENTRY_SIZE);
	if (*mapped_block >= sb->nzones) {
	  return (EMB_IND_LOOKED_RANGE);
	} else {
	  return (EMB_NO_ERROR);
	}
      } else {
	*mapped_block = ZONES_PER_BLOCK + fsc->N_DIRECT;
	return (EMB_IND_RANGE);
      }
    }
    blknr -= ZONES_PER_BLOCK;
  }

  /* Ok, maybe it's in the double indirect blocks */
  if (fsc->X2_INDIRECT) {
    if (blknr<(ZONES_PER_BLOCK*ZONES_PER_BLOCK)) {
      block = zone_index[fsc->X2_INDIRECT];
      if (block==0UL) {
	*mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(ZONES_PER_BLOCK+1);
	return (EMB_2IND_ZERO);
      } else if (block<sb->nzones) {
	ind = cache_read_block(block,NULL,CACHEABLE);
	block = block_pointer(ind,(unsigned long)(blknr/ZONES_PER_BLOCK),
			      fsc->ZONE_ENTRY_SIZE);
	if (block==0UL) {
                                                         /* 2 here, 1 for indirect block, 1 for current 2xindirect */
	  *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(2+blknr/ZONES_PER_BLOCK); 
	  /* *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*2 + blknr; */
	  return (EMB_2IND_L1_ZERO);
	} else if (block<sb->nzones) {
	  ind = cache_read_block(block,NULL,CACHEABLE);
	  *mapped_block = 
	    block_pointer(ind,(unsigned long)
			  (blknr%ZONES_PER_BLOCK),fsc->ZONE_ENTRY_SIZE);
	  if (*mapped_block >= sb->nzones) {
	    *mapped_block = 0UL;
	    return (EMB_2IND_LOOKED_RANGE);
	  } else {
	    return (EMB_NO_ERROR);
	  }
	} else {
	  *mapped_block = fsc->N_DIRECT +
	    ZONES_PER_BLOCK*(2+blknr/ZONES_PER_BLOCK);
	  /* *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*2 + blknr; */
	  return (EMB_2IND_L1_RANGE);
	}
      } else {
	*mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(ZONES_PER_BLOCK+1);
	return (EMB_2IND_RANGE);
      }
    }
    blknr -= (ZONES_PER_BLOCK*ZONES_PER_BLOCK);
  }

  /* Shouldn't we be able to do this recursively??  It can do indirect 
   * and 2x indirects, whats another level of indirection? easy.
   */
  if (fsc->X3_INDIRECT) {
    if (blknr<(ZONES_PER_BLOCK*ZONES_PER_BLOCK*ZONES_PER_BLOCK)) {
      block = zone_index[fsc->X3_INDIRECT];
      if (block==0UL) {
	*mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(ZONES_PER_BLOCK+1) +
	  ZONES_PER_BLOCK*ZONES_PER_BLOCK*ZONES_PER_BLOCK;
	return (EMB_3IND_ZERO);
#ifdef BETA_CODE
      } else if (block<sb->nzones) {
	/* Read in the triple indirect block */
	ind = cache_read_block(block,NULL,CACHEABLE);
	block = block_pointer(ind,(unsigned long)
			      (blknr/(ZONES_PER_BLOCK*ZONES_PER_BLOCK)),
			      fsc->ZONE_ENTRY_SIZE);
	/* Block is now a pointer to a 2x indirect */
	if (block==0UL) {
	  /* Add extra 2x indirect block to *mapped block */
	  *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK +  
	    ZONES_PER_BLOCK*
	    ZONES_PER_BLOCK*(2 + blknr/(ZONES_PER_BLOCK*ZONES_PER_BLOCK));
	  return (EMB_3IND_L1_ZERO);
	} else if (block<sb->nzones) {
	  /* Read in a 2x indirect */
	  ind = cache_read_block(block,NULL,CACHEABLE);
	  block = block_pointer(ind,  (unsigned long)
				((blknr%(ZONES_PER_BLOCK*ZONES_PER_BLOCK))/ZONES_PER_BLOCK),
				fsc->ZONE_ENTRY_SIZE);
	  /* block now points to a 1x indirect */
	  if (block==0UL) {
	    /* Add extra indirect block to *mapped block */
	    *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(2 + blknr/ZONES_PER_BLOCK) +  
	      ZONES_PER_BLOCK*ZONES_PER_BLOCK;
	    return (EMB_3IND_L2_ZERO);
	  } else if (block<sb->nzones) {
	    /* Read in 1x indirect */
	    ind = cache_read_block(block,NULL,CACHEABLE);
	    *mapped_block = block_pointer(ind,(unsigned long)
					  ((blknr%(ZONES_PER_BLOCK*ZONES_PER_BLOCK))%ZONES_PER_BLOCK),
					  fsc->ZONE_ENTRY_SIZE);
	    /* *mapped block now points to the block we want */
	    if (*mapped_block >= sb->nzones) {
	      *mapped_block = 0UL;
	      return (EMB_3IND_LOOKED_RANGE);
	    } else {
	      return (EMB_NO_ERROR);
	    }
	  } else {
	    *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(2 + blknr/ZONES_PER_BLOCK) +  
	      ZONES_PER_BLOCK*ZONES_PER_BLOCK;
	    return (EMB_3IND_L2_RANGE);
	  }
	  
	} else {
	  *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK +  
	    ZONES_PER_BLOCK*ZONES_PER_BLOCK*(2 + blknr/(ZONES_PER_BLOCK*ZONES_PER_BLOCK));
	  return (EMB_3IND_L1_RANGE);
	}
	
      } else {
	*mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(ZONES_PER_BLOCK+1) +
	  ZONES_PER_BLOCK*ZONES_PER_BLOCK*ZONES_PER_BLOCK;
	return (EMB_3IND_RANGE);
#else /* BETA CODE undef */
      } else {
	static char warn_once = 0;         /* Only warn once about not using 3x indirects */
        *mapped_block = 0UL;
        if (!warn_once) {
          lde_warn("Triple indirects are ignored: Recompile with -DBETA_CODE to handle triple indirect blocks");
          warn_once = 1;
	}
#endif
      }
    }
    blknr -= (ZONES_PER_BLOCK*ZONES_PER_BLOCK*ZONES_PER_BLOCK);
  }

  return (EMB_WAY_OUT_OF_RANGE);

}

int advance_zone_pointer(unsigned long zone_index[], unsigned long *currblk, 
		     unsigned long *ipointer, long increment)
{
  int result;
  unsigned long blknr = 0UL, local_ipointer=0UL;
  long sincrement,i;

  /* Check that we are at a block that is indexed by this inode */
  if ((FS_cmd.map_block(zone_index, *ipointer, &blknr))||(blknr!=(*currblk))) {
    /* We aren't: look it up using brute force */
    for (;;) {
      result = FS_cmd.map_block(zone_index, local_ipointer, &blknr);
      if (result < EMB_HALT) {
	return AZP_BAD_START;
      } else if (result < EMB_SKIP) {
	local_ipointer = blknr;
      } else if ((result==EMB_NO_ERROR) && (blknr==(*currblk))) {
	break;
      } else {
	local_ipointer++;
      }
    }
  } else {
    local_ipointer = *ipointer;
  }

  sincrement = (increment < 0)?-1:1;
  increment *= sincrement;

  /* Now adjust the pointer and find us a block */
  for (i=0; i<increment; i++) {
    for (;;) {
      local_ipointer += sincrement;
      result = FS_cmd.map_block(zone_index, local_ipointer, &blknr);
      if (result < EMB_HALT) {
	return result;
      } else if (result < EMB_SKIP) {
	local_ipointer = blknr - sincrement;
	continue;
      } else if (result==EMB_NO_ERROR) {
	break;
      }
    }
  }

  if (blknr) {
    *ipointer = local_ipointer;
    *currblk = blknr;
    return 0;
  }

  return AZP_UNCHANGED;
}

/* This is the non-magic undelete.  inode will contain a 
 * possibly bogus inode, only the blocks are looked at --
 * we copy all the blocks (including those indexed indirectly)
 * to the file specified in fp.  Usually, a zero block number
 * in the inode block map would signal then end of the chain,
 * but for editing files, it might be nice to dump block 0
 * sometimes.  Therefore, if the first entry of the inode block
 * map is zero, I will dump block zero, in any other position,
 * it signals the end of the map.
 */
int recover_file(int fp,unsigned long zone_index[],unsigned long filesize)
{
  unsigned char *dind;
  unsigned long nr, written=0UL, skipped=0UL;
  int j, result;
  size_t write_count;

  j = 0;
  lde_flags.quit_now = 0;

  /* Lookup system version, warn about linux 2.0.33 */
#ifdef HAVE_UNAME /* Why bother with a HAVE_UNAME check,
                   *  I don't check for Linux name, just version */
  if (!lde_flags.blanked_indirects) {
    struct utsname nm;
    uname(&nm);
    if ( (nm.release[0]=='2')&&(nm.release[2]=='0') ) {
      lde_warn("Linux 2.0.* users should activate the blanked_indirects"
	       " flag for proper recovery!  See the manual!");
      sleep(5);
    }
  }
#endif

  while (1) {

    /* User has pressed ctrl-c, abort recover */
    if (lde_flags.quit_now) {
      lde_warn("User aborted recovery, partial data may have been written to file.");
      return -2;
    }

    /* Lookup block number from inode */
#ifdef BETA_CODE
    if (lde_flags.blanked_indirects) 
      result = hacked_map_block(zone_index,j,&nr,&skipped);
    else 
      result=FS_cmd.map_block(zone_index,j,&nr);
#else
    result=FS_cmd.map_block(zone_index,j,&nr);
#endif

    /* Block successfully looked up? */
    if (result) {
      if (result < EMB_HALT) {
	return 0;  /* we hit last entry in this inode, return success */
      } else if (result < EMB_SKIP ) {
	j = nr;    /* block has zero entry, go back and get another one */
	continue;
      } else {
	nr = 0UL;  /* Don't do any processing on this block, but fall 
		    * through to increment j */
      }
    }

    /* Have a valid block number? */
    if (nr) {
      int k = 0;
#ifdef MSDOSHACK /* Also added k in line above and below */
#warning MSDOS HACK ENABLED
      for ( k=0; k<sb->zonesize; k++) { /* MSDOS HACK */
#endif
      /* Read data from disk */
      dind = cache_read_block(nr+k,NULL,CACHEABLE);
      /* Check for small block if we opened a file instead of a device */
      write_count = (size_t) lookup_blocksize(nr);
      /* Add what we're about to write to total written */
      written += write_count;
      /* If gone to far, don't want to write out all this data */
      if ((filesize)&&(written>filesize))
	write_count -= written - filesize;
      /* Write out data to recovery file */
      if (write (fp, dind, write_count) != write_count) {
	lde_warn("Write error: unable to write block (%ld) to recover file, recover aborted",nr);
	return -1;
      }
      /* Quit when we've written enough */
      if ((filesize)&&(written>=filesize))
	return 0;
#ifdef MSDOSHACK
      } /* END MSDOS HACK */
#endif
    }
    j++;
  }

  return 0;
}

/* Goes through and checks to see if the file can be recovered.  I.e. is another
 * file using blocks that were once used by this inode */
int check_recover_file(unsigned long zone_index[],unsigned long filesize)
{
  unsigned long nr, lookedup=0UL, checked=0UL;
  int j, result, cr_result = 0;
  j = 0;
  lde_flags.quit_now = 0;

  while (1) {
    /* Stop if we specified a filesize and have covered that many blocks */
    if ((filesize)&&(checked>filesize))
      break;
    
    /* Terminate search on ^C */
    if (lde_flags.quit_now) {
      lde_flags.quit_now = 0;
      lde_warn("Search terminated.");
      return 1;
    }

    result=FS_cmd.map_block(zone_index,j,&nr);

    if (result) {
      if (result < EMB_HALT) {
	break;
      } else if (result < EMB_SKIP ) {
	j = nr;
	continue;
      } else {
	nr = 0UL;
      }
    } else {
      checked += lookup_blocksize(nr);
    }

    if ((nr)&&FS_cmd.zone_in_use(nr)) {
      lde_warn("Block %ld (0x%lX) in use by another file. Hit any key to continue (q=abort, l=lookup inode).",nr,nr);
      result = mgetch();
      if (result == 'q') {
        lde_warn("Check aborted.");
	return 1;
      }	else if (result == 'l') {
        while (1) {
	  lde_warn("Searching for inode reference . . .");
          lookedup = find_inode(nr, lookedup);
	  if (lookedup == 0UL) {
	    lde_warn("Can't find an inode referencing block %ld (0x%lX).  Hit any key to continue.",nr,nr);
	    (void) mgetch();
	    break;
	  } else {
	    lde_warn("Block %ld (0x%lX) is used by inode %ld (0x%lX).  Hit any key to continue search ('q' to quit).",
		 nr,nr,lookedup,lookedup);
	    if (tolower(mgetch())=='q') {
	      break;
	    }
	  }
	}
      }
      cr_result = 1;
    }
    j++;
  }
  
  if (!cr_result) {
    lde_warn("Check complete, file looks recoverable.");
  } else {
    lde_warn("Check complete.  Recovery may not be possible.");
  }
  
  return cr_result;
}


/* Search through all the inodes for one which references the specified block,
 * if search_all is not set, it will only search UNused inodes.
 */
unsigned long find_inode(unsigned long nr, unsigned long last_nr)
{
  unsigned long inode_nr, b, test_block;
  struct Generic_Inode *GInode=NULL;
  int result;

  lde_flags.quit_now = 0;

  for (inode_nr=(last_nr+1UL);(inode_nr<sb->ninodes);inode_nr++) {
    if ((lde_flags.search_all)||(!FS_cmd.inode_in_use(inode_nr))) {
      GInode = FS_cmd.read_inode(inode_nr);
      b = 0UL;
      while (1) {
	if (lde_flags.quit_now)
	  return 0L;
	if ((result=FS_cmd.map_block(GInode->i_zone, b++, &test_block))) {
	  if (result < EMB_HALT ) {
	    break;
	  } else if (result < EMB_SKIP ) {
	    b = test_block;
	  }
	} else {
	  if (nr==test_block) return inode_nr;
	}
      }
    }
  }

  return 0UL;
}

/* Parses output from grep to locate inode which may still
 * be intact.
 */
void parse_grep(void)
{
  unsigned long inode_nr, blknr;
  int i, miss_count=0;

  while ( (i=scanf("%ld\n",&blknr))!=EOF ) {
    if (i) {
      miss_count = 0;
      if (blknr) {
	blknr = blknr / sb->blocksize + 1;
	inode_nr = find_inode(blknr, 0UL);
	if (inode_nr) {
	  printf("Block 0x%lX indexed by inode 0x%lX",blknr,inode_nr);
	  if (FS_cmd.inode_in_use(inode_nr))
	    printf(" (This inode is marked in use)");
	  if (FS_cmd.zone_in_use(blknr))
	    printf(" (This zone is marked in use)");
	  printf("\n");
	}
	else
	  printf("Block 0x%lX is not referenced by any inode.\n",blknr);
      }
    }
    else /* EOF doesn't seem to work */
      if (++miss_count>100) return;
  }
}

/*=== EXPERIMENTAL SEARCH CODE ===*/
/* - Don't just tell me it sucks, send me new code to replace it ;) */    
/* Tweakable parameters */
#define HB_COUNT  100
#define SEQ_COUNT 10

/* Someday this will form the basis for a search routine, it's
 * not too exciting now, but it works.  Of course grepping the
 * partition is probably faster.
 */
void search_fs(unsigned char *search_string, int search_len, int search_off, unsigned long start_nr)
{
  unsigned long nr, inode_nr;
  int i, j, no_match_flag, largest_match, match_count[5];
  int match_total = 0, matched;
  unsigned char *dind;
  unsigned char match[5];
  struct Generic_Inode *GInode;

  lde_flags.quit_now = 0;

  /* Where should we start the search? */
  if (start_nr!=0UL) {
    lde_warn("Resuming search from block 0x%lX",start_nr);
    nr = start_nr;
  } else {
    nr=sb->first_data_zone;
  }

  for (/*nr set above*/;nr<sb->nzones;nr++) {

    if (lde_flags.quit_now) {
      lde_warn("Search aborted at block 0x%lX",nr);
      break;
    }

    /* Do all the searches in the unused data space */
    if ((!FS_cmd.zone_in_use(nr))||(lde_flags.search_all)) {
      dind = cache_read_block(nr,NULL,CACHEABLE);

      /* Search codes --
       * see /etc/magic or make a similar file 
       *     and read off the first few bytes 
       * gzipped tar (made with tar cvfz ): 31 138 8 0
       * gzip (any): 31 139 ( 0x1f 0x8c )
       * scripts ("#!/b"): 35 33 47 98
       */
      if (search_len) {
	i = 0;
	matched = 1;
	do {
	  if (dind[i+search_off]!=search_string[i]) {
	    matched = 0;
	    break;
	  }
	} while (++i<search_len);
	
	if (matched) {
	  fprintf(stderr,"Match at block 0x%lX",nr);
	  if (lde_flags.inode_lookup) {
	    if ( (inode_nr = find_inode(nr, 0UL)) ) {
	      fprintf(stderr,", check inode 0x%lX",inode_nr);
	      if (lde_flags.check_recover) {
		lde_warn = no_warn;  /* Suppress output */
		GInode = FS_cmd.read_inode(nr);
		fprintf(stderr,", recovery %spossible",(check_recover_file(GInode->i_zone,GInode->i_size)?"":"NOT ") );
		lde_warn = tty_warn; /* Reinstate output */
	      }
	    } else {
	      fprintf(stderr,", no %sinode found",((lde_flags.search_all)?"":"unused ") );
	    }
	  }
	  fprintf(stderr,".\n");
	}
      }

      if (lde_flags.indirect_search) {
	/* Search for something which might be an indirect block */

	/* This method spits out only indirect blocks on my system, but
	 * it doesn't get very many of them.  Somehow, all the zeroes at
	 * the end of a small indirect block have to be accounted for, but
	 * we don't want to look at every small file and think it is an 
	 * indirect block.
	 */
	/* First check the high bytes: we expect a lot of them to be in
	 * the same area.  This next block of code counts the number of 
	 * occurances of the first 5 distinct high bytes in the first
	 * quarter of the block.
	 */
	match_count[0] = match_count[1] = match_count[2] = match_count[3] =
	  match_count[4] = largest_match = 0;
	match[0] = dind[1];
	for (i=3;i<sb->blocksize/4;i+=fsc->ZONE_ENTRY_SIZE) {
	  no_match_flag = 1;
	  for (j=-1;j++<largest_match;)
	    if (dind[i]==match[j]) {
	      no_match_flag = 0;
	      match_count[j]++;
	    }
	  if (no_match_flag)
	    if (largest_match<4) {
	      largest_match++;
	      match[largest_match] = dind[i];
	    }
	  
	}
	
	i = 0;
	for (j=0;j<largest_match;j++)
	  i+=match_count[j];
	
	/* A perfect match is 128, so maybe 100 is a little outrageous */
	if (i>HB_COUNT) {
	  /* The next block of code will search the low bytes for sequences.
	   * Hopefully, a fair number of the low bytes will be in order.
	   */ 
	  match[0] = dind[0]; /* number in sequence 0,1,2,etc */
	  match[1] = 0;       /* Length of current sequence */
	  match[2] = 0;       /* Number of sequences longer than 3  */
	  for (i=2;i<sb->blocksize/4;i+=fsc->ZONE_ENTRY_SIZE) {
	    if (dind[i]==(++match[0]))
	      match[1]++;
	    else {
	      if (match[1]>3) match[2]+=match[1];
	      match[1] = 0;
	      match[0] = dind[i];
	    }
	  }
	  
	  if (match[2]>SEQ_COUNT) {
	    fprintf(stderr,"%d hits on possible indirect block at 0x%lX\n",
		    match[2],nr);
	    match_total++;
	  }
	}      
      }

    }
  }

  if (lde_flags.indirect_search)
    printf("Total indirect blocks %d.\n",match_total);

}

/* Do a regular expression search across the filesystem */
int search_blocks(char *searchstring, unsigned long sbnr, unsigned long *mbnr, int *moffset) {
  int     retval=0;

#if HAVE_MEMMEM
  int     rs = 500;
  size_t  bytesread, rbs = 0, lsearchstring;
  char    *buffer = 0, *match;

  if ( (!searchstring) || (!*searchstring) ) {
    lde_warn("NULL searchstring ignored");
    return -1;
  }
  
  if (rs>sb->nzones) {
    rs = sb->nzones - sbnr;
  }
  
  /* Try to read 501 blocks at a time */
  buffer = malloc((rs+1)*sb->blocksize);
  if (!buffer) {
    /* too ambitious? */
    rs = 100;
    buffer = malloc((rs+1)*sb->blocksize);
  }
  if (!buffer) {
    /* still too ambitious? */
    rs = 1;
    buffer = malloc((rs+1)*sb->blocksize);
  }
  if (!buffer) {
    lde_warn("Out of memory, can't complete search");
    return -1;
  }
  
  rbs = (rs+1)*sb->blocksize;
  lsearchstring=strlen(searchstring);
  
  for ( ; sbnr<sb->nzones; sbnr+=rs) {
    bytesread = nocache_read_block(sbnr, buffer, rbs);
    if (bytesread < 0) {
      retval = -1;
      break;
    }
    if ( (match = memmem(buffer, bytesread, searchstring, lsearchstring)) ) {
      *mbnr    = ((int)(match - buffer))/sb->blocksize + sbnr;
      *moffset = ((int)(match - buffer))%sb->blocksize;
      lde_warn("matches block=0x%lx offset=0x%lx",*mbnr,*moffset+sbnr*sb->blocksize);
      retval   = 1;
      break;
    }
    if (lde_flags.quit_now) {
      lde_warn("Search aborted at block 0x%lx",sbnr);
      retval = -2;
      break;
    }
  }
  
  free(buffer);
#else
#warning No memmem() found, block search disabled.
  lde_warn("Block search needs memmem, try recompiling.");
#endif

  if (retval==0)
     lde_warn("No match");
  
  return retval;
}


int search_for_superblocks(int fs_type) {
  unsigned long sbnr = 0;
  int i;
  char *buffer[512*500];
  size_t  bytesread;
 
  /* Want to do all searches on 512 byte boundries, in case something has gotten screwed up */
  NOFS_init(NULL,512);

  if ( fs_type >= LAST_FSTYPE || fs_type == NONE || fs_type < AUTODETECT ) {
    lde_warn("Bad filetype specified (i.e. why would \"no\" have a superblock) . . . Aborting.");
    return -1;
  }

  lde_warn("Searching disk for %s superblocks . . .",(fs_type==AUTODETECT)?"":lde_typedata[fs_type].name);

  for ( ; sbnr<sb->nzones; ++sbnr) {
    bytesread = nocache_read_block(sbnr, buffer, 512);
    if (bytesread < 0)
      break;

    if ( fs_type == AUTODETECT ) {
      for ( i = NONE+1 ; i<LAST_FSTYPE; i++) {
	if (lde_typedata[i].test(buffer,0)) {
	  lde_warn("Found %s superblock at 0x%lx",lde_typedata[i],sbnr);
	}
      }
    } else {
      if (lde_typedata[fs_type].test(buffer,0)) {
	lde_warn("Found %s superblock at 0x%lx",lde_typedata[fs_type],sbnr);
      }
    }
    
    if (lde_flags.quit_now) {
      lde_warn("Search aborted");
      return -1;
    }
  }

  lde_warn("Search complete");
  return 0;
}
