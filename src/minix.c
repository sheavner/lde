/*
 *  lde/minix.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: minix.c,v 1.4 1994/04/01 09:42:53 sdh Exp $
 */

/* 
 *  This file contain all the minix specific code.
 *  Some minix calls are made by the xiafs code.
 */
 
#include "lde.h"

#undef Inode
#define Inode (((struct minix_inode *) inode_buffer)-1+nr)

#undef Super
#define Super (*(struct minix_super_block *)sb_buffer)

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
  1, /*   unsigned long  i_zone[0]; */
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
  0, /*   unsigned long  i_zone[14]; */
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

struct Generic_Inode* MINIX_read_inode(unsigned long nr)
{
  static struct Generic_Inode GInode;
  static unsigned long MINIX_last_inode = -1L; 
  int i;

  if ((nr<1)||(nr>sb->ninodes)) {
    warn("inode out of range in MINIX_read_inode");
    return NOFS_init_junk_inode();
  }

  if (MINIX_last_inode == nr) return &GInode;
  MINIX_last_inode = nr;

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
  for (i=MINIX_constants.N_BLOCKS; i<EXT2_N_BLOCKS; i++)
    GInode.i_zone[i] = 0UL;

  return &GInode;
}

int MINIX_write_inode(unsigned long nr, struct Generic_Inode *GInode)
{
  unsigned long bnr;
  int i;

  Inode->i_mode        = (unsigned short) GInode->i_mode;
  Inode->i_uid         = (unsigned short) GInode->i_uid;
  Inode->i_size        = (unsigned long)  GInode->i_size;
  Inode->i_time        = (unsigned long)  GInode->i_mtime;
  Inode->i_gid         = (unsigned short) GInode->i_gid;
  Inode->i_nlinks      = (unsigned short) GInode->i_links_count;
  
  for (i=0; i<MINIX_constants.N_BLOCKS; i++)
    Inode->i_zone[i] = (unsigned short) GInode->i_zone[i];

  bnr = (nr-1)/sb->INODES_PER_BLOCK + sb->imap_blocks + 2 + sb->zmap_blocks;

  return write_block( bnr, (struct minix_inode *) inode_buffer+((nr-1)/sb->INODES_PER_BLOCK) );   
}

int MINIX_inode_in_use(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return bit(inode_map,nr);
}

int MINIX_zone_in_use(unsigned long nr)
{
  if (nr < sb->first_data_zone) 
    return 1;
  else if ( nr > sb->nzones )
    return 0;
  return bit(zone_map,(nr-sb->first_data_zone+1));
}

char *MINIX_dir_entry(int i, char *block_buffer, unsigned long *inode_nr)
{
  static char cname[32];
  memset(cname,65,32);
  strncpy(cname, block_buffer+(i*sb->dirsize+fsc->INODE_ENTRY_SIZE), sb->namelen);
  *inode_nr = block_pointer(block_buffer,(i*sb->dirsize)/fsc->INODE_ENTRY_SIZE,fsc->INODE_ENTRY_SIZE);
  return (cname);
}

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

  FS_cmd.inode_in_use = MINIX_inode_in_use;
  FS_cmd.zone_in_use = MINIX_zone_in_use;
  FS_cmd.dir_entry = MINIX_dir_entry;
  FS_cmd.read_inode = MINIX_read_inode;
  FS_cmd.write_inode = MINIX_write_inode;

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

