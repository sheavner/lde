/*
 *  lde/minix.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: minix.h,v 1.3 1995/06/01 05:59:26 sdh Exp $
 */

int MINIX_inode_in_use(unsigned long inode_nr);
int MINIX_zone_in_use(unsigned long inode_nr);
void MINIX_read_tables(void);
void MINIX_init(void *sb_buffer);
int MINIX_test(void *sb_buffer);




