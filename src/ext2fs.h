/*
 *  lde/ext2fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.h,v 1.5 2002/01/28 01:04:25 scottheavner Exp $
 *
 */

void EXT2_init(void *sb_buffer);
int EXT2_test(void *sb_buffer, int use_offset);

#ifdef JQDIR
/* struct to hold directory entries */
int EXT2_dir_undelete(int entry, lde_buffer *buffer);
int EXT2_is_deleted(void);
void EXT2_dir_check_entry(int entry, lde_buffer *buffer);
#endif /* JQDIR */
