/*
 *  lde/ext2fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.h,v 1.4 2001/11/26 00:07:23 scottheavner Exp $
 *
 */

void EXT2_init(void *sb_buffer);
int EXT2_test(void *sb_buffer, int use_offset);
