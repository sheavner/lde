/*
 *  lde/msdos_fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: msdos_fs.c,v 1.4 1996/10/11 00:33:04 sdh Exp $
 */

/* 
 *  This file contain all the msdos specific code.
 */

#include <unistd.h>
#include <string.h>
#include <linux/msdos_fs.h>
 
#include "lde.h"
#include "no_fs.h"
#include "msdos_fs.h"

static struct Generic_Inode *DOS_read_inode(unsigned long nr);
static char* DOS_dir_entry(int i, void *block_buffer, unsigned long *inode_nr);
static void DOS_sb_init(void * sb_buffer);

static struct inode_fields DOS_inode_fields = {
  0,   /*   unsigned short i_mode; */
  0,   /*   unsigned short i_uid; */
  0,   /*   unsigned long  i_size; */
  0,   /*   unsigned short i_links_count; */
  0,   /*   ()             i_mode_flags; */
  0,   /*   unsigned short i_gid; */
  0,   /*   unsigned long  i_blocks; */
  0,   /*   unsigned long  i_atime; */
  0,   /*   unsigned long  i_ctime; */
  0,   /*   unsigned long  i_mtime; */
  0,   /*   unsigned long  i_dtime; */
  0,   /*   unsigned long  i_flags; */
  0,   /*   unsigned long  i_reserved1; */
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
  0,   /*   unsigned long  i_version; */
  0,   /*   unsigned long  i_file_acl; */
  0,   /*   unsigned long  i_dir_acl; */
  0,   /*   unsigned long  i_faddr; */
  0,   /*   unsigned char  i_frag; */
  0,   /*   unsigned char  i_fsize; */
  0,   /*   unsigned short i_pad1; */
  1,   /*   unsigned long  i_reserved2[2]; */
};

static struct fs_constants DOS_constants = {
  DOS,                          /* int FS */
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
  &DOS_inode_fields,
};

static struct Generic_Inode DOS_junk_inode;

struct Generic_Inode *DOS_init_junk_inode(void)
{
  int i;

  DOS_junk_inode.i_mode        = 0UL;
  DOS_junk_inode.i_uid         = 0UL;
  DOS_junk_inode.i_size        = 0UL;
  DOS_junk_inode.i_atime       = 0UL;
  DOS_junk_inode.i_ctime       = 0UL;
  DOS_junk_inode.i_mtime       = 0UL;
  DOS_junk_inode.i_gid         = 0UL;
  DOS_junk_inode.i_links_count = 0UL;
  
  for (i=0; i<INODE_BLKS; i++)
    DOS_junk_inode.i_zone[i] = 0UL;

  return &DOS_junk_inode;
}

/* Align values in boot block, converts char[2] values (defined in msdos_fs.h) to integer */
int cvt_c2(char cp[2]);
int cvt_c2(char cp[2])
{
  int i = 0;
  memcpy(&i, cp, 2);
  return i;
}

static struct Generic_Inode *DOS_read_inode(unsigned long nr)
{
  return &DOS_junk_inode;
}

static char* DOS_dir_entry(int i, void *block_buffer, unsigned long *inode_nr)
{
  static char cname[MSDOS_NAME+2];
  struct msdos_dir_entry *dir;

  if (i*sb->dirsize >= sb->blocksize) {
    cname[0] = 0;
  } else {
    memset(cname,MSDOS_NAME+2,' ');
    dir = block_buffer+(i*sb->dirsize);
    strncpy(cname, dir->name, 8); /* File name */
    cname[8] = '.';
    strncpy(&cname[9], dir->ext, 3);  /* File extension */
    cname[MSDOS_NAME+1] = 0;
    *inode_nr = (unsigned long) dir->start;
  }
  return (cname);
}

static void DOS_sb_init(void *sb_buffer)
{
  struct msdos_boot_sector *Boot;
  Boot = sb_buffer;

  sb->blocksize = (unsigned long) cvt_c2(Boot->sector_size);
  if ( cvt_c2(Boot->sectors) )
    sb->nzones = (unsigned long) cvt_c2(Boot->sectors);
  else
    sb->nzones = Boot->total_sect;

  sb->last_block_size = sb->blocksize;

  fsc->ROOT_INODE = 1+2*Boot->fat_length;

  /* In order to prevent division by zeroes, set junk entries to 1 */
  sb->ninodes = sb->nzones/Boot->cluster_size;
  sb->imap_blocks = 1;
  sb->zmap_blocks = 2*Boot->fat_length;
  sb->first_data_zone = 0;
  sb->max_size = 1;
  sb->zonesize = Boot->cluster_size;
  sb->magic = 0;

  sb->I_MAP_SLOTS = 1;
  sb->Z_MAP_SLOTS = 1;
  sb->INODES_PER_BLOCK = 1;
  sb->norm_first_data_zone = 0;
}

void DOS_init(void *sb_buffer)
{
  fsc = &DOS_constants;

  DOS_sb_init(sb_buffer);

  (void) DOS_init_junk_inode();

  sb->namelen = MSDOS_NAME+2;
  sb->dirsize = sizeof(struct msdos_dir_entry);

  FS_cmd.inode_in_use = (int (*)(unsigned long n)) NOFS_one;
  FS_cmd.zone_in_use = (int (*)(unsigned long n)) NOFS_one;
  FS_cmd.dir_entry = DOS_dir_entry;
  FS_cmd.read_inode = DOS_read_inode;
  FS_cmd.write_inode = (int (*)(unsigned long inode_nr, struct Generic_Inode *GInode)) NOFS_null_call;
  FS_cmd.map_inode = (unsigned long (*)(unsigned long n)) NOFS_one;
}


int DOS_test(void *sb_buffer)
{
  struct msdos_boot_sector *Boot;
  Boot = sb_buffer;

  if ( !(strncmp(Boot->system_id,"MSDOS",5)) || !(strncmp(Boot->system_id,"IBM  ",5)) ) {
    lde_warn("Found msdos_fs on device");
    return 1;
  }

  return 0;
}
