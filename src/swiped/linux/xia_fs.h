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
#define _XIAFS_BAD_INO  2

#define _XIAFS_NAME_LEN 248

#define _XIAFS_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof(struct xiafs_inode)))

struct xiafs_inode {		/* 64 bytes */
    __u16  i_mode;
    __u16  i_nlinks;
    __u16  i_uid;
    __u16  i_gid;
    __u32  i_size;		/* 8 */
    __u32  i_ctime;
    __u32  i_atime;
    __u32  i_mtime;
    __u32  i_zone[8];
    __u32  i_ind_zone;
    __u32  i_dind_zone;
};

/*
 * linux super-block data on disk
 */
struct xiafs_super_block {
    __u8    s_boot_segment[512];	/*  1st sector reserved for boot */
    __u32  s_zone_size;		/*  0: the name says it		 */
    __u32  s_nzones;			/*  1: volume size, zone aligned */ 
    __u32  s_ninodes;			/*  2: # of inodes		 */
    __u32  s_ndatazones;		/*  3: # of data zones		 */
    __u32  s_imap_zones;		/*  4: # of imap zones           */
    __u32  s_zmap_zones;		/*  5: # of zmap zones		 */
    __u32  s_firstdatazone;		/*  6: first data zone           */
    __u32  s_zone_shift;		/*  7: z size = 1KB << z shift   */
    __u32  s_max_size;			/*  8: max size of a single file */
    __u32  s_reserved0;		/*  9: reserved			 */
    __u32  s_reserved1;		/* 10: 				 */
    __u32  s_reserved2;		/* 11:				 */
    __u32  s_reserved3;		/* 12:				 */
    __u32  s_firstkernzone;		/* 13: first kernel zone	 */
    __u32  s_kernzones;		/* 14: kernel size in zones	 */
    __u32  s_magic;			/* 15: magic number for xiafs    */
};

struct xiafs_direct {
    __u32  d_ino;
    __u16  d_rec_len;
    __u8   d_name_len;
    __s8   d_name[_XIAFS_NAME_LEN+1];
};

/*
 * XIA_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define XIA_DIR_PAD		 	4
#define XIA_DIR_ROUND 			(XIA_DIR_PAD - 1)
#define XIA_DIR_REC_LEN(name_len)	(((name_len) + 8 + XIA_DIR_ROUND) & \
					 ~XIA_DIR_ROUND)

#endif  /* _XIA_FS_H */








