/*
 *  lde/no_fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.c,v 1.5 1994/04/24 20:36:28 sdh Exp $
 */

/* 
 *   No file system specified.  Block edits ok.
 */
 
#include "lde.h"

struct inode_fields NOFS_inode_fields = {
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
  0, /*   unsigned long  i_zone[0]; */
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
  0, /*   unsigned long  i_zone[14]; */
  0, /*   unsigned long  i_version; */
  0, /*   unsigned long  i_file_acl; */
  0, /*   unsigned long  i_dir_acl; */
  0, /*   unsigned long  i_faddr; */
  0, /*   unsigned char  i_frag; */
  0, /*   unsigned char  i_fsize; */
  0, /*   unsigned short i_pad1; */
  1, /*   unsigned long  i_reserved2[2]; */
};

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
  &NOFS_inode_fields,
};

struct Generic_Inode NOFS_junk_inode;

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
  
  for (i=0; i<EXT2_N_BLOCKS; i++)
    NOFS_junk_inode.i_zone[i] = 0UL;

  return &NOFS_junk_inode;
}

struct Generic_Inode *NOFS_read_inode(unsigned long nr)
{
  return &NOFS_junk_inode;
}

unsigned long NOFS_null_call(void)
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

  (void) NOFS_init_junk_inode();

  sb->namelen = 1;
  sb->dirsize = 1;

  FS_cmd.inode_in_use = (int (*)()) NOFS_one;
  FS_cmd.zone_in_use = (int (*)()) NOFS_one;
  FS_cmd.dir_entry = NOFS_dir_entry;
  FS_cmd.read_inode = NOFS_read_inode;
  FS_cmd.write_inode = (int (*)()) NOFS_null_call;

}
