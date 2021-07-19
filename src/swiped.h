/*
 *  lde/swiped.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1996  Scott D. Heavner
 *
 *  $Id: swiped.h,v 1.5 2003/12/07 01:35:53 scottheavner Exp $
 */

/* cnews/getdate.y */
#include <time.h>
extern time_t lde_getdate(char *p);

/* fileutils-3.12/filemode.c  */
extern void mode_string(unsigned short mode, char *str);
