/*
 *  lde/bitops.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 */

extern int set_bit(int nr,void * addr);
extern int clear_bit(int nr, void * addr);
extern int test_bit(int nr, const void * addr);
