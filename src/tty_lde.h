/*
 *  lde/tty_lde.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: tty_lde.h,v 1.3 1996/06/01 05:00:33 sdh Exp $
 */

#define CACHEABLE 0
#define FORCE_READ 1

void log_error(char *echo_string);
void tty_warn(char *fmt, ...);
void no_warn(char *fmt, ...);
int  tty_mgetch(void);
unsigned long lookup_blocksize(unsigned long nr);
unsigned long read_num(char *cinput);
char * cache_read_block (unsigned long block_nr, int force);
int write_block (unsigned long block_nr, void *data_buffer);
void ddump_block(unsigned long nr);
void dump_block(unsigned long nr);
void dump_inode(unsigned long nr);
char *entry_type(unsigned long imode);
