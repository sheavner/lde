#ifndef _XIA_FS_SB_H
#define _XIA_FS_SB_H

/*
 * include/linux/xia_fs_sb.h
 *
 * Copyright (C) Q. Frank Xia, 1993.
 *
 * Based on Linus' minix_fs_sb.h.
 * Copyright (C) Linus Torvalds, 1991, 1992.
 */

#define _XIAFS_IMAP_SLOTS 8
#define _XIAFS_ZMAP_SLOTS 32

struct xiafs_sb_info
{
  __u32 s_nzones;
  __u32 s_ninodes;
  __u32 s_ndatazones;
  __u32 s_imap_zones;
  __u32 s_zmap_zones;
  __u32 s_firstdatazone;
  __u32 s_zone_shift;
  __u32 s_max_size;                                  /*  32 bytes */
  struct buffer_head *s_imap_buf[_XIAFS_IMAP_SLOTS]; /*  32 bytes */
  struct buffer_head *s_zmap_buf[_XIAFS_ZMAP_SLOTS]; /* 128 bytes */
  __u32 s_imap_iznr[_XIAFS_IMAP_SLOTS];              /*  32 bytes */
  __u32 s_zmap_zznr[_XIAFS_ZMAP_SLOTS];              /* 128 bytes */
  __u8 s_imap_cached;                                /* flag for cached imap */
  __u8 s_zmap_cached;                                /* flag for cached imap */
};

#endif /* _XIA_FS_SB_H */
