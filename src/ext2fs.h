/*
 *  lde/ext2fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.h,v 1.3 1995/06/01 06:01:10 sdh Exp $
 *
 */

void EXT2_init(void *sb_buffer);
int EXT2_test(void *sb_buffer);
