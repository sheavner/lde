/*
 *  lde/msdos_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: msdos_fs.h,v 1.2 2001/11/26 00:07:23 scottheavner Exp $
 */

struct Generic_Inode *DOS_init_junk_inode(void);
void DOS_init(char *sb_buffer);
int DOS_test(char *sb_buffer, int use_offset);


