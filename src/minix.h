/*
 *  lde/minix.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: minix.h,v 1.2 1994/09/06 01:26:04 sdh Exp $
 */

int MINIX_inode_in_use(unsigned long inode_nr);
int MINIX_zone_in_use(unsigned long inode_nr);
void MINIX_read_tables(void);
void MINIX_init(char * sb_buffer);
int MINIX_test(char * sb_buffer);




