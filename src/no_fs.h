/*
 *  lde/no_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.h,v 1.9 2001/11/26 03:10:41 scottheavner Exp $
 */

struct Generic_Inode *NOFS_init_junk_inode(void); /* export to minix.c */
void NOFS_init(void * sb_buffer, unsigned long blocksize);




