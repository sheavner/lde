#include <linux/ext2_fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/* (C) 1994 Scott Heavner (sdh@po.cwru.edu) */

/* find_ext2_fs.c -- greps a full disk devices for remnants of an ext2 partition.
 *                   Will be most useful to recover from a bad fdisk experience.
 *
 * Usage: find_ext2_fs --best-guess /dev/name
 *        find_ext2_fs /dev/name
 *        find_ext2_fs                           -- defaults to /dev/hda
 *
 * If it is called with 3 parameters as shown above (--best-guess can be anything),
 * it will only display one entry for the most likely filesystem cantidates.
 * Otherwise, it will print out any blocks which contain EXT2_MAGIC, which may or
 * may not be superblocks.  Also, there are many copies of the super block on 
 * each filesystem, they will all yield the same size, but will have different
 * starts and ends.  It may be necessary to leave out the best guess option if
 * nothing shows up on the first run.
 *
 * Changes:
 *
 * October 6, 1997 - modified to search 512 byte blocks instead of 1024.  If
 *   the filesystem didn't start on a 1024 byte interval, it would have been
 *   skipped.
 * - comment out EXT2_PRE_02B_MAGIC - must not be supported in kernels anymore
 * - compile with "gcc -O6 -s -o find_ext2_fs find_ext2_fs.c" for a 4k binary
 * - why is this thing so slow?
 */ 

/* The ext2 superblock begins 1024 bytes after the start of the ext2 file
 * system, we want to report the start of the filesystem, not the location
 * of the super block */
#define C -2UL

int main(int argc, char **argv)
{
  unsigned long i=0UL, lastblock, last_lastblock=0UL, blocksize, last_size=0UL, ranch_size=0UL, ranch_last=0UL;
  char *device_name="/dev/hdb";
  int fd, best_guess=0, this_is_it, j;
  struct ext2_super_block *sb;
  
  sb = malloc(1024);
  if (!sb) {
    printf("Out of memory - Woe is the machine without a spare kB!\n");
    exit(-42);
  }
  
  /* Parse command line arguments */
  if (argc==3) {
    best_guess = 1;
    device_name = argv[2];
  } else if (argc==2) {
    device_name = argv[1];
  } else if (argc>3) {
    printf("Usage: %s --best-only devicename\n",argv[0]);
    exit(-argc);
  }
  
  if ((fd=open(device_name,O_RDONLY))<0) {
    printf("Can't open device %s\n",device_name);
    exit(-1);
  }
  
  printf("Checking %s for ext2 filesystems %s\n",device_name,
	 (best_guess?"(Best guesses only)":"(Display even the faintest shreds)"));

  printf("\nIt would be wise to go grab a beverage, this will take a while . . .\n");

  printf("\nIt also wouldn't hurt to save the output:\n\t\"");
  for (j=0; j<argc; j++)
    printf("%s ",argv[j]);
  printf("2>&1 | tee DiskRemnants\"\n\n");
 
  /* Make 1st block on disk #0 (where partition table lives), like fdisk would */  
  for(i=0UL;;) {

    /* Grab a block - dump it into memory defined as an ext2_super_block */
    if ( read(fd, sb, 512) != 512 ) {
      printf("Error reading block or EOF encountered (EOF is good)\n");
      exit(-2);
    }
    
    /* It probably isn't it */
    this_is_it = 0;
    
    /* Check if the ext2 magic number is in the right place */
    /* if ( (sb->s_magic== EXT2_SUPER_MAGIC)||(sb->s_magic == EXT2_PRE_02B_MAGIC) ) { */
    if (sb->s_magic== EXT2_SUPER_MAGIC) {

      /* When we get a match, figure out where this partiton would end */
      blocksize = (unsigned long)EXT2_MIN_BLOCK_SIZE << sb->s_log_block_size;
      lastblock = (i+C)+(sb->s_blocks_count*(blocksize/512UL)-1UL); /* Start + (size-1): i.e. 100 block fs starting at block 0 runs 0->99 */

      if ( ((i+C) > last_lastblock) || (last_size != sb->s_blocks_count) ) {
	if (!best_guess)
	  printf("THESE ENTRIES ARE PROBABLY ALL FOR THE SAME PARTITION:\n");
        last_lastblock = lastblock;
        last_size = sb->s_blocks_count;
	if ( (blocksize >= EXT2_MIN_BLOCK_SIZE) && (blocksize <= EXT2_MAX_BLOCK_SIZE) ) {
	  if ( !(ranch_size) || ((ranch_size==last_size)&&(ranch_last==last_lastblock)) ) {
            if (!best_guess)
	      printf("**** I'D BET THE RANCH ON THIS NEXT ENTRY *************\n");
            this_is_it = 1;
	    ranch_size = last_size;
	    ranch_last = last_lastblock;
	  }
	}
      }
      if ((!best_guess)||(this_is_it)) {
        printf("   * Found ext2_magic in sector %lu (512 byte sectors).\n",i+C);
        printf("     This file system is %lu blocks long (each block is %lu bytes)\n",
  	       sb->s_blocks_count,blocksize);
        printf("     Filesystem runs %lu : %lu (512 byte sectors)\n",i+C,lastblock);
      }
    }

    /* Be a wuss and increment it down here, it makes the code a little more readable */
    i++;
  }

  
  exit(0);
}
