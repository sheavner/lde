/*
 *  lde/iso9660.h -- The Linux Disk Editor
 *
 *  Copyright (C) 2001  Scott D. Heavner
 *
 *  $Id: iso9660.h,v 1.3 2001/11/26 00:07:23 scottheavner Exp $
 */

void ISO9660_init(char * sb_buffer);
int ISO9660_test(void *sb_buffer, int use_offset);



