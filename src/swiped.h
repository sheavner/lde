/*
 *  lde/swiped.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1996  Scott D. Heavner
 *
 *  $Id: swiped.h,v 1.2 1996/10/12 21:12:12 sdh Exp $
 */

#include <sys/timeb.h>        /* Include this here for getdate() prototype below */

/* fileutils-3.12/filemode.c  */
void mode_string(unsigned short mode, char *str);

/* cnews/getdate.y */
time_t getdate(char *p, struct timeb *now);


