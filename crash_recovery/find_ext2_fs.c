#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "../src/swiped/linux/ext2_fs.h"

/* (C) 1994 Scott Heavner */

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
 * starts and ends.  The first one should show the actual start and end of the
 * partition, the rest think that the partition begins just before this copy of
 * the superblock, so the start and end will be incorrect.  It may be necessary
 * to leave out the best guess option if nothing shows up on the first run.
 *
 * $Id: find_ext2_fs.c,v 1.3 2002/01/10 19:33:33 scottheavner Exp $
 *
 * Changes:
 *
 * January 10, 2001 - Seems debian is still using/compiling this thing.
 *   Switch to include our ext_fs.h but may need to define HAVE_ASM_TYPES
 *   when compiling.
 *
 * October 6, 1997 - modified to search 512 byte blocks instead of 1024.  If
 *   the filesystem didn't start on a 1024 byte interval, it would have been
 *   skipped.
 * - comment out EXT2_PRE_02B_MAGIC - must not be supported in kernels anymore
 * - compile with this for a 4k i386 binary
 *     "gcc -O6 -s -DHAVE_ASM_TYPES_H=1 -o find_ext2_fs find_ext2_fs.c"
 * - why is this thing so slow?
 *
 * October 7, 1997 - try to speed things up by reading in more blocks at a
 *   time
 * - NB=32 is still painfully slow, maybe we should revert back to old version
 *   that only read in 512 bytes at a time.  If NB=1, #ifdefs should revert
 *   back to old version.
 * - /usr/bin/time trials to search to ext2fs at sector 204624 of an ide disk:
 *        NB=     32             8              1
 *        real    1m17.916s      1m28.996s      1m40.012s   
 *        user    0m0.200s       0m0.530s       0m0.490s
 *        sys     0m57.270s      0m43.970s      0m49.060s
 * - Might be nice to write a new tool that starts at block 1 of a disk,
 *   figures out what's on it (msdos or linux partition), computes and displays
 *   its size/start/end, the jumps to the end and looks for the next bit of
 *   partition info.
 */

/* The ext2 superblock begins 1024 bytes after the start of the ext2 file
 * system, we want to report the start of the filesystem, not the location
 * of the super block */
#define C -2UL

/* Speed enhancement - set the number of 512 byte blocks we read at a time
 * Feel free to twiddle this parameter, if you make it too big, performance
 * probably won't improve - it will also control how close to the end of
 * the partition you can get as the last NB sectors won't be checked, must be
 * at least 1 */
#define NB 500

int main(int argc, char **argv)
{
  unsigned long i = 0UL, lastblock, last_lastblock = 0UL, blocksize,
                last_size = 0UL, ranch_size = 0UL, ranch_last = 0UL;
  char *device_name = "/dev/hdb", buffer[NB * 512];
  int fd, best_guess = 0, this_is_it, j;
  struct ext2_super_block *sb;

#if NB < 2
  sb = (void *)buffer;
#endif

  /* Parse command line arguments */
  if (argc == 3) {
    best_guess = 1;
    device_name = argv[2];
  } else if (argc == 2) {
    device_name = argv[1];
  } else if (argc > 3) {
    printf("Usage: %s --best-only devicename\n", argv[0]);
    exit(-argc);
  }

  if ((fd = open(device_name, O_RDONLY)) == -1) {
    printf("Can't open device %s\n", device_name);
    exit(-1);
  }

  printf("Checking %s for ext2 filesystems %s\n",
    device_name,
    (best_guess ? "(Best guesses only)"
                : "(Display even the faintest shreds)"));

  printf(
    "\nIt would be wise to go grab a beverage, this will take a while . . .\n");

  printf("\nIt also wouldn't hurt to save the output:\n\t\"");
  for (j = 0; j < argc; j++)
    printf("%s ", argv[j]);
  printf("2>&1 | tee DiskRemnants\"\n\n");

  /* Make 1st block on disk #0 (where partition table lives), like fdisk would */
  for (i = 0UL;;) {

    /* Grab a block - dump it into memory defined as an ext2_super_block */
    if (read(fd, buffer, NB * 512) != NB * 512) {
      printf("Error reading block or EOF encountered (EOF is good)\n");
      exit(-2);
    }

#if NB > 1
    /* Operate on all the blocks we've read into memory */
    for (j = 0; j < NB; j++) {

      sb = (void *)(buffer + j * 512);
#endif

      /* It probably isn't it */
      this_is_it = 0;

      /* Check if the ext2 magic number is in the right place */
      /* if ( (sb->s_magic== EXT2_SUPER_MAGIC)||(sb->s_magic == EXT2_PRE_02B_MAGIC) ) { */
      if (sb->s_magic == EXT2_SUPER_MAGIC) {

        /* When we get a match, figure out where this partiton would end */
        blocksize = (unsigned long)EXT2_MIN_BLOCK_SIZE << sb->s_log_block_size;
        /* END = START + (SIZE-1): i.e. 100 block fs starting at block 0 runs 0->99 */
        lastblock = (i + C) + (sb->s_blocks_count * (blocksize / 512UL) - 1UL);

        if (((i + C) > last_lastblock) || (last_size != sb->s_blocks_count)) {
          if (!best_guess)
            printf("THESE ENTRIES ARE PROBABLY ALL FOR THE SAME PARTITION:\n");
          last_lastblock = lastblock;
          last_size = sb->s_blocks_count;
          if ((blocksize >= EXT2_MIN_BLOCK_SIZE) &&
              (blocksize <= EXT2_MAX_BLOCK_SIZE)) {
            if (!(ranch_size) ||
                ((ranch_size == last_size) && (ranch_last == last_lastblock))) {
              if (!best_guess)
                printf(
                  "**** I'D BET THE RANCH ON THIS NEXT ENTRY *************\n");
              this_is_it = 1;
              ranch_size = last_size;
              ranch_last = last_lastblock;
            }
          }
        }
        if ((!best_guess) || (this_is_it)) {
          printf(
            "   * Found ext2_magic in sector %lu (512 byte sectors).\n", i + C);
          printf(
            "     This file system is %u blocks long (each block is %lu bytes)\n",
            sb->s_blocks_count,
            blocksize);
          printf("     Filesystem runs %lu : %lu (512 byte sectors)\n",
            i + C,
            lastblock);
        }
      }
      i++; /* Needs to be inside j loop */
#if NB > 1
    }
#endif
  }

  exit(0);
}
