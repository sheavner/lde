#ifndef _XIA_FS_I_H
#define _XIA_FS_I_H

/*
 * include/linux/xia_fs_i.h
 *
 * Copyright (C) Q. Frank Xia, 1993.
 * 
 * Based on Linus' minix_fs_i.h.
 * Copyright (C) Linus Torvalds, 1991, 1992.
 */

struct xiafs_inode_info
{ /* for data zone pointers */
  __u32 i_zone[8];
  __u32 i_ind_zone;
  __u32 i_dind_zone;
};

#endif /* _XIA_FS_I_H */
