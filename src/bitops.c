/*
 *  lde/bitops.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: bitops.c,v 1.5 2002/01/10 20:59:15 scottheavner Exp $
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

/* If you are using this on a computer that is not Linux, your best bet (and
 * worst performance) will be to define NO_CLI_STI, but not USE_KERNEL_BITOPS.
 * If you are on an emerging Linux system, you're on your own.  Also, after you
 * have modified this, comment out the warning line below, it's just there for
 * people who don't read documentation.
 */

#ifdef NO_KERNEL_BITOPS


/* Hopefully, cli() and sti() are in asm/system.h for all new 
 *  Linux architectures */
#ifdef HAVE_ASM_SYSTEM_H
#include <asm/system.h>
#endif

/* As of January 2002, lde doesn't modify any bits, so we don't really
 *  nedd cli()/sti().  Also, we're operating on our own memory, so there
 *  isn't much point in locking it, we're single threaded, no one else
 *  would be touching it.  I'm not sure why I'm keeping this here? 
 *  Probably because I just wrote the autoconf macro... */
#ifdef NO_CLI_STI
#define sti()
#define cli()
#endif


#if HAVE_ASM_TYPES_H
#include <asm/types.h>
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

#if 0  /* Set and clear are unused today */
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
#endif

int test_bit(int nr, void * addr)
{
	int		mask;
	const __u32	*ADDR = (const __u32 *) addr;

	ADDR += nr / 32;
	mask = 1 << (nr & 0x1f);
	return ((mask & *ADDR) != 0);
}

#endif	/* defined(USE_KERNEL_BITOPS) */
