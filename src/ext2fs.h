/*
 *  lde/ext2fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.h,v 1.2 1994/09/06 01:28:26 sdh Exp $
 *
 */

void EXT2_init(char * sb_buffer);
int EXT2_test(char * sb_buffer);
