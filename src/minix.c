/*
 *  lde/minix.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: minix.c,v 1.8 1995/06/01 06:02:54 sdh Exp $
 */

/* 
 *  This file contain all the minix specific code.
 *  Some minix calls are made by the xiafs code.
 */

#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <linux/fs.h>
#include <linux/minix_fs.h> 

#include "minix.h"
#include "lde.h"
#include "no_fs.h"
#include "tty_lde.h"
#include "recover.h"
#include "bitops.h"

static struct Generic_Inode* MINIX_read_inode(unsigned long inode_nr);
static int MINIX_write_inode(unsigned long inode_nr, struct Generic_Inode *GInode);
static char *MINIX_dir_entry(int i, void *block_buffer, unsigned long *inode_nr);
static void MINIX_sb_init(void *sb_buffer);

struct inode_fields MINIX_inode_fields = {
  1, /*   unsigned short i_mode; */
  1, /*   unsigned short i_uid; */
  1, /*   unsigned long  i_size; */
  1, /*   unsigned short i_links_count; */
  1, /*   ()             i_mode_flags; */
  1, /*   unsigned short i_gid; */
  0, /*   unsigned long  i_blocks; */
  0, /*   unsigned long  i_atime; */
  0, /*   unsigned long  i_ctime; */
  1, /*   unsigned long  i_mtime; */
  0, /*   unsigned long  i_dtime; */
  0, /*   unsigned long  i_flags; */
  0, /*   unsigned long  i_reserved1; */
  { 1, /*   unsigned long  i_zone[0]; */
    1, /*   unsigned long  i_zone[1]; */
    1, /*   unsigned long  i_zone[2]; */
    1, /*   unsigned long  i_zone[3]; */
    1, /*   unsigned long  i_zone[4]; */
    1, /*   unsigned long  i_zone[5]; */
    1, /*   unsigned long  i_zone[6]; */
    1, /*   unsigned long  i_zone[7]; */
    1, /*   unsigned long  i_zone[8]; */
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
  0, /*   unsigned long  i_reserved2[2]; */
};

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
  &MINIX_inode_fields,          /* struct * inode_fields */
};

static struct Generic_Inode* MINIX_read_inode(unsigned long inode_nr)
{
  static struct Generic_Inode GInode;
  static unsigned long MINIX_last_inode = -1L;
  struct minix_inode *Inode;
  int i;

  if ((inode_nr<1)||(inode_nr>sb->ninodes)) {
    warn("inode (%lu) out of range in MINIX_read_inode",inode_nr);
    return NOFS_init_junk_inode();
  }

  if (MINIX_last_inode == inode_nr) return &GInode;
  MINIX_last_inode = inode_nr;

  Inode = ((struct minix_inode *) inode_buffer)-1+inode_nr;

  GInode.i_mode        = (unsigned short) Inode->i_mode;
  GInode.i_uid         = (unsigned short) Inode->i_uid;
  GInode.i_size        = (unsigned long)  Inode->i_size;
  GInode.i_atime       = (unsigned long)  Inode->i_time;
  GInode.i_ctime       = (unsigned long)  Inode->i_time;
  GInode.i_dtime       = (unsigned long)  Inode->i_time;
  GInode.i_mtime       = (unsigned long)  Inode->i_time;
  GInode.i_gid         = (unsigned short) Inode->i_gid;
  GInode.i_links_count = (unsigned short) Inode->i_nlinks;
  
  for (i=0; i<MINIX_constants.N_BLOCKS; i++)
    GInode.i_zone[i] = (unsigned short) Inode->i_zone[i];
  for (i=MINIX_constants.N_BLOCKS; i<INODE_BLKS; i++)
    GInode.i_zone[i] = 0UL;

  return &GInode;
}

static int MINIX_write_inode(unsigned long inode_nr, struct Generic_Inode *GInode)
{
  unsigned long bnr;
  int i;
  struct minix_inode *Inode;

  Inode = ((struct minix_inode *) inode_buffer)-1+inode_nr;

  Inode->i_mode        = (unsigned short) GInode->i_mode;
  Inode->i_uid         = (unsigned short) GInode->i_uid;
  Inode->i_size        = (unsigned long)  GInode->i_size;
  Inode->i_time        = (unsigned long)  GInode->i_mtime;
  Inode->i_gid         = (unsigned short) GInode->i_gid;
  Inode->i_nlinks      = (unsigned short) GInode->i_links_count;
  
  for (i=0; i<MINIX_constants.N_BLOCKS; i++)
    Inode->i_zone[i] = (unsigned short) GInode->i_zone[i];

  bnr = (inode_nr-1)/sb->INODES_PER_BLOCK + sb->imap_blocks + 2 + sb->zmap_blocks;

  return write_block( bnr, (struct minix_inode *) inode_buffer+((inode_nr-1)/sb->INODES_PER_BLOCK) );   
}

int MINIX_inode_in_use(unsigned long inode_nr)
{
  if ((!inode_nr)||(inode_nr>sb->ninodes)) inode_nr = 1;
  return test_bit(inode_nr,inode_map);
}

int MINIX_zone_in_use(unsigned long inode_nr)
{
  if (inode_nr < sb->first_data_zone) 
    return 1;
  else if ( inode_nr > sb->nzones )
    return 0;
  return test_bit((inode_nr-sb->first_data_zone+1),zone_map);
}

static char *MINIX_dir_entry(int i, void *block_buffer, unsigned long *inode_nr)
{
  static char cname[32];
  
  if (i*sb->dirsize+fsc->INODE_ENTRY_SIZE >= sb->blocksize) {
    cname[0] = 0;
  } else {
    memset(cname,65,32);
    strncpy(cname, block_buffer+(i*sb->dirsize+fsc->INODE_ENTRY_SIZE), sb->namelen);
    *inode_nr = block_pointer(block_buffer,(i*sb->dirsize)/fsc->INODE_ENTRY_SIZE,fsc->INODE_ENTRY_SIZE);
  }
  return (cname);
}

static void MINIX_sb_init(void *sb_buffer)
{
  struct minix_super_block *Super;
  Super =(void *)(sb_buffer + 1024);

  sb->ninodes         = Super->s_ninodes;
  sb->nzones          = Super->s_nzones;
  sb->imap_blocks     = Super->s_imap_blocks;
  sb->zmap_blocks     = Super->s_zmap_blocks;
  sb->first_data_zone = Super->s_firstdatazone;
  sb->max_size        = Super->s_max_size;
  sb->zonesize        = Super->s_log_zone_size;
  sb->blocksize       = 1024;
  sb->magic           = Super->s_magic;

  sb->I_MAP_SLOTS = MINIX_I_MAP_SLOTS;
  sb->Z_MAP_SLOTS = MINIX_Z_MAP_SLOTS;
  sb->INODES_PER_BLOCK = MINIX_INODES_PER_BLOCK;
  sb->norm_first_data_zone = (sb->imap_blocks+2+sb->zmap_blocks+INODE_BLOCKS);
}

void MINIX_read_tables()
{
  /* Allocate space */
  if (inode_map) free(inode_map);
  if (zone_map) free(zone_map);
  if (inode_buffer) free(inode_buffer);

  inode_map = malloc(sb->blocksize*sb->I_MAP_SLOTS);
  zone_map = malloc(sb->blocksize*sb->Z_MAP_SLOTS);
  inode_buffer = malloc(INODE_BUFFER_SIZE);

  if ((!inode_buffer)||(!zone_map)||(!inode_map))
    die("Unable to allocate buffer space in MINIX_read_tables()");

  /* Check super block info, don't die here -- just print warnings */
  if (sb->zonesize != 0 || sb->blocksize != 1024)
    warn("Only 1k blocks/zones supported");
  
  if (!sb->imap_blocks || sb->imap_blocks > sb->I_MAP_SLOTS)
    warn("bad s_imap_blocks field in super-block");
  
  if (!sb->zmap_blocks || sb->zmap_blocks > sb->Z_MAP_SLOTS)
    warn("bad s_zmap_blocks field in super-block");

  if (sb->norm_first_data_zone != sb->first_data_zone)
    warn("Warning: Firstzone != Norm_firstzone");

  /* Now read in tables */
  if (sb->imap_blocks*sb->blocksize != read(CURR_DEVICE,inode_map,sb->imap_blocks*sb->blocksize)) {
    warn("Unable to read inode map");
    bzero(inode_map, sb->imap_blocks*sb->blocksize);
  }
  if (sb->zmap_blocks*sb->blocksize != read(CURR_DEVICE,zone_map,sb->zmap_blocks*sb->blocksize)) {
    warn("Unable to read zone map");
    bzero(zone_map, sb->zmap_blocks*sb->blocksize);
  }
  if (INODE_BUFFER_SIZE != read(CURR_DEVICE,inode_buffer,INODE_BUFFER_SIZE)) {
    warn("Unable to read inodes");
    bzero(inode_buffer, INODE_BUFFER_SIZE);
  }
}

void MINIX_init(void *sb_buffer)
{
  fsc = &MINIX_constants;

  MINIX_sb_init(sb_buffer);

  /* Support for original and 30 char directories */
  if (sb->magic == MINIX_SUPER_MAGIC) {
    sb->namelen = 14;
    sb->dirsize = 16;
  } else if (sb->magic == MINIX_SUPER_MAGIC2) {
    sb->namelen = 30;
    sb->dirsize = 32;
  }

  FS_cmd.inode_in_use = MINIX_inode_in_use;
  FS_cmd.zone_in_use = MINIX_zone_in_use;
  FS_cmd.dir_entry = MINIX_dir_entry;
  FS_cmd.read_inode = MINIX_read_inode;
  FS_cmd.write_inode = MINIX_write_inode;

  MINIX_read_tables();

  (void) check_root();
}

int MINIX_test(void *buffer)
{
  struct minix_super_block *Super;
  Super = (void *) (buffer + 1024);

  if ( (Super->s_magic == MINIX_SUPER_MAGIC) || (Super->s_magic == MINIX_SUPER_MAGIC2) ) {
    warn("Found a minixfs on device.");
    return 1;
  }

  return 0;
}

