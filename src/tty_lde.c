/*
 *  lde/tty_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: tty_lde.c,v 1.34 2003/12/06 08:01:46 scottheavner Exp $
 */

#include "lde_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_IO_H
#include <io.h>
#endif
#include <time.h>
#if HAVE_PWD_H
#include <pwd.h>
#endif
#if HAVE_GRP_H
#include <grp.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "lde.h"
#include "tty_lde.h"
#include "swiped.h"

/* llseek seems to appear in different places on different systems,
 * and some docs say call it via syscall5, this seems to work on
 * most recent systems */
#if HAVE_LLSEEK
#ifndef UNISTD_LLSEEK_PROTO
extern loff_t llseek(int fd, loff_t offset, int whence);
#endif
#endif
#if NEED_LSEEK64_PROTO
extern off64_t lseek64(int __fd, off64_t __offset, int __whence);
#endif

/* We don't need no stinkin' linked list */
char *error_save[ERRORS_SAVED];
int current_error = -1;

/* Stores errors/warnings */
void log_error(char *echo_string)
{
  FILE *fp;
  time_t t;

  if (lde_flags.logtofile && (fp = fopen("/tmp/ldeerrors", "a"))) {
    time(&t);
    fprintf(fp, "%s - %s\n", lde_ctime(&t), echo_string);
    fclose(fp);
  }
  if ((++current_error) >= ERRORS_SAVED)
    current_error = 0;
  error_save[current_error] =
    realloc(error_save[current_error], (strlen(echo_string) + 1));
  strcpy(error_save[current_error], echo_string);
}

/* Immediate printing of warnings and errors to standard err */
void tty_warn(char *fmt, ...)
{
  va_list argp;
  char echo_string[256];

  va_start(argp, fmt);
  vsprintf(echo_string, fmt, argp);
  va_end(argp);

  log_error(echo_string);
  fprintf(stderr, "%s\n", echo_string);
}

/* Crippled warning function */
void no_warn(char *fmt, ...) {}

/* No input  available in tty mode */
int tty_mgetch()
{
  return 13;
}

/* This looks up the size of the block, the only block which might have
 * an irregular size is the last block
 */
unsigned long lookup_blocksize(unsigned long nr)
{
  return (((sb->nzones - 1) == nr) ? sb->last_block_size : sb->blocksize);
}

/* This reads a long int from the user.  It recognizes indicators as to the 
 * type of input for decimal, hex, or octal.  For example if we wanted to
 * input 15, we could enter as hex (0xf, xf, $f), decimal (15), or octal
 * (\017, 017).  sscanf is probably smart enough to recognize these, but 
 * what the hell.
 */
unsigned long read_num(char *cinput)
{
  unsigned long i;

  if (strlen(cinput) > 0) {
    if ((cinput[0] == '$') || (cinput[0] == 'x') || (cinput[0] == 'X')) {
      sscanf(cinput, "%*1c%lx", &i);
    } else if (cinput[0] == '\\') {
      sscanf(cinput, "%*1c%lo", &i);
    } else if (cinput[0] == '0') {
      if ((cinput[1] == 'x') || (cinput[1] == 'X')) {
        sscanf(cinput, "%*2c%lx", &i);
      } else {
        if (cinput[1] != 0)
          sscanf(cinput, "%*1c%lo", &i);
        else
          i = 0;
      }
    } else {
      sscanf(cinput, "%lu", &i);
    }
    return i;
  }

  return 0; /* This should be a safe return if something went wrong */
}

unsigned long lde_seek_block(unsigned long block_nr)
{
#if HAVE_LSEEK64

  off64_t dbnr = (off64_t)block_nr * (off64_t)sb->blocksize;

  if (lseek64(CURR_DEVICE, dbnr, SEEK_SET) == dbnr)
    return block_nr;

#else
#if HAVE_LLSEEK

  loff_t dbnr = (loff_t)block_nr * sb->blocksize;

  if (llseek(CURR_DEVICE, dbnr, SEEK_SET) == dbnr)
    return block_nr;

#else
#if (SIZEOF_OFF_T > SIZEOF_UNSIGNED_LONG)

  off_t dbnr = (off_t)block_nr * sb->blocksize;

  if (lseek(CURR_DEVICE, dbnr, SEEK_SET) == dbnr)
    return block_nr;

#else
#define MAX_OFF_T (~(1L << (sizeof(off_t) * 8 - 1)))
#warning System does not have llseek() or lseek64(), using slow lookups for blocks > 2GB

  unsigned long MaxIndexableBlock = MAX_OFF_T / sb->blocksize;
  unsigned long b = block_nr;
  int whence = SEEK_SET;

  if ((b > MaxIndexableBlock) && (!(MAX_OFF_T % sb->blocksize))) {
    if (0 != lseek(CURR_DEVICE, 0, SEEK_SET)) {
      lde_warn("lde_seek(1): seek failed, errno=%d", errno);
      return 0UL;
    }
    while (b > MaxIndexableBlock) {
      off_t moved = lseek(CURR_DEVICE, MAX_OFF_T, SEEK_CUR);
      if (moved != MAX_OFF_T) {
        lde_warn("lde_seek(1): seek failed, errno=%d", errno);
        lseek(CURR_DEVICE, 0, SEEK_SET);
        return 0UL;
      }
      b -= MaxIndexableBlock;
    }
    whence = SEEK_CUR;
  }

  b *= sb->blocksize;

  if (lseek(CURR_DEVICE, (off_t)b, whence) == b)
    return block_nr;
#endif /* off_t > ul */
#endif /* HAVE_LLSEEK */
#endif /* HAVE_LSEEK64 */

  lde_warn("lde_seek_block: seek failed, errno=%d", errno);

  /* On error, seek to beginning of device */
  lseek(CURR_DEVICE, 0, SEEK_SET);
  return 0;
}

/* mask_bad_block: 
 *  - if we get an error reading from the disk, we look in a directory
 *    for the a file with the blocknumbers name in uppercase hex with no
 *    leading zeros or spaces.  I.e. you'll specify /tmp/badblocks on
 *    the commandline and this will look for /tmp/badblocks/A if we
 *    need block 10.
 */
static size_t mask_bad_block(unsigned long block_nr,
  void *dest,
  size_t read_size)
{
  int f;
  size_t act_size = 0;
  char filename[256];

  sprintf(filename, "%s/%lX", badblocks_directory, block_nr);
  f = open(filename, O_RDONLY);
  if (-1 == f)
    return -1; /* return error, no override file found */
  act_size = read(f, dest, read_size);
  close(f);
  return act_size;
}

/* Reads a block w/o caching results */
size_t nocache_read_block(unsigned long block_nr, void *dest, size_t read_size)
{
  size_t act_size = 0;

  /* Try to read it in, on error we return 0 for act_size, buffer
   * contents are not changed */
  if (lde_seek_block(block_nr) != block_nr)
    lde_warn("Read error: unable to seek to block"
             "0x%lx in nocache_read_block, errno=%d",
      block_nr,
      errno);
  else if ((act_size = read(CURR_DEVICE, dest, read_size)) != read_size) {
    lde_warn("Unable to read full block (%lu) in nocache_read_block,"
             " errno=%d",
      block_nr,
      errno);
    return mask_bad_block(block_nr, dest, read_size);
  }

  return act_size;
}

static cached_block diskcache[LDE_DISKCACHE];
static cached_block *diskcachestart = diskcache;

/* Assign pointers to link list, add bogus values to block number and size */
void init_disk_cache()
{
  int i;
  for (i = 1; i < (LDE_DISKCACHE - 1); i++) {
    diskcache[i].prev = &diskcache[i - 1];
    diskcache[i].next = &diskcache[i + 1];
    diskcache[i].bnr = -1;
    diskcache[i].size = 0;
  }

  diskcache[0].prev = &diskcache[LDE_DISKCACHE - 1];
  diskcache[0].next = &diskcache[1];
  diskcache[0].bnr = -1;
  diskcache[0].size = 0;

  diskcache[LDE_DISKCACHE - 1].prev = &diskcache[LDE_DISKCACHE - 2];
  diskcache[LDE_DISKCACHE - 1].next = &diskcache[0];
  diskcache[LDE_DISKCACHE - 1].bnr = -1;
  diskcache[LDE_DISKCACHE - 1].size = 0;
}

/* Whopping cache of one block returns pointer to a cache_block structure
 *  The start of this struct is a buffer so it is possible to access the
 *  data buffer contents by referring to the structs address. */
void *cache_read_block(unsigned long block_nr, void *dest, int force)
{
  cached_block *bp, *cp;
  int i;

  /* User has specified another buffer */
  if (force & NEVER_CACHE) {
    if (!dest) {
      lde_warn("PROGRAM ERROR: dest undefined in cache_read_block"
               "using NEVER_CACHE");
      return NULL;
    }
    bp = dest;
  } else {
    /* Search cache, see if we have a copy of this block */
    bp = diskcachestart;
    for (i = 0; i < LDE_DISKCACHE; i++) {
      if (bp->bnr == block_nr) {
        break;
      }
      bp = bp->next;
    }

    /* Move this block to the top of the chain */
    if (bp != diskcachestart) {
      /* pull this block out of the chain */
      bp->prev->next = bp->next;

      /* jam it back at the top of the linked list */
      bp->next = diskcachestart->next;
      bp->prev = diskcachestart;
      diskcachestart->next = bp;

      /* save as new block in cache chain */
      diskcachestart = bp;
    }
  }

  if ((force & FORCE_READ) || (block_nr != bp->bnr)) {
    int read_size;

    memset(bp->data, 0, sb->blocksize);

    /* Lookup size of block (if it's the last block in a file,
     * it may be less than the blocksize of the device */
    read_size = lookup_blocksize(block_nr);

    bp->size = nocache_read_block(block_nr, bp->data, read_size);
    if (bp->size == -1) {
      bp->size = read_size; /* Lousy handling */
      memset(bp->data, '!', bp->size);
    }
    if (bp->size)
      bp->bnr = block_nr;
    else
      bp->bnr = 0UL;
  }

  /* Need to copy data to dest? */
  if ((dest) && (bp != dest)) {
    cp = dest;
    memcpy(cp, bp, bp->size);
    cp->bnr = bp->bnr;
    cp->size = bp->size;
  }

  return bp;
}

int write_block(unsigned long block_nr, void *data_buffer)
{
  size_t write_count;

  if (!lde_flags.write_ok) {
    lde_warn("Disk not writable, block (%lu) not written", block_nr);
    return -1;
  }
#ifndef PARANOID
  if (lde_seek_block(block_nr) != block_nr) {
    lde_warn(
      "Write error: unable to seek to block (%lu) in write_block", block_nr);
    return -1;
  } else {
    write_count = (size_t)lookup_blocksize(block_nr);

    if (write(CURR_DEVICE, data_buffer, write_count) != write_count) {
      lde_warn(
        "Write error: unable to write block (%d) in write_block", block_nr);
      return -1;
    }
  }
#endif
  return 0;
}

/* Dumps a blocks data to stdout, similar to cat or dd */
void ddump_block(unsigned long nr)
{
  unsigned char *dind;

  dind = cache_read_block(nr, NULL, CACHEABLE);
  fwrite(dind, lookup_blocksize(nr), 1, stdout);
}

/* Dumps a block to STDOUT, formatted as hex and ascii -- could merge with curses version someday? */
void dump_block(unsigned long nr)
{
  int i, j = 0;
  unsigned char *dind, c;
  size_t blocksize;

  dind = cache_read_block(nr, NULL, CACHEABLE);
  blocksize = lookup_blocksize(nr);

  while (j * 16 < blocksize) {
    printf("\n0x%08lX  ", j * 16 + nr * sb->blocksize);
    for (i = j * 16; i < (j * 16 + 8); i++)
      if (i < blocksize)
        printf("%2.2X ", dind[i]);
      else
        printf("   ");
    printf(": ");
    for (i = (j * 16 + 8); i < (j * 16 + 16); i++)
      if (i < blocksize)
        printf("%2.2X ", dind[i]);
      else
        printf("   ");

    printf(" ");
    for (i = (j * 16); ((i < (j * 16 + 16)) && (i < blocksize)); i++) {
      c = dind[i];
      c = ((c > 31) && (c < 127)) ? c : '.';
      printf("%c", c);
    }
    j++;
  }
}

/* Dump inode contents to STDOUT -- could also merge with the curses one someday? */
void dump_inode(unsigned long nr)
{
  int j;
  char f_mode[12];
  struct Generic_Inode *GInode;
#if HAVE_GETPWUID
  struct passwd *NC_PASS = NULL;
#endif
#if HAVE_GETGRGID
  struct group *NC_GROUP = NULL;
#endif

  GInode = FS_cmd.read_inode(nr);

  /* Print inode number and file type */
  printf(
    "-------------------------------------------------------------------------------\n");
  printf("INODE: %-6lu (0x%8.8lX) ", nr, nr);

  if (FS_cmd.inode_in_use(nr))
    printf("\n");
  else
    printf("(NOT USED)\n");

  /*--- Print second line like a directory entry ---*/
  /* drwxr-xr-x field */
  mode_string((unsigned short)GInode->i_mode, f_mode);
  f_mode[10] = 0; /* Junk from canned mode_string */
  printf("%10.10s	 ", f_mode);
  /* UID field */
  if (fsc->inode->i_uid) {
#if HAVE_GETPWUID
    if (!lde_flags.nosymbolic_guid &&
        ((NC_PASS = getpwuid(GInode->i_uid)) != NULL))
      printf("%-8s ", NC_PASS->pw_name);
    else
#endif
      printf("%-8d ", GInode->i_uid);
  } else {
    printf("         ");
  }
  /* GID field */
  if (fsc->inode->i_gid) {
#if HAVE_GETGRGID
    if (!lde_flags.nosymbolic_guid &&
        ((NC_GROUP = getgrgid(GInode->i_gid)) != NULL))
      printf("%-8s ", NC_GROUP->gr_name);
    else
#endif
      printf("%-8d ", GInode->i_gid);
  } else {
    printf("         ");
  }
  if (fsc->inode->i_size)
    printf("%9ld ", GInode->i_size);
  else
    printf("%10s", "");
  if (fsc->inode->i_mtime)
    printf("%24s\n", lde_ctime(&GInode->i_mtime));
  else
    printf("%24s\n", "");

  /*--- TYPE on a line ---*/
  if (fsc->inode->i_mode)
    printf("TYPE:                  %14s\n", entry_type(GInode->i_mode));

  /*--- LINKS on a line ---*/
  if (fsc->inode->i_links_count)
    printf("LINKS:                 %d\n", GInode->i_links_count);

  /*--- MODE on a line ---*/
  if ((fsc->inode->i_mode) || (fsc->inode->i_mode_flags)) {
    printf("MODEFLAGS.MODE:        %03o.%04o\n",
      (GInode->i_mode & 0x1ff000) >> 12,
      GInode->i_mode & 0xfff);
  }

  /*--- SIZE on a line ---*/
  if (fsc->inode->i_size)
    printf("SIZE:                  %-8ld\n", GInode->i_size);
  if (fsc->inode->i_blocks)
    printf("BLOCK COUNT:           %-8ld\n", GInode->i_blocks);

  /*--- OWNER ---*/
  if (fsc->inode->i_uid) {
    printf("UID:                   ");
    printf("%05d", GInode->i_uid);
#if HAVE_GETPWUID
    if (NC_PASS != NULL)
      printf(" (%s)", NC_PASS->pw_name);
#endif
    printf("\n");
  }

  /*--- GROUP ---*/
  if (fsc->inode->i_gid) {
    printf("GID:                   ");
    printf("%05d", GInode->i_gid);
#if HAVE_GETGRGID
    if (NC_GROUP != NULL)
      printf(" (%s)", NC_GROUP->gr_name);
#endif
    printf("\n");
  }

  /*--- Display times ---*/
  if (fsc->inode->i_atime)
    printf("ACCESS TIME:           %24s\n", lde_ctime(&GInode->i_atime));
  if (fsc->inode->i_ctime)
    printf("CREATION TIME:         %24s\n", lde_ctime(&GInode->i_ctime));
  if (fsc->inode->i_mtime)
    printf("MODIFICATION TIME:     %24s\n", lde_ctime(&GInode->i_mtime));
  if (fsc->inode->i_dtime)
    printf("DELETION TIME:         %24s\n", lde_ctime(&GInode->i_dtime));

  /*--- Display blocks ---*/
  j = -1;
  if (fsc->inode->i_zone[0]) {
    printf("DIRECT BLOCKS:         ");
    while (++j < INODE_BLKS) {
      if ((j < fsc->N_DIRECT) && (j) && (j % 4 == 0)) {
        printf("\n                       ");
      } else if ((fsc->INDIRECT) && (j == fsc->INDIRECT)) {
        printf("\nINDIRECT BLOCK:        ");
      } else if ((fsc->X2_INDIRECT) && (j == fsc->X2_INDIRECT)) {
        printf("\nDOUBLE INDIRECT BLOCK: ");
      } else if ((fsc->X3_INDIRECT) && (j == fsc->X3_INDIRECT)) {
        printf("\nTRIPLE INDIRECT BLOCK: ");
      }
      if (GInode->i_zone[j])
        printf("0x%8.8lX ", GInode->i_zone[j]);
    }
  }

  printf("\n");
}

char *entry_type(unsigned short imode)
{
  if (S_ISREG(imode))
    return "regular file  ";
  else if (S_ISDIR(imode))
    return "directory     ";
  else if (S_ISLNK(imode))
    return "symbolic link ";
  else if (S_ISCHR(imode))
    return "char device   ";
  else if (S_ISBLK(imode))
    return "block device  ";
  else if (S_ISFIFO(imode))
    return "named pipe    ";
  else if (S_ISSOCK(imode))
    return "socket        ";
  return "???           ";
}

#ifdef _MSC_VER
#define localtime_r(a, b) localtime_s((b), (a))
#endif

const char *lde_ctime(const time_t *input)
{
  static char sbuf[70]; // we're single threaded
  struct tm tbuf;

  localtime_r(input, &tbuf); // MSVC version does not return pointer to tbuf
  if (strftime(sbuf, sizeof(sbuf), "%c", &tbuf)) {
    sbuf[sizeof(sbuf) - 1] = 0;
    return sbuf;
  } else {
    return "[CONV ERR]";
  }
}