/*
 *  lde/msdos_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: msdos_fs.h,v 1.1 1994/11/20 21:11:29 sdh Exp $
 */

struct Generic_Inode *DOS_init_junk_inode(void);
void DOS_init(void *sb_buffer);
int DOS_test(void *sb_buffer);


