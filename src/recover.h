/*
 *  lde/recover.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: recover.h,v 1.1 1994/09/06 01:23:12 sdh Exp $
 */

unsigned long block_pointer(unsigned char *ind, unsigned long blknr, int zone_entry_size);
unsigned long map_block(unsigned long zone_index[], unsigned long blknr);
void recover_file(int fp,unsigned long zone_index[]);
unsigned long find_inode(unsigned long nr);
void parse_grep(void);
void search_fs(unsigned char *search_string, int search_len);
