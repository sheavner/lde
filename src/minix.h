/*
 *  lde/minix.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: minix.h,v 1.1 1994/09/05 21:22:07 sdh Exp $
 */

struct Generic_Inode* MINIX_read_inode(unsigned long inode_nr);
int MINIX_write_inode(unsigned long inode_nr, struct Generic_Inode *GInode);
int MINIX_inode_in_use(unsigned long inode_nr);
int MINIX_zone_in_use(unsigned long inode_nr);
char *MINIX_dir_entry(int i, char *block_buffer, unsigned long *inode_nr);
void MINIX_sb_init(char * sb_buffer);
void MINIX_read_tables();
void MINIX_init(char * sb_buffer);
int MINIX_test(char * sb_buffer);




