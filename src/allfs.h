/*
 *  lde/allfs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 2001  Scott D. Heavner
 *
 *  $Id: allfs.h,v 1.1 2001/11/26 03:10:41 scottheavner Exp $
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

enum lde_fstypes { AUTODETECT, NONE, MINIX, XIAFS, EXT2, DOS, ISO9660, LAST_FSTYPE };


struct _lde_typedata {
  char *name;
  /* Test function, pass in buffer and use_offset flag */
  int (*test)(void *buffer, int use_offset);
  /* Init function, pass address of super block start */
  void (*init)(void *sb_buffer);
};
extern struct _lde_typedata lde_typedata[];


#define LDE_ALLTYPES { \
  { "autodetect", 0, 0 }, \
  { "no file system", 0, 0 }, \
  { "minix", MINIX_test, MINIX_init }, \
  { "xiafs", XIAFS_test, XIAFS_init }, \
  { "ext2fs", EXT2_test, EXT2_init }, \
  { "msdos", DOS_test, DOS_init }, \
  { "iso9660", ISO9660_test, ISO9660_init }, \
  { NULL, 0, 0 } \
}

#endif /* LDE_ALLFS_H */
