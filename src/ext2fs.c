/*
 *  lde/ext2fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.c,v 1.4 1994/03/19 20:46:49 sdh Exp $
 *
 *  The following routines were taken almost verbatim from
 *  the e2fsprogs-0.4a package by Remy Card. 
 *    Copyright (C) 1992, 1993  Remy Card <card@masi.ibp.fr> 
 *       EXT2_read_inode_bitmap()
 *       EXT2_read_block_bitmap()
 *       EXT2_read_tables()
 */

#include "lde.h"

#undef Inode
#define Inode (((struct ext2_inode *) inode_buffer)-1)

#undef Super
#define Super (*(struct ext2_super_block *)sb_buffer)

#define NORM_FIRSTBLOCK             1UL
#undef  FIRSTBLOCK
#define FIRSTBLOCK                  sb->first_data_zone
#define L_EXT2_DESC_PER_BLOCK       (sb->blocksize / sizeof(struct ext2_group_desc))

struct fs_constants EXT2_constants = {
  EXT2,                         /* int FS */
  EXT2_ROOT_INO,                /* int ROOT_INODE */
  (sizeof(struct ext2_inode)),  /* int INODE_SIZE */
  EXT2_NDIR_BLOCKS,             /* unsigned short N_DIRECT */
  EXT2_IND_BLOCK,               /* unsigned short INDIRECT */
  EXT2_DIND_BLOCK,              /* unsigned short X2_INDIRECT */
  EXT2_TIND_BLOCK,              /* unsigned short X3_INDIRECT */
  EXT2_N_BLOCKS,                /* unsigned short N_BLOCKS */
  4,                            /* int ZONE_ENTRY_SIZE */
  4,                            /* int INODE_ENTRY_SIZE */
};

unsigned long group_desc_count;
struct ext2_group_desc * group_desc = NULL;

static EXT2_last_inode = 0;
static struct ext2_inode local_inode;

struct ext2_inode * EXT2_read_inode (unsigned long ino)
{
  unsigned long group;
  unsigned long block;
  unsigned long block_nr;
  int i;
  char * buffer;

  if (EXT2_last_inode == ino) return &local_inode;
  EXT2_last_inode = ino;
  group = (ino - 1) / sb->s_inodes_per_group;
  block = ((ino - 1) % sb->s_inodes_per_group) /
    (sb->blocksize/fsc->INODE_SIZE);
  i = ((ino - 1) % sb->s_inodes_per_group) %
    (sb->blocksize/fsc->INODE_SIZE);
  block_nr = group_desc[group].bg_inode_table + block;
  buffer = cache_read_block(block_nr);
  memcpy (&local_inode, (struct ext2_inode *) buffer + i,
	  sizeof (struct ext2_inode));
  return &local_inode;
}

unsigned short EXT2_i_mode(unsigned long nr)
{
  struct ext2_inode *inode;
  inode = EXT2_read_inode(nr);
  return (unsigned short) inode->i_mode;
}

unsigned short EXT2_i_uid(unsigned long nr)
{
  struct ext2_inode *inode;
  inode = EXT2_read_inode(nr);
  return (unsigned short) inode->i_uid;
}

unsigned long EXT2_i_size(unsigned long nr)
{
  struct ext2_inode *inode;
  inode = EXT2_read_inode(nr);
  return (unsigned long) inode->i_size;
}

unsigned long EXT2_i_atime(unsigned long nr)
{
  struct ext2_inode *inode;
  inode = EXT2_read_inode(nr);
  return (unsigned long) inode->i_atime;
}

unsigned long EXT2_i_ctime(unsigned long nr)
{
  struct ext2_inode *inode;
  inode = EXT2_read_inode(nr);
  return (unsigned long) inode->i_ctime;
}

unsigned long EXT2_i_mtime(unsigned long nr)
{
  struct ext2_inode *inode;
  inode = EXT2_read_inode(nr);
  return (unsigned long) inode->i_mtime;
}

unsigned short EXT2_i_gid(unsigned long nr)
{
  struct ext2_inode *inode;
  inode = EXT2_read_inode(nr);
  return (unsigned short) inode->i_gid;
}

unsigned short EXT2_i_links_count(unsigned long nr)
{
  struct ext2_inode *inode;
  inode = EXT2_read_inode(nr);
  return (unsigned short) inode->i_links_count;
}

unsigned long EXT2_zoneindex(unsigned long nr, unsigned long znr)
{
  struct ext2_inode * inode;
  inode = EXT2_read_inode(nr);
  return inode->i_block[znr];
}

#ifndef READ_FULL_TABLES
static int inode_cache = -1;
static int block_cache = -1;
#endif

static void EXT2_read_inode_bitmap (unsigned long nr)
{
  int i;
  
#ifdef READ_FULL_TABLES
  for (i = 0; i < group_desc_count; i++) {
#else  
  i = nr / sb->s_inodes_per_group;
  if ( i != inode_cache ) {
    inode_cache = i;
#endif
    if (lseek (CURR_DEVICE, group_desc[i].bg_inode_bitmap * sb->blocksize,
	       SEEK_SET) !=
	group_desc[i].bg_inode_bitmap * sb->blocksize)
      die ("seek failed in EXT2_read_inode_bitmap");
    if (read (CURR_DEVICE, inode_map, sb->s_inodes_per_group / 8) !=
	sb->s_inodes_per_group / 8)
      die ("read failed in EXT2_read_inode_bitmap");
  }
}

int EXT2_inode_in_use(unsigned long nr)
{
  nr--;
#ifndef READ_FULL_TABLES
  EXT2_read_inode_bitmap(nr);
#endif
  return bit(inode_map,nr%sb->s_inodes_per_group);
}

static void EXT2_read_block_bitmap(unsigned long nr)
{
  int i;

#ifdef READ_FULL_TABLES
  for (i = 0; i < group_desc_count; i++) {
#else  
  i = nr / sb->s_blocks_per_group;
  if ( i != block_cache ) {
    block_cache = i;
#endif
    if (lseek (CURR_DEVICE, group_desc[i].bg_block_bitmap * sb->blocksize,
	       SEEK_SET) !=
	group_desc[i].bg_block_bitmap * sb->blocksize)
      die ("seek failed in EXT2_read_block_bitmap");
    if (read (CURR_DEVICE, zone_map, sb->s_blocks_per_group / 8) !=
	sb->s_blocks_per_group / 8)
      die ("read failed in EXT2_read_block_bitmap");
  }
}

int EXT2_zone_in_use(unsigned long nr)
{
  if (nr < sb->first_data_zone) return 1;
  nr -= sb->first_data_zone;
#ifndef READ_FULL_TABLES
  EXT2_read_block_bitmap(nr);
#endif
  return bit(zone_map,nr%sb->s_blocks_per_group);
}
                                                                                                                                
void EXT2_read_tables()
{
  int isize, addr_per_block, inode_blocks_per_group;
  unsigned long group_desc_size;
  unsigned long desc_blocks;
  char notify[100];
  long desc_loc;

  if (inode_map) free(inode_map);
  if (zone_map) free(zone_map);

  addr_per_block = sb->blocksize/sizeof(unsigned long);
  inode_blocks_per_group = sb->s_inodes_per_group / (sb->blocksize/fsc->INODE_SIZE);
  
  group_desc_count = (sb->nzones - NORM_FIRSTBLOCK) / sb->s_blocks_per_group;
  if ((group_desc_count * sb->s_blocks_per_group) != (sb->nzones - NORM_FIRSTBLOCK))
    group_desc_count++;
  if (group_desc_count % L_EXT2_DESC_PER_BLOCK )
    desc_blocks = (group_desc_count / L_EXT2_DESC_PER_BLOCK) + 1;
  else
    desc_blocks = group_desc_count / L_EXT2_DESC_PER_BLOCK;
  group_desc_size = desc_blocks * sb->blocksize;
  group_desc = malloc (group_desc_size);
  
  if (!group_desc)
    die ("Unable to allocate buffers for group descriptors");
  desc_loc = (((MIN_BLOCK_SIZE) / sb->blocksize) + 1) * sb->blocksize;
  if (lseek (CURR_DEVICE, desc_loc, SEEK_SET) != desc_loc)
    die ("seek failed");
  if (read (CURR_DEVICE, group_desc, group_desc_size) != group_desc_size)
    die ("Unable to read group descriptors");

#ifdef READ_FULL_TABLES
  isize = (sb->ninodes / 8) + 1;
#else
  isize = (sb->s_inodes_per_group / 8) + 1;
#endif
  sb->imap_blocks = (sb->ninodes / 8 / sb->blocksize) + 1;
  inode_map = malloc (isize);
  if (!inode_map)
    die ("Unable to allocate inodes bitmap");
  memset (inode_map, 0, isize);
  EXT2_read_inode_bitmap((unsigned long) 1);

#ifdef READ_FULL_TABLES
  isize = (sb->nzones / 8) + 1;
#else
  isize = (sb->s_blocks_per_group / 8) + 1;
#endif
  sb->zmap_blocks = sb->nzones / 8 / sb->blocksize + 1;
  zone_map = malloc(isize);
  if (!zone_map)
    die ("Unable to allocate blocks bitmap");
  memset (zone_map, 0, isize);
  EXT2_read_block_bitmap((unsigned long) 1);

/*
  bad_map = malloc (((sb->nzones - FIRSTBLOCK) / 8) + 1);
  if (!bad_map)
    die ("Unable to allocate bad block bitmap");
  memset (bad_map, 0, ((sb->nzones - FIRSTBLOCK) / 8) + 1);
 */

  if (NORM_FIRSTBLOCK != FIRSTBLOCK) {
    sprintf(notify, "Warning: First block (%lu)"
	    " != Normal first block (%lu)\n",
	    FIRSTBLOCK, NORM_FIRSTBLOCK);
    warn(notify);
  }
}

void EXT2_sb_init(char * sb_buffer)
{
  double temp;

  sb->ninodes = Super.s_inodes_count;
  sb->nzones = Super.s_blocks_count;
  sb->imap_blocks = 1;
  sb->zmap_blocks = 1;
  sb->first_data_zone = Super.s_first_data_block;
  sb->max_size = 0;
  sb->zonesize = Super.s_log_block_size;
  sb->blocksize = MIN_BLOCK_SIZE << Super.s_log_block_size;
  sb->magic = Super.s_magic;
  sb->s_inodes_per_group = Super.s_inodes_per_group;
  sb->s_blocks_per_group = Super.s_blocks_per_group;

  sb->INODES_PER_BLOCK = sb->blocksize / sizeof (struct ext2_inode);
  sb->namelen = EXT2_NAME_LEN;
  temp = ((double) sb->blocksize) / fsc->ZONE_ENTRY_SIZE;
  sb->max_size = fsc->N_DIRECT + temp * ( 1 + temp + temp * temp);
  sb->norm_first_data_zone = FIRSTBLOCK;
}

void EXT2_init(char * sb_buffer)
{
  fsc = &EXT2_constants;

  EXT2_sb_init(sb_buffer);

  DInode.i_mode = EXT2_i_mode;
  DInode.i_uid = EXT2_i_uid;
  DInode.i_size = EXT2_i_size;
  DInode.i_atime = EXT2_i_atime;
  DInode.i_ctime = EXT2_i_ctime;
  DInode.i_mtime = EXT2_i_mtime;
  DInode.i_gid = EXT2_i_gid;
  DInode.i_links_count = EXT2_i_links_count;
  DInode.i_zone = EXT2_zoneindex;

  FS_cmd.inode_in_use = EXT2_inode_in_use;
  FS_cmd.zone_in_use = EXT2_zone_in_use;

  EXT2_read_tables();

  (void) check_root();
}

int EXT2_test(char * sb_buffer)
{
   if (Super.s_magic == EXT2_SUPER_MAGIC) {
     printf("Found ext2fs on device.\n");
     return 0;
   }
   return -1;
}
