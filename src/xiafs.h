/*
 *  lde/xiafs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: xiafs.h,v 1.5 2001/11/26 00:07:23 scottheavner Exp $
 */

int XIAFS_init(void *sb_buffer);
int XIAFS_test(void *sb_buffer, int use_offset);
