/*
 *  lde/no_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.h,v 1.7 1998/07/03 23:28:47 sdh Exp $
 */

struct Generic_Inode *NOFS_init_junk_inode(void); /* export to minix.c */
void NOFS_init(char * sb_buffer);




