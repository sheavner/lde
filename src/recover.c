/*
 *  lde/recover.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: recover.c,v 1.8 1996/06/01 04:59:55 sdh Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <linux/fs.h>

#include "lde.h"
#include "recover.h"
#include "tty_lde.h"

/* This takes care of the mapping from a char pointer to unsigned long/short, depending
 * on the file system.
 */
unsigned long block_pointer(unsigned char *ind, unsigned long blknr, int zone_entry_size)
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

/* This is used to pull the correct block from the inode block table which
 * should have been copied into zone_index. 
 */
int map_block(unsigned long zone_index[], unsigned long blknr, unsigned long *mapped_block)
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
      block = zone_index[fsc->INDIRECT];
      if (block==0UL) {
	*mapped_block = ZONES_PER_BLOCK + fsc->N_DIRECT;
	return (EMB_IND_ZERO);
      } else if (block<sb->nzones) {
	ind = cache_read_block(block,CACHEABLE);
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
	ind = cache_read_block(block,CACHEABLE);
	block = block_pointer(ind,(unsigned long)(blknr/ZONES_PER_BLOCK),fsc->ZONE_ENTRY_SIZE);
	if (block==0UL) {
                                                         /* 2 here, 1 for indirect block, 1 for current 2xindirect */
	  *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(2+blknr/ZONES_PER_BLOCK);
	  return (EMB_2IND_L1_ZERO);
	} else if (block<sb->nzones) {
	  ind = cache_read_block(block,CACHEABLE);
	  *mapped_block = block_pointer(ind,(unsigned long)(blknr%ZONES_PER_BLOCK),fsc->ZONE_ENTRY_SIZE);
	  if (*mapped_block >= sb->nzones) {
	    *mapped_block = 0UL;
	    return (EMB_2IND_LOOKED_RANGE);
	  } else {
	    return (EMB_NO_ERROR);
	  }
	} else {
	  *mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(2+blknr/ZONES_PER_BLOCK);
	  return (EMB_2IND_L1_RANGE);
	}
      } else {
	*mapped_block = fsc->N_DIRECT + ZONES_PER_BLOCK*(ZONES_PER_BLOCK+1);
	return (EMB_2IND_RANGE);
      }
    }
    blknr -= (ZONES_PER_BLOCK*ZONES_PER_BLOCK);
  }

  /* Shouldn't we be able to do this recursively??  It can do indirect and 2x indirects, whats another
   * level of indirection? easy.
   */
  if (fsc->X3_INDIRECT) {
    *mapped_block = 0UL;
#   ifdef LDE_CURSES /* Don't wan't too many of these when not using curses */
        warn("Teach me how to handle triple indirect blocks :)");
#   endif
    return (EMB_3IND_NOT_YET);
  }

  return (EMB_WAY_OUT_OF_RANGE);
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
void recover_file(int fp,unsigned long zone_index[])
{
  unsigned char *dind;
  unsigned long nr;
  int j, result;
  size_t write_count;

  j = 0;
  while (1) {
    if ((result=map_block(zone_index,j,&nr))) {
      if (result < EMB_HALT) {
	break;
      } else if (result < EMB_SKIP ) {
	j = nr;
	continue;
      } else {
	nr = 0UL;
      }
    }

    dind = cache_read_block(nr,CACHEABLE);
    if (nr) {
      write_count = (size_t) lookup_blocksize(nr);
      /* warn("Setting write count block size %d",write_count); */
      if (write (fp, dind, write_count) != write_count) {
	warn("Write error: unable to write block (%ld) to recover file, recover aborted",nr);
	return;
      }
    }
    j++;
  }
}

/* Goes through and checks to see if the file can be recovered.  I.e. is another
 * file using blocks that were once used by this inode */
int check_recover_file(unsigned long zone_index[])
{
  unsigned long nr, lookedup=0UL;
  int j, result, cr_result = 0;
  
  j = 0;
  while (1) {
    if ((result=map_block(zone_index,j,&nr))) {
      if (result < EMB_HALT) {
	break;
      } else if (result < EMB_SKIP ) {
	j = nr;
	continue;
      } else {
	nr = 0UL;
      }
    }
    if ((nr)&&FS_cmd.zone_in_use(nr)) {
      warn("Block %ld (0x%lX) in use by another file. Hit any key to continue (q=abort, l=lookup inode).",nr,nr);
      result = mgetch();
      if (result == 'q') {
        warn("Check aborted.");
	return 1;
      }	else if (result == 'l') {
        while (1) {
	  warn("Searching for inode reference . . .");
          lookedup = find_inode(nr, lookedup);
	  if (lookedup == 0UL) {
	    warn("Can't find an inode referencing block %ld (0x%lX).  Hit any key to continue.",nr,nr);
	    (void) mgetch();
	    break;
	  } else {
	    warn("Block %ld (0x%lX) is used by inode %ld (0x%lX).  Hit any key to continue search ('q' to quit).",
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
    warn("Check complete, file looks recoverable.");
  } else {
    warn("Check complete.  Recovery may not be possible.");
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

  for (inode_nr=(last_nr+1UL);(inode_nr<sb->ninodes);inode_nr++) {
    if ((lde_flags.search_all)||(!FS_cmd.inode_in_use(inode_nr))) {
      GInode = FS_cmd.read_inode(inode_nr);
      b = 0UL;
      while (1) {
	if ((result=map_block(GInode->i_zone, b++, &test_block))) {
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
void search_fs(unsigned char *search_string, int search_len, int search_off)
{
  unsigned long nr, inode_nr;
  int i, j, no_match_flag, largest_match, match_count[5];
  int match_total = 0, matched;
  unsigned char *dind;
  unsigned char match[5];

  for (nr=sb->first_data_zone;nr<sb->nzones;nr++) {
    
    /* Do all the searches in the unused data space */
    if ((!FS_cmd.zone_in_use(nr))||(lde_flags.search_all)) {
      dind = cache_read_block(nr,CACHEABLE);

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
	  printf("Pattern match at start of block 0x%lX",nr);
	  if ( (lde_flags.inode_lookup)&&(inode_nr = find_inode(nr, 0UL)) )
	    printf(" check inode 0x%lX",inode_nr);
	  printf(".\n");
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
	for (i=3;i<BLOCK_SIZE/4;i+=fsc->ZONE_ENTRY_SIZE) {
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
	  for (i=2;i<BLOCK_SIZE/4;i+=fsc->ZONE_ENTRY_SIZE) {
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



