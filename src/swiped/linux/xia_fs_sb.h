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
  uint32_t s_nzones;
  uint32_t s_ninodes;
  uint32_t s_ndatazones;
  uint32_t s_imap_zones;
  uint32_t s_zmap_zones;
  uint32_t s_firstdatazone;
  uint32_t s_zone_shift;
  uint32_t s_max_size;                               /*  32 bytes */
  struct buffer_head *s_imap_buf[_XIAFS_IMAP_SLOTS]; /*  32 bytes */
  struct buffer_head *s_zmap_buf[_XIAFS_ZMAP_SLOTS]; /* 128 bytes */
  uint32_t s_imap_iznr[_XIAFS_IMAP_SLOTS];           /*  32 bytes */
  uint32_t s_zmap_zznr[_XIAFS_ZMAP_SLOTS];           /* 128 bytes */
  uint8_t s_imap_cached;                             /* flag for cached imap */
  uint8_t s_zmap_cached;                             /* flag for cached imap */
};

#endif /* _XIA_FS_SB_H */
