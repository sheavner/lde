/*
 *  lde/nc_dir.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_dir.c,v 1.2 1994/04/24 20:36:54 sdh Exp $
 */

#include "nc_lde.h"

/* Help for directory_popup() function */
static char *dp_help[] = {
  "d      : expand directory under cursor",
  "D      : expand directory under cursor and make it the current inode.",
  "i      : make inode under cursor the current inode.",
  "I      : make inode under cursor the current inode and view it.",
  "n      : view next block in directory (if called from inode mode).",
  "arrows : scroll window.",
  NULL
};

/* Dumps a one line display of a directory entry */
int dump_dir_entry(WINDOW *win, int i, int off, unsigned long bnr, unsigned long *inode_nr)
{
#ifdef ALPHA_CODE
  char *fname = NULL, *block_buffer = NULL;
  struct Generic_Inode *GInode;
  char f_mode[12];

  /* Need to re-read this every time because the inode operations
   * may have read another block into the buffer_cache. */
  block_buffer = cache_read_block(bnr,CACHEABLE);

  fname = FS_cmd.dir_entry(i+off, block_buffer, inode_nr);
  if (!strlen(fname)) return 0;
 
  mvwprintw(win,i,0,"0x%8.8lX:", *inode_nr);
  if (*inode_nr > sb->ninodes) *inode_nr = 0UL;
  if (*inode_nr) {
    GInode = FS_cmd.read_inode(*inode_nr);
    mode_string( (unsigned short)GInode->i_mode, f_mode);
    f_mode[10] = 0;
    mvwprintw(win,i,12,"%9s %3d %9ld", 
	      f_mode, GInode->i_links_count,
	      GInode->i_size);
  }
  mvwprintw(win,i,37, fname);
#endif
  return 1;
}

/* Let's highlight things the way curses intended, or at least the way I
 * intended after reading the curses man pages. */
void highlight_dir_entry(WINDOW *win, int nr)
{
#ifdef NCURSES_IS_COOL
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
#endif
  wmove(win, nr, 0);
}

/* Display a scrollable directory window.  It only displays info on the current
 * block, if there are fs's which have directory entries which span more than one
 * block, they may run into trouble. */
int directory_popup(unsigned long bnr)
{
#ifdef ALPHA_CODE
  int i, c, redraw, flag, max_entries, screen_off, current;
  unsigned long inode_nr;
  char *block_buffer;
  struct Generic_Inode *GInode;
  WINDOW *win;

  win = newwin(VERT,COLS,HEADER_SIZE,0);
  max_entries = screen_off = current = 0;
  scrollok(win, TRUE);
  
  flag=1; c = ' ';
  while (flag||(c = mgetch())) {
    flag = 0;
    redraw = 0;
    switch (c) {
      case 'i':
      case 'I':
	(void) FS_cmd.dir_entry(current+screen_off, cache_read_block(bnr,CACHEABLE), &inode_nr);
	if (inode_nr) {
	  current_inode = inode_nr;
	  update_header();
	  if (c=='I') {
	    clobber_window(win);
	    return 'i';
	  }
	}
	break;
      case 'N':
      case 'n':
      case 'Q':
      case 'q':
        clobber_window(win);
        refresh_ht();
        return (tolower(c) == 'q') ? ' ' : c;
	break;
      case 'D':
      case 'd':
	(void) FS_cmd.dir_entry(current+screen_off, cache_read_block(bnr,CACHEABLE), &inode_nr);
	if (inode_nr) {
	  GInode = FS_cmd.read_inode(inode_nr);
	  if (S_ISDIR(GInode->i_mode)) {
	    bnr = GInode->i_zone[0];
	    max_entries = current = screen_off = 0;
	    redraw = 1;
	    if (c =='D') {
	      current_inode = inode_nr;
	      update_header();
	    }
	  }
	}
	break;
      case CTRL('L'):
        refresh_ht();
	redraw = 1;
        break;
      case CTRL('N'):
      case KEY_DOWN:
      case 'J':
      case 'j':
	if ((current<(VERT-1))&&((current+screen_off)<max_entries))
	  current++;
	else if ((current==(VERT-1))&&((current+screen_off)<max_entries)) {
	  screen_off++;
	  wscrl(win,1);
	  (void) dump_dir_entry(win, current, screen_off, bnr, &inode_nr);
	  wrefresh(win);
	}
	break;
      case CTRL('P'):
      case KEY_UP:
      case 'K':
      case 'k':
	if (current>0)
	  current--;
	else if ((current==0)&&(screen_off>0)) {
	  screen_off--;
	  wscrl(win,-1);
	  dump_dir_entry(win, current, screen_off, bnr, &inode_nr);
	  wrefresh(win);
	}
	break;
      case '?':
      case KEY_F(1):
      case CTRL('H'):
      case META('H'):
      case META('h'):
        do_scroll_help(dp_help, FANCY);
	redraw_win(win);
        redraw = 0;
        break;
      case ' ':
	redraw = 1;
	break;
    }

    if (!max_entries) {
      block_buffer = cache_read_block(bnr,CACHEABLE);
      i = -1;
      while ( strlen(FS_cmd.dir_entry(++i, block_buffer, &inode_nr)) );
      max_entries = i - 1;
    }

    if (redraw) {
      wclear(win);
      for (i=0;((i<VERT)&&(i<=max_entries));i++)
	(void) dump_dir_entry(win, i, screen_off, bnr, &inode_nr);
    }

    highlight_dir_entry(win, current);
    wrefresh(win);

  }
#endif ALPHA_CODE
  return 0;
}
