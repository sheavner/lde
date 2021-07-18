/*
 *  lde/no_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.h,v 1.10 2002/01/30 20:47:32 scottheavner Exp $
 */

struct Generic_Inode *NOFS_init_junk_inode(void); /* export to minix.c */
void NOFS_init(char * sb_buffer, unsigned long blocksize);
unsigned long NOFS_get_device_size(void);




