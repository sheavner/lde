/*
 *  lde/nc_dir.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_dir.c,v 1.18 2001/02/21 20:01:04 sdh Exp $
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>

#include "lde.h"
#include "tty_lde.h"
#include "curses.h"
#include "nc_lde.h"
#include "nc_dir.h"
#include "keymap.h"
#include "recover.h"
#include "swiped.h"

static int dump_dir_entry(WINDOW *win, int i, int off, lde_buffer *block_buffer, int highlight);
static void highlight_dir_entry(WINDOW *win, int nr, int *last, int screen_off, lde_buffer *buffer);
static void redraw_dir_window(WINDOW *win,int max_entries, int screen_off, lde_buffer *buffer);
static int reread_dir(lde_buffer *block_buffer, unsigned long bnr);

/* Help for directory_popup() function */
static lde_menu dp_help[] = {
  { CMD_EXPAND_SUBDIR,"expand directory under cursor"},
  { CMD_EXPAND_SUBDIR_MC,"expand directory under cursor and make it the current inode"},
  { CMD_INODE_MODE,"make inode under cursor the current inode"},
  { CMD_INODE_MODE_MC,"make inode under cursor the current inode and view it"},
  { 0, NULL }
};

/* default keymap for directory mode */
static lde_keymap dirmode_keymap[] = {
  { 'i', CMD_SET_CURRENT_INODE },
  { 'I', CMD_INODE_MODE_MC },
  { 'Q', CMD_EXIT },
  { LDE_ESC, CMD_EXIT },
  { 'd', CMD_EXPAND_SUBDIR },
  { LDE_CTRL('J'), CMD_EXPAND_SUBDIR },
  { LDE_CTRL('M'), CMD_EXPAND_SUBDIR },
  { 'D', CMD_EXPAND_SUBDIR_MC },
  { KEY_NPAGE, CMD_NEXT_SCREEN },
  { LDE_CTRL('V'), CMD_NEXT_SCREEN },
  { LDE_CTRL('D'), CMD_NEXT_SCREEN },
  { LDE_META('V'), CMD_PREV_SCREEN },
  { KEY_PPAGE, CMD_PREV_SCREEN },
  { LDE_CTRL('U'), CMD_PREV_SCREEN },
  { 0, 0 }
};

/* Dumps a one line display of a directory entry */
static int dump_dir_entry(WINDOW *win, int i, int off, lde_buffer *block_buffer, int highlight)
{
  char *fname = NULL;
  struct Generic_Inode *GInode;
  unsigned long inode_nr;
  char f_mode[12] = "----------";

  fname = FS_cmd.dir_entry(i+off, block_buffer, &inode_nr);
  if (!strlen(fname)) return 0;

  if (highlight) wattron(win,WHITE_ON_RED);

  if (inode_nr > sb->ninodes) inode_nr = 0UL;
  if (inode_nr) {
    GInode = FS_cmd.read_inode(inode_nr);
    mode_string( (unsigned short)GInode->i_mode, f_mode);
    f_mode[10] = 0; 
    /* Use COLS-38 to fill to 1 col before edge of screen, COLS-37 makes a mess */
    mvwprintw(win,i,0,"0x%8.8lX: %9s %3d %9ld %-*s", inode_nr,
	      f_mode, GInode->i_links_count,
	      GInode->i_size, COLS-38, fname);
  } else {
    mvwprintw(win,i,0,"0x%8.8lX: %24s %-*s", inode_nr,
	      " ", COLS-38, fname);
  }

  if (highlight) wattroff(win,WHITE_ON_RED);

  return 1;
}

/* Let's highlight things the way curses intended, or at least the way I
 * intended after reading the curses man pages. */
static void highlight_dir_entry(WINDOW *win, int nr, int *last, int screen_off,lde_buffer *buffer)
{
#ifdef MVINSNSTR_FIXED
  static int last_entry = 0;
  char *str = "TESTING";

  /* Just in case we scrolled recently */
  if (nr==0) {
    mvwinsnstr(win, 1, 0, str, 10);
    mvwprintw(win, 1, 0, str);
  } else if (nr==(VERT-1)) {
    mvwinsnstr(win, (VERT-2), 0, str, 10);
    mvwprintw(win, (VERT-2), 0, str);
  }

  /* Turn off highlight at last postion */
  mvwinsnstr(win, last_entry, 0, str, 10);
  mvwprintw(win, last_entry, 0, str);

  /* Highlight new position */
  mvwinsnstr(win, nr, 0, str, 10);
  wattron(win,WHITE_ON_RED);
  mvwprintw(win, nr, 0, str);
  wattroff(win,WHITE_ON_RED);
  
  last_entry = nr;
#else
  /* Can get rid of all calling parameters after 'last' and remove 'last' from directory_popup() if NCURSES gets fixed,
   * also we can remove the highlight feature of dump_dir_entry() */
  (void) dump_dir_entry(win, *last, screen_off, buffer, 0);
  (void) dump_dir_entry(win, nr, screen_off, buffer, 1);
  *last = nr;
#endif
  wmove(win, nr, 0);
  wrefresh(win);
}

/* Redraw the popup window, rescan info */
static void redraw_dir_window(WINDOW *win,int max_entries, int screen_off, lde_buffer *buffer)
{
  int i;
  werase(win);
  for (i=0;((i<VERT)&&(i<=max_entries));i++)
    (void) dump_dir_entry(win, i, screen_off, buffer, 0);
  redraw_win(win);
}

/* Read the block buffer from disk, return the number of dir entries contained in this block */
static int reread_dir(lde_buffer *block_buffer, unsigned long bnr)
{
  int i;
  unsigned long inode_nr;

  if (bnr)
    memcpy(block_buffer->start, cache_read_block(bnr,NULL,CACHEABLE),block_buffer->size);
  for (i=-1; strlen(FS_cmd.dir_entry(++i, block_buffer, &inode_nr)); );
  return (--i);
}

/* Read the block buffer from disk, return the number of dir entries contained in this block */
static int get_inode_info(unsigned long inode_nr, lde_buffer *buffer)
{
  struct Generic_Inode *GInode;
  unsigned long bnr, c, count;

  GInode = FS_cmd.read_inode(inode_nr);
  if (buffer->start)
    free(buffer->start);
  buffer->size = GInode->i_size+sb->blocksize;
  buffer->start = malloc(buffer->size);
  bzero(buffer->start, (int)buffer->size);
  c = count = 0UL;
  while (count < GInode->i_size) {
    map_block(GInode->i_zone,c,&bnr);
    if (bnr)
      count += (unsigned long) 
	       nocache_read_block(bnr,((buffer->start)+c*sb->blocksize),
				  lookup_blocksize(bnr));
    c++;
  }

  return 0;
}


/* Display a scrollable directory window.  It only displays info on the current
 * block, if there are fs's which have directory entries which span more than one
 * block, they may run into trouble. */
int directory_popup(unsigned long bnr, unsigned long inode_nr, unsigned long ipointer)
{
  int c, max_entries, screen_off, current, last;
  lde_buffer _block_buffer = EMPTY_LDE_BUFFER, *block_buffer = &_block_buffer;
  struct Generic_Inode *GInode;
  WINDOW *win;

  win = newwin(VERT,COLS,HEADER_SIZE,0);
  werase(win);
  scrollok(win, TRUE);
  screen_off = current = last = 0;

  /* If passed block number is 0, lookup passed inode */
  if (!bnr) {
    get_inode_info(inode_nr, block_buffer);
  } else {
    block_buffer->size = sb->blocksize;
    block_buffer->start = malloc(block_buffer->size);
  }

  max_entries = reread_dir(block_buffer, bnr);
  redraw_dir_window(win, max_entries, screen_off, block_buffer);
  highlight_dir_entry(win,current,&last,screen_off,block_buffer);
  
  while ( (c=lookup_key(mgetch(),dirmode_keymap)) ) {

    switch (c) {
      case CMD_SET_CURRENT_INODE: /* Set this inode to be the current inode */
      case CMD_INODE_MODE_MC:
	(void) FS_cmd.dir_entry(current+screen_off, block_buffer, &inode_nr);
	if (inode_nr) {
	  current_inode = inode_nr;
	  update_header();
	  if (c==CMD_INODE_MODE_MC) {
	    clobber_window(win);
	    free(block_buffer->start);
	    return CMD_INODE_MODE_MC;
	  }
	}
	break;

      case CMD_EXIT: /* Exit popup */
        clobber_window(win);
	free(block_buffer->start);
        return CMD_NO_ACTION;
	break;

      case CMD_EXPAND_SUBDIR: /* Expand this subdirectory - 'D' also sets current inode to be this subdir */
      case CMD_EXPAND_SUBDIR_MC:
	(void) FS_cmd.dir_entry(current+screen_off, block_buffer, &inode_nr);
	if (inode_nr) {
	  GInode = FS_cmd.read_inode(inode_nr);
	  if (S_ISDIR(GInode->i_mode)) {
	    get_inode_info(inode_nr, block_buffer);
	    current = screen_off = last = 0;
	    max_entries = reread_dir(block_buffer, 0UL);
	    redraw_dir_window(win, max_entries, screen_off, block_buffer);
	    if (c == CMD_EXPAND_SUBDIR_MC) {
	      current_inode = inode_nr;
	      update_header();
	    }
	  }
	}
	break;

      case CMD_REFRESH: /* Refresh screen */
        refresh_ht();   /* not refresh_all() b/c calling window is still active which yields ugly flashing */
        wclear(win);
	redraw_dir_window(win, max_entries, screen_off, block_buffer);
        break;

      case CMD_NEXT_SCREEN: /* Next screen w/scroll */
	/* Next two lines remove red bar for a second while we scroll */
	(void) dump_dir_entry(win, last, screen_off, block_buffer, 0);
	wrefresh(win);
	/* Now just do CMD_PREV_LINE to cover 1/2 the screen */
        for (c=0; c<VERT/2;c++) {
	  if ((current<(VERT-1))&&((current+screen_off)<max_entries)) {
	    current++;
	  } else if ((current==(VERT-1))&&((current+screen_off)<max_entries)) {
	    last--;
	    wscrl(win,1);
	    (void) dump_dir_entry(win, current, ++screen_off, block_buffer, 0);
	  }
	}
	break;

      case CMD_NEXT_LINE: /* Next line w/scroll */
	if ((current<(VERT-1))&&((current+screen_off)<max_entries)) {
	  current++;
	} else if ((current==(VERT-1))&&((current+screen_off)<max_entries)) {
	  last--;
	  wscrl(win,1);
	  (void) dump_dir_entry(win, current, ++screen_off, block_buffer, 0);
	}
	break;

      case CMD_PREV_SCREEN: /* Previous screen w/scroll */
	/* Next two lines remove red bar for a second while we scroll */
	(void) dump_dir_entry(win, current, screen_off, block_buffer, 0);
	wrefresh(win);
	/* Now just do CMD_PREV_LINE to cover 1/2 the screen */
        for (c=0; c<VERT/2;c++) {
	  if (current>0) {
	    current--;
	  } else if ((current==0)&&(screen_off>0)) {
	    last++;
	    wscrl(win,-1);
	    (void) dump_dir_entry(win, current, --screen_off, block_buffer, 0);
	  }
	}
	break;

      case CMD_PREV_LINE: /* Previous line w/scroll */
	if (current>0) {
	  current--;
	} else if ((current==0)&&(screen_off>0)) {
	  last++;
	  wscrl(win,-1);
	  (void) dump_dir_entry(win, current, --screen_off, block_buffer, 0);
	}
	break;

      case CMD_HELP: /* Help */
        do_new_scroll_help(dp_help, dirmode_keymap, FANCY);
	touchwin(win);
        break;

      default:
        continue;
    }

    highlight_dir_entry(win,current,&last,screen_off,block_buffer);
  }

  return 0;
}









