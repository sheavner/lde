/*
 *  lde/xiafs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: xiafs.c,v 1.3 1994/03/23 05:58:31 sdh Exp $
 */

#include "lde.h"

/* minix.c */
void MINIX_read_tables();
int MINIX_inode_in_use();
int MINIX_zone_in_use();

#undef Inode
#define Inode (((struct xiafs_inode *) inode_buffer)-1+nr)

#undef Super
#define Super (*(struct xiafs_super_block *)sb_buffer)

struct fs_constants XIAFS_constants = {
  XIAFS,                        /* int FS */
  _XIAFS_ROOT_INO,              /* int ROOT_INODE */
  (sizeof(struct xiafs_inode)), /* int INODE_SIZE */
  8,                            /* unsigned short N_DIRECT */
  8,                            /* unsigned short N_INDIRECT */
  9,                            /* unsigned short N_2X_INDIRECT */
  0,                            /* unsigned short N_3X_INDIRECT */
  10,                           /* unsigned short N_BLOCKS */
  4,                            /* int ZONE_ENTRY_SIZE */
  4,                            /* int INODE_ENTRY_SIZE */
};

unsigned short XIAFS_i_mode(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return (unsigned short) Inode->i_mode;
}

unsigned short XIAFS_i_uid(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return (unsigned short) Inode->i_uid;
}

unsigned long XIAFS_i_size(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return (unsigned long) Inode->i_size;
}

unsigned long XIAFS_i_atime(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return (unsigned long) Inode->i_atime;
}

unsigned long XIAFS_i_ctime(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return (unsigned long) Inode->i_ctime;
}

unsigned long XIAFS_i_mtime(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return (unsigned long) Inode->i_mtime;
}

unsigned short XIAFS_i_gid(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return (unsigned short) Inode->i_gid;
}

unsigned short XIAFS_i_links_count(unsigned long nr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  return (unsigned short) Inode->i_nlinks;
}

unsigned long XIAFS_zoneindex(unsigned long nr, unsigned long znr)
{
  if ((!nr)||(nr>sb->ninodes)) nr = 1;
  if (znr<XIAFS_constants.N_DIRECT)
    return (unsigned short) Inode->i_zone[znr];
  else if (znr==XIAFS_constants.INDIRECT)
    return (unsigned short) Inode->i_ind_zone;
  else if (znr==XIAFS_constants.X2_INDIRECT)
    return (unsigned short) Inode->i_dind_zone;
  return 0;
}

/* Could use some optimization maybe?? */
char* XIAFS_dir_entry(int i, char *block_buffer, unsigned long *inode_nr)
{
  char *bp;
  int j;
  static char cname[_XIAFS_NAME_LEN];

  bzero(cname,_XIAFS_NAME_LEN);
  bp = block_buffer;

  if (i)
    for (j = 0; j < i ; j++) {
      bp += block_pointer(bp,(unsigned long)(fsc->INODE_ENTRY_SIZE/2),2);
    }
  *inode_nr = block_pointer(bp,0UL,fsc->INODE_ENTRY_SIZE);
  strncpy(cname, (bp+fsc->INODE_ENTRY_SIZE+sizeof(unsigned short)+sizeof(unsigned char)),
	  bp[fsc->INODE_ENTRY_SIZE+sizeof(unsigned short)+sizeof(unsigned char)]);
  return (cname);
}

void XIAFS_sb_init(char * sb_buffer)
{
  sb->ninodes = Super.s_ninodes;
  sb->nzones = Super.s_nzones;
  sb->imap_blocks = Super.s_imap_zones;
  sb->zmap_blocks = Super.s_zmap_zones;
  sb->first_data_zone = Super.s_firstdatazone;
  sb->max_size = Super.s_max_size;
  sb->zonesize = Super.s_zone_shift;
  sb->blocksize = 1024;
  sb->magic = Super.s_magic;

  sb->I_MAP_SLOTS = _XIAFS_IMAP_SLOTS;
  sb->Z_MAP_SLOTS = _XIAFS_ZMAP_SLOTS;
  sb->INODES_PER_BLOCK = _XIAFS_INODES_PER_BLOCK;
  sb->namelen = _XIAFS_NAME_LEN;
  sb->norm_first_data_zone = (sb->imap_blocks+1+sb->zmap_blocks+INODE_BLOCKS);
}

int XIAFS_init(char * sb_buffer)
{
  fsc = &XIAFS_constants;

  XIAFS_sb_init(sb_buffer);

  DInode.i_mode = XIAFS_i_mode;
  DInode.i_uid = XIAFS_i_uid;
  DInode.i_size = XIAFS_i_size;
  DInode.i_atime = XIAFS_i_atime;
  DInode.i_ctime = XIAFS_i_ctime;
  DInode.i_mtime = XIAFS_i_mtime;
  DInode.i_gid = XIAFS_i_gid;
  DInode.i_links_count = XIAFS_i_links_count;
  DInode.i_zone = XIAFS_zoneindex;

  FS_cmd.inode_in_use = MINIX_inode_in_use;
  FS_cmd.zone_in_use = MINIX_zone_in_use;
  FS_cmd.dir_entry = XIAFS_dir_entry;

  MINIX_read_tables();

  return check_root();
}

int XIAFS_test(char * sb_buffer)
{
   if (Super.s_magic == _XIAFS_SUPER_MAGIC) {
     printf("Found xia_fs on device\n");
     return 0;
   }
   return -1;
}


