/*
 *  lde/minix.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: minix.c,v 1.2 1994/03/19 20:00:11 sdh Exp $
 */

/* 
 *  This file contain all the minix specific code.
 *  Some minix calls are made by the xiafs code.
 */
 
#include "lde.h"

#undef Inode
#define Inode (((struct minix_inode *) inode_buffer)-1)

#undef Super
#define Super (*(struct minix_super_block *)sb_buffer)

struct fs_constants MINIX_constants = {
  MINIX,                        /* int FS */
  MINIX_ROOT_INO,               /* int ROOT_INODE */
  (sizeof(struct minix_inode)), /* int INODE_SIZE */
  7,                            /* unsigned short N_DIRECT */
  7,                            /* unsigned short INDIRECT */
  8,                            /* unsigned short X2_INDIRECT */
  0,                            /* unsigned short X3_INDIRECT */
  9,                            /* unsigned short N_BLOCKS */
  2,                            /* int ZONE_ENTRY_SIZE */
  2,                            /* int INODE_ENTRY_SIZE */
};

unsigned short MINIX_i_mode(unsigned long nr)
{
  struct minix_inode * inode = (Inode + nr);
  return (unsigned short) inode->i_mode;
}

unsigned short MINIX_i_uid(unsigned long nr)
{
  struct minix_inode * inode = (Inode + nr);
  return (unsigned short) inode->i_uid;
}

unsigned long MINIX_i_size(unsigned long nr)
{
  struct minix_inode * inode = (Inode + nr);
  return (unsigned long) inode->i_size;
}

unsigned long MINIX_i_time(unsigned long nr)
{
  struct minix_inode * inode = (Inode + nr);
  return (unsigned long) inode->i_time;
}

unsigned short MINIX_i_gid(unsigned long nr)
{
  struct minix_inode * inode = (Inode + nr);
  return (unsigned short) inode->i_gid;
}

unsigned short MINIX_i_links_count(unsigned long nr)
{
  struct minix_inode * inode = (Inode + nr);
  return (unsigned short) inode->i_nlinks;
}

unsigned long MINIX_zoneindex(unsigned long nr, unsigned long znr)
{
  struct minix_inode * inode = (Inode + nr);
  return (unsigned long) inode->i_zone[znr];
}

int MINIX_inode_in_use(unsigned long nr)
{
  return bit(inode_map,nr);
}

int MINIX_zone_in_use(unsigned long nr)
{
  if (nr < sb->first_data_zone) return 1;
  return bit(zone_map,(nr-sb->first_data_zone+1));
}

unsigned long MINIX_null_call()
{
  return 0;
}

struct fs_constants MINIX_fs_constants;

void MINIX_sb_init(char * sb_buffer)
{
  sb->ninodes = Super.s_ninodes;
  sb->nzones = Super.s_nzones;
  sb->imap_blocks = Super.s_imap_blocks;
  sb->zmap_blocks = Super.s_zmap_blocks;
  sb->first_data_zone = Super.s_firstdatazone;
  sb->max_size = Super.s_max_size;
  sb->zonesize = Super.s_log_zone_size;
  sb->blocksize = 1024;
  sb->magic = Super.s_magic;

  sb->I_MAP_SLOTS = MINIX_I_MAP_SLOTS;
  sb->Z_MAP_SLOTS = MINIX_Z_MAP_SLOTS;
  sb->INODES_PER_BLOCK = MINIX_INODES_PER_BLOCK;
  sb->norm_first_data_zone = (sb->imap_blocks+2+sb->zmap_blocks+INODE_BLOCKS);
}

void MINIX_read_tables()
{
  if (inode_map) free(inode_map);
  if (zone_map) free(zone_map);

  inode_map = malloc(sb->blocksize*sb->I_MAP_SLOTS);
  zone_map = malloc(sb->blocksize*sb->Z_MAP_SLOTS);

  memset(inode_map,0,sb->blocksize*sb->I_MAP_SLOTS);
  memset(zone_map,0,sb->blocksize*sb->Z_MAP_SLOTS);
  
  if (sb->zonesize != 0 || sb->blocksize != 1024)
    die("Only 1k blocks/zones supported");
  
  if (!sb->imap_blocks || sb->imap_blocks > sb->I_MAP_SLOTS)
    die("bad s_imap_blocks field in super-block");
  
  if (!sb->zmap_blocks || sb->zmap_blocks > sb->Z_MAP_SLOTS)
    die("bad s_zmap_blocks field in super-block");
  
  inode_buffer = malloc(INODE_BUFFER_SIZE);
  
  if (!inode_buffer)
    die("Unable to allocate buffer for inodes");

  inode_count = malloc(sb->ninodes);
  if (!inode_count)
    die("Unable to allocate buffer for inode count");

  zone_count = malloc(sb->nzones);
  if (!zone_count)
    die("Unable to allocate buffer for zone count");

  if (sb->imap_blocks*sb->blocksize != read(CURR_DEVICE,inode_map,sb->imap_blocks*sb->blocksize))
    die("Unable to read inode map");
  if (sb->zmap_blocks*sb->blocksize != read(CURR_DEVICE,zone_map,sb->zmap_blocks*sb->blocksize))
    die("Unable to read zone map");
  if (INODE_BUFFER_SIZE != read(CURR_DEVICE,inode_buffer,INODE_BUFFER_SIZE))
    die("Unable to read inodes");
  if (sb->norm_first_data_zone != sb->first_data_zone)
    printf("Warning: Firstzone != Norm_firstzone\n");
}

void MINIX_init(char * sb_buffer)
{
  fsc = &MINIX_constants;

  MINIX_sb_init(sb_buffer);

  if (sb->magic == MINIX_SUPER_MAGIC) {
    sb->namelen = 14;
    sb->dirsize = 16;
  } else if (sb->magic == MINIX_SUPER_MAGIC2) {
    sb->namelen = 30;
    sb->dirsize = 32;
  }

  DInode.i_mode = MINIX_i_mode;
  DInode.i_uid = MINIX_i_uid;
  DInode.i_size = MINIX_i_size;
  DInode.i_atime = MINIX_i_time;
  DInode.i_ctime = MINIX_i_time;
  DInode.i_mtime = MINIX_i_time;
  DInode.i_gid = MINIX_i_gid;
  DInode.i_links_count = MINIX_i_links_count;
  DInode.i_zone = MINIX_zoneindex;

  FS_cmd.inode_in_use = MINIX_inode_in_use;
  FS_cmd.zone_in_use = MINIX_zone_in_use;

  MINIX_read_tables();

  (void) check_root();
}

int MINIX_test(char * sb_buffer)
{
   if ((Super.s_magic == MINIX_SUPER_MAGIC)||(Super.s_magic == MINIX_SUPER_MAGIC2)) {
     printf("Found a minixfs on device.\n");
     return 0;
   }
   return -1;
 }

