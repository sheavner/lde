/*
 *  lde/bitops.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: bitops.h,v 1.9 2002/01/14 20:58:20 scottheavner Exp $
 *
 */

#if defined(NO_KERNEL_BITOPS)

extern int set_bit(int nr,void * addr);
extern int clear_bit(int nr,void * addr);
extern int test_bit(int nr,void * addr);

#else

#include <asm/bitops.h>

#endif

#if HAVE_EXT2_TEST_BIT
#defibe lde_test_bit ext2_test_bit
#else
#if HAVE_TEST_LE_BIT
#define lde_test_bit test_le_bit
#else
#define lde_test_bit test_bit
#endif
#endif
