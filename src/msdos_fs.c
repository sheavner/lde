/*
 *  lde/msdos_fs.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: msdos_fs.c,v 1.22 2002/01/30 20:47:32 scottheavner Exp $
 */

/* 
 *  This file contain all the msdos specific code.
 *
 *  There will be a few redefinitions for MSDOS:
 *
 *  Inode => FAT
 */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "swiped/linux/msdos_fs.h"

#include "lde.h"
#include "no_fs.h"
#include "msdos_fs.h"
#include "tty_lde.h"
#include "recover.h"

/* Hack for redefinition of msdos_boot_sector by some rogue patch? */
#ifdef MSDOS_BS_NAMED_FAT
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
static unsigned short align_ushort(char cp[2]);

static struct inode_fields DOS_inode_fields = {
  1,   /*   unsigned short i_mode; */
  0,   /*   unsigned short i_uid; */
  1,   /*   unsigned long  i_size; */
  1,   /*   unsigned short i_links_count; */
  0,   /*   ()             i_mode_flags; */
  0,   /*   unsigned short i_gid; */
  1,   /*   unsigned long  i_blocks; */
  1,   /*   unsigned long  i_atime; */
  1,   /*   unsigned long  i_ctime; */
  1,   /*   unsigned long  i_mtime; */
  0,   /*   unsigned long  i_dtime; */
  0,   /*   unsigned long  i_flags; */
  0,   /*   unsigned long  i_reserved1; */
  { 1, /*   unsigned long  i_zone[0]; */
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
  "msdos",                      /* char *text_name */
  0                             /* unsigned long supertest_offset */
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

/***************** STOLEN FROM LINUX KERNEL *******************/
/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */

static int date_dos2unix(unsigned short time,unsigned short date);
static int date_dos2unix(unsigned short time,unsigned short date)
{
   /* Linear day numbers of the respective 1sts in non-leap years. */
   static int day_n[] = { 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 };
                        /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */

	int month,year,secs;

	month = ((date >> 5) & 15)-1;
	year = date >> 9;
	secs = (time & 31)*2+60*((time >> 5) & 63)+(time >> 11)*3600+86400*
	    ((date & 31)-1+day_n[month]+(year/4)+year*365-((year & 3) == 0 &&
	    month < 2 ? 1 : 0)+3653);
			/* days since 1.1.70 plus 80's leap day */
/*	secs += sys_tz.tz_minuteswest*60; */
/*	if (sys_tz.tz_dsttime) secs -= 3600; */
	return secs;
}
/***************** END: STOLEN FROM LINUX KERNEL *******************/

#if 1
static unsigned long DOS_map_inode(unsigned long ino)
{
  unsigned long block;

  /* Find disk block containing FAT entry */
  block = ino * (unsigned long) fsc->INODE_SIZE / sb->blocksize + 2 * sb->zonesize;  /* WHY 2*zonesize? */
/*  if (block > sb->ninodes) */
/*   block = 0; */
  return block;
}
#endif

struct Generic_Inode *DOS_init_junk_inode(void)
{
  int i;

  DOS_junk_inode.i_mode        = S_IFDIR;  /* Fake all into being dirs */
  DOS_junk_inode.i_uid         = 0UL;
  DOS_junk_inode.i_size        = 1UL;      /* Always putting in 1 block? */
  DOS_junk_inode.i_atime       = 0UL;
  DOS_junk_inode.i_ctime       = 0UL;
  DOS_junk_inode.i_mtime       = 0UL;
  DOS_junk_inode.i_gid         = 0UL;
  DOS_junk_inode.i_links_count = 1UL;
  
  for (i=0; i<INODE_BLKS; i++)
    DOS_junk_inode.i_zone[i] = 0UL;

  return &DOS_junk_inode;
}

/* Align values in boot block, converts char[2] values 
 * (defined in msdos_fs.h) to unsigned short, making sure we observe
 * word boundries */
static unsigned short align_ushort(char cp[2])
{
  unsigned short i = 0;
  memcpy(&i, cp, 2);
  return i;
}

/* Another hack, need to read the inode info from the directory entry */
static unsigned long DOS_dir_inode = 0UL;

static struct Generic_Inode *DOS_read_inode(unsigned long nr)
{
  /* char * inode_buffer; */
  unsigned long nextfat ;

  /* Might have already filled in some of the inode info
   *  from the directory entry we just looked at (nc_dir calls
   *  dir_entry() followed by read_inode().  If so, don't clobber
   *  file type and size info.  This might result in displaying old
   *  info, but the inode chain will be filled in correctly below,
   *  it's just the file attrs that may be displayed out of sync */
  if ( !DOS_dir_inode || (DOS_dir_inode != nr) )
     DOS_init_junk_inode();

  if (nr>sb->ninodes) {
    lde_warn("inode (%lu) out of range DOS_read_inode", nr);
    nr = 0;
  }

  /* Compute offset relative to block 0 on disk for data indexed by FAT */
  DOS_junk_inode.i_zone[0] = nr*sb->zonesize + sb->first_data_zone;

#if 1
  inode_buffer = cache_read_block(DOS_map_inode(nr),NULL,CACHEABLE);
  nr = nr % (sb->blocksize/fsc->INODE_SIZE);
  // DOS_junk_inode.i_zone[1] = block_pointer(inode_buffer, nr, fsc->INODE_SIZE) + sb->zmap_blocks + 1UL;

  nextfat = block_pointer(inode_buffer, nr, fsc->INODE_SIZE);
  if ( nextfat >= 0xFFFFFF8 ) nextfat = 0;
  DOS_junk_inode.i_blocks = nextfat;
  DOS_junk_inode.i_zone[1] = nextfat;
#endif

  return &DOS_junk_inode;
}

static char* DOS_dir_entry(int i, lde_buffer *block_buffer, unsigned long *inode_nr)
{
  static char cname[MSDOS_NAME+2+14];
  struct msdos_dir_entry *dir;
  struct msdos_dir_slot  *slot;

  if (i*sb->dirsize >= block_buffer->size) {
    cname[0] = 0;
  } else {
    dir = block_buffer->start+(i*sb->dirsize);

    *inode_nr = (unsigned long) ldeswab16(dir->start) + (((unsigned long)ldeswab16(dir->starthi))<<16);
    DOS_dir_inode = *inode_nr;

    slot = (void *)dir;
    if ( (slot->attr == 0xF) && (!slot->reserved) ) {
      int i;
      for (i=0; i<5; i++) {
	cname[i] = slot->name0_4[i*2];
      }
      for (i=0; i<6; i++) {
	cname[i+5] = slot->name5_10[i*2];
      }
      cname[11] = slot->name11_12[0];
      cname[12] = slot->name11_12[2];
      cname[13] = 0;
    } else {
      memset(cname,' ',MSDOS_NAME+2);
      strncpy(cname, dir->name, 8); /* File name */
      cname[8] = '.';
      strncpy(&cname[9], dir->ext, 3);  /* File extension */
      cname[MSDOS_NAME+1] = 0;
    }

    DOS_junk_inode.i_mode = S_IRUSR|S_IROTH|S_IRGRP;
    DOS_junk_inode.i_size = ldeswab32(dir->size);
    if (dir->attr&ATTR_DIR) {
       DOS_junk_inode.i_mode |= S_IFDIR|S_IXOTH|S_IXUSR|S_IXGRP;
       /* nc_dir.c:get_inode_info() gets pissy if dir length == 0 */
       DOS_junk_inode.i_size = sb->zonesize*sb->blocksize;
       /* .. seems to show up as inode 0 fairly often -- bump it to 2 */
       if (*inode_nr==0UL)
          *inode_nr = 2UL;
    } else {
       DOS_junk_inode.i_mode |= S_IFREG;
    }
    if (dir->attr&ATTR_SYS)
       DOS_junk_inode.i_mode |= S_IFSOCK;
    if (dir->attr&ATTR_RO)
       DOS_junk_inode.i_mode |= S_IWUSR|S_IWGRP|S_IWOTH;
    DOS_junk_inode.i_ctime = date_dos2unix(ldeswab16(dir->ctime),ldeswab16(dir->cdate));
    DOS_junk_inode.i_atime = date_dos2unix(0,ldeswab16(dir->adate));
    DOS_junk_inode.i_mtime = date_dos2unix(ldeswab16(dir->time),ldeswab16(dir->date));
  }
  return (cname);
}

/* This is used to pull the correct block from the inode block table which
 * should have been copied into zone_index */
static int DOS_map_block(unsigned long zone_index[], unsigned long blknr,
	      unsigned long *mapped_block)
{
  struct Generic_Inode *GInode=NULL;
  *mapped_block = zone_index[0];
  if ( zone_index[0] && zone_index[1] ) {
    GInode = FS_cmd.read_inode(zone_index[1]);
    zone_index[0] = GInode->i_zone[0];
    zone_index[1] = GInode->i_blocks;  /* Really next entry in FAT chain */
    return (EMB_NO_ERROR);
  } else if ( zone_index[0] ) {
    zone_index[0] = 0;
    return (EMB_NO_ERROR);
  } /* else { */
  return (EMB_WAY_OUT_OF_RANGE);
}


static void DOS_sb_init(void *sb_buffer)
{
  struct fat_boot_sector *Boot;
  Boot = sb_buffer;

  sb->blocksize = (unsigned long) ldeswab16(align_ushort(Boot->sector_size));
  sb->nzones = (unsigned long) ldeswab16(align_ushort(Boot->sectors));
  if ( !sb->nzones )
    sb->nzones = ldeswab32(Boot->total_sect);

  sb->last_block_size = sb->blocksize;

  fsc->ROOT_INODE = 2;

  /* In order to prevent division by zeroes, set junk entries to 1 */
  sb->ninodes = sb->nzones/Boot->cluster_size;
  sb->imap_blocks = 1;
  if (!Boot->fats)  /* Could this ever happen? */
     Boot->fats = 2;
  if (ldeswab16(Boot->fat_length))  /* FAT12/16 */
     sb->zmap_blocks = Boot->fats*ldeswab16(Boot->fat_length);
  else {
     sb->zmap_blocks = Boot->fats*ldeswab32(Boot->fat32_length);
     fsc->INODE_SIZE = 4;
  }
  sb->first_data_zone = ldeswab16(Boot->reserved) + sb->zmap_blocks - 2 * Boot->cluster_size;
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
  FS_cmd.map_inode = NULL /* DOS_map_inode */ ;
  FS_cmd.map_block = DOS_map_block;
}


int DOS_test(void *sb_buffer, int use_offset)
{
  struct fat_boot_sector *Boot;
  Boot = sb_buffer;

  if ( !(strncmp(Boot->system_id,"MSDOS",5)) ||
       !(strncmp(Boot->system_id,"IBM  ",5)) ||
       !(strncmp(Boot->system_id,"NWDOS",5)) ||
       !(strncmp(Boot->system_id,"OPENDOS",7)) ||
       !(strncmp(Boot->system_id,"DRDOS",5)) ||
       !(strncmp(Boot->system_id,"MSWIN",5)) ) {
    if (use_offset) lde_warn("Found msdos_fs on device");
    return 1;
  }

  return 0;
}
