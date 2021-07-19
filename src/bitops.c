/*
 *  lde/bitops.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: bitops.c,v 1.6 2003/12/07 02:47:52 scottheavner Exp $
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
#include "lde.h"

int lde_test_bit(int nr, void *addr)
{
  int mask;
  const uint32_t *ADDR = (const uint32_t *)addr;

  ADDR += nr / 32;
  mask = 1 << (nr & 0x1f);
  return ((mask & ldeswab32(*ADDR)) != 0);
}
