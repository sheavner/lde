/*
 *  lde/recover.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: recover.c,v 1.6 1994/09/06 01:29:51 sdh Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
unsigned long map_block(unsigned long zone_index[], unsigned long blknr)
{
  unsigned char *ind = NULL;
  unsigned long block, result;

  result = 0;

  if (blknr<fsc->N_DIRECT)
    return (zone_index[blknr] < sb->nzones) ? zone_index[blknr] : 0 ;
  blknr -= fsc->N_DIRECT;

  if (fsc->INDIRECT) {
    if (blknr<ZONES_PER_BLOCK) {
      block = zone_index[fsc->INDIRECT];
      if ((block>0)&&(block<sb->nzones)) {
	ind = cache_read_block(block,CACHEABLE);
	result = block_pointer(ind,blknr,fsc->ZONE_ENTRY_SIZE);
      }
      return result;
    }
    blknr -= ZONES_PER_BLOCK;
  }

  if (fsc->X2_INDIRECT) {
    if (blknr<(ZONES_PER_BLOCK*ZONES_PER_BLOCK)) {
      block = zone_index[fsc->X2_INDIRECT];
      if ((block>0)&&(block<sb->nzones)) {
	ind = cache_read_block(block,CACHEABLE);
	block = block_pointer(ind,(unsigned long)(blknr/ZONES_PER_BLOCK),fsc->ZONE_ENTRY_SIZE);
	if ((block>0)&&(block<sb->nzones)) {
	  ind = cache_read_block(block,CACHEABLE);
	  result = block_pointer(ind,(unsigned long)(blknr%ZONES_PER_BLOCK),fsc->ZONE_ENTRY_SIZE);
	}
	return result;
      }
    }
    blknr -= (ZONES_PER_BLOCK*ZONES_PER_BLOCK);
  }

  if (fsc->X3_INDIRECT)
    warn("Teach me how to handle triple indirect blocks :)");

  return 0;
}

/* This is the non-magic undelete.  inode will contain a 
 * possibly bogus inode, only the blocks are looked at --
 * we copy all the blocks (including those indexed indirectly)
 * to the file specified in fp
 */
void recover_file(int fp,unsigned long zone_index[])
{
  unsigned char *dind;
  unsigned long nr;
  int j;

  j = 0;
  while ((nr=map_block(zone_index,j))) {
    dind = cache_read_block(nr,CACHEABLE);
    write(fp, dind, sb->blocksize);
    j++;
  }
}

unsigned long find_inode(unsigned long nr)
{
  unsigned long inode_nr, b, test_block;
  struct Generic_Inode *GInode=NULL;

  for (inode_nr=1;inode_nr<sb->ninodes;inode_nr++) {
    if ((!FS_cmd.inode_in_use(inode_nr))||(rec_flags.search_all)) {
      GInode = FS_cmd.read_inode(inode_nr);
      b = 0;
      while ( (b<fsc->N_BLOCKS)&&(test_block=map_block(GInode->i_zone, b++)) ) {
	if (nr==test_block) return inode_nr;
      }
    }
  }
  return 0;
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
	inode_nr = find_inode(blknr);
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
    
#ifdef ALPHA_CODE
/* Tweakable parameters */
#define HB_COUNT  100
#define SEQ_COUNT 10

/* Someday this will form the basis for a search routine, it's
 * not too exciting now, but it works.  Of course grepping the
 * partition is probably faster.
 */
void search_fs(unsigned char *search_string, int search_len)
{
  unsigned long nr, inode_nr;
  int i, j, no_match_flag, largest_match, match_count[5];
  int match_total = 0;
  unsigned char *dind;
  unsigned char match[5];

  for (nr=sb->first_data_zone;nr<sb->nzones;nr++) {
    
    /* Do all the searches in the unused data space */
    if ((!FS_cmd.zone_in_use(nr))||(rec_flags.search_all)) {
      dind = cache_read_block(nr,CACHEABLE);

      /* Search codes for a gzipped tar file --
       * see /etc/magic or make a similar file and read off the first
       * few bytes 
       * gzipped tar (made with tar cvfz ): 31 138 8 0
       * gzip (any): 31 139 ( 0x1f 0x8c )
       * scripts ("#!/b"): 35 33 47 98
       */
      i = 0;
      do {
	if (dind[i]!=search_string[i]) break;
      } while (++i<search_len);

      if (i==search_len) {
	printf("Pattern match at start of block 0x%lX",nr);
	if ( (inode_nr = find_inode(nr)) )
	  printf(" check inode 0x%lX",inode_nr);
	printf(".\n");
      }

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
  printf("Total indirect blocks %d.\n",match_total);

}
#endif



