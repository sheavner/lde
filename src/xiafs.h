/*
 *  lde/xiafs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: xiafs.h,v 1.3 1995/06/01 05:55:31 sdh Exp $
 */

int XIAFS_init(void *sb_buffer);
void XIAFS_scrub(int flag);
int XIAFS_test(void *sb_buffer);
