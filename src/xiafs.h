/*
 *  lde/xiafs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: xiafs.h,v 1.1 1994/09/05 22:22:08 sdh Exp $
 */

struct Generic_Inode* XIAFS_read_inode(unsigned long nr);
int XIAFS_write_inode(unsigned long nr, struct Generic_Inode *GInode);
char* XIAFS_dir_entry(int i, char *block_buffer, unsigned long *inode_nr);
void XIAFS_sb_init(char * sb_buffer);
int XIAFS_init(char * sb_buffer);
void XIAFS_scrub(int flag);
int XIAFS_test(char * sb_buffer);
