/*
 *  lde/bitops.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  Pulled from Ted's ext2fs v0.5 library code, updated to use
 *  kernel bitops if available.  Otherwise fall back to Ted's C.
 *  For this application, cli() and sti() are not required, but if
 *  you use them to access shared memory, they are a must.
 *
 *  bitops.c --- Bitmap frobbing code.
 *
 *  Copyright (C) 1993, 1994 Theodore Ts'o.  This file may be
 *  redistributed under the terms of the GNU Public License.
 * 
 *  Taken from <asm/bitops.h>, Copyright 1992, Linus Torvalds.
 */

#include "bitops.h"

#ifndef USE_KERNEL_BITOPS

#ifndef NO_CLI_STI
/* Hopefully, cli() and sti() are in here for all new Linux architectures */
#include <asm/system.h>
#else
/* They won't matter for this application, but will if we ever access shared memory */
#define sti()
#define cli()
#endif

/*
 * For the benefit of those who are trying to port Linux to another
 * architecture, here are some C-language equivalents.  You should
 * recode these in the native assmebly language, if at all possible.
 * To guarantee atomicity, these routines call cli() and sti() to
 * disable interrupts while they operate.  (You have to provide inline
 * routines to cli() and sti().)
 *
 * Also note, these routines assume that you have 32 bit integers.
 * You will have to change this if you are trying to port Linux to the
 * Alpha architecture or to a Cray.  :-)
 * 
 * C language equivalents written by Theodore Ts'o, 9/26/92
 */

int set_bit(int nr,void * addr)
{
	int	mask, retval;
	int	*ADDR = (int *) addr;

	ADDR += nr >> 5;
	mask = 1 << (nr & 0x1f);
	cli();
	retval = (mask & *ADDR) != 0;
	*ADDR |= mask;
	sti();
	return retval;
}

int clear_bit(int nr, void * addr)
{
	int	mask, retval;
	int	*ADDR = (int *) addr;

	ADDR += nr >> 5;
	mask = 1 << (nr & 0x1f);
	cli();
	retval = (mask & *ADDR) != 0;
	*ADDR &= ~mask;
	sti();
	return retval;
}

int test_bit(int nr, void * addr)
{
	int		mask;
	const int	*ADDR = (const int *) addr;

	ADDR += nr >> 5;
	mask = 1 << (nr & 0x1f);
	return ((mask & *ADDR) != 0);
}
#endif	/* defined(USE_KERNEL_BITOPS) */
