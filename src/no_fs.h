/*
 *  lde/no_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.h,v 1.5 1996/10/12 20:51:11 sdh Exp $
 */

struct Generic_Inode *NOFS_init_junk_inode(void);
void NOFS_init(char * sb_buffer);
int NOFS_null_call(void);
int NOFS_one(unsigned long nr);
unsigned long NOFS_get_device_size(int fd, unsigned long blocksize);




