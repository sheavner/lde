/*
 *  lde/iso9660.h -- The Linux Disk Editor
 *
 *  Copyright (C) 2001  Scott D. Heavner
 *
 *  $Id: iso9660.h,v 1.4 2001/11/26 03:10:41 scottheavner Exp $
 */

void ISO9660_init(char *sb_buffer);
int ISO9660_test(char *sb_buffer, int use_offset);
