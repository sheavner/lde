/*
 *  lde/swiped.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1996  Scott D. Heavner
 *
 *  $Id: swiped.h,v 1.4 2002/01/13 07:35:00 scottheavner Exp $
 */

/* cnews/getdate.y */
#if HAVE_STRUCT_TIMEB
#include <time.h>
#if HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
time_t lde_getdate(char *p, struct timeb *now);
#endif
#else
#define lde_getdate(a,b) time(NULL)
#endif

/* fileutils-3.12/filemode.c  */
void mode_string(unsigned short mode, char *str);

