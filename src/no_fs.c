/*
 *  lde/no_fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.c,v 1.3 1994/03/23 05:58:58 sdh Exp $
 */

/* 
 *   No file system specified.  Block edits ok.
 */
 
#include "lde.h"

struct fs_constants NOFS_constants = {
  NONE,                         /* int FS */
  1,                            /* int ROOT_INODE */
  4,                            /* int INODE_SIZE */
  1,                            /* unsigned short N_DIRECT */
  0,                            /* unsigned short INDIRECT */
  0,                            /* unsigned short X2_INDIRECT */
  0,                            /* unsigned short X3_INDIRECT */
  1,                            /* unsigned short N_BLOCKS */
  4,                            /* int ZONE_ENTRY_SIZE */
  4,                            /* int INODE_ENTRY_SIZE */
};

unsigned long NOFS_null_call()
{
  return 0UL;
}

/* Returns 1 always */
unsigned long NOFS_one(unsigned long nr)
{
  return 1UL;
}

char* NOFS_dir_entry(int i, char *block_buffer, unsigned long *inode_nr)
{
  *inode_nr = 1UL;
  return ( (char *) "" );
}

void NOFS_sb_init(char * sb_buffer)
{
  sb->blocksize = 1024;
  sb->last_block_size = lseek(CURR_DEVICE,0,SEEK_END);
  sb->nzones = (sb->last_block_size / sb->blocksize);
  sb->last_block_size = sb->last_block_size % sb->blocksize;

  /* How do we find the size of a block device?? */
  if (!sb->nzones) sb->nzones = -1L;

  /* In order to prevent division by zeroes, set junk entries to 1 */
  sb->ninodes = 1;
  sb->imap_blocks = 1;
  sb->zmap_blocks = 1;
  sb->first_data_zone = 0;
  sb->max_size = 1;
  sb->zonesize = 1;
  sb->magic = 0;

  sb->I_MAP_SLOTS = 1;
  sb->Z_MAP_SLOTS = 1;
  sb->INODES_PER_BLOCK = 1;
  sb->norm_first_data_zone = 0;
}

void NOFS_init(char * sb_buffer)
{
  fsc = &NOFS_constants;

  NOFS_sb_init(sb_buffer);

  sb->namelen = 1;
  sb->dirsize = 1;

  DInode.i_mode = (unsigned short (*)()) NOFS_null_call;
  DInode.i_uid = (unsigned short (*)()) NOFS_null_call;
  DInode.i_size = (unsigned long (*)()) NOFS_null_call;
  DInode.i_atime = (unsigned long (*)()) NOFS_null_call;
  DInode.i_ctime = (unsigned long (*)()) NOFS_null_call;
  DInode.i_mtime = (unsigned long (*)()) NOFS_null_call;
  DInode.i_gid = (unsigned short (*)()) NOFS_null_call;
  DInode.i_links_count = (unsigned short (*)()) NOFS_null_call;
  DInode.i_zone = (unsigned long (*)()) NOFS_null_call;

  FS_cmd.inode_in_use = (int (*)()) NOFS_one;
  FS_cmd.zone_in_use = (int (*)()) NOFS_one;
  FS_cmd.dir_entry = NOFS_dir_entry;

}
