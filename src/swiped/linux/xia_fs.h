#ifndef _XIA_FS_H
#define _XIA_FS_H

/*
 * include/linux/xia_fs.h
 *
 * Copyright (C) Q. Frank Xia, 1993.
 *
 * Based on Linus' minix_fs.h.
 * Copyright (C) Linus Torvalds, 1991, 1992.
 */

#define _XIAFS_SUPER_MAGIC 0x012FD16D
#define _XIAFS_ROOT_INO 1
#define _XIAFS_BAD_INO 2

#define _XIAFS_NAME_LEN 248

#define _XIAFS_INODES_PER_BLOCK ((BLOCK_SIZE) / (sizeof(struct xiafs_inode)))

struct xiafs_inode
{ /* 64 bytes */
  uint16_t i_mode;
  uint16_t i_nlinks;
  uint16_t i_uid;
  uint16_t i_gid;
  uint32_t i_size; /* 8 */
  uint32_t i_ctime;
  uint32_t i_atime;
  uint32_t i_mtime;
  uint32_t i_zone[8];
  uint32_t i_ind_zone;
  uint32_t i_dind_zone;
};

/*
 * linux super-block data on disk
 */
struct xiafs_super_block
{
  uint8_t s_boot_segment[512]; /*  1st sector reserved for boot */
  uint32_t s_zone_size;        /*  0: the name says it		 */
  uint32_t s_nzones;           /*  1: volume size, zone aligned */
  uint32_t s_ninodes;          /*  2: # of inodes		 */
  uint32_t s_ndatazones;       /*  3: # of data zones		 */
  uint32_t s_imap_zones;       /*  4: # of imap zones           */
  uint32_t s_zmap_zones;       /*  5: # of zmap zones		 */
  uint32_t s_firstdatazone;    /*  6: first data zone           */
  uint32_t s_zone_shift;       /*  7: z size = 1KB << z shift   */
  uint32_t s_max_size;         /*  8: max size of a single file */
  uint32_t s_reserved0;        /*  9: reserved			 */
  uint32_t s_reserved1;        /* 10: 				 */
  uint32_t s_reserved2;        /* 11:				 */
  uint32_t s_reserved3;        /* 12:				 */
  uint32_t s_firstkernzone;    /* 13: first kernel zone	 */
  uint32_t s_kernzones;        /* 14: kernel size in zones	 */
  uint32_t s_magic;            /* 15: magic number for xiafs    */
};

struct xiafs_direct
{
  uint32_t d_ino;
  uint16_t d_rec_len;
  uint8_t d_name_len;
  int8_t d_name[_XIAFS_NAME_LEN + 1];
};

/*
 * XIA_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define XIA_DIR_PAD 4
#define XIA_DIR_ROUND (XIA_DIR_PAD - 1)
#define XIA_DIR_REC_LEN(name_len)                                              \
  (((name_len) + 8 + XIA_DIR_ROUND) & ~XIA_DIR_ROUND)

#endif /* _XIA_FS_H */
