/*
 *  lde/bitops.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: bitops.h,v 1.10 2002/01/14 21:01:31 scottheavner Exp $
 *
 */

#if defined(NO_KERNEL_BITOPS)

extern int set_bit(int nr,void * addr);
extern int clear_bit(int nr,void * addr);
extern int test_bit(int nr,void * addr);

#else

#include <asm/bitops.h>

#endif

extern int lde_test_bit(int nr,void * addr);

#if HAVE_EXT2_TEST_BIT
#define lde_test_bit ext2_test_bit
#else
#if HAVE_TEST_LE_BIT
#define lde_test_bit test_le_bit
#else
#define lde_test_bit test_bit
#endif
#endif
