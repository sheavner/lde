/*
 *  lde/ext2fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.c,v 1.15 1998/01/12 01:32:04 sdh Exp $
 *
 *  The following routines were taken almost verbatim from
 *  the e2fsprogs-0.4a package by Remy Card. 
 *    Copyright (C) 1992, 1993  Remy Card <card@masi.ibp.fr> 
 *       EXT2_read_inode_bitmap()
 *       EXT2_read_block_bitmap()
 *       EXT2_read_tables()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/ext2_fs.h>

/* I'm not going to support these until someone cleans up the ext2_fs.h file.
 * The multi-architecture support looks like it was just hacked in, there really
 * should be a better way to do it.
 */
#undef i_translator
#undef i_frag
#undef i_fsize
#undef i_uid_high
#undef i_gid_high
#undef i_author
#undef i_reserved1
#undef i_reserved2

#include "lde.h"
#include "ext2fs.h"
#include "tty_lde.h"
#include "recover.h"
#include "bitops.h"

static void EXT2_read_inode_bitmap (unsigned long nr);
static void EXT2_read_block_bitmap(unsigned long nr);
static unsigned long EXT2_map_inode(unsigned long ino);
static struct Generic_Inode *EXT2_read_inode (unsigned long ino);
static int EXT2_write_inode(unsigned long ino, struct Generic_Inode *GInode);
static int EXT2_inode_in_use(unsigned long nr);
static int EXT2_zone_in_use(unsigned long nr);
static char* EXT2_dir_entry(int i, lde_buffer *block_buffer, unsigned long *inode_nr);
static void EXT2_read_tables(void);
static void EXT2_sb_init(void * sb_buffer);

/* Haven't defined ACL and stuff because inode mode doesn't do anything with them. */
/* Should probably see what flags is, sorry, but I don't claim to be an ext2fs guru. */
static struct inode_fields EXT2_inode_fields = {
  1, /*   unsigned short i_mode; */
  1, /*   unsigned short i_uid; */
  1, /*   unsigned long  i_size; */
  1, /*   unsigned short i_links_count; */
  1, /*   ()             i_mode_flags; */
  1, /*   unsigned short i_gid; */
  1, /*   unsigned long  i_blocks; */
  1, /*   unsigned long  i_atime; */
  1, /*   unsigned long  i_ctime; */
  1, /*   unsigned long  i_mtime; */
  1, /*   unsigned long  i_dtime; */
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
    1, /*   unsigned long  i_zone[10]; */
    1, /*   unsigned long  i_zone[11]; */
    1, /*   unsigned long  i_zone[12]; */
    1, /*   unsigned long  i_zone[13]; */
    1  /*   unsigned long  i_zone[14]; */
  },
  0, /*   unsigned long  i_version; */
  0, /*   unsigned long  i_file_acl; */
  0, /*   unsigned long  i_dir_acl; */
  0, /*   unsigned long  i_faddr; */
  0, /*   unsigned char  i_frag; */
  0, /*   unsigned char  i_fsize; */
  0, /*   unsigned short i_pad1; */
  0, /*   unsigned long  i_reserved2; */
};

static struct fs_constants EXT2_constants = {
  EXT2,                         /* int FS */
  EXT2_ROOT_INO,                /* int ROOT_INODE */
  (sizeof(struct ext2_inode)),  /* int INODE_SIZE */
  EXT2_NDIR_BLOCKS,             /* unsigned short N_DIRECT */
  EXT2_IND_BLOCK,               /* unsigned short INDIRECT */
  EXT2_DIND_BLOCK,              /* unsigned short X2_INDIRECT */
  EXT2_TIND_BLOCK,              /* unsigned short X3_INDIRECT */
  EXT2_N_BLOCKS,                /* unsigned short N_BLOCKS */
  0,                            /* unsigned long  FIRST_MAP_BLOCK */
  4,                            /* int ZONE_ENTRY_SIZE */
  4,                            /* int INODE_ENTRY_SIZE */
  &EXT2_inode_fields,
};

static unsigned long group_desc_count;
static struct ext2_group_desc * group_desc = NULL;

unsigned long EXT2_map_inode(unsigned long ino)
{
  unsigned long group;
  unsigned long block;

  group = (ino - 1) / sb->s_inodes_per_group;
  block = ((ino - 1) % sb->s_inodes_per_group) /
    (sb->blocksize/fsc->INODE_SIZE);
  return (unsigned long) group_desc[group].bg_inode_table + block;
}

#define INODE_POINTER 

static struct Generic_Inode *EXT2_read_inode (unsigned long ino)
{
  static EXT2_last_inode = 0; /* cacheable inode */
  static struct Generic_Inode GInode;
  char * inode_buffer;

  if (EXT2_last_inode == ino) return &GInode;
  EXT2_last_inode = ino;

  if ((ino<1)||(ino>sb->ninodes)) {
    lde_warn("inode (%lu) out of range in EXT2_read_inode", ino);
    EXT2_last_inode = ino = 1;
  }

  inode_buffer = cache_read_block(EXT2_map_inode(ino),CACHEABLE);
  memcpy (&GInode, 
	  ((struct ext2_inode *) inode_buffer +
	   ((ino - 1) % sb->s_inodes_per_group) %
	   (sb->blocksize/fsc->INODE_SIZE)),
	  sizeof (struct ext2_inode));

  return &GInode;
}

static int EXT2_write_inode(unsigned long ino, struct Generic_Inode *GInode)
{
  char * inode_buffer;
  unsigned long blknr;

  blknr = EXT2_map_inode(ino);
  inode_buffer = cache_read_block(blknr,CACHEABLE);
  memcpy ( ((struct ext2_inode *) inode_buffer +
	    ((ino - 1) % sb->s_inodes_per_group) %
	    (sb->blocksize/fsc->INODE_SIZE)),
	  GInode, sizeof (struct ext2_inode));

  return write_block( blknr, inode_buffer);
}

#ifdef READ_PART_TABLES
static int inode_map_cache = -1;
static int block_map_cache = -1;
#endif

static void EXT2_read_inode_bitmap (unsigned long nr)
{
  int i;
  char *local_map;

  local_map = inode_map;

#ifndef READ_PART_TABLES
  for (i = 0; i < group_desc_count; i++) {
#else  
  i = nr / sb->s_inodes_per_group;
  if ( i != inode_map_cache ) {
    inode_map_cache = i;
#endif
    if (lseek (CURR_DEVICE, group_desc[i].bg_inode_bitmap * sb->blocksize,
	       SEEK_SET) != group_desc[i].bg_inode_bitmap * sb->blocksize)
      die ("seek failed in EXT2_read_inode_bitmap");
    if (read (CURR_DEVICE, local_map, sb->s_inodes_per_group / 8) !=
	sb->s_inodes_per_group / 8)
      die ("read failed in EXT2_read_inode_bitmap");
    local_map += sb->s_inodes_per_group / 8;
  }
}
  
static int EXT2_inode_in_use(unsigned long nr)
{
  nr--;
#ifdef READ_PART_TABLES
  EXT2_read_inode_bitmap(nr);
  return test_bit(inode_map,nr%sb->s_inodes_per_group);
#endif
  return test_bit(nr,inode_map);
}

static void EXT2_read_block_bitmap(unsigned long nr)
{
  int i;
  char *local_map;

  local_map = zone_map;

#ifndef READ_PART_TABLES
  for (i = 0; i < group_desc_count; i++) {
#else  
  i = nr / sb->s_blocks_per_group;
  if ( i != block_map_cache ) {
    block_map_cache = i;
#endif
    if (lseek (CURR_DEVICE, group_desc[i].bg_block_bitmap * sb->blocksize,
	       SEEK_SET) != group_desc[i].bg_block_bitmap * sb->blocksize)
      die ("seek failed in EXT2_read_block_bitmap");
    if (read (CURR_DEVICE, local_map, sb->s_blocks_per_group / 8) !=
	sb->s_blocks_per_group / 8)
      die ("read failed in EXT2_read_block_bitmap");
    local_map += sb->s_blocks_per_group / 8;
  }
}
  
static int EXT2_zone_in_use(unsigned long nr)
{
  if (nr < sb->first_data_zone) return 1;
  nr -= sb->first_data_zone;
#ifdef READ_PART_TABLES
  EXT2_read_block_bitmap(nr);
  return test_bit(nr%sb->s_blocks_per_group,zone_map);
#endif
  return test_bit(nr,zone_map);
}

/* Could use some optimization maybe?? */
static char* EXT2_dir_entry(int i, lde_buffer *block_buffer, unsigned long *inode_nr)
{
  char *bp;
  int j;
  static char EXT2_cname[EXT2_NAME_LEN+1];

  bp = block_buffer->start;

  if (i)
    for (j = 0; j < i ; j++) {
      bp += block_pointer(bp,(unsigned long)(fsc->INODE_ENTRY_SIZE/2),2);
    }
  if ( (void *)bp >= (block_buffer->start + block_buffer->size) ) {
    EXT2_cname[0] = 0;
  } else {
    bzero(EXT2_cname,EXT2_NAME_LEN+1);
    *inode_nr = block_pointer(bp,0UL,fsc->INODE_ENTRY_SIZE);
    strncpy(EXT2_cname, (bp+fsc->INODE_ENTRY_SIZE+2*sizeof(unsigned short)),
	    block_pointer(bp,(unsigned long)( (fsc->INODE_ENTRY_SIZE+sizeof(unsigned short))/sizeof(unsigned short) ),
			  sizeof(unsigned short)));
  }
  return (EXT2_cname);
}

static void EXT2_read_tables()
{
  int isize, addr_per_block, inode_blocks_per_group;
  unsigned long group_desc_size;
  unsigned long desc_blocks;
  long desc_loc;
  
  if (inode_map) free(inode_map);
  if (zone_map) free(zone_map);
  
  addr_per_block = sb->blocksize/sizeof(unsigned long);
  inode_blocks_per_group = sb->s_inodes_per_group / (sb->blocksize/fsc->INODE_SIZE);
  
  group_desc_count = (sb->nzones - sb->first_data_zone) / sb->s_blocks_per_group;
  if ((group_desc_count * sb->s_blocks_per_group) != (sb->nzones - sb->first_data_zone))
    group_desc_count++;
  if (group_desc_count % (sb->blocksize / sizeof(struct ext2_group_desc)) )
    desc_blocks = (group_desc_count / (sb->blocksize / sizeof(struct ext2_group_desc))) + 1;
  else
    desc_blocks = group_desc_count / (sb->blocksize / sizeof(struct ext2_group_desc));
  group_desc_size = desc_blocks * sb->blocksize;
  group_desc = (struct ext2_group_desc *) malloc(group_desc_size);
  
  if (!group_desc)
    die ("Unable to allocate buffers for group descriptors");
  desc_loc = (((EXT2_MIN_BLOCK_SIZE) / sb->blocksize) + 1) * sb->blocksize;
  if (lseek (CURR_DEVICE, desc_loc, SEEK_SET) != desc_loc)
    die ("seek failed");
  if (read (CURR_DEVICE, group_desc, group_desc_size) != group_desc_size)
    die ("Unable to read group descriptors");
  
#ifndef READ_PART_TABLES
  if (sb->ninodes > sb->s_inodes_per_group) /* Allocate room for at least one block */
    isize = (sb->ninodes / 8) + 1;
  else
#endif
    isize = (sb->s_inodes_per_group / 8) + 1;
  inode_map = (char *) malloc(isize);
  if (!inode_map)
    die ("Unable to allocate inodes bitmap");
  memset (inode_map, 0, isize);
  EXT2_read_inode_bitmap(0UL);
  
#ifndef READ_PART_TABLES
  if (sb->nzones > sb->s_blocks_per_group) /* Allocate at least room for one block */
    isize = (sb->nzones / 8) + 1;
  else
#endif
    isize = (sb->s_blocks_per_group / 8) + 1;

  zone_map = (char *) malloc(isize);
  if (!zone_map)
    die ("Unable to allocate blocks bitmap");
  memset (zone_map, 0, isize);
  EXT2_read_block_bitmap(0UL);
  
  /*
     bad_map = malloc (((sb->nzones - sb->first_data_zone) / 8) + 1);
     if (!bad_map)
     die ("Unable to allocate bad block bitmap");
     memset (bad_map, 0, ((sb->nzones - sb->first_data_zone) / 8) + 1);
   */
  
  if (sb->first_data_zone != 1UL) {
    lde_warn("Warning: First block (%lu)"
	    " != Normal first block (%lu)",
	    sb->first_data_zone, 1UL);
  }
}

static void EXT2_sb_init(void *sb_buffer)
{
  double temp;
  struct ext2_super_block *Super;
  Super = (void *)(sb_buffer+1024);

  sb->ninodes            = Super->s_inodes_count;
  sb->nzones             = Super->s_blocks_count;
  sb->first_data_zone    = Super->s_first_data_block;
  sb->max_size           = 0;
  sb->zonesize           = Super->s_log_block_size;
  sb->blocksize          = EXT2_MIN_BLOCK_SIZE << Super->s_log_block_size;
  sb->magic              = Super->s_magic;
  sb->s_inodes_per_group = Super->s_inodes_per_group;
  sb->s_blocks_per_group = Super->s_blocks_per_group;
  sb->imap_blocks        = (sb->ninodes / 8 / sb->blocksize) + 1;
  sb->zmap_blocks        = sb->nzones / 8 / sb->blocksize + 1;

  sb->INODES_PER_BLOCK = sb->blocksize / sizeof (struct ext2_inode);
  sb->namelen = EXT2_NAME_LEN;
  temp = ((double) sb->blocksize) / fsc->ZONE_ENTRY_SIZE;
  sb->max_size = fsc->N_DIRECT + temp * ( 1 + temp + temp * temp);
  sb->norm_first_data_zone = 1UL;

  sb->last_block_size = sb->blocksize;
}

void EXT2_init(void *sb_buffer)
{
  fsc = &EXT2_constants;

  EXT2_sb_init(sb_buffer);

  FS_cmd.inode_in_use = EXT2_inode_in_use;
  FS_cmd.zone_in_use  = EXT2_zone_in_use;
  FS_cmd.dir_entry    = EXT2_dir_entry;
  FS_cmd.read_inode   = EXT2_read_inode;
  FS_cmd.write_inode  = EXT2_write_inode;
  FS_cmd.map_inode    = EXT2_map_inode;

  EXT2_read_tables();

  (void) check_root();
}

int EXT2_test(void *sb_buffer)
{
  struct ext2_super_block *Super;
  Super = (void *)(sb_buffer+1024);

   if (Super->s_magic == EXT2_SUPER_MAGIC) {
     lde_warn("Found ext2fs on device.");
     return 1;
   }
   return 0;
}
