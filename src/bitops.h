/*
 *  lde/bitops.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: bitops.h,v 1.8 2002/01/10 20:59:15 scottheavner Exp $
 *
 */

#if defined(NO_KERNEL_BITOPS)

extern int set_bit(int nr,void * addr);
extern int clear_bit(int nr,void * addr);
extern int test_bit(int nr,void * addr);

#else

#include <asm/bitops.h>

#endif
