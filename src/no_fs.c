/*
 *  lde/no_fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.c,v 1.1 1994/03/21 08:41:40 sdh Exp $
 */

/* 
 *   No file system specified.  Block edits ok.
 */
 
#include "lde.h"

/* In order to prevent division by zeroes, set junk entries to 1 */
struct fs_constants NOFS_constants = {
  NONE,                         /* int FS */
  1,                            /* int ROOT_INODE */
  1,                            /* int INODE_SIZE */
  1,                            /* unsigned short N_DIRECT */
  1,                            /* unsigned short INDIRECT */
  1,                            /* unsigned short X2_INDIRECT */
  1,                            /* unsigned short X3_INDIRECT */
  1,                            /* unsigned short N_BLOCKS */
  4,                            /* int ZONE_ENTRY_SIZE */
  4,                            /* int INODE_ENTRY_SIZE */
};

/* Returns 1 always */
unsigned long one(unsigned long nr)
{
  return 1;
}

void NOFS_sb_init(char * sb_buffer)
{
  sb->blocksize = 1024;
  sb->last_block_size = lseek(CURR_DEVICE,0,SEEK_END);
  sb->nzones = (sb->last_block_size / sb->blocksize);
  sb->last_block_size = sb->last_block_size % sb->blocksize;

  /* How do we find the size of a block device?? */
  if (!sb->nzones) sb->nzones = -1L;

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

  DInode.i_mode = (unsigned short (*)()) MINIX_null_call;
  DInode.i_uid = (unsigned short (*)()) MINIX_null_call;
  DInode.i_size = (unsigned long (*)()) MINIX_null_call;
  DInode.i_atime = (unsigned long (*)()) MINIX_null_call;
  DInode.i_ctime = (unsigned long (*)()) MINIX_null_call;
  DInode.i_mtime = (unsigned long (*)()) MINIX_null_call;
  DInode.i_gid = (unsigned short (*)()) MINIX_null_call;
  DInode.i_links_count = (unsigned short (*)()) MINIX_null_call;
  DInode.i_zone = (unsigned long (*)()) MINIX_null_call;

  FS_cmd.inode_in_use = (int (*)()) one;
  FS_cmd.zone_in_use = (int (*)()) one;

}
