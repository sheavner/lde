/*
 *  lde/ext2fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.c,v 1.22 1998/07/05 18:20:33 sdh Exp $
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
 * The multi-architecture support looks like it was just hacked in, there
 * really should be a better way to do it.
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
static int EXT2_is_system_block(unsigned long nr);
static char* EXT2_dir_entry(int i, lde_buffer *block_buffer, 
			    unsigned long *inode_nr);
static void EXT2_read_tables(void);
static void EXT2_sb_init(void * sb_buffer);

/* Haven't defined ACL and stuff because inode mode doesn't do anything 
 * with them.  Should probably see what flags is, sorry, but I don't 
 * claim to be an ext2fs guru. */
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

static unsigned long group_desc_count, group_desc_size;
static struct ext2_group_desc * group_desc = NULL;

/* Compute disk block containing desired inode */
unsigned long EXT2_map_inode(unsigned long ino)
{
  unsigned long group;
  unsigned long block;

  group = (ino - 1) / sb->s_inodes_per_group;
  block = ((ino - 1) % sb->s_inodes_per_group) /
    (sb->blocksize/fsc->INODE_SIZE);
  return (unsigned long) group_desc[group].bg_inode_table + block;
}

/* Read an inode from disk */
static struct Generic_Inode *EXT2_read_inode (unsigned long ino)
{
  static unsigned long EXT2_last_inode = 0; /* cacheable inode */
  static struct Generic_Inode GInode;
  char * inode_buffer;

  if (EXT2_last_inode == ino) return &GInode;
  EXT2_last_inode = ino;

  if ((ino<1)||(ino>sb->ninodes)) {
    lde_warn("inode (%lu) out of range in EXT2_read_inode", ino);
    EXT2_last_inode = ino = 1;
  }

  inode_buffer = cache_read_block(EXT2_map_inode(ino),NULL,CACHEABLE);
  memcpy (&GInode, 
	  ((struct ext2_inode *) inode_buffer +
	   ((ino - 1) % sb->s_inodes_per_group) %
	   (sb->blocksize/fsc->INODE_SIZE)),
	  sizeof (struct ext2_inode));

  return &GInode;
}

/* Write a modified inode back to disk */
static int EXT2_write_inode(unsigned long ino, struct Generic_Inode *GInode)
{
  char * inode_buffer;
  unsigned long blknr;

  blknr = EXT2_map_inode(ino);
  inode_buffer = cache_read_block(blknr,NULL,CACHEABLE);
  memcpy ( ((struct ext2_inode *) inode_buffer +
	    ((ino - 1) % sb->s_inodes_per_group) %
	    (sb->blocksize/fsc->INODE_SIZE)),
	  GInode, sizeof (struct ext2_inode));

  return write_block( blknr, inode_buffer);
}


/* Reads the table indicating used/unused inodes from disk */
static void EXT2_read_inode_bitmap (unsigned long nr)
{
  int i;
  char *local_map = inode_map;
  size_t bytes_read = sb->s_inodes_per_group / 8;

#ifdef READ_PART_TABLES
  static int inode_map_cache = -1;

  i = nr / sb->s_inodes_per_group;
  if ( i != inode_map_cache ) {
    inode_map_cache = i;
#else  
  for (i = 0; i < group_desc_count; i++) {
#endif
    if ( nocache_read_block(group_desc[i].bg_inode_bitmap,
			    local_map, bytes_read) !=
	 bytes_read )
      die ("EXT2_read_tables: read failed in EXT2_read_inode_bitmap");
    local_map += bytes_read;
  }
}

/* Checks if a particular inode is in use */
static int EXT2_inode_in_use(unsigned long nr)
{
  nr--;
#ifdef READ_PART_TABLES
  EXT2_read_inode_bitmap(nr);
  nr %= sb->s_inodes_per_group;
#endif
  return test_bit(nr,inode_map);
}

/* Reads the table indicating used/unused data blocks from disk */
static void EXT2_read_block_bitmap(unsigned long nr)
{
  int i;
  char *local_map = zone_map;

#ifdef READ_PART_TABLES
  static int block_map_cache = -1;

  i = nr / sb->s_blocks_per_group;
  if ( i != block_map_cache ) {
    block_map_cache = i;
#else  
  for (i = 0; i < group_desc_count; i++) {
#endif
    if ( nocache_read_block( group_desc[i].bg_block_bitmap,
			     local_map, sb->s_blocks_per_group / 8) !=
	sb->s_blocks_per_group / 8)
      die ("read failed in EXT2_read_block_bitmap");
    local_map += sb->s_blocks_per_group / 8;
  }
}
  
/* Checks if a data block is part of the ext2 system (i.e. not a data block) */
static int EXT2_is_system_block(unsigned long nr)
{
  int i;

  /* Group descriptor tables and first super block */
  if (nr < (((EXT2_MIN_BLOCK_SIZE)/sb->blocksize)+1) + 
      (group_desc_size+sb->blocksize-1)/sb->blocksize)
    return 1;

  for (i = 0; i < group_desc_count; i++) {
    /* Is it a superblock copy? */
    if (nr==(i*sb->s_blocks_per_group+1))
      return 1;
    /* Is it part of the block map? */
    if ( (nr>group_desc[i].bg_block_bitmap) &&
	 (nr<group_desc[i].bg_block_bitmap+
            ((2*sb->s_blocks_per_group-1)/8+sb->blocksize-1)/sb->blocksize) )
       return 1;
    /* No.  How about the inode map? */
    if ( (nr>group_desc[i].bg_inode_bitmap*sb->blocksize) &&
	 (nr<group_desc[i].bg_inode_bitmap*sb->blocksize+
            ((2*sb->s_inodes_per_group-1)/8+sb->blocksize-1)/sb->blocksize) )
       return 1;
    /* No.  How about the inode table? */
    if ( (nr>group_desc[i].bg_inode_table*sb->blocksize) &&
	 (nr<group_desc[i].bg_inode_table*sb->blocksize+
            ((2*sb->s_inodes_per_group-1)/8+sb->blocksize-1)/sb->blocksize) )
       return 1;
  }

  /* How about that, it wasn't a system block */
  return 0;
}

/* Checks if a particular data block is in use */
static int EXT2_zone_in_use(unsigned long nr)
{
  if (nr < sb->first_data_zone) return 1;
  nr -= sb->first_data_zone;
#ifdef READ_PART_TABLES
  EXT2_read_block_bitmap(nr);
  nr %= sb->s_blocks_per_group;
#endif
  return test_bit(nr,zone_map);
}

/* Could use some optimization maybe?? */
static char* EXT2_dir_entry(int i, lde_buffer *block_buffer,
			    unsigned long *inode_nr)
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
	    block_pointer(bp,(unsigned long)
                               ( (fsc->INODE_ENTRY_SIZE+
                                  sizeof(unsigned short))
                                  /sizeof(unsigned short) ),
			  sizeof(unsigned short)));
  }
  return (EXT2_cname);
}

/* Reads the inode/block in use tables from disk */
static void EXT2_read_tables()
{
  size_t        isize, addr_per_block, inode_blocks_per_group;
  unsigned long desc_loc, desc_blocks;

  /* Free up any memory we may have previously allocated 
   *  (I don't think this will ever happen -- EXT2_read_tables is only
   *   called at startup) */
  if (inode_map)  free(inode_map);
  if (zone_map)   free(zone_map);
  if (group_desc) free(group_desc);
  
  addr_per_block = sb->blocksize/sizeof(unsigned long);
  inode_blocks_per_group = sb->s_inodes_per_group /
                               (sb->blocksize/fsc->INODE_SIZE);
  
  /* Compute number of groups.  Add (sb->s_blocks_per_group - 1) to 
   * account for last group that may be incomplete */
  group_desc_count = (sb->nzones - sb->first_data_zone + 
                        sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;
  if (!group_desc_count)
    die ("No group descriptors.  Something is wrong with this ext2fs?");

  /* Compute number of blocks used by groups 
   * - 1st use desc_blocks as temp variable to compute number
   *   of descriptors per block  */
  desc_blocks = sb->blocksize / sizeof(struct ext2_group_desc);
  desc_blocks = (group_desc_count + desc_blocks - 1) / desc_blocks;

  /* Figure out space for group descriptors in bytes and allocate */
  group_desc_size = desc_blocks * sb->blocksize;
  group_desc = (struct ext2_group_desc *) malloc(group_desc_size);
  if (!group_desc)
    die ("EXT2_read_tables: Unable to allocate buffers for group descriptors");

  /* Compute location of group descriptors on disk and read them in 
   *  (What's the +1 for?) */
  desc_loc = (((EXT2_MIN_BLOCK_SIZE) / sb->blocksize) + 1);
  if ( nocache_read_block(desc_loc, group_desc, group_desc_size) !=
       group_desc_size)
    die ("EXT2_read_tables: Unable to read group descriptors");

  /* Allocate and read in inode map */
  isize = sb->s_inodes_per_group / 8;
#ifndef READ_PART_TABLES
  isize *= group_desc_count;
#endif
  inode_map = (char *) calloc(isize+1, 1);
  if (!inode_map)
    die ("Unable to allocate inodes bitmap");
  EXT2_read_inode_bitmap(0UL);
  
  /* Allocate and read in zone/block map */
  isize = sb->s_blocks_per_group / 8;
#ifndef READ_PART_TABLES
  isize *= group_desc_count;
#endif
  zone_map = (char *) calloc(isize+1, 1);
  if (!zone_map)
    die ("EXT2_read_tables: Unable to allocate blocks bitmap");
  EXT2_read_block_bitmap(0UL);
  
  /* Why do we warn here? */  
  if (sb->first_data_zone != 1UL) {
    lde_warn("Warning: First block (%lu)"
	    " != Normal first block (%lu)",
	    sb->first_data_zone, 1UL);
  }
}

/* Copy superblock info from disk into lde's sb structure */
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

/* After determining that we are working with an ext2 file system,
 * init lde to use the proper function calls, fill in lde's superblock,
 * and read the inode/block in use tables from disk. */
void EXT2_init(void *sb_buffer)
{
  fsc = &EXT2_constants;

  EXT2_sb_init(sb_buffer);

  FS_cmd.inode_in_use     = EXT2_inode_in_use;
  FS_cmd.zone_in_use      = EXT2_zone_in_use;
  FS_cmd.is_system_block  = EXT2_is_system_block;

  FS_cmd.dir_entry    = EXT2_dir_entry;
  FS_cmd.read_inode   = EXT2_read_inode;
  FS_cmd.write_inode  = EXT2_write_inode;
  FS_cmd.map_inode    = EXT2_map_inode;

  EXT2_read_tables();

  (void) check_root();
}

/* Tests if this disk/file has the EXT2MAGIC in the proper location */
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
