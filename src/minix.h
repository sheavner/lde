/*
 *  lde/minix.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: minix.h,v 1.5 1998/06/20 17:47:35 sdh Exp $
 */
#ifndef LDE_MINIX_H
#define LDE_MINIX_H

extern int MINIX_inode_in_use(unsigned long inode_nr);
extern int MINIX_zone_in_use(unsigned long inode_nr);
extern int MINIX_is_system_block(unsigned long nr);
extern unsigned long MINIX_map_inode(unsigned long nr);
extern void MINIX_read_tables(void);
extern void MINIX_init(void *sb_buffer);
extern int MINIX_test(void *sb_buffer);

#endif
