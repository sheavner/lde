/*
 *  lde/xiafs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: xiafs.c,v 1.28 2002/01/14 21:09:16 scottheavner Exp $
 */

#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

#include "swiped/linux/minix_fs.h"
#include "swiped/linux/xia_fs.h"
#include "swiped/linux/xia_fs_sb.h"

#include "lde.h"
#include "minix.h"
#include "recover.h"
#include "tty_lde.h"
#include "xiafs.h"

static struct Generic_Inode* XIAFS_read_inode(unsigned long nr);
static int XIAFS_write_inode(unsigned long nr, struct Generic_Inode *GInode);
static char* XIAFS_dir_entry(int i, lde_buffer *block_buffer, unsigned long *inode_nr);
static void XIAFS_sb_init(void * sb_buffer);

static struct inode_fields XIAFS_inode_fields = {
  1, /*   unsigned short i_mode; */
  1, /*   unsigned short i_uid; */
  1, /*   unsigned long  i_size; */
  1, /*   unsigned short i_links_count; */
  1, /*   unsigned short i_gid; */
  1, /*   ()             i_mode_flags; */
  0, /*   unsigned long  i_blocks; */
  1, /*   unsigned long  i_atime; */
  1, /*   unsigned long  i_ctime; */
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
    1, /*   unsigned long  i_zone[9]; */
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

static struct fs_constants XIAFS_constants = {
  _XIAFS_ROOT_INO,              /* int ROOT_INODE */
  (sizeof(struct xiafs_inode)), /* int INODE_SIZE */
  8,                            /* unsigned short N_DIRECT */
  8,                            /* unsigned short N_INDIRECT */
  9,                            /* unsigned short N_2X_INDIRECT */
  0,                            /* unsigned short N_3X_INDIRECT */
  10,                           /* unsigned short N_BLOCKS */
  1,                            /* unsigned long  FIRST_MAP_BLOCK */
  4,                            /* int ZONE_ENTRY_SIZE */
  4,                            /* int INODE_ENTRY_SIZE */
  &XIAFS_inode_fields,
  "xiafs",                      /* char *text_name */
  0                             /* unsigned long supertest_offset */
};

static struct Generic_Inode* XIAFS_read_inode(unsigned long nr)
{
  static struct Generic_Inode GInode;
  static unsigned long XIAFS_last_inode = -1L; 
  struct xiafs_inode *Inode;
  int i;

  if ((nr<1)||(nr>sb->ninodes)) {
    lde_warn("inode (%lu) out of range in XIAFS_read_inode",nr);
    nr = 1;
  }

  if (XIAFS_last_inode == nr) return &GInode;
  XIAFS_last_inode = nr;

  Inode = ((struct xiafs_inode *) inode_buffer) - 1 + nr;

  GInode.i_mode        = (unsigned short) ldeswab16(Inode->i_mode);
  GInode.i_uid         = (unsigned short) ldeswab16(Inode->i_uid);
  GInode.i_size        = (unsigned long)  ldeswab32(Inode->i_size);
  GInode.i_atime       = (unsigned long)  ldeswab32(Inode->i_atime);
  GInode.i_ctime       = (unsigned long)  ldeswab32(Inode->i_ctime);
  GInode.i_mtime       = (unsigned long)  ldeswab32(Inode->i_mtime);
  GInode.i_gid         = (unsigned short) ldeswab16(Inode->i_gid);
  GInode.i_links_count = (unsigned short) ldeswab16(Inode->i_nlinks);
  
  for (i=0; i<XIAFS_constants.N_DIRECT; i++)
    GInode.i_zone[i] = (unsigned short) ldeswab32(Inode->i_zone[i]);
  GInode.i_zone[XIAFS_constants.INDIRECT] = (unsigned short) ldeswab32(Inode->i_ind_zone);
  GInode.i_zone[XIAFS_constants.X2_INDIRECT] = (unsigned short) ldeswab32(Inode->i_dind_zone);
  for (i=XIAFS_constants.N_BLOCKS; i<INODE_BLKS; i++)
    GInode.i_zone[i] = 0UL;

  return &GInode;
}

static int XIAFS_write_inode(unsigned long nr, struct Generic_Inode *GInode)
{
  unsigned long bnr;
  int i;
  struct xiafs_inode *Inode;

  Inode = ((struct xiafs_inode *) inode_buffer) - 1 + nr;

  Inode->i_mode        = (unsigned short) GInode->i_mode;
  Inode->i_uid         = (unsigned short) GInode->i_uid;
  Inode->i_size        = (unsigned long)  GInode->i_size;
  Inode->i_atime       = (unsigned long)  GInode->i_atime;
  Inode->i_ctime       = (unsigned long)  GInode->i_ctime;
  Inode->i_mtime       = (unsigned long)  GInode->i_mtime;
  Inode->i_gid         = (unsigned short) GInode->i_gid;
  Inode->i_nlinks      = (unsigned short) GInode->i_links_count;
  
  for (i=0; i<XIAFS_constants.N_DIRECT; i++)
    Inode->i_zone[i] = (unsigned short) GInode->i_zone[i];
  Inode->i_ind_zone = (unsigned short) GInode->i_zone[XIAFS_constants.INDIRECT];
  Inode->i_dind_zone = (unsigned short) GInode->i_zone[XIAFS_constants.X2_INDIRECT];

  bnr = (nr-1)/sb->INODES_PER_BLOCK + sb->imap_blocks + 1 + sb->zmap_blocks;

  return write_block( bnr, (struct xiafs_inode *) inode_buffer+((nr-1)/sb->INODES_PER_BLOCK) );   
}

/* Could use some optimization maybe??  -- Same as ext2's */
static char* XIAFS_dir_entry(int i, lde_buffer *block_buffer, unsigned long *inode_nr)
{
  static char cname[_XIAFS_NAME_LEN+1];

  int j, name_len;
  void *end;
  struct xiafs_direct *dir;

  dir = (void *) block_buffer->start;
  end = block_buffer->start + block_buffer->size;

  *inode_nr = 0;
  cname[0] = 0;

  /* Directories are variable length, we have to examine all the previous ones to get to the current one */
  for (j=0; j<i; j++) {
    dir = (void *)dir + dir->d_rec_len;
    if ( (void *)dir >= end ) {
      return (cname);
    }
  }

  /* Test for overflow, could be spanning multiple blocks */
  if ( (void *)dir + sizeof(dir->d_ino) <= end ) { 
    *inode_nr = dir->d_ino;
  }

  /* Chance this could overflow ? */
  name_len = (int)dir->d_name_len;
  if ( (void *)dir->d_name + name_len > end ) {
    name_len = end - (void *)dir->d_name;
  }
  if ( name_len > _XIAFS_NAME_LEN ) {
    name_len = _XIAFS_NAME_LEN;
  }
  strncpy(cname, dir->d_name, name_len);
  cname[name_len] = 0;
    
  return cname;


}

static void XIAFS_sb_init(void *sb_buffer)
{
  struct xiafs_super_block *Super;
  Super = sb_buffer;

  sb->ninodes         = ldeswab32(Super->s_ninodes);
  sb->nzones          = ldeswab32(Super->s_nzones);
  sb->imap_blocks     = ldeswab32(Super->s_imap_zones);
  sb->zmap_blocks     = ldeswab32(Super->s_zmap_zones);
  sb->first_data_zone = ldeswab32(Super->s_firstdatazone);
  sb->max_size        = ldeswab32(Super->s_max_size);
  sb->zonesize        = ldeswab32(Super->s_zone_shift);
  sb->blocksize       = 1024;
  sb->magic           = ldeswab32(Super->s_magic);

  sb->I_MAP_SLOTS = _XIAFS_IMAP_SLOTS;
  sb->Z_MAP_SLOTS = _XIAFS_ZMAP_SLOTS;
  sb->INODES_PER_BLOCK = _XIAFS_INODES_PER_BLOCK;
  sb->namelen = _XIAFS_NAME_LEN;
  sb->norm_first_data_zone = (sb->imap_blocks+1+sb->zmap_blocks+INODE_BLOCKS);

  sb->last_block_size = sb->blocksize;
}

void XIAFS_init(void *sb_buffer)
{
  fsc = &XIAFS_constants;

  XIAFS_sb_init(sb_buffer);

  FS_cmd.inode_in_use     = MINIX_inode_in_use;
  FS_cmd.zone_in_use      = MINIX_zone_in_use;
  FS_cmd.is_system_block  = MINIX_is_system_block;

  FS_cmd.dir_entry    = XIAFS_dir_entry;
  FS_cmd.read_inode   = XIAFS_read_inode;
  FS_cmd.write_inode  = XIAFS_write_inode;
  FS_cmd.map_inode    = MINIX_map_inode;
  FS_cmd.map_block    = map_block;

  MINIX_read_tables();

  (void) check_root();
}

int XIAFS_test(void *sb_buffer, int use_offset)
{
  struct xiafs_super_block *Super;
  Super = sb_buffer;

  if ( !use_offset ) {
    use_offset = (int) ( (void *)(&(Super->s_magic)) -
                         (void *)(&(Super->s_zone_size)) ) ;
    if ( *(__u32 *)(sb_buffer + use_offset) == ldeswab32(_XIAFS_SUPER_MAGIC) )
       return 1;
    else
       return 0;
  }

  if (Super->s_magic == ldeswab32(_XIAFS_SUPER_MAGIC)) {
    if (use_offset) lde_warn("Found xia_fs on device");
    return 1;
  }

  return 0;
}
