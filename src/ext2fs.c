/*
 *  lde/ext2fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *  Portions Copyright (C) 2002 John Quirk
 *
 *  $Id: ext2fs.c,v 1.38 2002/02/09 12:57:54 jquirk Exp $
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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#include "lde.h"
#include "swiped/linux/ext2_fs.h"
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
static int EXT2_dir_entry(int i, lde_buffer *block_buffer, lde_dirent *d);
static void EXT2_read_tables(void);
static void EXT2_sb_init(char * sb_buffer);

#ifdef JQDIR
static unsigned long EXT2_next_entry(struct ext2_dir_entry_2 *bp, void *end );

/* this variable holds a pointer to the last directory entry returned */
static struct ext2_dir_entry_2 *dir_last;
static int in_del = 0;        /* processing deleted entry */
static int ext2_check_valid_name(__u8 len, char *name,void *end);
#endif /* JQDIR */

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
  "ext2",                       /* char *text_name */
  1024                          /* unsigned long supertest_offset */
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
  struct ext2_inode *Inode;
  char * inode_buffer;
  int    i;

  if (EXT2_last_inode == ino) return &GInode;
  EXT2_last_inode = ino;

  if ((ino<1)||(ino>sb->ninodes)) {
    lde_warn("inode (%lu) out of range in EXT2_read_inode", ino);
    EXT2_last_inode = ino = 1;
  }

  inode_buffer = cache_read_block(EXT2_map_inode(ino),NULL,CACHEABLE);
  Inode = ( (struct ext2_inode *) inode_buffer +
            ((ino - 1) % sb->s_inodes_per_group) %
            (sb->blocksize/fsc->INODE_SIZE) ) ;
  GInode.i_mode        = (unsigned short) ldeswab16(Inode->i_mode);
  GInode.i_uid         = (unsigned short) ldeswab16(Inode->i_uid);
  GInode.i_size        = (unsigned long)  ldeswab32(Inode->i_size);
  GInode.i_atime       = (unsigned long)  ldeswab32(Inode->i_atime);
  GInode.i_ctime       = (unsigned long)  ldeswab32(Inode->i_ctime);
  GInode.i_mtime       = (unsigned long)  ldeswab32(Inode->i_mtime);
  GInode.i_dtime       = (unsigned long)  ldeswab32(Inode->i_dtime);
  GInode.i_gid         = (unsigned short) ldeswab16(Inode->i_gid);
  GInode.i_links_count = (unsigned short) ldeswab16(Inode->i_links_count);
  GInode.i_blocks      = (unsigned long)  ldeswab32(Inode->i_blocks);
  GInode.i_flags       = (unsigned long)  ldeswab32(Inode->i_flags);
  GInode.i_version     = (unsigned long)  ldeswab32(Inode->i_version);
  GInode.i_file_acl    = (unsigned long)  ldeswab32(Inode->i_file_acl);
  GInode.i_dir_acl     = (unsigned long)  ldeswab32(Inode->i_dir_acl);
  GInode.i_faddr       = (unsigned long)  ldeswab32(Inode->i_faddr);
  GInode.i_frag        = (unsigned char)  Inode->i_frag;
  GInode.i_fsize       = (unsigned char)  Inode->i_fsize;

  for (i=0; i<INODE_BLKS; i++)
    GInode.i_zone[i] = (unsigned long)  ldeswab32(Inode->i_block[i]);

  return &GInode;
}

/* Write a modified inode back to disk */
static int EXT2_write_inode(unsigned long ino, struct Generic_Inode *GInode)
{
  struct ext2_inode *Inode;
  char * inode_buffer;
  unsigned long blknr;
  int i;

  /* read block with inode from disk, so we don't mess up
   * unused fields + other inodes on this block */
  blknr = EXT2_map_inode(ino);
  inode_buffer = cache_read_block(blknr,NULL,CACHEABLE);
  Inode = ( (struct ext2_inode *) inode_buffer +
            ((ino - 1) % sb->s_inodes_per_group) %
            (sb->blocksize/fsc->INODE_SIZE) ) ;

  Inode->i_mode        = ldeswab16(GInode->i_mode);
  Inode->i_uid         = ldeswab16(GInode->i_uid);
  Inode->i_size        = ldeswab32(GInode->i_size);
  Inode->i_atime       = ldeswab32(GInode->i_atime);
  Inode->i_ctime       = ldeswab32(GInode->i_ctime);
  Inode->i_mtime       = ldeswab32(GInode->i_mtime);
  Inode->i_dtime       = ldeswab32(GInode->i_dtime);
  Inode->i_gid         = ldeswab16(GInode->i_gid);
  Inode->i_links_count = ldeswab16(GInode->i_links_count);
  Inode->i_blocks      = ldeswab32(GInode->i_blocks);
  Inode->i_flags       = ldeswab32(GInode->i_flags);
  Inode->i_version     = ldeswab32(GInode->i_version);
  Inode->i_file_acl    = ldeswab32(GInode->i_file_acl);
  Inode->i_dir_acl     = ldeswab32(GInode->i_dir_acl);
  Inode->i_faddr       = ldeswab32(GInode->i_faddr);
  Inode->i_frag        = GInode->i_frag;
  Inode->i_fsize       = GInode->i_fsize;

  for (i=0; i<INODE_BLKS; i++)
    Inode->i_block[i] = ldeswab32(GInode->i_zone[i]);

  return write_block( blknr, inode_buffer);
}


/* Reads the table indicating used/unused inodes from disk */
static void EXT2_read_inode_bitmap (unsigned long nr)
{
  unsigned long i;
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
  return lde_test_bit(nr,inode_map);
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
  return lde_test_bit(nr,zone_map);
}

/* Could use some optimization maybe?? -- same as xiafs's
 * - Update to include return value:
 *    1 when inodenr != 0 or name is non-null
 */
int EXT2_dir_entry(int i, lde_buffer *block_buffer, lde_dirent *d)
{
  static char cname[EXT2_NAME_LEN+1];

  int j, name_len, retval = 0;
  char *end;
  struct ext2_dir_entry_2 *dir;

  dir = (void *) block_buffer->start;
  end = block_buffer->start + block_buffer->size;

  bzero(d,sizeof(lde_dirent));
  d->name = cname;
  cname[0] = 0;

  /* Directories are variable length, we have to examine all the previous ones to get to the current one */
  for (j=0; j<i; j++) {

    /* Test we haven't moved pointer past end of allocated memory */
    if ( (char *)&(dir->rec_len) >= end ) {
      return 0;
    }

#ifdef JQDIR
    if(lde_flags.displaydeleted) {
      dir = (char *)dir + EXT2_next_entry(dir, end);
      d->isdel = EXT2_is_deleted();
    } else
#endif
      dir = (struct ext2_dir_entry_2 *)((char *)dir + ldeswab16(dir->rec_len));

    if ( (char *)dir >= end ) {
      return 0;
    }
  }

  /* Test for overflow, could be spanning multiple blocks */
  if ( (char *)dir + sizeof(dir->inode) <= end ) { 
    d->inode_nr = ldeswab32(dir->inode);
    if (d->inode_nr) retval = 1;
  }

  /* Chance this could overflow ? */
  if ( (char *)&(dir->name_len) < end ) {
    name_len = (int)dir->name_len;
    if ( dir->name + name_len > end ) {
      name_len = end - dir->name;
    }
    if ( name_len > EXT2_NAME_LEN ) {
      name_len = EXT2_NAME_LEN;
    }
    strncpy(cname, dir->name, name_len);
    cname[name_len] = 0;
    if (name_len) retval = 1;
  }

#ifdef JQDIR
  dir_last = dir; /* set so we can access in other code */  
#endif /* JQDIR */

  return retval;
}

/* Reads the inode/block in use tables from disk */
static void EXT2_read_tables()
{
  size_t        isize, addr_per_block, inode_blocks_per_group;
  unsigned long desc_loc, desc_blocks;
  int           i;

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

  for (i = 0; i < group_desc_count; i++) {
    group_desc[i].bg_block_bitmap = ldeswab32(group_desc[i].bg_block_bitmap);
    group_desc[i].bg_inode_bitmap = ldeswab32(group_desc[i].bg_inode_bitmap);
    group_desc[i].bg_inode_table = ldeswab32(group_desc[i].bg_inode_table);
    group_desc[i].bg_free_blocks_count = ldeswab32(group_desc[i].bg_free_blocks_count);
    group_desc[i].bg_free_inodes_count = ldeswab32(group_desc[i].bg_free_inodes_count);
    group_desc[i].bg_used_dirs_count = ldeswab32(group_desc[i].bg_used_dirs_count);
  }

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
static void EXT2_sb_init(char *sb_buffer)
{
  double temp;
  struct ext2_super_block *Super;
  Super = (void *)(sb_buffer+1024);

  sb->ninodes            = ldeswab32(Super->s_inodes_count);
  sb->nzones             = ldeswab32(Super->s_blocks_count);
  sb->first_data_zone    = ldeswab32(Super->s_first_data_block);
  sb->max_size           = 0;
  sb->zonesize           = ldeswab32(Super->s_log_block_size);
  sb->blocksize          = EXT2_MIN_BLOCK_SIZE << sb->zonesize;
  sb->magic              = ldeswab16(Super->s_magic);
  sb->s_inodes_per_group = ldeswab32(Super->s_inodes_per_group);
  sb->s_blocks_per_group = ldeswab32(Super->s_blocks_per_group);
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
void EXT2_init(char *sb_buffer)
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
  FS_cmd.map_block    = map_block;

  EXT2_read_tables();

  (void) check_root();
}

/* Tests if this disk/file has the EXT2MAGIC in the proper location */
int EXT2_test(char *sb_buffer, int use_offset)
{
  struct ext2_super_block *Super;

  if (use_offset)
    Super = (void *)(sb_buffer+EXT2_constants.supertest_offset);
  else
    Super = (void *)(sb_buffer);

   if (Super->s_magic == ldeswab16(EXT2_SUPER_MAGIC)) {
     if (use_offset) lde_warn("Found ext2fs on device.");
     return 1;
   }
   return 0;
}

#ifdef JQDIR
/* This code will attempt to look into the buffer to find a deleted dir
 * entry and try to return a valid buffer pointer. It will do consitancy checks 
 * to see if the deleted entry is ok. 
 */
static unsigned long EXT2_next_entry( struct ext2_dir_entry_2 *bp, void *end )
{
  static struct ext2_dir_entry_2 *tag_bp = 0; /* this is set to if we called as part of a
						 linear dir read */
  static struct ext2_dir_entry_2 *save_entry_sz = 0; /* Place to store entry size
							when looking at deleted
							entries */
	
  unsigned short entry_size; /* Our Calculated entry size */

  if(tag_bp != bp) /* reset state */
    {
      in_del = 0; /* this flag turns del procesing in effect reseting */
    }
  
  entry_size = EXT2_DIR_REC_LEN(bp->name_len);
  /* checks to see if an entry can fit is spare space */
  if( entry_size < ldeswab16(bp->rec_len) && entry_size > EXT2_DIR_REC_LEN(0)) {
    /* Possible deleted entry coming up*/
    if(!in_del) /* starting to transverse deleted enteries save get out
		   pointer if we get lost */
      save_entry_sz = (void *)bp + ldeswab16(bp->rec_len);
    in_del = 1; /* processing deleted entries */
  } else {
    /* not pre deleted entry use entry offical size */
    entry_size = ldeswab16(bp->rec_len);
  }
  /* sanity check test deleted inode to see if valid */
  
  tag_bp = (void *)bp + entry_size; /* look ahead to next entry */

  if ( (void *)tag_bp > end ) return entry_size;

  /* look ahead in buffer to see if inode valid */
  /* 20-01-02 removed test for zeroed inode */
  if(in_del && (tag_bp->inode > sb->ninodes
		|| ext2_check_valid_name(tag_bp->name_len,tag_bp->name,end)
		|| tag_bp->name_len < 1
		|| tag_bp->name_len > sb->namelen) )
    {/* bad inode possibly reading garbage */
      entry_size = ldeswab16(bp->rec_len); /*  */
      tag_bp = (void *)bp+ entry_size; /* set tag to new value */
    }

/*
 * This section checks if we are in deleted processing have we left the deleted
 * entries or have gone past the next valid entry. Before entering the deleted
 * sections we noted the pointer to next valid structure so if our deleted entries
 * end up pointing past this point our valid enteries may get lost if this the
 * case we will adjust the entry size back to the correct value to pick up the valid
 * entry. It will also clear the in_del flag.
 */
  if(in_del)
    {
      if(tag_bp > save_entry_sz )/* opps going to skip valid entries*/
	{
	  entry_size = entry_size - (tag_bp-save_entry_sz);
	  tag_bp = save_entry_sz;
	  in_del = 0;
	}
      else if(tag_bp == save_entry_sz) /* leaving delete area */
	in_del = 0;
    }
  
  return(entry_size);
}


/* This is the undelete code it will do some checks then undelete the file
 * hopefully all will be ok.
 * This function already has an inode so it does need to search the disk for
 * a lost inode as in the recover function. But it does need to check basic things
 * like is the inode still deleted, blocks still free.
 *
 * You can end up with two deleted states first is item removed from directory
 * and inode deleted, the other is inode delete but entry intact. This seems to
 * occur when directory has been deleted, makes our job easier.
 * 
 */
int EXT2_dir_undelete(int entry, lde_buffer *buffer)
{
  unsigned long inode_nr, bnr,temp, inode_nr2;
  int i;
  char *fname;
  struct Generic_Inode *GInode, dotNode;
  struct ext2_dir_entry_2 *ent1 , *ent2;
  lde_dirent d;

  i = 0;
  EXT2_dir_entry(i, buffer, &d);
  fname = d.name;
  inode_nr = d.inode_nr;
  /* A Basic check to make sure the Dot entry is ok and
   * is in fact the dot entry! If it not we maybe dealing with
   * a fragment so its not really a good idea to undelete entry
   */
  if(inode_nr == 0 || fname[0] != '.')
    {
      lde_warn("Bad current directory. Try copy instead");
      return 1;
    }

  GInode = EXT2_read_inode(inode_nr);
  memcpy(&dotNode,GInode,sizeof(dotNode));
  if(GInode->i_links_count == 0)
    {
      lde_warn("Current directory deleted undelete first");
      return 1;
    }
  i = 1;
  EXT2_dir_entry(i,buffer,&d);
  inode_nr = d.inode_nr;
  fname = d.name;
  /* Same basic checks as above but looking for dot dot */

  if(inode_nr == 0 || fname[0] != '.' || fname[1] != '.' )
    {
      lde_warn("Bad parent directory. Try copy instead");
      return 1;
    }

  GInode = EXT2_read_inode(inode_nr);

  if(GInode->i_links_count == 0)
    {
      lde_warn("Parent directory deleted undelete first");
      return 1;
    }

/*
 * If we get here the basic directory info seems valid so now we look
 * for our entry
 */
  EXT2_dir_entry(entry,buffer,&d);
  fname = d.name;
  inode_nr = d.inode_nr;
  ent1 = 	dir_last; /* actual directory entry */
  GInode = EXT2_read_inode(inode_nr);
  /* We can have a possible of three states here if inode is not unlinked
   * 1) Inode has been reused
   * 2) File was had multipul hard links
   * 3) File isn't deleted at all!
   */
  if(GInode->i_links_count != 0)
    {
    if(d.isdel)
      lde_warn("Inode has been reused or was multi linked file");
    else
      lde_warn("File is not deleted nothing to do");
      return 1;
    }
  inode_nr2 = inode_nr;
  if(EXT2_is_deleted()) /* is our entry deleted in dir */
    {
      /* Yes, Now look for last valid entry */
      for( i = 1; i < entry; i++)
	{
	  EXT2_dir_entry(i,buffer,&d);
	  fname = d.name;
	  inode_nr = d.inode_nr;
	  if ((ldeswab16(dir_last->rec_len) + (void *)dir_last) >(void*) ent1)
	    break;
	}
      ent2 = (void *)dir_last + ldeswab16(dir_last->rec_len);
      /* we should have three entries last valid one,
       * deleted one and next one.
       * note to self what if last is end of block.
       */
      if(!(ent2 > ent1))
	{
	  lde_warn("Unable to reconstruct directory entry. Try copy instead");
	  return 1;
	}
      temp = (void *)ent2 - (void *)ent1;
      ent1->rec_len = ldeswab16(temp);
      temp = (void *)ent1 - (void *)dir_last;
      dir_last->rec_len = ldeswab16(temp);
      /* We now use the save dot inode to write out this directory
       * which is why I need it to be undeleted before we start or we could
       * overwriting system data. Again this code asumes that the dot inode
       * is valid with out doing any checks other than those at the start
       */
      i = 0;
      while (i < dotNode.i_blocks) { /* using block because this is kept */
	FS_cmd.map_block(dotNode.i_zone,i,&bnr);
	if (bnr) 
	  write_block(bnr,((buffer->start)+i*sb->blocksize));
	i++;
      }
      
    }
  /* We now have the dir entry restored now we need to fix up inode
   * for now a fairly quick and dirty fixup 
   */
  GInode->i_links_count = 1; /* no science here just undeleted */ 	
  GInode->i_dtime =0;        /* fsck complains if not done */
  /*Check size and block count make size correct for block count */
  if( GInode->i_size == 0 && GInode->i_blocks !=0)
    GInode->i_size = GInode->i_blocks * 512;
  /* Ok basic things done, but blocks have not been checked.
   * The file could contain blocks that are reused. 
   * Now lets write out to the disk!*/
  EXT2_write_inode(inode_nr2, GInode);
  
  return 0;	
}	

int EXT2_is_deleted(void)
{
  return in_del;
}	

static int ext2_check_valid_name(__u8 len, char *name, void *end)
{
  int i;
  for( i=0; i < len; i++, name++)
    {
      if(!isprint(*name)|| (void *)name > end)
	return(1);
    }
  return(0);
}

int EXT2_dir_check_entry(int entry, lde_buffer *buffer)
{
  lde_dirent d;
  struct Generic_Inode *GInode;
  
  if (!EXT2_dir_entry(entry,buffer,&d)) return 0;

  GInode = EXT2_read_inode(d.inode_nr);
  /*Check size and block count make size correct for block count */
  if( GInode->i_size == 0 && GInode->i_blocks !=0)
    GInode->i_size = GInode->i_blocks * 512;
  return check_recover_file(GInode->i_zone,GInode->i_size);
}

#endif /* JQDIR */
