/*
 *  lde/no_fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.c,v 1.9 1996/10/11 02:40:04 sdh Exp $
 */

/* 
 *   No file system specified.  Block edits ok.
 */
#include <sys/stat.h>
#include <unistd.h>
 
#include "lde.h"
#include "no_fs.h"

static struct Generic_Inode *NOFS_read_inode(unsigned long nr);
static char* NOFS_dir_entry(int i, void *block_buffer, unsigned long *inode_nr);
static void NOFS_sb_init(char * sb_buffer);

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

int NOFS_null_call(void)
{
  return 0;
}

/* Returns 1 always */
int NOFS_one(unsigned long nr)
{
  return 1;
}

static char* NOFS_dir_entry(int i, void *block_buffer, unsigned long *inode_nr)
{
  *inode_nr = 1UL;
  return ( (char *) "" );
}

static void NOFS_sb_init(char * sb_buffer)
{
  struct stat statbuf;

  fstat(CURR_DEVICE, &statbuf);

  sb->blocksize = 1024;

  /* Try to look up the size of the file/device */
  sb->nzones = ((unsigned long)statbuf.st_size+(sb->blocksize-1UL))/sb->blocksize;
  if (!sb->nzones)
    sb->nzones = -1UL;

  sb->last_block_size = (unsigned long)statbuf.st_size % sb->blocksize;

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
}

void NOFS_init(char * sb_buffer)
{
  fsc = &NOFS_constants;

  NOFS_sb_init(sb_buffer);

  (void) NOFS_init_junk_inode();

  sb->namelen = 1;
  sb->dirsize = 1;

  FS_cmd.inode_in_use = (int (*)(unsigned long n)) NOFS_one;
  FS_cmd.zone_in_use = (int (*)(unsigned long n)) NOFS_one;
  FS_cmd.zone_is_bad = (int (*)(unsigned long n)) NOFS_null_call;
  FS_cmd.dir_entry = NOFS_dir_entry;
  FS_cmd.read_inode = NOFS_read_inode;
  FS_cmd.write_inode = (int (*)(unsigned long inode_nr, struct Generic_Inode *GInode)) NOFS_null_call;
  FS_cmd.map_inode = (unsigned long (*)(unsigned long n)) NOFS_one;
}

