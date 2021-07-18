/*
 *  lde/nc_block.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_block.c,v 1.35 2003/12/03 18:27:32 scottheavner Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lde.h"
#include "recover.h"
#include "tty_lde.h"
#include "curses.h"
#include "nc_lde.h"
#include "nc_block.h"
#include "nc_block_help.h"
#include "nc_dir.h"
#include "keymap.h"

static void cdump_block(unsigned long nr, bm_cursor *curs);
static int cwrite_block(cached_block *this_block,
  bm_cursor *curs,
  bm_flags *flags);
static void highlight_helper(int row,
  int col,
  bm_cursor *curs,
  bm_flags *flags);
static void highlight_block(bm_cursor *curs, bm_flags *flags);
static void update_block_help(void);
static int update_block_contents(cached_block **passed_block,
  unsigned long new_bnr,
  bm_cursor *curs,
  bm_irecord *iptr);
static void clear_data_cache(cached_block *this_block, bm_cursor *curs);
static int update_block_data(cached_block **passed_block,
  unsigned long new_bnr,
  bm_irecord *iptr,
  bm_cursor *curs,
  bm_flags *flags,
  long sowadjust);

static inline int calc_offset(bm_cursor *curs)
{
  return curs->row * curs->rs + curs->col + curs->sow;
}

/* Dump as much of a block as we can to the screen and format it in a nice 
 * hex mode with ASCII printables off to the right. */
static void cdump_block(unsigned long nr, bm_cursor *curs)
{
  int i, j, offset;
  unsigned char c;
  const char *block_status;
  const char block_is_used[10] = "::::::::::";
  const char block_not_used[10] = ":NOT:USED:";
  const char block_is_bad[10] = ":BAD::BAD:";

  /* indicate we are in block mode */
  strcpy(ldemode, "Block     ");
  update_header();

  werase(workspace);

  /* We want to indicate bad blocks and unused blocks */
  block_status = (FS_cmd.zone_in_use(nr)) ? block_is_used : block_not_used;
  if (FS_cmd.zone_is_bad(nr))
    block_status = block_is_bad;
  j = 0;

  while ((j < curs->wl) && (j * curs->rs + curs->sow < curs->eod)) {

    /* When rowsize is 16, display hex and ASCII */
    if (curs->rs == 16) {
      offset = j * 16 + curs->sow - curs->sob + nr * sb->blocksize;
      if (offset % sb->blocksize < 16) {
        wattron(workspace, WHITE_ON_BLUE);
        mvwprintw(workspace, j, 0, "%08X ", offset);
        wattroff(workspace, WHITE_ON_BLUE);
      } else {
        mvwprintw(workspace, j, 0, "%08X ", offset);
      }
      for (i = 0; ((i < 8) && ((j * 16 + i + curs->sow) < curs->eod)); i++)
        mvwprintw(workspace,
          j,
          10 + i * 3,
          "%2.2X",
          curs->data[j * 16 + i + curs->sow]);
      mvwprintw(workspace, j, 35, "%c", block_status[j % 10]);
      for (i = 0; ((i < 8) && ((j * 16 + i + 8 + curs->sow) < curs->eod)); i++)
        mvwprintw(workspace,
          j,
          38 + i * 3,
          "%2.2X",
          curs->data[j * 16 + i + 8 + curs->sow]);
    }

    for (i = 0;
         ((i < curs->rs) && ((j * curs->rs + i + curs->sow) < curs->eod));
         i++) {
      c = curs->data[j * curs->rs + i + curs->sow];
      c = isprint(c) ? c : '.';
      if (curs->rs == 16)
        mvwaddch(workspace, j, 64 + i, c);
      else
        mvwaddch(workspace, j, i, c);
    }
    j++;
  }

  wrefresh(workspace);
  update_header();
  return;
}

/* Asks the user if it is ok to write the block to the disk, then goes
 * ahead and does it if the medium is writable.  The user has access to the
 * flags menu from this routine, to toggle the write flag */
static int cwrite_block(cached_block *this_block,
  bm_cursor *curs,
  bm_flags *flags)
{
  int c;
  char *warning = "(NOTE: write permission not set on disk,"
                  " use 'F' to set flags before 'Y') ";

  if (flags->modified) {

    for (;;) {

      /* This will return only y, d, c, or f - y cannot be
       * selected unless the disk is writable */
      c = cquery("WRITE OUT BLOCK DATA TO DISK "
                 "[Yes/Discard changes/Continue edit]? ",
        "ydcf",
        (lde_flags.write_ok) ? "" : warning);

      if (c == 'f') {
        flag_popup();
      } else if (c == 'y') {
        if (lde_flags.write_ok != 0)
          break;
      } else if (c == 'c') {
        return 1;
      } else if (c == 'd') { /* Discard changes, copy old data into buffer */
        memcpy(curs->data + curs->sob, this_block->data, this_block->size);
        flags->redraw = 1;
        flags->modified = flags->ascii_mode = flags->edit_block = 0;
        return 0;
      }
    }

    refresh_all(); /* Have to refresh screen before write or 
		    * error messages will be lost */
    write_block(this_block->bnr, curs->data + curs->sob);
    flags->modified = flags->ascii_mode = flags->edit_block = 0;
  }

  return 0;
}

/* (Un)Highlights the current position in the block in both ASCII + hex */
static void highlight_helper(int row, int col, bm_cursor *curs, bm_flags *flags)
{
  unsigned char c;
  int s_col;

  /* Don't do anything if we're passed the end of our data */
  s_col = row * curs->rs + col + curs->sow;
  if (s_col >= curs->eod)
    return;

  c = curs->data[s_col];

  if (curs->rs == 16) {
    /* Calculcate location of column, second 8 fields have 
     * to skip an extra 4 spaces, display hex */
    s_col = 10 + col * 3 + ((col < 8) ? 0 : 4);
    mvwprintw(workspace, row, s_col, "%2.2X", c);

    /* Display ascii */
    c = isprint(c) ? c : '.';
    mvwprintw(workspace, row, col + 64, "%c", c);

    /* Leave cursor parked over current edit mode */
    if (flags->ascii_mode)
      wmove(workspace, row, col + 64);
    else
      wmove(workspace, row, s_col);
  } else {
    /* Display is all ascii */
    c = isprint(c) ? c : '.';
    mvwprintw(workspace, row, col, "%c", c);
    wmove(workspace, row, col);
  }
}

/* Highlight current cursor position, turn off last highlight */
static void highlight_block(bm_cursor *curs, bm_flags *flags)
{
  static int prev_row = 0, prev_col = 0;

  /* First turn off last position */
  highlight_helper(prev_row, prev_col, curs, flags);
  prev_col = curs->col;
  prev_row = curs->row;

  /* Now highlight current */
  wattron(workspace, WHITE_ON_RED);
  highlight_helper(curs->row, curs->col, curs, flags);
  wattroff(workspace, WHITE_ON_RED);

  return;
}

/* Modifies the help screen display to show the proper sizes for 
 * block and inode pointers */
static void update_block_help(void)
{
  static char help_1[80], help_8[80];

  sprintf(help_1,
    "View block under cursor (%s block ptr is %d bytes)",
    fsc->text_name,
    fsc->ZONE_ENTRY_SIZE);
  sprintf(help_8,
    "View inode under cursor (%s inode ptr is %d bytes)",
    fsc->text_name,
    fsc->INODE_ENTRY_SIZE);
  block_help[BHELP_BLOCK].description = help_1;
  block_help[BHELP_INODE].description = help_8;

  return;
}

/* Make sure our memory copy of 2*curs->bps+1 blocks is updated,
 *   and centered around our current block.  If not, re-read
 *   necessary blocks from disk. */
static int update_block_contents(cached_block **passed_block,
  unsigned long new_bnr,
  bm_cursor *curs,
  bm_irecord *iptr)
{
  unsigned long other_bnr;
  int i, c;
  cached_block *cbptr, *this_block;

  this_block = *passed_block;

  /* If iptr is specified, we're working on a set of linked blocks */
  if (iptr) {
    iptr->inode = FS_cmd.read_inode(current_inode);
    /* Make sure that this block is indexed somewhere
     * in the current inode */
    c = advance_zone_pointer(
      iptr->inode->i_zone, &new_bnr, &(iptr->inr), iptr->increment);
    if ((c) || (iptr->inode == NULL) || (current_block == 0)) {
      lde_warn("Can only index blocks contained in the current inode.");
      return 1;
    }
  } else if ((this_block->size) && (this_block->bnr == new_bnr)) {
    /* Make sure there's something to do */
    return 0;
  }

  /* Check if newblock is cached somewhere, else grab from disk */
  for (i = 0; i < 2 * curs->bps + 1; i++) {
    this_block = this_block->next;
    if ((this_block->size) && (this_block->bnr == new_bnr))
      break;
    if ((!this_block->size) || (this_block->bnr != new_bnr))
      this_block = cache_read_block(new_bnr, this_block, CACHEABLE);
  }

  /* Save return value for passed_block */
  *passed_block = this_block;

  /* Make sure prev blocks are loaded into memory */
  cbptr = this_block;
  for (i = 1; i <= curs->bps; i++) {
    cbptr = cbptr->prev;
    if (iptr) {
      other_bnr = new_bnr;
      if (advance_zone_pointer(
            iptr->inode->i_zone, &other_bnr, &(iptr->inr), -i))
        other_bnr = sb->nzones;
    } else {
      other_bnr = new_bnr - i;
    }
    if (other_bnr < sb->nzones) {
      if ((!cbptr->size) || (cbptr->bnr != other_bnr)) {
        cache_read_block(other_bnr, cbptr, CACHEABLE);
      }
    } else {
      cbptr->size = 0;
    }
  }

  /* Make sure next blocks are loaded into memory */
  cbptr = this_block;
  for (i = 1; i <= curs->bps; i++) {
    cbptr = cbptr->next;
    if (iptr) {
      other_bnr = new_bnr;
      if (advance_zone_pointer(
            iptr->inode->i_zone, &other_bnr, &(iptr->inr), +i))
        other_bnr = sb->nzones;
    } else {
      other_bnr = new_bnr + i;
    }
    if (other_bnr < sb->nzones) {
      if ((!cbptr->size) || (cbptr->bnr != other_bnr)) {
        cache_read_block(other_bnr, cbptr, CACHEABLE);
      }
    } else {
      cbptr->size = 0;
    }
  }

  return 0;
}

static void clear_data_cache(cached_block *this_block, bm_cursor *curs)
{
  int i;

  for (i = 0; i < curs->bps * 2 + 1; i++) {
    this_block->size = 0;
    this_block = this_block->next;
  }
}

static int update_block_data(cached_block **passed_block,
  unsigned long new_bnr,
  bm_irecord *iptr,
  bm_cursor *curs,
  bm_flags *flags,
  long sowadjust)
{
  int i;
  cached_block *cbptr, *this_block;
  unsigned char *cbuffer;

  /* Make sure we write out any changed blocks.  If user aborts write, 
   * cwrite_block returns 1 and we should continue editing this block */
  if (cwrite_block(*passed_block, curs, flags))
    return 1;

  cbuffer = curs->data;

  if (update_block_contents(passed_block, new_bnr, curs, iptr))
    return 1;

  this_block = *passed_block;

  curs->eod = 0;

  /* back up, cbptr=block we're interested in, which is in
   *  center of linked list */
  cbptr = this_block;
  for (i = 0; i < curs->bps; i++)
    cbptr = cbptr->prev;

  /* Copy prev. blocks into cbuffer */
  for (i = 0; i < curs->bps; i++) {
    if (cbptr->size) {
      memcpy(cbuffer, cbptr->data, cbptr->size);
      cbuffer += cbptr->size;
      curs->eod += cbptr->size;
    }
    cbptr = cbptr->next;
  }

  /* Copy current block into cbuffer */
  cbptr = this_block;
  curs->sob = curs->sow = curs->eod;
  memcpy(cbuffer, this_block->data, this_block->size);
  cbuffer += this_block->size;
  curs->eod += this_block->size;
  curs->eob = curs->eod;

  /* Copy next blocks into cbuffer */
  for (i = 0; i < curs->bps; i++) {
    cbptr = cbptr->next;
    if (cbptr->size) {
      memcpy(cbuffer, cbptr->data, cbptr->size);
      cbuffer += cbptr->size;
      curs->eod += cbptr->size;
    }
  }

  curs->sow += sowadjust;

  current_block = this_block->bnr;
  update_header();

  *passed_block = this_block;

  return 0;
}

/* This is the curses menu-type system for block mode, displaying a block
 * on the screen, next/previous block, paging this block, etc., etc. */
int block_mode(void)
{
  static unsigned char *copy_buffer = NULL;
  int icount = 0, val, v1 = 0, c = CMD_REFRESH, i;
  char *HEX_PTR, *HEX_NOS = "0123456789ABCDEF", *s;
  unsigned long a, temp_ptr, search_iptr = 0UL, inode_ptr[2] = { 0UL, 0UL };
  bm_flags flags = { 0, 0, 0, 0, 0, 1 };
  bm_cursor curs = { 0, 0, 0, 0, 0, 0, 16, 24 };
  bm_irecord irecord = { 0 }, *irecptr = NULL;
  cached_block *this_block, *cb_buffer, *tmp_block;

  /* Need to init separately as VERT is not a compile time constant */
  curs.wl = VERT;

  /* Want to allocate enough room for two screenfuls of data, 
   * compute the size of one screen full here */
  curs.bps =
    (((long)VERT) * (COLS - HOFF) + (sb->blocksize - 1)) / sb->blocksize + 1;

  /* Allocate space for buffers, abort on error */
  curs.data = malloc(sb->blocksize * (2 * curs.bps + 1));
  cb_buffer = malloc(sizeof(cached_block) * (2 * curs.bps + 1));
  if ((!curs.data) || (!cb_buffer)) {
    lde_warn("Not enough memory for block_mode buffer");
    return 0;
  }

  /* Get rid of old window and create a new one */
  clobber_window(workspace);
  workspace = newwin(VERT, (COLS - HOFF), HEADER_SIZE, HOFF);

  /* Adjust block pointer to a valid block */
  if (current_block >= sb->nzones)
    current_block = sb->nzones - 1;

  /* Update the help window to display proper size of pointers */
  update_block_help();

  /* Stick some helpful? info in the trailer window */
  display_trailer("F1/? for help.  F2/^O for menu.  Q to quit");

  /* Fill in next/prev pointers for linked list */
  for (i = 0; i < 2 * curs.bps + 1; i++) {
    if ((val = i + 1) > 2 * curs.bps)
      val -= 2 * curs.bps + 1;
    cb_buffer[i].next = &(cb_buffer[val]);
    if ((val = i - 1) < 0)
      val += 2 * curs.bps + 1;
    cb_buffer[i].prev = &(cb_buffer[val]);
  }

  /* Grab a copy of the current block */
  this_block = cb_buffer;
  update_block_data(&this_block, current_block, irecptr, &curs, &flags, 0);

  /* Start main loop - should never exit while */
  while (flags.dontwait || (c = mgetch())) {

    /* Horrible, Horrible hack */
    if (curs.rs != 16) { /* Should probably create a flag to indicate we
			  * are in all ASCII MODE */
      flags.ascii_mode = 1;
    }

    /* By default we don't want to redraw the screen or move the cursor
     * each time through the loop */
    flags.redraw = flags.highlight = 0;

    if (flags.dontwait) { /* If dontwait is set a command 
				     * is already queued up in c */

      /* Don't do anything yet, but allow mouse handler and edit
       * parser a chance at the result of getch if dontwait is clear */

    } else if (IS_LDE_MOUSE(c)) { /* Handle mouse events here, before
				    * lookup_key() and edit code */
#ifdef BETA_CODE
      if ((LDE_MOUSEX(c) > HOFF) && (LDE_MOUSEY(c) > HEADER_SIZE) &&
          (LDE_MOUSEX(c) <= (HOFF + WIN_COL)) &&
          (LDE_MOUSEY(c) <= (HEADER_SIZE + VERT))) {
        curs.row = LDE_MOUSEY(c) - HEADER_SIZE - 1; /* Row calc is easy */
        curs.col = LDE_MOUSEX(c) - HOFF - 1; /* Col will depend on mode */
        if (curs.rs == 16) {                 /* Hex/Ascii mode is tougher */
          if (curs.col >= 64) {              /* In ascii portion */
            curs.col -= 64;
            flags.ascii_mode = 1;
          } else if (curs.col > 35) { /* In right half of hex portion */
            curs.col = (curs.col - 35 - 3) / 3 + 8;
            flags.ascii_mode = 0;
          } else { /* In left half of hex portion */
            curs.col = (curs.col - 10) / 3;
            flags.ascii_mode = 0;
          }
        }
        if (curs.col < 0) /* Bounds checking on column */
          curs.col = 0;
        else if (curs.col >= curs.rs)
          curs.col = curs.rs - 1;
        c = CMD_ANY_ACTION;  /* Need a command that will fall through 
to refresh */
        flags.dontwait = 1;  /* Don't call lookup_key() on c, already 
has command */
        flags.highlight = 1; /* Update cursor position */
        icount = 0;          /* Abort any data entry in progress */
      }
#endif                             /* BETA_CODE */
    } else if (flags.edit_block) { /* Handle edit keys in edit mode */
      if (flags.ascii_mode) {
        if (isprint(c)) {
          curs.data[calc_offset(&curs)] = c;
          flags.highlight = flags.modified = flags.dontwait = 1;
          c = CMD_NEXT_FIELD; /* Advance the cursor */
        }
      } else {
        HEX_PTR = strchr(HEX_NOS, toupper(c));
        if ((HEX_PTR != NULL) && (*HEX_PTR)) {
          val = (int)(HEX_PTR - HEX_NOS);
          if (!icount) {
            icount = 1;
            v1 = val;
            wattron(workspace, WHITE_ON_RED);
            waddch(workspace, HEX_NOS[val]);
            wattroff(workspace, WHITE_ON_RED);
            flags.dontwait = 1;
            c = CMD_NO_ACTION;
          } else {
            icount = 0;
            v1 = v1 * curs.rs + val;
            curs.data[calc_offset(&curs)] = v1;
            flags.modified = flags.highlight = flags.dontwait = 1;
            c = CMD_NEXT_FIELD; /* Advance the cursor */
          }
        }
      }
    }

    if (flags.dontwait) { /* If dontwait is set a command 
				     * is already queued up in c */
      flags.dontwait = 0;

    } else {
      c = lookup_key(c, blockmode_keymap);
    }

    switch (c) {

      case CMD_ALL_ASCII: /* Suppress hex output */
        val = calc_offset(&curs);
        if (curs.rs == 16)
          curs.rs = 80;
        else
          curs.rs = 16;
        flags.redraw = 1;
        flags.ascii_mode = 1;
        /* Need to recalc offsets for new rowsize */
        curs.sow = val / (curs.rs * VERT);
        curs.sow *= curs.rs * VERT;
        val -= curs.sow;
        curs.row = val / curs.rs;
        curs.col = val - curs.row * curs.rs;
        break;

      case CMD_TOGGLE_ASCII: /* Toggle Ascii/Hex edit mode */
        if (curs.rs == 16) { /* Should probably create a flag to indicate we
			      * are in all ASCII MODE */
          flags.ascii_mode = 1 - flags.ascii_mode;
          flags.highlight = 1;
        }
        break;

      case CMD_NEXT_FIELD:
        flags.highlight = 1;
        icount = 0;
        /* Don't allow move past end of block if on incomplete row */
        curs.col++;
        if (calc_offset(&curs) >= curs.eod) {
          curs.col--;
          break;
        }
        if ((calc_offset(&curs) >= curs.eob) && (this_block->next->size)) {
          if (update_block_data(&this_block,
                this_block->next->bnr,
                irecptr,
                &curs,
                &flags,
                curs.sow - curs.eob)) {
            curs.col--;
            break;
          }
        }
        if (curs.col >= curs.rs) {
          curs.col = 0;
          curs.row++;
          if (calc_offset(&curs) >= curs.eod) {
            curs.col = curs.rs - 1;
            curs.row--;
          } else if (curs.row >= VERT) {
            if (curs.sow + VERT * curs.rs < curs.eob)
              curs.sow += VERT * curs.rs;
            else
              c = CMD_NEXT_BLOCK;
            curs.row = 0;
            flags.redraw = 1;
          }
        }
        break;

      case CMD_PREV_FIELD:
        flags.highlight = 1;
        icount = 0;
        curs.col--;
        if ((calc_offset(&curs) < curs.sob) && (this_block->prev->size)) {
          if (update_block_data(&this_block,
                this_block->prev->bnr,
                irecptr,
                &curs,
                &flags,
                this_block->prev->size - (curs.sob - curs.sow))) {
            curs.col++;
            break;
          }
        }
        if (curs.col < 0) {
          curs.col = curs.rs - 1;
          if (--curs.row < 0) {
            if (curs.sow - VERT * curs.rs >= 0) {
              curs.sow -= VERT * curs.rs;
              curs.row = VERT - 1;
              flags.redraw = 1;
            } else {
              curs.row = curs.col = 0;
            }
          }
        }
        break;

#ifdef BETA_CODE
      case CMD_NEXT_SCREEN:
        if (curs.sow + VERT * curs.rs < curs.eod)
          curs.sow += VERT * curs.rs;
        if (calc_offset(&curs) >= curs.eob) {

          /* It is possible we may need to go back more than one block */
          tmp_block = this_block;
          c = calc_offset(&curs);
          while ((c > 0) && (curs.eob <= c)) {
            if (tmp_block->next->size)
              tmp_block = tmp_block->next;
            else
              break;
            c -= tmp_block->size;
          }
          /* Not very robust yet */
          c = curs.sow - (curs.sob + (calc_offset(&curs) - c));
          if (update_block_data(
                &this_block, tmp_block->bnr, irecptr, &curs, &flags, c)) {
            curs.sow -= VERT * curs.rs;
          }
        }
        flags.redraw = 1;
        icount = 0;
        break;

      case CMD_PREV_SCREEN:
        if (curs.sow - VERT * curs.rs >= 0) {
          curs.sow -= VERT * curs.rs;
        } else if (!(this_block->bnr)) {
          curs.sow = 0;
        }
        if (calc_offset(&curs) < curs.sob) {

          /* It is possible we may need to go back more than one block */
          tmp_block = this_block;
          c = curs.sob;
          while ((c > 0) && (calc_offset(&curs) < c)) {
            if (tmp_block->prev->size)
              tmp_block = tmp_block->prev;
            else
              break;
            c -= tmp_block->size;
          }
          /* Not very robust yet: (curs.sob-c) should be a
	   * multiple of blocksize */
          c = (curs.sob - c) - (curs.sob - curs.sow);
          /* if ( c < -sb->blocksize ) lde_warn("C < blocksize - %d < %ld",c,sb->blocksize); */
          if (update_block_data(
                &this_block, tmp_block->bnr, irecptr, &curs, &flags, c)) {
            curs.sow += VERT * curs.rs;
          }
        }
        icount = 0;
        flags.redraw = 1;
        break;
#else
      case CMD_NEXT_SCREEN:
        if (curs.sow + VERT * curs.rs < curs.eod)
          curs.sow += VERT * curs.rs;
        if ((calc_offset(&curs) >= curs.eob) && (this_block->next->size)) {
          if (update_block_data(&this_block,
                this_block->next->bnr,
                irecptr,
                &curs,
                &flags,
                curs.sow - curs.eob))
            curs.sow -= VERT * curs.rs;
        }
        flags.redraw = 1;
        icount = 0;
        break;

      case CMD_PREV_SCREEN:
        if (curs.sow - VERT * curs.rs >= 0) {
          curs.sow -= VERT * curs.rs;
        } else {
          curs.sow = 0;
        }
        if ((calc_offset(&curs) < curs.sob) && (this_block->prev->size)) {
          if (update_block_data(&this_block,
                this_block->prev->bnr,
                irecptr,
                &curs,
                &flags,
                this_block->prev->size - (curs.sob - curs.sow))) {
            curs.sow += VERT * curs.rs;
          }
        }
        icount = 0;
        flags.redraw = 1;
        break;
#endif

      case CMD_NEXT_LINE:
        flags.highlight = 1;
        /* Don't allow move past end of block if on incomplete row */
        curs.row++;
        if (calc_offset(&curs) >= curs.eod) {
          curs.row--;
          break;
        }
        if ((calc_offset(&curs) >= curs.eob) && (this_block->next->size)) {
          if (update_block_data(&this_block,
                this_block->next->bnr,
                irecptr,
                &curs,
                &flags,
                curs.sow - curs.eob)) {
            curs.row--;
            break;
          }
        }
        if (curs.row >= VERT) {
          flags.redraw = 1;
          if (curs.sow + VERT * curs.rs < curs.eod)
            curs.sow += VERT * curs.rs;
          curs.row = 0;
        }
        icount = 0;
        break;

      case CMD_PREV_LINE:
        flags.highlight = 1;
        icount = 0;
        curs.row--;
        if ((calc_offset(&curs) < curs.sob) && (this_block->prev->size)) {
          if (update_block_data(&this_block,
                this_block->prev->bnr,
                irecptr,
                &curs,
                &flags,
                this_block->prev->size - (curs.sob - curs.sow))) {
            curs.row++;
            break;
          }
        }
        if (curs.row < 0) {
          flags.redraw = 1;
          if (curs.sow - VERT * curs.rs >= 0) {
            curs.sow -= VERT * curs.rs;
            curs.row = VERT - 1;
          } else {
            curs.row = flags.redraw = 0;
          }
        }
        break;

#ifdef BETA_CODE
      case CMD_NEXT_IND_BLOCK:
      case CMD_PREV_IND_BLOCK:
        irecord.increment = 0;
        if (!update_block_data(
              &this_block, this_block->bnr, &irecord, &curs, &flags, 0)) {
          irecptr = &irecord;
          irecord.increment = (c == CMD_NEXT_IND_BLOCK) ? +1 : -1;
          clear_data_cache(this_block, &curs);
          update_block_data(
            &this_block, this_block->bnr, &irecord, &curs, &flags, 0);
          /* All other routines will automatically advance block using
	   * next/prev linked list in this_block, set increment to 0 for
	   * them */
          irecord.increment = 0;
          flags.edit_block = curs.col = curs.row = 0;
          flags.redraw = 1;
          search_iptr = 0L;
        }
        break;
#endif

      case CMD_NEXT_BLOCK:
      case CMD_PREV_BLOCK:
        /* Try to move to next/prev block.  If not available, abort */
        if ((c == CMD_NEXT_BLOCK) && (this_block->next->size))
          current_block = this_block->next->bnr;
        else if ((c == CMD_PREV_BLOCK) && (this_block->prev->size))
          current_block = this_block->prev->bnr;
        else
          break;
        /* See if we can access new data, if not restore old current_block */
        if (update_block_data(
              &this_block, current_block, irecptr, &curs, &flags, 0)) {
          current_block = this_block->bnr;
          break;
        }
        flags.edit_block = curs.col = curs.row = 0;
        flags.redraw = 1;
        search_iptr = 0L;
        break;

      case CMD_WRITE_CHANGES: /* Write out modified block to disk */
        cwrite_block(this_block, &curs, &flags);
        break;

      case CMD_ABORT_EDIT: /* Abort any changes to block */
        flags.modified = flags.ascii_mode = flags.edit_block = 0;
        update_block_data(&this_block,
          current_block,
          irecptr,
          &curs,
          &flags,
          (-curs.sob + curs.sow));
        flags.redraw = flags.highlight = 1;
        break;

      case CMD_FIND_STRING: /* Search disk for data */
        if (cwrite_block(this_block, &curs, &flags))
          break;
        if (ncread("Enter search string:", NULL, &s) > 0) {
          unsigned long mbnr;
          int moffset;
          if ((search_blocks(s, current_block, &mbnr, &moffset) > 0)) {
            irecptr = NULL;
            clear_data_cache(this_block, &curs);
            update_block_data(&this_block, mbnr, irecptr, &curs, &flags, 0);
            moffset -= curs.sow;
            curs.row = moffset / curs.rs;
            curs.col = moffset % curs.rs;
            flags.edit_block = 0;
            flags.redraw = 1;
            search_iptr = 0L;
          }
        }
        break;

      case CMD_FIND_INODE:
      case CMD_FIND_INODE_MC: /* Find an inode which references this block */
        lde_warn(
          "Searching for inode containing block 0x%lX . . .", current_block);
        if ((search_iptr = find_inode(current_block, search_iptr))) {
          lde_warn("Block is indexed under inode 0x%lX. "
                   " Repeat to search for more occurances.",
            search_iptr);
          if (c == CMD_FIND_INODE_MC) {
            current_inode = search_iptr;
            free(curs.data);
            free(cb_buffer);
            return CMD_INODE_MODE;
          }
        } else {
          if (lde_flags.quit_now)
            lde_warn("Search terminated.");
          else if (lde_flags.search_all)
            lde_warn("Unable to find inode referenece.");
          else
            lde_warn("Unable to find inode referenece "
                     "try activating the --all option.");
          search_iptr = 0L;
        }
        break;

      case REC_FILE0: /* Add current block to recovery list at position 'n' */
      case REC_FILE1:
      case REC_FILE2:
      case REC_FILE3:
      case REC_FILE4:
      case REC_FILE5:
      case REC_FILE6:
      case REC_FILE7:
      case REC_FILE8:
      case REC_FILE9:
      case REC_FILE10:
      case REC_FILE11:
      case REC_FILE12:
      case REC_FILE13:
      case REC_FILE14:
        fake_inode_zones[c - REC_FILE0] = current_block;
        update_header();
        beep();
        break;

      case CMD_COPY: /* Copy block to copy buffer */
        if (!copy_buffer)
          copy_buffer = malloc(sb->blocksize);
        memcpy(copy_buffer, curs.data + curs.sob, sb->blocksize);
        lde_warn("Block (%lu) copied into copy buffer.", current_block);
        break;

      case CMD_PASTE: /* Paste block from copy buffer */
        if (copy_buffer) {
          flags.modified = flags.redraw = 1;
          memcpy(curs.data + curs.sob, copy_buffer, sb->blocksize);
          if (!lde_flags.write_ok)
            lde_warn("Turn on write permissions before saving this block");
        }
        break;

      case CMD_EDIT: /* Edit the current block */
        flags.edit_block = 1;
        icount = 0;
        if (!lde_flags.write_ok)
          lde_warn("Disk not writeable, change status flags with (F)");
        break;

      case CMD_VIEW_AS_DIR: /* View the current block as a directory */
        if (directory_popup(current_block, 0UL, 0UL) == CMD_INODE_MODE_MC) {
          c = CMD_INODE_MODE;
          flags.dontwait = 1;
        }
        flags.redraw = 1;
        break;

      case CMD_BLOCK_MODE_MC: /* View the block under the cursor */
        if (cwrite_block(this_block, &curs, &flags))
          break;
        temp_ptr = block_pointer(&(curs.data[calc_offset(&curs)]),
          (unsigned long)0,
          fsc->ZONE_ENTRY_SIZE);
        if (temp_ptr < sb->nzones) {
          irecptr = NULL;
          clear_data_cache(this_block, &curs);
          update_block_data(&this_block, temp_ptr, irecptr, &curs, &flags, 0);
          flags.edit_block = curs.col = curs.row = 0;
          flags.redraw = 1;
        } else {
          lde_warn("Block (0x%lX) out of range in block_mode().", temp_ptr);
        }
        break;

      case CMD_FLAG_ADJUST: /* Popup menu of user adjustable flags */
        flag_popup();
        break;

      case CMD_INODE_MODE_MC: /* Switch to Inode mode,
			       * make this inode the current inode */
        if (cwrite_block(this_block, &curs, &flags))
          break;
        temp_ptr = block_pointer(&(curs.data[calc_offset(&curs)]),
          (unsigned long)0,
          fsc->INODE_ENTRY_SIZE);
        if (temp_ptr <= sb->ninodes) {
          current_inode = temp_ptr;
          free(curs.data);
          free(cb_buffer);
          return CMD_INODE_MODE;
        } else {
          lde_warn("Inode (%lX) out of range in block_mode().", temp_ptr);
        }
        break;

      case CMD_EXIT_PROG: /* Switch to another mode */
      case CMD_VIEW_SUPER:
      case CMD_INODE_MODE:
      case CMD_RECOVERY_MODE:
        if (!cwrite_block(this_block, &curs, &flags)) {
          free(curs.data);
          free(cb_buffer);
          return c;
        }
        break;

      case CMD_DISPLAY_LOG: /* View error log */
        c = error_popup();
        break;

      case CMD_DO_RECOVER: /* Append current block to the recovery file */
        inode_ptr[0] = current_block;
        crecover_file(inode_ptr, lookup_blocksize(current_block));
        break;

      case CMD_NUMERIC_REF: /* Jump to a block by numeric reference */
        if (cwrite_block(this_block, &curs, &flags))
          break;
        if (ncread("Enter block number (leading 0x or $ indicates hex):",
              &a,
              NULL)) {
          if (a >= sb->nzones)
            a = sb->nzones - 1;
          irecptr = NULL;
          clear_data_cache(this_block, &curs);
          update_block_data(&this_block, a, irecptr, &curs, &flags, 0);
          flags.edit_block = curs.col = curs.row = 0;
          flags.redraw = 1;
          search_iptr = 0L;
        }
        break;

      case CMD_HELP: /* HELP */
        do_new_scroll_help(block_help, blockmode_keymap, FANCY);
        refresh_all();
        break;

      case CMD_CALL_MENU: /* POPUP MENU */
        c = do_popup_menu(block_menu, blockmode_keymap);
        if (c == CMD_CALL_MENU)
          c = do_popup_menu(edit_menu, blockmode_keymap);
        if (c != CMD_NO_ACTION)
          flags.dontwait = 1;
        break;

      case CMD_REFRESH: /* Refresh screen */
        icount = 0;     /* We want to see actual values? */
        refresh_all();
        flags.redraw = 1;
        break;

      case CMD_ANY_ACTION: /* Unknown command, but still want to execute stuff */
        break;             /* after loop */

      default: /* Unknown command, get another */
        continue;
    }

    /* More room on screen, but have we gone past the end of the block? */
    if (curs.row * curs.rs + curs.sow >= curs.eod)
      curs.row = (curs.eod - curs.sow - 1) / curs.rs;

    if (flags.redraw) {
      cdump_block(current_block, &curs);
      flags.highlight = 1;
    }

    /* Highlight current cursor position */
    if (flags.highlight)
      highlight_block(&curs, &flags);

    wrefresh(workspace);

    /* Moderate scrolling through blocks */
    if (!flags.edit_block)
      flushinp();
  }

  free(curs.data);
  free(cb_buffer);
  return 0;
}
