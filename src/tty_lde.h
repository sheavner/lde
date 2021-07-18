/*
 *  lde/tty_lde.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: tty_lde.h,v 1.7 2002/02/14 19:44:51 scottheavner Exp $
 */

#ifndef TTY_LDE_H
#define TTY_LDE_H

#include "lde.h"

#define LDE_DISKCACHE 20

struct _cached_block
{
  char data[MAX_BLOCK_SIZE]; /* Start the struct with our data buffer.  We
			       * can cast the struct's address to a (char *)
			       * and access the data w/o knowing the structs
			       * members - maintains compatibility with old
			       * version of cache_read_block() -- SO LONG AS NO
			       * ONE TRIES TO GO PAST THE FIRST MAC_BLOCK_SIZE
			       * BYTES */
  int size;                  /* Size of data in this block (0 on error) */
  unsigned long bnr;         /* Where did the data for this block come from?*/
  struct _cached_block *next;
  struct _cached_block *prev;
};
typedef struct _cached_block cached_block;

#define CACHEABLE 1
#define FORCE_READ 2
#define NEVER_CACHE 4
#define CHARBUFFER 8

void log_error(char *echo_string);
void tty_warn(char *fmt, ...);
void no_warn(char *fmt, ...);
int tty_mgetch(void);
unsigned long lookup_blocksize(unsigned long nr);
unsigned long read_num(char *cinput);
void *cache_read_block(unsigned long block_nr, void *dest, int force);
size_t nocache_read_block(unsigned long block_nr, void *dest, size_t size);
void init_disk_cache(void);
unsigned long lde_seek_block(unsigned long block_nr);
int write_block(unsigned long block_nr, void *data_buffer);
void ddump_block(unsigned long nr);
void dump_block(unsigned long nr);
void dump_inode(unsigned long nr);
char *entry_type(unsigned long imode);

#endif
