/*
 *  lde/no_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.h,v 1.3 1995/06/01 05:55:48 sdh Exp $
 */

struct Generic_Inode *NOFS_init_junk_inode(void);
void NOFS_init(char * sb_buffer);
int NOFS_null_call(void);
int NOFS_one(unsigned long nr);



