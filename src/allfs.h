/*
 *  lde/allfs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 2001  Scott D. Heavner
 *
 *  $Id: allfs.h,v 1.2 2002/01/11 20:08:19 scottheavner Exp $
 */

#ifndef LDE_ALLFS_H
#define LDE_ALLFS_H

#include "lde.h"
#include "iso9660.h"
#include "ext2fs.h"
#include "minix.h"
#include "msdos_fs.h"
#include "xiafs.h"
#include "no_fs.h"

struct _lde_typedata {
  char *name;
  /* Test function, pass in buffer and use_offset flag */
  int (*test)(void *buffer, int use_offset);
  /* Init function, pass address of super block start */
  void (*init)(void *sb_buffer);
};
extern struct _lde_typedata lde_typedata[];


/* Order of enum must match order of LDE_ALLTYPES */
enum lde_fstypes { AUTODETECT, NONE, EXT2, MINIX, DOS, ISO9660, XIAFS, LAST_FSTYPE };


#define LDE_ALLTYPES { \
  { "autodetect", 0, 0 }, \
  { "no file system", 0, 0 }, \
  { "ext2fs", EXT2_test, EXT2_init }, \
  { "minix", MINIX_test, MINIX_init }, \
  { "msdos", DOS_test, DOS_init }, \
  { "iso9660", ISO9660_test, ISO9660_init }, \
  { "xiafs", XIAFS_test, XIAFS_init }, \
  { NULL, 0, 0 } \
}

#endif /* LDE_ALLFS_H */
