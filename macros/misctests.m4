
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
                AC_DEFINE(NO_KERNEL_BITOPS)
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
