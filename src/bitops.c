/*
 *  lde/bitops.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  Pulled from Ted's ext2fs v0.5 library code, but trashed the whole
 *  idea of a separate bitops.h and stuck all the inline functions in
 *  here, this got rid of the inline.c and INCLUDE_INLINE_FUNCS
 *  requirements.
 *
 *  bitops.c --- Bitmap frobbing code.
 *
 *  Copyright (C) 1993, 1994 Theodore Ts'o.  This file may be
 *  redistributed under the terms of the GNU Public License.
 * 
 *  Taken from <asm/bitops.h>, Copyright 1992, Linus Torvalds.
 */


/*
 * The inline routines themselves...
 * 
 * If NO_INLINE_FUNCS is defined, then we won't try to do inline
 * functions at all!
 */
#if !defined(NO_INLINE_FUNCS)

#if (defined(__i386__) || defined(__i486__) || defined(__i586__))
/*
 * These are done by inline assembly for speed reasons.....
 *
 * All bitoperations return 0 if the bit was cleared before the
 * operation and != 0 if it was not.  Bit 0 is the LSB of addr; bit 32
 * is the LSB of (addr+1).
 */

/*
 * Some hacks to defeat gcc over-optimizations..
 */
struct __dummy_h { unsigned long a[100]; };
#define ADDR (*(struct __dummy_h *) addr)
#define CONST_ADDR (*(const struct __dummy_h *) addr)	

inline int set_bit(int nr, void * addr)
{
	int oldbit;

	__asm__ __volatile__("btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"r" (nr));
	return oldbit;
}

inline int clear_bit(int nr, void * addr)
{
	int oldbit;

	__asm__ __volatile__("btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"r" (nr));
	return oldbit;
}

inline int test_bit(int nr, const void * addr)
{
	int oldbit;

	__asm__ __volatile__("btl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit)
		:"m" (CONST_ADDR),"r" (nr));
	return oldbit;
}

#undef ADDR

#else	/* i386 */
#define  NO_INLINE_FUNCTIONS
#endif	/* i386 */

#endif  /* !defined(NO_INLINE_FUNCS) */

/* Re-check since we may have reset it in the last block if */
#if defined(NO_INLINE_FUNCS)

/* Hopefully, cli() and sti() are in here for all new Linux architectures */
#include <asm/system.h>

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

int test_bit(int nr, const void * addr)
{
	int		mask;
	const int	*ADDR = (const int *) addr;

	ADDR += nr >> 5;
	mask = 1 << (nr & 0x1f);
	return ((mask & *ADDR) != 0);
}
#endif	/* defined(NO_INLINE_FUNCS) */





