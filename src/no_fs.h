/*
 *  lde/no_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.h,v 1.2 1994/09/06 01:23:41 sdh Exp $
 */

struct Generic_Inode *NOFS_init_junk_inode(void);
void NOFS_init(char * sb_buffer);


