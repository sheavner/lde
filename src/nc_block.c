/*
 *  lde/nc_block.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_block.c,v 1.7 1995/06/01 06:02:41 sdh Exp $
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

/* default keymap for block mode */
static lde_keymap blockmode_keymap[] = {
  { CTRL('L'), CMD_REFRESH },
  { 'z', CMD_CALL_MENU },
  { KEY_F(2), CMD_CALL_MENU },
  { CTRL('O'), CMD_CALL_MENU },
  { '?', CMD_HELP },
  { KEY_F(1), CMD_HELP },
  { CTRL('H'), CMD_HELP },
  { META('H'), CMD_HELP },
  { META('h'), CMD_HELP },
  { '#', CMD_NUMERIC_REF },
  { 'v', CMD_DISPLAY_LOG },
  { 'V', CMD_DISPLAY_LOG },
  { 'i', CMD_INODE_MODE },
  { 's', CMD_VIEW_SUPER },
  { 'S', CMD_VIEW_SUPER },
  { 'r', CMD_RECOVERY_MODE },
  { 'R', CMD_RECOVERY_MODE },
  { 'Q', CMD_EXIT_PROG },
  { 'q', CMD_EXIT_PROG },
  { 'I', CMD_INODE_MODE_MC },
  { 'B', CMD_BLOCK_MODE_MC },
  { 'd', CMD_VIEW_AS_DIR },
  { 'D', CMD_VIEW_AS_DIR },
  { 'e', CMD_EDIT },
  { 'E', CMD_EDIT },
  { 'c', CMD_COPY },
  { 'C', CMD_COPY },
  { 'p', CMD_PASTE },
  { 'P', CMD_PASTE },
  { 'w', CMD_DO_RECOVER },
  { 'W', CMD_DO_RECOVER },
  { CTRL('W'), CMD_WRITE_CHANGES },
  { CTRL('A'), CMD_ABORT_EDIT },
  { CTRL('V'), CMD_NEXT_BLOCK },
  { CTRL('D'), CMD_NEXT_BLOCK },
  { KEY_NPAGE, CMD_NEXT_BLOCK },
  { CTRL('U'), CMD_PREV_BLOCK },
  { META('V'), CMD_PREV_BLOCK },
  { KEY_PPAGE, CMD_PREV_BLOCK },
  { 'h', CMD_PREV_FIELD },
  { 'H', CMD_PREV_FIELD },
  { CTRL('B'), CMD_PREV_FIELD },
  { KEY_BACKSPACE, CMD_PREV_FIELD },
  { KEY_DC, CMD_PREV_FIELD },
  { KEY_LEFT, CMD_PREV_FIELD },
  { 'l', CMD_NEXT_FIELD },
  { 'L', CMD_NEXT_FIELD },
  { CTRL('F'), CMD_NEXT_FIELD },
  { KEY_RIGHT, CMD_NEXT_FIELD },
  { CTRL('N'), CMD_NEXT_LINE },
  { KEY_DOWN, CMD_NEXT_LINE },
  { 'J', CMD_NEXT_LINE },
  { 'j', CMD_NEXT_LINE },
  { CTRL('P'), CMD_PREV_LINE },
  { KEY_UP, CMD_PREV_LINE },
  { 'K', CMD_PREV_LINE },
  { 'k', CMD_PREV_LINE },
  { '+', CMD_NEXT_SCREEN },
  { KEY_SRIGHT, CMD_NEXT_SCREEN },
  { '-', CMD_PREV_SCREEN },
  { KEY_SLEFT, CMD_PREV_SCREEN },
  { 'f',CMD_FLAG_ADJUST },
  { 'F',CMD_FLAG_ADJUST },
  { CTRL('I'),CMD_TOGGLE_ASCII },
  { CTRL('R'),CMD_FIND_INODE },
  { '0', REC_FILE0 },
  { '1', REC_FILE1 },
  { '2', REC_FILE2 },
  { '3', REC_FILE3 },
  { '4', REC_FILE4 },
  { '5', REC_FILE5 },
  { '6', REC_FILE6 },
  { '7', REC_FILE7 },
  { '8', REC_FILE8 },
  { '9', REC_FILE9 },
  { '!', REC_FILE10 },
  { '@', REC_FILE11 },
  { '#', REC_FILE12 },
  { '$', REC_FILE13 },
  { '%', REC_FILE14 },
  { 0, 0 }
};

static void highlight_block(int cur_row,int cur_col,int win_start,unsigned char *block_buffer, int ascii_flag);
static void full_highlight_block(int cur_row,int cur_col, int *prev_row, int *prev_col, 
			  int win_start,unsigned char *block_buffer, int ascii_flag);
static void update_block_help(void);


/* Dump as much of a block as we can to the screen and format it in a nice 
 * hex mode with ASCII printables off to the right. */
void cdump_block(unsigned long nr, unsigned char *dind, int win_start, int win_size)
{
  int i,j;
  unsigned char c;
  char *block_status, block_not_used[10]=":NOT:USED:"; 
  char block_is_used[10] = "::::::::::";
 
  clobber_window(workspace); 
  workspace = newwin(VERT,(COLS-HOFF),HEADER_SIZE,HOFF);
  werase(workspace);
  
  block_status = (FS_cmd.zone_in_use(nr)) ? block_is_used : block_not_used; 
  j = 0;

  while ((j<win_size)&&(j*16+win_start<sb->blocksize)) {
    mvwprintw(workspace,j,0,"0x%04X = ",j*16+win_start);
    for (i=0;i<8;i++)
      mvwprintw(workspace,j,9+i*3,"%2.2X",dind[j*16+i+win_start]);
    mvwprintw(workspace,j,34,"%c",block_status[j%10]);
    for (i=0;i<8;i++)
      mvwprintw(workspace,j,37+i*3,"%2.2X",dind[j*16+i+8+win_start]);
    
    for (i=0;i<16;i++) {
      c = dind[j*16+i+win_start];
      c = ((c>31)&&(c<127)) ? c : '.';
      mvwaddch(workspace,j,63+i,c);
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
void cwrite_block(unsigned long block_nr, void *data_buffer, char *modified)
{
  int c;
  char *warning;

  if (*modified) {
    if (!write_ok)
      warning = "(NOTE: write permission not set on disk, use 'F' to set flags before 'Y') ";
    else
      warning = "";
    
    while ( (c = cquery("WRITE OUT BLOCK DATA TO DISK [Y/N]? ","ynfq",warning)) == 'f') {
      flag_popup();
      if (write_ok) warning = "";
    }

    refresh_all(); /* Have to refresh screen before write or error messages will be lost */
    if (c == 'y')
      write_block(block_nr, data_buffer);
  }
  
  *modified = 0;
}

/* Highlights the current position in the block in both ASCII + hex */
static void highlight_block(int cur_row,int cur_col,int win_start,unsigned char *block_buffer, int ascii_flag)
{
  unsigned char c;
  int s_col;

  if (cur_col < 8)
    s_col = 9 + cur_col*3;
  else
    s_col = 13 + cur_col*3;
  mvwprintw(workspace,cur_row,s_col,"%2.2X",block_buffer[cur_row*16+cur_col+win_start]);
  c = block_buffer[cur_row*16+cur_col+win_start];
  c = ((c>31)&&(c<127)) ? c : '.';
  mvwprintw(workspace,cur_row,cur_col+63,"%c",c);
  if (ascii_flag) 
    wmove(workspace,cur_row,cur_col+63);
  else
    wmove(workspace,cur_row,s_col);
}

/* Highlight current cursor position, turn off last highlight */
static void full_highlight_block(int cur_row,int cur_col, int *prev_row, int *prev_col, 
			  int win_start,unsigned char *block_buffer, int ascii_flag)
{
  /* First turn off last position */
  highlight_block(*prev_row,*prev_col,win_start,block_buffer,ascii_flag);

  /* Now highlight current */
  wattron(workspace,WHITE_ON_RED);
  *prev_col = cur_col;
  *prev_row = cur_row;
  highlight_block(cur_row,cur_col,win_start,block_buffer,ascii_flag);
  wattroff(workspace,WHITE_ON_RED);

  return;
}

/* Modifies the help screen display to show the proper sizes for block and inode pointers */
static void update_block_help(void)
{
  static char help_1[80], help_8[80];

  sprintf(help_1,"B       : View block under cursor (%s block ptr is %d bytes).",
	  text_names[fsc->FS],fsc->ZONE_ENTRY_SIZE);
  sprintf(help_8,"I       : View inode under cursor (%s inode ptr is %d bytes).",
	  text_names[fsc->FS],fsc->INODE_ENTRY_SIZE);
  block_help[1] = help_1;
  block_help[8] = help_8;

  return;
}

/* This is the curses menu-type system for block mode, displaying a block
 * on the screen, next/previous block, paging this block, etc., etc. */
int block_mode(void) {
  int icount = 0, val, v1 = 0, c = CMD_REFRESH;
  int win_start = 0;
  int cur_row = 0, cur_col = 0;
  int prev_row = 0, prev_col = 0;
  static unsigned char *copy_buffer = NULL;
  unsigned char block_buffer[MAX_BLOCK_SIZE];
  char *HEX_PTR, *HEX_NOS = "0123456789ABCDEF";
  unsigned long temp_ptr, inode_ptr[2] = { 0UL, 0UL };
  struct bm_flags flags = { 0, 0, 0, 0, 0, 1 };

  if (current_block >= sb->nzones)
    current_block=sb->nzones-1;

  update_block_help();

  display_trailer("H/F1 for help.  F2/^O for menu.  Q to quit",
		  "PG_UP/DOWN = previous/next block, or '#' to enter block number");

  while (flags.dontwait||(c = mgetch())) {
    flags.redraw = flags.highlight = 0;
    if (!flags.dontwait && flags.edit_block) {
      if (!flags.ascii_mode) {
	HEX_PTR = strchr(HEX_NOS, toupper(c));
	if ((HEX_PTR != NULL)&&(*HEX_PTR)) {
	  val = HEX_PTR - HEX_NOS;
	  if (!icount) {
	    icount = 1;
	    v1 = val;
	    wattron(workspace,WHITE_ON_RED);
	    waddch(workspace,HEX_NOS[val]);
	    wattroff(workspace,WHITE_ON_RED);
	    c = CMD_NO_ACTION;
	  } else {
	    icount = 0;
	    v1 = v1*16 + val;
	    block_buffer[cur_row*16+cur_col+win_start] = v1;
	    flags.modified = flags.highlight = 1;
	    c = KEY_RIGHT; /* Advance the cursor */
	  }
	}
      } else { /* ASCII MODE */
	if ((c>31)&&(c<127)) {
	  block_buffer[cur_row*16+cur_col+win_start] = c;
	  flags.highlight = flags.modified = 1;
	  c = KEY_RIGHT; /* Advance the cursor */
	}
      }
    }
    
    if (flags.dontwait) {
      flags.dontwait = 0;
    } else {
      c = lookup_key(c, blockmode_keymap);
    }
    
    switch(c) {

      case CMD_TOGGLE_ASCII: /* Toggle Ascii/Hex edit mode */
	flags.ascii_mode = 1 - flags.ascii_mode;
	flags.highlight = 1;
        break;

      case CMD_NEXT_FIELD:
	flags.highlight = 1;
	icount = 0;
	if (++cur_col > 15) {
	  cur_col = 0;
	  if ((++cur_row)*16+win_start>=sb->blocksize) {
	    cur_col = 15;
	    cur_row--;
	  } else if (cur_row >= VERT) {
	    if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	    prev_row = prev_col = cur_row = 0;
	    flags.redraw = 1;
	  }
	}
	break;

      case CMD_PREV_FIELD:
	flags.highlight = 1;
	icount = 0;
	if (--cur_col < 0) {
	  cur_col = 15;
	  if (--cur_row < 0) {
	    if (win_start - VERT*16 >= 0) {
	      win_start -= VERT*16;
	      cur_row = VERT - 1;
	      flags.redraw = 1;
	    } else
	      cur_row = cur_col = 0;
	  }
	}
	break;

      case CMD_NEXT_SCREEN:
        if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	flags.redraw = 1;
	icount = 0;
	break;

      case CMD_PREV_SCREEN:
	if (win_start - VERT*16 >= 0)  win_start -= VERT*16;
	icount = 0;
	flags.redraw = 1;
	break;

      case CMD_NEXT_LINE:
	flags.highlight = 1;
	if (++cur_row >= VERT) {
	  flags.redraw = 1;
	  if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	  prev_row = prev_col = cur_row = 0;
	}
	icount = 0;
	break;

      case CMD_PREV_LINE:
	flags.highlight = 1;
	icount = 0;
	if (--cur_row<0) {
	  flags.redraw = 1;
	  if (win_start - VERT*16 >= 0) {
	    win_start -= VERT*16;
	    cur_row = VERT - 1;
	  } else
	    cur_row = flags.redraw = 0;
	  prev_col = prev_row = 0;
	}
	break;

      case CMD_NEXT_BLOCK:
        cwrite_block(current_block, block_buffer, &flags.modified);
	if (++current_block >= sb->nzones)
	  current_block=sb->nzones-1;
	flags.edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	flags.redraw = 1;
	break;

      case CMD_PREV_BLOCK:
        cwrite_block(current_block, block_buffer, &flags.modified);
	if (current_block!=0) current_block--;
	flags.edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	flags.redraw = 1;
	break;

      case CMD_WRITE_CHANGES: /* Write out modified block to disk */
	flags.edit_block = 0;
	cwrite_block(current_block, block_buffer, &flags.modified);
	break;

      case CMD_ABORT_EDIT: /* Abort any changes to block */
	flags.modified = flags.ascii_mode = flags.edit_block = 0;
	memcpy(block_buffer, cache_read_block(current_block,CACHEABLE),
	       sb->blocksize);
	flags.redraw = flags.highlight = 1;
	break;

      case CMD_FIND_INODE: /* Find an inode which references this block */
	if ( (temp_ptr = find_inode(current_block)) )
	  warn("Block is indexed under inode 0x%lX.",temp_ptr);
	else
	  if (rec_flags.search_all)
	    warn("Unable to find inode referenece.");
	  else
	    warn("Unable to find inode referenece try activating the --all option.\n");
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
	fake_inode_zones[c-REC_FILE0] = current_block;
	update_header();
	beep();
	break;

      case CMD_COPY: /* Copy block to copy buffer */
	if (!copy_buffer) copy_buffer = malloc(sb->blocksize);
	memcpy(copy_buffer,block_buffer,sb->blocksize);
	warn("Block (%lu) copied into copy buffer.",current_block);
	break;

      case CMD_PASTE: /* Paste block from copy buffer */
	if (copy_buffer) {
	  flags.modified = 1;
	  memcpy(block_buffer, copy_buffer, sb->blocksize);
	  if (!write_ok) 
	    warn("Turn on write permissions before saving this block");
	}
	break;

      case CMD_EDIT: /* Edit the current block */
	flags.edit_block = 1;
	icount = 0;
	if (!write_ok)
	  warn("Disk not writeable, change status flags with (F)");
	break;

      case CMD_VIEW_AS_DIR: /* View the current block as a directory */
	c = directory_popup(current_block);
	flags.redraw = 1;
	break;

      case CMD_BLOCK_MODE_MC: /* View the block under the cursor */
	temp_ptr = block_pointer(&block_buffer[cur_row*16+cur_col+win_start],
				 (unsigned long) 0, fsc->ZONE_ENTRY_SIZE);
	if (temp_ptr < sb->nzones) {
	  current_block = temp_ptr;
	  flags.edit_block = win_start = prev_col = prev_row =
	    cur_col = cur_row = 0;
	  flags.redraw = 1;
	}
	break;

      case CMD_FLAG_ADJUST: /* Popup menu of user adjustable flags */
	flag_popup();
	break;

      case CMD_INODE_MODE_MC: /* Switch to Inode mode,
			       * make this inode the current inode */
	cwrite_block(current_block, block_buffer, &flags.modified);
	temp_ptr = block_pointer(&block_buffer[cur_row*16+cur_col+win_start],
				 (unsigned long) 0, fsc->INODE_ENTRY_SIZE);
	if (temp_ptr <= sb->ninodes) {
	  current_inode = temp_ptr;
	  return CMD_INODE_MODE;
	} else
	  warn("Inode (%lu) out of range in block_mode().",temp_ptr);
	break;

      case CMD_EXIT_PROG: /* Switch to another mode */
      case CMD_VIEW_SUPER:
      case CMD_INODE_MODE:
      case CMD_RECOVERY_MODE:
        cwrite_block(current_block, block_buffer, &flags.modified);
	return c;
	break;

      case CMD_DISPLAY_LOG: /* View error log */
	c = error_popup();
	break;

      case CMD_DO_RECOVER: /* Append current block to the recovery file */
	inode_ptr[0] = current_block;
	crecover_file(inode_ptr);
	break;

      case CMD_NUMERIC_REF: /* Jump to a block by numeric reference */
        cwrite_block(current_block, block_buffer, &flags.modified);
        if (cread_num("Enter block number (leading 0x or $ indicates hex):",&current_block)) {
	  flags.edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	  flags.redraw = 1;
	  if (current_block >= sb->nzones)
	    current_block=sb->nzones-1;
	}
	break;

      case CMD_HELP: /* HELP */
	do_scroll_help(block_help, FANCY);
	refresh_all();
	break;

      case CMD_CALL_MENU: /* POPUP MENU */
	c = do_popup_menu(block_menu);
	if (c==CMD_CALL_MENU)
	  c = do_popup_menu(edit_menu);
	if (c!=CMD_NO_ACTION)
	  flags.dontwait = 1;
	break;

      case CMD_REFRESH: /* Refresh screen */
	icount = 0;  /* We want to see actual values? */
	refresh_ht();
	flags.redraw = 1;
	break;

      default:
	continue;
    }

    /* More room on screen, but have we gone past the end of the block? */
    if (cur_row*16+win_start>=sb->blocksize)
      cur_row = (sb->blocksize-win_start)/16 - 1;

    if (flags.redraw) {
      if (!flags.edit_block)
	memcpy(block_buffer, cache_read_block(current_block,CACHEABLE),
	       sb->blocksize);
      cdump_block(current_block,block_buffer,win_start,VERT);
      flags.highlight = 1;
    }

    /* Highlight current cursor position */
    if (flags.highlight)
      full_highlight_block(cur_row,cur_col,&prev_row,&prev_col,
			   win_start,block_buffer,flags.ascii_mode);

    wrefresh(workspace);
  }
  return 0;
}


