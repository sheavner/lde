/*
 *  lde/ext2fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.h,v 1.1 1994/09/05 21:32:56 sdh Exp $
 *
 */

unsigned long EXT2_map_inode(unsigned long ino);
struct Generic_Inode *EXT2_read_inode (unsigned long ino);
int EXT2_write_inode(unsigned long ino, struct Generic_Inode *GInode);
int EXT2_inode_in_use(unsigned long nr);
static void EXT2_read_block_bitmap(unsigned long nr);
int EXT2_zone_in_use(unsigned long nr);
char* EXT2_dir_entry(int i, char *block_buffer, unsigned long *inode_nr);
void EXT2_read_tables(void);
void EXT2_sb_init(char * sb_buffer);
void EXT2_init(char * sb_buffer);
int EXT2_test(char * sb_buffer);
