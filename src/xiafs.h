/*
 *  lde/xiafs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: xiafs.h,v 1.6 2001/11/26 03:10:41 scottheavner Exp $
 */

void XIAFS_init(void *sb_buffer);
int XIAFS_test(void *sb_buffer, int use_offset);
