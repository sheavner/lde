/*
 *  lde/tty_lde.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: tty_lde.h,v 1.1 1994/09/06 00:13:49 sdh Exp $
 */

void log_error(char *echo_string);
void tty_warn(char *fmt, ...);
long read_num(char *cinput);
char * cache_read_block (unsigned long block_nr, int force);
int write_block (unsigned long block_nr, void *data_buffer);
void ddump_block(unsigned long nr);
void dump_block(unsigned long nr);
void dump_inode(unsigned long nr);
char *entry_type(unsigned long imode);
