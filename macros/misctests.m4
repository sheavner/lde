
dnl -------------- Linux kernel based bitops routines ---------------------

dnl Check that <asm/bitops.h> exists and works
AC_DEFUN(AC_CHECK_KERNEL_BITOPS,[
        AC_CACHE_CHECK(for asm/bitops.h with usable set_bit(), 
                       ac_cv_has_asmbitops,
                       AC_TRY_LINK([#include <asm/bitops.h>],
                                   [int i; set_bit(1,&i);return i;],
                                   ac_cv_has_asmbitops="yes",
                                   ac_cv_has_asmbitops="no"
                        ) 
        )
        if test x$ac_cv_has_asmbitops = xno ; then
		AC_CHECK_KERNEL_CLISTI
                AC_DEFINE(NO_KERNEL_BITOPS)
        fi
])

dnl Check for cli/sti functions
AC_DEFUN(AC_CHECK_KERNEL_CLISTI,[
	AC_CHECK_HEADERS(asm/system.h)
        AC_CACHE_CHECK(for cli()/sti() functions,
                       ac_cv_has_clisti,
                       AC_TRY_LINK([
#ifdef HAVE_ASM_SYSTEM_H
#include <asm/system.h>
#endif
],
                                   [cli(); sti(); return 0;],
                                   ac_cv_has_clisti="yes",
                                   ac_cv_has_clisti="no"
                        ) 
        )
        if test x$ac_cv_has_clisti = xno ; then
                AC_DEFINE(NO_CLI_STI)
        fi
])


dnl -------------- Check if we can supress -Wall ------------------------
dnl Check if compiler accepts -w (supress all warnings)
AC_SUBST(shutup)
AC_DEFUN(AC_CC_NOWARN,[
        AC_CACHE_CHECK(whether ${CC-cc} accepts -w, ac_cv_prog_cc_w,
                [echo 'void f(){}' > conftest.c
                 if test -z "`${CC-cc} -w -c conftest.c 2>&1`"; then
                        ac_cv_prog_cc_w=yes
                 else
                        ac_cv_prog_cc_w=no
                 fi
                 rm -f conftest*
                ])
        if test x$ac_cv_prog_cc_w = xyes ; then
                shutup="-w"
        fi
])

dnl -------------- Check variable sizes ------------------------
AC_DEFUN(ACLDE_SETVARSIZES,[
	AC_CHECK_HEADERS(asm/types.h,[
        	AC_CACHE_CHECK([for __u8 in <asm/types.h>],
				ac_cv_lde_haveu8,AC_TRY_COMPILE([#include <asm/types.h>],
				[__u8 i=0;],[ac_cv_lde_haveu8=yes],[ac_cv_lde_haveu8=no]))
        	AC_CACHE_CHECK([for __s8 in <asm/types.h>],
				ac_cv_lde_haves8,AC_TRY_COMPILE([#include <asm/types.h>],
				[__s8 i=0;],[ac_cv_lde_haves8=yes],[ac_cv_lde_haves8=no]))
        	AC_CACHE_CHECK([for __u16 in <asm/types.h>],
				ac_cv_lde_haveu16,AC_TRY_COMPILE([#include <asm/types.h>],
				[__u16 i=0;],[ac_cv_lde_haveu16=yes],[ac_cv_lde_haveu16=no]))
        	AC_CACHE_CHECK([for __s16 in <asm/types.h>],
				ac_cv_lde_haves16,AC_TRY_COMPILE([#include <asm/types.h>],
				[__s16 i=0;],[ac_cv_lde_haves16=yes],[ac_cv_lde_haves16=no]))
        	AC_CACHE_CHECK([for __u32 in <asm/types.h>],
				ac_cv_lde_haveu32,AC_TRY_COMPILE([#include <asm/types.h>],
				[__u32 i=0;],[ac_cv_lde_haveu32=yes],[ac_cv_lde_haveu32=no]))
        	AC_CACHE_CHECK([for __s32 in <asm/types.h>],
				ac_cv_lde_haves32,AC_TRY_COMPILE([#include <asm/types.h>],
				[__s32 i=0;],[ac_cv_lde_haves32=yes],[ac_cv_lde_haves32=no]))
        	AC_CACHE_CHECK([for __u64 in <asm/types.h>],
				ac_cv_lde_haveu64,AC_TRY_COMPILE([#include <asm/types.h>],
				[__u64 i=0;],[ac_cv_lde_haveu64=yes],[ac_cv_lde_haveu64=no]))
        	AC_CACHE_CHECK([for __s64 in <asm/types.h>],
				ac_cv_lde_haves64,AC_TRY_COMPILE([#include <asm/types.h>],
				[__s64 i=0;],[ac_cv_lde_haves64=yes],[ac_cv_lde_haves64=no]))
	])

        if test x$ac_cv_lde_haveu8 != xyes; then
		AC_CHECK_SIZEOF(unsigned char,0)
       		if test x$ac_cv_sizeof_unsigned_char = x1 ; then
			AC_CHECK_TYPE(__u8, unsigned char)
       		else
			AC_CHECK_SIZEOF(char,0)
			if test x$ac_cv_sizeof_char = x1 ; then
				AC_CHECK_TYPE(__u8, char)
			else
				AC_MSG_ERROR([Can't determine 8-bit unsigned type for this machine])
       			fi
		fi
	fi
        if test x$ac_cv_lde_haves8 != xyes; then
		AC_CHECK_SIZEOF(signed char,0)
       		if test x$ac_cv_sizeof_signed_char = x1 ; then
			AC_CHECK_TYPE(__s8, signed char)
       		else
			if test x$ac_cv_sizeof_char != x0 ; then
				AC_CHECK_SIZEOF(char,0)
			fi
			if test x$ac_cv_sizeof_char = x1 ; then
				AC_CHECK_TYPE(__s8, char)
			else
				AC_MSG_ERROR([Can't determine 8-bit signed type for this machine])
       			fi
		fi
	fi

        if test x$ac_cv_lde_haveu16 != xyes; then
		AC_CHECK_TYPE(__u16, unsigned short)
		if test x$ac_cv_type___u16 = xno ; then
			AC_CHECK_SIZEOF(unsigned short,0)
			if test x$ac_cv_sizeof_unsigned_short != x2 ; then
				AC_MSG_ERROR([Can't determine 16-bit unsigned type for this machine])
			fi
		fi
	fi
        if test x$ac_cv_lde_haves16 != xyes; then
		AC_CHECK_TYPE(__s16, short)
		if test x$ac_cv_type___s16 = xno ; then
			AC_CHECK_SIZEOF(short,0)
			if test x$ac_cv_sizeof_short != x2 ; then
				AC_MSG_ERROR([Can't determine 16-bit signed type for this machine])
			fi
		fi
	fi
				
        if test x$ac_cv_lde_haveu32 != xyes; then
		AC_CHECK_TYPE(__u32, unsigned long)
		if test x$ac_cv_type___u32 = xno ; then
			AC_CHECK_SIZEOF(unsigned long,0)
			if test x$ac_cv_sizeof_unsigned_long != x4 ; then
				AC_MSG_ERROR([Can't determine 32-bit unsigned type for this machine])
			fi
		fi
	fi
        if test x$ac_cv_lde_haves32 != xyes; then
		AC_CHECK_TYPE(__s32, long)
		if test x$ac_cv_type___s32 = xno ; then
			AC_CHECK_SIZEOF(long,0)
			if test x$ac_cv_sizeof_long != x4 ; then
				AC_MSG_ERROR([Can't determine 32-bit signed type for this machine])
			fi
		fi
	fi
				
        if test x$ac_cv_lde_haveu64 != xyes; then
		AC_CHECK_TYPE(__u64, unsigned long long)
		if test x$ac_cv_type___u64 = xno ; then
			AC_CHECK_SIZEOF(unsigned long long,0)
			if test x$ac_cv_sizeof_unsigned_long_long != x8 ; then
				AC_MSG_ERROR([Can't determine 64-bit unsigned type for this machine])
			fi
		fi
	fi
        if test x$ac_cv_lde_haves64 != xyes; then
		AC_CHECK_TYPE(__s64, long long)
		if test x$ac_cv_type___s64 = xno ; then
			AC_CHECK_SIZEOF(long long,0)
			if test x$ac_cv_sizeof_long_long != x8 ; then
				AC_MSG_ERROR([Can't determine 64-bit signed type for this machine])
			fi
		fi
	fi
				
])
