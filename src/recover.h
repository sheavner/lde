/*
 *  lde/recover.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: recover.h,v 1.11 2003/12/03 18:07:21 scottheavner Exp $
 */

unsigned long block_pointer(unsigned char *ind,
  unsigned long blknr,
  int zone_entry_size);
int map_block(unsigned long zone_index[],
  unsigned long blknr,
  unsigned long *mapped_block);
int advance_zone_pointer(unsigned long zone_index[],
  unsigned long *currblk,
  unsigned long *ipointer,
  long increment);
int recover_file(int fp, unsigned long zone_index[], unsigned long filesize);
int check_recover_file(unsigned long zone_index[], unsigned long);
unsigned long find_inode(unsigned long nr, unsigned long last_nr);
void parse_grep(void);
void search_fs(unsigned char *search_string,
  int search_len,
  int search_off,
  unsigned long start_nr);
int search_blocks(char *searchstring,
  unsigned long sbnr,
  unsigned long *mbnr,
  int *moffset);
int search_for_superblocks(int fs_type);

enum map_block_errors
{
  EMB_NO_ERROR = 0,
  EMB_3IND_NOT_YET =
    -200, /* Sorry, I haven't progammed this yet, stop checking when encountered */
  EMB_WAY_OUT_OF_RANGE, /* This blocknumber could not exist on this filesystem */
  EMB_HALT,             /* If you get an error below this, halt the program */
  EMB_IND_ZERO,  /* Indirect pointer itself is zero, move on to 2x indirect */
  EMB_IND_RANGE, /* Indirect pointer itself is out of range, 
						 * probably corrupt, move on to 2x indirect */
  EMB_2IND_ZERO, /* 2xIndirect pointer itself is zero, move on to 3x indirect */
  EMB_2IND_RANGE,    /* 2xIndirect pointer itself is out of range, 
						 * probably corrupt, move on to 2x indirect */
  EMB_2IND_L1_ZERO,  /* Entry in 2xIndirect block is zero 
						 * (entry doesn't point to an indirect block), 
						 * move to next entry in block. */
  EMB_2IND_L1_RANGE, /* Entry in 2xIndirect block is out of range,
						 * (entry doesn't point to an indirect block), 
						 * move to next entry in block. */
  EMB_3IND_ZERO,     /* 3xIndirect pointer itself is zero, stop */
  EMB_3IND_RANGE,    /* 3xIndirect pointer itself is out of range, 
						 * probably corrupt, move on to 4x? indirect */
  EMB_3IND_L1_ZERO,  /* Entry in 2xindirect block of 3xIndirect block is zero 
						 * (entry doesn't point to an indirect block), 
						 * move to next 2xindirect entry in block. */
  EMB_3IND_L2_ZERO,  /* Entry in indirect block of 3xIndirect block is zero 
						 * (entry doesn't point to an indirect block), 
						 * move to next indirect block entry in block. */
  EMB_3IND_L2_RANGE, /* Entry in indirect block of 3xIndirect block is out of 
						 * range (entry doesn't point to an indirect block), 
						 * move to next indirect block entry in block. */
  EMB_3IND_L1_RANGE, /* Entry in 2xindirect block of 3xIndirect block is out of 
						 * range (entry doesn't point to an indirect block), 
						 * move to next indirect block entry in block. */
  EMB_SKIP, /* Skip to number returned in mapped block if below this error */
  EMB_DIRECT_RANGE,     /* Block pointed to by direct pointer is out of range */
  EMB_IND_LOOKED_RANGE, /* Entry in indirect block is zero, move to next entry */
  EMB_2IND_LOOKED_RANGE, /* Block is zero, move to next entry */
  EMB_3IND_LOOKED_RANGE, /* Block is zero, move to next entry */

  AZP_BAD_START, /* Block is not indexed by current inode */
  AZP_UNCHANGED  /* Can't find next/prev block, current pointer unchanged */
};
