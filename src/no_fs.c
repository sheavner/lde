/*
 *  lde/no_fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.c,v 1.13 1998/01/24 21:43:39 sdh Exp $
 *
 *  The following routines were taken almost verbatim from
 *  the e2fsprogs-1.02 package by Theodore Ts'o and Remy Card.
 *       NOFS_get_device_size()
 *       valid_offset()
 */

/* 
 *   No file system specified.  Block edits ok.
 */
#include <sys/stat.h>
#include <unistd.h>
 
#include "lde.h"
#include "no_fs.h"

static struct Generic_Inode *NOFS_read_inode(unsigned long nr);
static char* NOFS_dir_entry(int i, lde_buffer *block_buffer, unsigned long *inode_nr);
static void NOFS_sb_init(char * sb_buffer);
static int NOFS_write_inode_NOT(unsigned long ino,
				struct Generic_Inode *GInode);
static int NOFS_one_i__ul(unsigned long nr);
static unsigned long NOFS_one_ul__ul(unsigned long nr);
static int NOFS_zero_i__ul(unsigned long nr);

static struct inode_fields NOFS_inode_fields = {
  0, /*   unsigned short i_mode; */
  0, /*   unsigned short i_uid; */
  0, /*   unsigned long  i_size; */
  0, /*   unsigned short i_links_count; */
  0, /*   ()             i_mode_flags; */
  0, /*   unsigned short i_gid; */
  0, /*   unsigned long  i_blocks; */
  0, /*   unsigned long  i_atime; */
  0, /*   unsigned long  i_ctime; */
  0, /*   unsigned long  i_mtime; */
  0, /*   unsigned long  i_dtime; */
  0, /*   unsigned long  i_flags; */
  0, /*   unsigned long  i_reserved1; */
  { 0, /*   unsigned long  i_zone[0]; */
    0, /*   unsigned long  i_zone[1]; */
    0, /*   unsigned long  i_zone[2]; */
    0, /*   unsigned long  i_zone[3]; */
    0, /*   unsigned long  i_zone[4]; */
    0, /*   unsigned long  i_zone[5]; */
    0, /*   unsigned long  i_zone[6]; */
    0, /*   unsigned long  i_zone[7]; */
    0, /*   unsigned long  i_zone[8]; */
    0, /*   unsigned long  i_zone[9]; */
    0, /*   unsigned long  i_zone[10]; */
    0, /*   unsigned long  i_zone[11]; */
    0, /*   unsigned long  i_zone[12]; */
    0, /*   unsigned long  i_zone[13]; */
    0  /*   unsigned long  i_zone[14]; */
  },
  0, /*   unsigned long  i_version; */
  0, /*   unsigned long  i_file_acl; */
  0, /*   unsigned long  i_dir_acl; */
  0, /*   unsigned long  i_faddr; */
  0, /*   unsigned char  i_frag; */
  0, /*   unsigned char  i_fsize; */
  0, /*   unsigned short i_pad1; */
  1, /*   unsigned long  i_reserved2[2]; */
};

static struct fs_constants NOFS_constants = {
  NONE,                         /* int FS */
  1,                            /* int ROOT_INODE */
  4,                            /* int INODE_SIZE */
  1,                            /* unsigned short N_DIRECT */
  0,                            /* unsigned short INDIRECT */
  0,                            /* unsigned short X2_INDIRECT */
  0,                            /* unsigned short X3_INDIRECT */
  1,                            /* unsigned short N_BLOCKS */
  1,                            /* unsigned long  FIRST_MAP_BLOCK */
  4,                            /* int ZONE_ENTRY_SIZE */
  4,                            /* int INODE_ENTRY_SIZE */
  &NOFS_inode_fields,
};

static struct Generic_Inode NOFS_junk_inode;

struct Generic_Inode *NOFS_init_junk_inode(void)
{
  int i;

  NOFS_junk_inode.i_mode        = 0UL;
  NOFS_junk_inode.i_uid         = 0UL;
  NOFS_junk_inode.i_size        = 0UL;
  NOFS_junk_inode.i_atime       = 0UL;
  NOFS_junk_inode.i_ctime       = 0UL;
  NOFS_junk_inode.i_mtime       = 0UL;
  NOFS_junk_inode.i_gid         = 0UL;
  NOFS_junk_inode.i_links_count = 0UL;
  
  for (i=0; i<INODE_BLKS; i++)
    NOFS_junk_inode.i_zone[i] = 0UL;

  return &NOFS_junk_inode;
}

static struct Generic_Inode *NOFS_read_inode(unsigned long nr)
{
  return &NOFS_junk_inode;
}

/* Always returns 0 */
static int NOFS_write_inode_NOT(unsigned long ino,
				struct Generic_Inode *GInode)
{
  return 0;
}

/* Returns 1 always */
static int NOFS_one_i__ul(unsigned long nr)
{
  return 1;
}

/* Returns 1 always */
static unsigned long NOFS_one_ul__ul(unsigned long nr)
{
  return 1;
}

/* Returns 0 always */
static int NOFS_zero_i__ul(unsigned long nr)
{
  return 0;
}

/* Returns an empty string */
static char* NOFS_dir_entry(int i, lde_buffer *block_buffer, 
			    unsigned long *inode_nr)
{
  *inode_nr = 1UL;
  return ( (char *) "" );
}


static void NOFS_sb_init(char * sb_buffer)
{
  static int firsttime=1;
  struct stat statbuf;

  fstat(CURR_DEVICE, &statbuf);

  sb->blocksize = 1024;

  /* Try to look up the size of the file/device */
  sb->nzones = ((unsigned long)statbuf.st_size+(sb->blocksize-1UL))/
               sb->blocksize;

  /* If we are operating on a file, figure the size of the last block */
  sb->last_block_size = (unsigned long)statbuf.st_size % sb->blocksize;
  if ((sb->last_block_size==0)&&(statbuf.st_size!=0))
    sb->last_block_size = sb->blocksize;

  /* If it is a partition, look it up with the slow NOFS_get_device_size().
   * Don't want to do this the first time through because the partition
   * will most likely have a file system on it and we won't have to 
   * resort to this */
  if ((!sb->nzones)&&(!firsttime))
    sb->nzones = NOFS_get_device_size(CURR_DEVICE, sb->blocksize);

  /* Both methods of size detection failed, just set it big */
  if (!sb->nzones)
    sb->nzones = -1UL;

  /* In order to prevent division by zeroes, set junk entries to 1 */
  sb->ninodes = 1UL;
  sb->imap_blocks = 1UL;
  sb->zmap_blocks = 1UL;
  sb->first_data_zone = 0UL;
  sb->max_size = 1UL;
  sb->zonesize = 1UL;
  sb->magic = 0UL;

  sb->I_MAP_SLOTS = 1;
  sb->Z_MAP_SLOTS = 1;
  sb->INODES_PER_BLOCK = 1;
  sb->norm_first_data_zone = 0UL;

  firsttime = 0;
}

void NOFS_init(char * sb_buffer)
{
  fsc = &NOFS_constants;

  NOFS_sb_init(sb_buffer);

  (void) NOFS_init_junk_inode();

  sb->namelen = 1;
  sb->dirsize = 1;

  FS_cmd.inode_in_use = NOFS_one_i__ul;
  FS_cmd.zone_in_use = NOFS_one_i__ul;
  FS_cmd.zone_is_bad = NOFS_zero_i__ul;
  FS_cmd.is_system_block = NOFS_zero_i__ul;
  
  FS_cmd.dir_entry = NOFS_dir_entry;
  FS_cmd.read_inode = NOFS_read_inode;
  FS_cmd.write_inode = NOFS_write_inode_NOT;
  FS_cmd.map_inode = NOFS_one_ul__ul;
}

static int valid_offset (int fd, unsigned long offset)
{
  char ch;
  
  if (lseek (fd, offset, SEEK_SET) < 0)
    return 0;
  if (read (fd, &ch, 1) < 1)
    return 0;
  return 1;
}

/* Returns the number of blocks in a partition */
unsigned long NOFS_get_device_size(int fd, unsigned long blocksize)
{
  unsigned long high, low;

  /* Do binary search to find the size of the partition.  */
  low = 0;
  for (high = 1024; valid_offset (fd, high); high *= 2)
    low = high;
  while (low < high - 1)
    {
      const unsigned long mid = (low + high) / 2;
      
      if (valid_offset (fd, mid))
	low = mid;
      else
	high = mid;
    }
  valid_offset (fd, 0);
  return ((low + 1) / blocksize);
}
