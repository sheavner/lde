/*
 *  lde/bitops.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 */

/* If you are using this on a computer that is not Linux, your best bet (and
 * worst performance) will be to define NO_CLI_STI, but not USE_KERNEL_BITOPS.
 * If you are on an emerging Linux system, you're on your own.  Also, after you
 * have modified this, comment out the warning line below, it's just there for
 * people who don't read documentation.
 */
#if defined(linux) && !defined(NO_KERNEL_BITOPS)
#define USE_KERNEL_BITOPS /* If you are using a Linux kernel with <asm/bitops.h> */
#undef  NO_CLI_STI        /* If your system does not support cli()/sti() via <asm/system.h> */
#else
#warning Did you edit bitops.h for your non-Linux machine??? /* */
#undef  USE_KERNEL_BITOPS /* If you are using a Linux kernel with <asm/bitops.h> */
#define NO_CLI_STI        /* If your system does not support cli()/sti() via <asm/system.h> */
#endif

#if defined(USE_KERNEL_BITOPS)
/* We don't use these yet, but to keep the compiler happy, prototype them now */
extern int change_bit(int nr, void * addr);
extern int find_first_zero_bit(void * addr, unsigned size);
extern int find_next_zero_bit (void * addr, int size, int offset);
extern unsigned long ffz(unsigned long word);
#include <asm/bitops.h>
#else
extern int set_bit(int nr,void * addr);
extern int clear_bit(int nr,void * addr);
extern int test_bit(int nr,void * addr);
#endif
