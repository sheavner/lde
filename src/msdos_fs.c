 /*
 *  lde/msdos_fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: msdos_fs.c,v 1.9 1998/06/05 20:55:44 sdh Exp $
 */

/* 
 *  This file contain all the msdos specific code.
 *
 *  There will be a few redefinitions for MSDOS:
 *
 *  Inode => FAT
 */

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <linux/msdos_fs.h>
#include "lde.h"
#include "no_fs.h"
#include "msdos_fs.h"
#include "tty_lde.h"
#include "recover.h"

/* Hack for redefinition of msdos_boot_sector by some rogue patch? */
#ifdef BS_NAMED_FAT
#define msdos_boot_sector fat_boot_sector
#endif

static struct Generic_Inode *DOS_read_inode(unsigned long nr);
static char* DOS_dir_entry(int i, lde_buffer *block_buffer, unsigned long *inode_nr);
static void DOS_sb_init(void * sb_buffer);
static unsigned long DOS_map_inode(unsigned long ino);
static int DOS_write_inode_NOTYET(unsigned long ino,
				struct Generic_Inode *GInode);
static int DOS_one_i__ul(unsigned long nr);
static int DOS_zero_i__ul(unsigned long nr);

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
  { 1, /*   unsigned long  i_zone[0]; */
    1, /*   unsigned long  i_zone[1]; */
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
  2,                            /* int ROOT_INODE */
  2,                            /* int INODE_SIZE */
  2,                            /* unsigned short N_DIRECT */
  0,                            /* unsigned short INDIRECT */
  0,                            /* unsigned short X2_INDIRECT */
  0,                            /* unsigned short X3_INDIRECT */
  2,                            /* unsigned short N_BLOCKS */
  1,                            /* unsigned long  FIRST_MAP_BLOCK */
  4,                            /* int ZONE_ENTRY_SIZE */
  4,                            /* int INODE_ENTRY_SIZE */
  &DOS_inode_fields,
};

static struct Generic_Inode DOS_junk_inode;

/* Always returns 0 */
static int DOS_write_inode_NOTYET(unsigned long ino,
				struct Generic_Inode *GInode)
{
  return 0;
}

/* Returns 1 always */
static int DOS_one_i__ul(unsigned long nr)
{
  return 1;
}

/* Returns 0 always */
static int DOS_zero_i__ul(unsigned long nr)
{
  return 0;
}

static unsigned long DOS_map_inode(unsigned long ino)
{
  unsigned long block;

  /* Find disk block containing FAT entry */
  block = (unsigned long) ino/sb->zonesize + 1UL;
  if (block > sb->ninodes)
    block = 0;
  return block;
}

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
  char * inode_buffer;

  DOS_init_junk_inode();

  if (nr>sb->ninodes) {
    lde_warn("inode (%lu) out of range DOS_read_inode", nr);
    nr = 0;
  }

  DOS_junk_inode.i_zone[0] = (nr-2UL) + sb->zmap_blocks + 1UL;

  inode_buffer = cache_read_block(DOS_map_inode(nr),NULL,CACHEABLE);
  nr = nr % (sb->blocksize/fsc->INODE_SIZE);
  DOS_junk_inode.i_zone[1] = block_pointer(inode_buffer, nr, fsc->INODE_SIZE) + sb->zmap_blocks + 1UL;

  return &DOS_junk_inode;
}

static char* DOS_dir_entry(int i, lde_buffer *block_buffer, unsigned long *inode_nr)
{
  static char cname[MSDOS_NAME+2];
  struct msdos_dir_entry *dir;

  if (i*sb->dirsize >= block_buffer->size) {
    cname[0] = 0;
  } else {
    memset(cname,MSDOS_NAME+2,' ');
    dir = block_buffer->start+(i*sb->dirsize);
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

  fsc->ROOT_INODE = 2;

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

  FS_cmd.inode_in_use = DOS_one_i__ul;
  FS_cmd.zone_in_use = DOS_one_i__ul;
  FS_cmd.zone_is_bad = DOS_zero_i__ul;
  FS_cmd.is_system_block = DOS_zero_i__ul;

  FS_cmd.dir_entry = DOS_dir_entry;
  FS_cmd.read_inode = DOS_read_inode;
  FS_cmd.write_inode = DOS_write_inode_NOTYET;
  FS_cmd.map_inode = DOS_map_inode;
}


int DOS_test(void *sb_buffer)
{
  struct msdos_boot_sector *Boot;
  Boot = sb_buffer;

  if ( !(strncmp(Boot->system_id,"MSDOS",5)) ||
       !(strncmp(Boot->system_id,"IBM  ",5)) ||
       !(strncmp(Boot->system_id,"NWDOS",5)) ||
       !(strncmp(Boot->system_id,"MSWIN",5)) ) {
    lde_warn("Found msdos_fs on device");
    return 1;
  }

  return 0;
}
