/*
 *  lde/no_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.h,v 1.6 1998/01/23 03:58:46 sdh Exp $
 */

struct Generic_Inode *NOFS_init_junk_inode(void); /* export to minix.c */
void NOFS_init(char * sb_buffer);
unsigned long NOFS_get_device_size(int fd, unsigned long blocksize);




