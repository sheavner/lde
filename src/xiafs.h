/*
 *  lde/xiafs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: xiafs.h,v 1.2 1994/09/06 01:22:43 sdh Exp $
 */

int XIAFS_init(char * sb_buffer);
void XIAFS_scrub(int flag);
int XIAFS_test(char * sb_buffer);
