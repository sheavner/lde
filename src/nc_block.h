/*
 *  lde/nc_block.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_block.h,v 1.2 1994/09/06 01:25:50 sdh Exp $
 */

void cdump_block(unsigned long nr, unsigned char *dind, int win_start, int win_size);
void cwrite_block(unsigned long block_nr, void *data_buffer, char *modified);
int block_mode(void);

struct bm_flags {
    char edit_block;
    char ascii_mode;
    char highlight;
    char modified;
    char redraw;
    char dontwait;
  };


