/*
 *  lde/no_fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: no_fs.h,v 1.1 1994/09/05 22:11:42 sdh Exp $
 */

struct Generic_Inode *NOFS_init_junk_inode(void);
struct Generic_Inode *NOFS_read_inode(unsigned long nr);
unsigned long NOFS_null_call(void);
unsigned long NOFS_one(unsigned long nr);
char* NOFS_dir_entry(int i, char *block_buffer, unsigned long *inode_nr);
void NOFS_sb_init(char * sb_buffer);
void NOFS_init(char * sb_buffer);
