/*
 *  lde/minix.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: minix.h,v 1.4 1996/06/01 04:57:39 sdh Exp $
 */
#ifndef LDE_MINIX_H
#define LDE_MINIX_H

extern int MINIX_inode_in_use(unsigned long inode_nr);
extern int MINIX_zone_in_use(unsigned long inode_nr);
extern unsigned long MINIX_map_inode(unsigned long nr);
extern void MINIX_read_tables(void);
extern void MINIX_init(void *sb_buffer);
extern int MINIX_test(void *sb_buffer);

#endif
