/*
 *  lde/bitops.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 */

/* If you are using this on a computer that is not Linux, your best bet (and
 * worst performance) will be to define NO_CLI_STI, but not USE_KERNEL_BITOPS.
 * If you are on an emerging Linux system, you're on your own.
 */

#ifdef __linux__
#define USE_KERNEL_BITOPS /* If you are using a Linux kernel with <asm/bitops.h> */
#undef  NO_CLI_STI        /* If your system does not support cli()/sti() via <asm/system.h> */
#warning LINUX BITOPS
#else
#undef  USE_KERNEL_BITOPS /* If you are using a Linux kernel with <asm/bitops.h> */
#define NO_CLI_STI        /* If your system does not support cli()/sti() via <asm/system.h> */
#warning ALT_BITOPS
#endif

extern int set_bit(int nr,void * addr);
extern int clear_bit(int nr, void * addr);
extern int test_bit(int nr, void * addr);

#if defined(USE_KERNEL_BITOPS)
/* We don't use these yet, but to keep the compiler happy, prototype them now */
extern int change_bit(int nr, void * addr);
extern int find_first_zero_bit(void * addr, unsigned size);
extern int find_next_zero_bit (void * addr, int size, int offset);
extern unsigned long ffz(unsigned long word);
#include <asm/bitops.h>
#endif
