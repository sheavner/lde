/*
 *  lde/nc_dir.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_dir.h,v 1.3 2002/01/27 23:11:51 scottheavner Exp $
 */

int directory_popup(unsigned long bnr, unsigned long inr, unsigned long ipointer);

/* A little routine to keep track of where we are reitive to start point.*/
void showpath(char *fname, unsigned long inode_nbr );
void freepath(void);
void fixname(char *,const char *, int );
int is_deleted(void);
