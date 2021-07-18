/*
 *  lde/ext2fs.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: ext2fs.h,v 1.6 2002/02/01 03:35:19 scottheavner Exp $
 *
 */

void EXT2_init(char *sb_buffer);
int EXT2_test(char *sb_buffer, int use_offset);

#ifdef JQDIR
/* struct to hold directory entries */
int EXT2_dir_undelete(int entry, lde_buffer *buffer);
int EXT2_is_deleted(void);
int EXT2_dir_check_entry(int entry, lde_buffer *buffer);
#endif /* JQDIR */
