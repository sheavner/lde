/*
 *  lde/nc_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_lde.c,v 1.22 1996/09/15 05:19:29 sdh Exp $
 */

#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#include "lde.h"
#include "recover.h"
#include "tty_lde.h"

#include "curses.h"
#include "nc_lde.h"
#include "nc_lde_help.h"
#include "nc_inode.h"
#include "nc_block.h"
#include "keymap.h"
  

/* This will recognize escapes keys as "META" strokes */
int nc_mgetch(void)
{
  int c;

  if ( (c=getch()) == ESC )
    if ( (c=getch()) == ESC )
      return c;
    else
      return META(c);

  return c;
}
    
/* Touches a window then refreshes it to make sure it gets updated */
void redraw_win(WINDOW *win)
{
  touchwin(win);
  wrefresh(win);
}

/* Print a string in a window, return lower case key pressed. */
int cquery(char *data_string, char *data_options, char *warn_string)
{
  WINDOW *win;
  int c;

  c = 255;
  win = newwin(5,WIN_COL,((VERT-5)/2+HEADER_SIZE),HOFF);
  werase(win);
  box(win,0,0);
  if (strlen(warn_string)) {
    if (!lde_flags.quiet) beep();
    mvwprintw(win,3,(WIN_COL-strlen(warn_string))/2-1,"%s",warn_string);
  }
  mvwprintw(win,2,(WIN_COL-strlen(data_string))/2-1,"%s",data_string);

  wrefresh(win);
  while (!strchr(data_options,c))
      c = tolower(mgetch());

  delwin(win);
  refresh_all();
  return c;
}

/* This moves to the bottom of the screen, writes whatever is in coutput,
 * and reads a long int from the user.  It recognizes indicators as to the 
 * type of input for decimal, hex, or octal.  For example if we wanted to
 * input 15, we could enter as hex (0xf, xf, $f), decimal (15), or octal
 * (\017).  sscanf is probably smart enough to recognize these, but 
 * what the hell.  If coutput begins with a space, cinput is returned in
 * coutput and the number processing does not occurr.
 */
int cread_num(char *coutput, unsigned long *a)
{
  char cinput[80];

#if TRAILER_SIZE>0
#define window_available trailer
#define LINE_NUMBER TRAILER_SIZE-1
#else
  win *window_avaliable;
  window_avaliable = newwin(5,WIN_COL,((VERT-5)/2+HEADER_SIZE),HOFF);
  werase(window_avaliable);
  box(window_avaliable,0,0);
#define LINE_NUMBER 0
#endif

  wmove(window_available,LINE_NUMBER,0);
  wclrtoeol(window_available);
  mvwprintw(window_available,LINE_NUMBER,
    (COLS-strlen(coutput))/2-5,"%s ",coutput);
  wrefresh(window_available);
  echo();
  wgetstr(window_available,cinput);
  noecho();
  wmove(window_available,LINE_NUMBER,0);
  wclrtoeol(window_available);
  wrefresh(window_available);
#if TRAILER_SIZE<=0
  delwin(window_available);
#endif

  if (coutput[0]!=' ') {
    *a = read_num(cinput);
    return strlen(cinput);
  } else {
    strncpy(coutput, cinput, 80);
    return strlen(cinput);
  }

  return 0;
}

/* Displays one line in the trailer window */
void display_trailer(char *line1)
{
#if TRAILER_SIZE>0
  werase(trailer);
  if (line1 != NULL)
    mvwaddstr(trailer,0,(COLS-strlen(line1))/2-1,line1);
  wrefresh(trailer);
#endif
}

/* This fills in the header window -- 
 * things like the program name, current inode, tagged recovered blocks
 */
void update_header(void)
{
#if HEADER_SIZE>0
  int i,j;

  mvwprintw(header,HEADER_SIZE-1,HOFF,"Inode: %10lu (0x%8.8lX) ",current_inode,current_inode);
  mvwprintw(header,HEADER_SIZE-1,HOFF+32,"Block: %10lu (0x%8.8lX) ",current_block,current_block);
  for (j=0;j<fsc->N_BLOCKS;j++) {
    if (fake_inode_zones[j]) { 
      mvwaddch(header,HEADER_SIZE-1,HOFF+64+j,'-');
    } else {
      for (i=0;global_keymap[i].action_code;i++)
        if ((j+REC_FILE0)==global_keymap[i].action_code) {
          mvwaddch(header,HEADER_SIZE-1,HOFF+64+j,global_keymap[i].key_code);
          break;
        }
      
      if (!global_keymap[i].key_code) {
        mvwaddch(header,HEADER_SIZE-1,HOFF+64+j,'?');
        continue;
      }
    }
  }
  redraw_win(header);
#endif
}

/* This should totally clear out an old window */
void clobber_window(WINDOW *win)
{
  werase(win);
  wrefresh(win);
  delwin(win);
}

/* This takes care of the reverse video in the header window,
 * and puts up the device and program name. */
void restore_header(void)
{
#if HEADER_SIZE>0
  int i,j;
  char echo_string[132];

  wattron(header,WHITE_ON_BLUE);

  /* A cute way to RVS video the header window, since erase and clear don't seem to
   * do it for me. */
  for (i=0;i<HEADER_SIZE;i++)
    for (j=0;j<COLS;j++)
      mvwaddch(header,i,j,' ');

  sprintf(echo_string,"%s v%s : %s : %s",
	  program_name,VERSION,text_names[fsc->FS],device_name);
  mvwprintw(header,0,(COLS-strlen(echo_string))/2-1,echo_string);
  update_header();
#endif
}

/* Redraw header and trailer, other routine does workspace -- ^L */
void refresh_ht(void)
{
#if TRAILER_SIZE>0
  redraw_win(trailer);
#endif
  restore_header();
}

/* Redraw everything -- ^L */
void refresh_all(void)
{
  redraw_win(stdscr);
  refresh_ht();
  redraw_win(workspace);
}

/* Displays a warning string in the trailer window (if it is defined)
 * and log the error for later */
void nc_warn(char *fmt, ...)
{
  va_list argp;
  char echo_string[132];

  va_start(argp, fmt);
  vsprintf(echo_string, fmt, argp);
  va_end(argp);

  log_error(echo_string);
 
  if (!lde_flags.quiet) beep();
#if TRAILER_SIZE>0
  wmove(trailer,TRAILER_SIZE-1,0);
  wclrtoeol(trailer);
  mvwaddstr(trailer,TRAILER_SIZE-1,(COLS-strlen(echo_string))/2-1,
	    echo_string);
  wrefresh(trailer);
  wmove(trailer,TRAILER_SIZE-1,0);
  wclrtoeol(trailer);
  /* Still has to be refreshed by someone */
#endif
}

/* Dump the error log to a window */
int error_popup(void)
{
  int  present_error, i;
  char *errors[ERRORS_SAVED+1];

  /* Stick a null in the last saved error to signal then end of the array
   * for do_scroll_help() call */
  errors[ERRORS_SAVED] = NULL;

  /* Sort errors, most recent on top (=0) */
  for (i=0;(i<ERRORS_SAVED); i++) {
    present_error = current_error - i;
    if (present_error<0) present_error += ERRORS_SAVED;
    errors[i] = error_save[present_error];
  }

  do_scroll_help(errors, (PLAIN|HELP_BOXED));

  refresh_all();
  return 0;
}

/* Popup menu */
int do_popup_menu(lde_menu menu[], lde_keymap keys[])
{
  WINDOW *win, *bigwin;
  int    c, window_length, window_width, length, width, last_highlight = 0, highlight = 0;

  /* Find size of longest descriptor entry */
  length = width = 0;
  while (menu[length].description!=NULL) {
    width = (strlen(menu[length].description) > width) ?
      strlen(menu[length].description)+1 : width;
    length++;
  }

  /* Our window size (has border 1 char wide on all sides) 
   * - adjust width for 4 character entries for key names */
  window_length = ((length+2)<VERT) ? (length+2) : VERT;
  window_width  = ((width+2+4)<WIN_COL) ? (width+2+4) : WIN_COL;

  /* Create the window */
  bigwin = newwin(window_length,window_width,HEADER_SIZE,0);

  /* Readjust our size parameters to account for the border */
  window_length -= 2;
  window_width  -= 2;

  /* Create a little with no borders inside the bordered window */
  win = derwin(bigwin, window_length, window_width, 1, 1);

  /* Set some curses flags and draw box around window */
  scrollok(win, TRUE);
  werase(bigwin);
  box(bigwin,0,0);

  /* Draw the menu once, highlight the appropriate entry */
  for (c=0; c < window_length; c++) {
    if (c==highlight)
      wattron(win,WHITE_ON_RED);
    mvwprintw(win,c,0,menu[c].description);
    if (c==highlight)
      wattroff(win,WHITE_ON_RED);
    mvwprintw(win,c,width,text_key(menu[c].action_code,keys,0));
  }
  wmove(bigwin,1,1);
  wrefresh(bigwin);

  while ( (c = mgetch()) ) {
    switch (c) {
      case CTRL('P'):
      case KEY_UP:
      case 'k':
      case 'K':
	if (highlight>0) highlight--;
	break;
      case CTRL('N'):
      case KEY_DOWN:
      case 'j':
      case 'J':
	if (highlight<(window_length-1)) highlight++;
	break;
      case CTRL('A'):
      case ESC:
	delwin(win);
	delwin(bigwin);
	refresh_all();
	return 0;
      case KEY_ENTER:
      case CTRL('M'):
      case CTRL('J'):
	delwin(win);
	delwin(bigwin);
	refresh_all();
	return menu[highlight].action_code;
      default:
	if ( (c = lookup_key(c, keys)) != CMD_NO_ACTION ) {
	  delwin(win);
	  delwin(bigwin);
	  refresh_all();
	  return c;
	}
    }

    /* Make sure the right menu choice is highlighted */
    if (highlight != last_highlight) { 
      mvwprintw(win,last_highlight,0,menu[last_highlight].description);
      wattron(win,WHITE_ON_RED);
      mvwprintw(win,highlight,0,menu[highlight].description);
      wattroff(win,WHITE_ON_RED);
      wmove(win,highlight,0);
      wrefresh(win);
      last_highlight = highlight;
    } 
    
  }
    
  return 0;
}

/* Convert action code to key which would ellicit this action */
char *three_text_keys(int c, lde_keymap *kmap)
{
  int i, j, k=0, newentry;
  static char return_string[HELP_KEY_SIZE];
  char *onestring;

  /* Clear the response string */
  return_string[0] = 0;

  for (i=0; i<3; i++) {

    /* Lookup a key that goes along with our command c */
    onestring = text_key(c, kmap, i);

    /* Copy the result from text_key() into our buffer */
    newentry = 0;
    for (j=0; j<5; j++) {
      if (onestring[j]==0) {
	break;
      } else if (onestring[j]!=' ') {
	return_string[k++] = onestring[j];
	newentry = 1;
      }
    }

    /* Break out of for loop if we are about to overflow buffer, remove last entry */
    if (k==HELP_KEY_SIZE) {
      while ((k)&&(onestring[k]!=',')) k--;
      break;
    }

    /* Add a comma after this key */
    if (newentry)
      return_string[k++] = ',';

  }

  if (k!=0)
    return_string[k-1] = 0;

  return return_string;
}

/* Display some help in a separate scrollable window */
void do_new_scroll_help(lde_menu help_text[], lde_keymap *kmap, int fancy)
{
  WINDOW *win, *bigwin;
  int c=CMD_NO_ACTION, i, length, banner_size, win_col, window_size, window_offset = 0;
  char *banner = " lde Help ";

  /* Display options, do we include a banner or a box? */
  banner_size = (fancy&HELP_NO_BANNER)?0:1;
  win_col     = (fancy&HELP_WIDE)?COLS:WIN_COL;
  fancy       = (fancy&HELP_BOXED)?2:0;

  /* Find window vertical size */
  length = -1;
  while (help_text[++length].action_code);
  window_size = ((length+fancy+banner_size)<VERT) ? (length+fancy+banner_size) : VERT;

  /* Create and clear the new window */
  bigwin = newwin(window_size,win_col,((VERT-window_size)/2+HEADER_SIZE),(win_col == WIN_COL) ? HOFF : 0);
  werase(bigwin);

  /* Put a box around the window, if desired */
  if (fancy) {
    win = derwin(bigwin, window_size - 2 - banner_size, win_col - 2, 1+banner_size, 1);
    box(bigwin,0,0);
    mvwprintw(bigwin,(window_size-1),(win_col/2-20)," Arrows to scroll, any other key to continue.");
  } else {
    win = bigwin;
  }

  /* Draw a 1 line header in the new window, if desired */
  if (banner_size) {
    wattron(bigwin,WHITE_ON_BLUE);
    for (i=1;i<win_col-1;i++)
      mvwaddch(bigwin,1,i,' ');
    mvwprintw(bigwin,1,(win_col-strlen(banner))/2-1,banner);
    wattroff(bigwin,WHITE_ON_BLUE);
  }

  leaveok(win,TRUE);
  scrollok(win, TRUE);

  /* Adjust the window size for borders and banners */
  window_size -= fancy + banner_size;

  for (i=0; i<window_size; i++) {
    mvwprintw(win,i,fancy,three_text_keys(help_text[i+window_offset].action_code, kmap));
    mvwprintw(win,i,fancy+HELP_KEY_SIZE,help_text[i+window_offset].description);
  }
  wmove(win,i,fancy);
    
  wrefresh(bigwin);

  /* Display and scroll help */
  while ( (c!=CMD_EXIT)&&(c=lookup_key(mgetch(),help_keymap)) ) {
    i = -1;
    switch (c) {
      case CMD_PREV_LINE:
	if (window_offset>0) {
	  window_offset--;
	  wscrl(win,-1);
	  i = 0;
	}
	break;
      case CMD_NEXT_LINE:
	if ((window_offset+window_size)<length) {
	  window_offset++;
	  wscrl(win,1);
	  i = window_size - 1;
	}
	break;
      default:
	c = CMD_EXIT;
	continue;
      }

    if (i>=0) {
      mvwprintw(win,i,fancy,three_text_keys(help_text[i+window_offset].action_code, kmap));
      mvwprintw(win,i,fancy+HELP_KEY_SIZE,help_text[i+window_offset].description);
      wmove(win,i,fancy);
      wrefresh(win);
    }
  }

  leaveok(win,FALSE);

  /* Cleanup windows */
  delwin(win);
  if (fancy)
    delwin(bigwin);

  return;
}


/* Display some help in a separate scrollable window */
void do_scroll_help(char **help_text, int fancy)
{
  WINDOW *win, *bigwin;
  int c=CMD_NO_ACTION, i, length, banner_size, win_col, window_size, window_offset = 0;
  char *banner = " lde Help ";

  /* Display options, do we include a banner or a box? */
  banner_size = (fancy&HELP_NO_BANNER)?0:1;
  win_col     = (fancy&HELP_WIDE)?COLS:WIN_COL;
  fancy       = (fancy&HELP_BOXED)?2:0;

  /* Find window vertical size */
  length = -1;
  while (help_text[++length]!=NULL);
  window_size = ((length+fancy+banner_size)<VERT) ? (length+fancy+banner_size) : VERT;

  /* Create and clear the new window */
  bigwin = newwin(window_size,win_col,((VERT-window_size)/2+HEADER_SIZE),(win_col == WIN_COL) ? HOFF : 0);
  werase(bigwin);

  /* Put a box around the window, if desired */
  if (fancy) {
    win = derwin(bigwin, window_size - 2 - banner_size, win_col - 2, 1+banner_size, 1);
    box(bigwin,0,0);
    mvwprintw(bigwin,(window_size-1),(win_col/2-20)," Arrows to scroll, any other key to continue.");
  } else {
    win = bigwin;
  }

  /* Draw a 1 line header in the new window, if desired */
  if (banner_size) {
    wattron(bigwin,WHITE_ON_BLUE);
    for (i=1;i<win_col-1;i++)
      mvwaddch(bigwin,1,i,' ');
    mvwprintw(bigwin,1,(win_col-strlen(banner))/2-1,banner);
    wattroff(bigwin,WHITE_ON_BLUE);
  }

  scrollok(win, TRUE);
  leaveok(win,TRUE);

  /* Adjust the window size for borders and banners */
  window_size -= fancy + banner_size;

  for (i=0; i<window_size; i++)
    mvwprintw(win,i,fancy,help_text[i+window_offset]);
  wmove(win,i,fancy);
    
  wrefresh(bigwin);

  /* Display and scroll help */
  while ( (c!=CMD_EXIT)&&(c=lookup_key(mgetch(),help_keymap)) ) {
    i = -1;
    switch (c) {
      case CMD_PREV_LINE:
	if (window_offset>0) {
	  window_offset--;
	  wscrl(win,-1);
	  i = 0;
	}
	break;
      case CMD_NEXT_LINE:
	if ((window_offset+window_size)<length) {
	  window_offset++;
	  wscrl(win,1);
	  i = window_size - 1;
	}
	break;
      default:
	c = CMD_EXIT;
	continue;
      }

    if (i>=0) {
      mvwprintw(win,i,fancy,help_text[i+window_offset]);
      wmove(win,i,fancy);
      wrefresh(win);
    }
  }

  leaveok(win,FALSE);

  /* Cleanup windows */
  delwin(win);
  if (fancy)
    delwin(bigwin);

  return;
}


/* Throw up a list of flags which the user can toggle */
void flag_popup(void)
{
  WINDOW *win;
  int c, redraw, flag;

  win = newwin(7,WIN_COL,((VERT-8)/2+HEADER_SIZE),HOFF);
  werase(win);

  flag = 1; c = ' ';
  while ((flag)||(c = mgetch())) {
    flag = 0;
    redraw = 0;
    switch (c) {
      case 'q':
      case 'Q':
        delwin(win);
	refresh_all();
        return;
	break;
      case 'W':
      case 'w':
        if (!lde_flags.paranoid)
	  lde_flags.write_ok = 1 - lde_flags.write_ok;
	else
	  warn("Device opened read only, do not specify '--paranoid' on the command line");
	redraw = 1;
        break;
      case 'A':
      case 'a':
        lde_flags.search_all = 1 - lde_flags.search_all;
	redraw = 1;
        break;
      case 'N':
      case 'n':
        lde_flags.quiet = 1 - lde_flags.quiet;
	redraw = 1;
        break;
      case CTRL('L'):
        refresh_all();
        break;
      case ' ':
	redraw = 1;
	break;
    }

    werase(win);
    wattron(win,RED_ON_BLACK);
    box(win,0,0);
    wattroff(win,RED_ON_BLACK);
    mvwprintw(win,1,15,"A: (%-3s) Search all blocks",lde_flags.search_all ? "YES" : "NO");
    mvwprintw(win,2,15,"N: (%-3s) Noise is off -- i.e. quiet",lde_flags.quiet ? "YES" : "NO");
    mvwprintw(win,3,15,"W: (%-3s) OK to write to file system",lde_flags.write_ok ? "YES" : "NO");
    mvwprintw(win,5,15,"Q: return to editing");
    wrefresh(win);
  }
}

/* Query the user for a file name, then ask for confirmation,
 * then dump the file from the list of inodes */
void crecover_file(unsigned long inode_zones[])
{
  int fp;
  static char recover_file_name[80];
  char recover_query[80];

  if (recover_file_name[0] == 0) strcpy(recover_file_name,"RECOVERED.file");

  sprintf(recover_query," Write data to file:");
  if (cread_num(recover_query, NULL))
    strncpy(recover_file_name, recover_query, 80);

  if ( (fp = open(recover_file_name,O_RDONLY)) > 0 ) {
    close(fp);
    fp = 0;
    switch (cquery("File exists, append data [Yes/Overwrite/Quit]: ","ynq","")) {
      case 'y':
        fp = open(recover_file_name,O_WRONLY|O_APPEND);
	break;
      case 'o':
	fp = open(recover_file_name,O_WRONLY|O_TRUNC);
	break;
      default:
	fp = 0;
	break;
      }
  } else if ( (fp = open(recover_file_name,O_WRONLY|O_CREAT,0644)) < 0 )
    warn("Cannot open file '%s'",recover_file_name);

  if (fp > 0) {
    recover_file(fp, inode_zones);
    close(fp);
    warn("Recovered data written to '%s'", recover_file_name);
  }
}
  
/* This lists all the tagged inodes */
int recover_mode(void)
{
  int j,c,next_cmd=CMD_REFRESH;
  unsigned long a;
  char recover_labels[INODE_BLKS];
  int vstart,vsize=fsc->N_BLOCKS;

  /* Fill in keys used to change blocks in recover mode */
  for (j=REC_FILE0; j<REC_FILE_LAST; j++)
    recover_labels[j-REC_FILE0] = (text_key(j,recover_keymap,0))[1];

  /* Do some bounds checking on our window */
  if (VERT>vsize) {
    vstart = ((VERT-vsize)/2+HEADER_SIZE);
  } else {
    vsize  = VERT;
    vstart = HEADER_SIZE;
  }

  clobber_window(workspace); 
  workspace = newwin(vsize,WIN_COL,vstart,HOFF);
  werase(workspace);
  
  display_trailer("Change blocks with adjacent characters.  Q to quit.  R to dump to file");

  while ( (c=next_cmd)||(c=lookup_key(mgetch(),recover_keymap)) ) {
    next_cmd = 0;

    switch (c) {
      case REC_FILE0: /* Enter block 'n' of recovery file */
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
	if (cread_num("Enter block number (leading 0x or $ indicates hex):", &a))
	  fake_inode_zones[c-REC_FILE0] = (unsigned long) a;
        break;
      case CMD_CLR_RECOVER:
	bzero(fake_inode_zones,sizeof(long)*MAX_BLOCK_POINTER);
	break;
      case CMD_BLOCK_MODE:
      case CMD_VIEW_SUPER:
      case CMD_EXIT:
      case CMD_EXIT_PROG:
      case CMD_INODE_MODE:
	return c;
	break;
      case CMD_FLAG_ADJUST:
	flag_popup();
	break;
      case CMD_CALL_MENU:
	next_cmd = do_popup_menu(recover_menu,recover_keymap);	
	break;
      case CMD_HELP:
	do_new_scroll_help(recover_help, recover_keymap, FANCY);
      case CMD_REFRESH:
	refresh_all();
	break;
      case CMD_DO_RECOVER:
	crecover_file(fake_inode_zones);
	break;
      case CMD_CHECK_RECOVER:
	(void) check_recover_file(fake_inode_zones);
	break;
      case CMD_DISPLAY_LOG:
	error_popup();
	break;
      default:
        continue;
    }

    mvwprintw(workspace,0,20,"DIRECT BLOCKS:" );
    for (j=0;j<fsc->N_DIRECT; j++)
      mvwprintw(workspace,j,40," %1c : 0x%8.8lX",recover_labels[j],fake_inode_zones[j]);
    if (fsc->INDIRECT) {
      mvwprintw(workspace,j,20,"INDIRECT BLOCK:" );
      mvwprintw(workspace,j,40," %1c : 0x%8.8lX",recover_labels[j],fake_inode_zones[fsc->INDIRECT]);
      j++;
    }
    if (fsc->X2_INDIRECT) {
      mvwprintw(workspace,j,20,"2x INDIRECT BLOCK:" );
      mvwprintw(workspace,j,40," %1c : 0x%8.8lX",recover_labels[j],fake_inode_zones[fsc->X2_INDIRECT]);
      j++;
    }
    if (fsc->X3_INDIRECT) {
      mvwprintw(workspace,j,20,"3x INDIRECT BLOCK:" );
      mvwprintw(workspace,j,40," %1c : 0x%8.8lX",recover_labels[j],fake_inode_zones[fsc->X3_INDIRECT]);
      j++;
    }
    wrefresh(workspace);
  }	

  return 0; /* Ain't gunna happen */
}


/* Format the super block for curses */
void show_super(void)
{
  int vstart,vsize=10;

  clobber_window(workspace);

  /* Do some bounds checking on our window */
  if (VERT>vsize) {
    vstart = ((VERT-vsize)/2+HEADER_SIZE);
  } else {
    vsize  = LINES-HEADER_SIZE-TRAILER_SIZE;
    vstart = HEADER_SIZE;
  }

  workspace = newwin(vsize,WIN_COL,vstart,HOFF);
  werase(workspace);
  
  mvwprintw(workspace,0,20,"Inodes:       %10ld (0x%8.8lX)",sb->ninodes, sb->ninodes);
  mvwprintw(workspace,1,20,"Blocks:       %10ld (0x%8.8lX)",sb->nzones, sb->nzones);
  mvwprintw(workspace,2,20,"Firstdatazone:%10ld (N=%lu)",sb->first_data_zone,sb->norm_first_data_zone);
  mvwprintw(workspace,3,20,"Zonesize:     %10ld (0x%4.4lX)",sb->blocksize, sb->blocksize);
  mvwprintw(workspace,4,20,"Maximum size: %10ld (0x%8.8lX)",sb->max_size,sb->max_size);
  mvwprintw(workspace,6,20,"* Directory entries are %d characters.",sb->namelen);
  mvwprintw(workspace,7,20,"* Inode map occupies %lu blocks.",sb->imap_blocks);
  mvwprintw(workspace,8,20,"* Zone map occupies %lu blocks.",sb->zmap_blocks);
  mvwprintw(workspace,9,20,"* Inode table occupies %lu blocks.",INODE_BLOCKS);
  wrefresh(workspace);

  return;
}

/* Lookup action associated with keypressed */
int lookup_key(int c, lde_keymap *kmap)
{
  int i;

  for (i=0;kmap[i].key_code;i++) {
    if (c==kmap[i].key_code) {
      return kmap[i].action_code;
    }
  }

  for (i=0;global_keymap[i].key_code;i++) {
    if (c==global_keymap[i].key_code) {
      return global_keymap[i].action_code;
    }
  }

  return CMD_NO_ACTION;
}

/* Check if we should display special keys 
 * (i.e. Function keys and cursors, etc.) */
char *check_special(int c) 
{
  static int i;
  
  for (i=0;special_keys[i].action_code;i++) {
    if (c==special_keys[i].action_code)
      return special_keys[i].description;
  }
  
  return NULL;
}

/* Convert action code to key which would ellicit this action */
char *text_key(int c, lde_keymap *kmap, int skip)
{
  int i, found=0;
  static char stat_return_string[5], *return_string;
  
  for (i=0;kmap[i].action_code;i++) {
    
    if (c==kmap[i].action_code) {

      /* Continue with for loop if we have not skipped enough entries */
      if (skip-(found++))
	continue;

      c = kmap[i].key_code;

      /* Look for special keys */
      if ((return_string=check_special(c))!=NULL)
	return return_string;
      
      return_string = stat_return_string;
      memset(return_string,0,5);

      /* Look for meta and ctrl keys */
      if (IS_META(c)) {
	strcpy(return_string,"M-");
	c = INV_META(c);
      } else if (IS_CTRL(c)) {
	strcpy(return_string,"C-");
	c = INV_CTRL(c);
      } else {
	strcpy(return_string," ");
      }

      if ((c>31)&&(c<127)) 
        return_string[strlen(return_string)] = c;
      return return_string;
    }
  }

  /* If we didn't find it in the requested keymap,
   * search the global keymap */
  if (kmap!=global_keymap)
    return text_key(c, global_keymap, (skip-found));

  return "   ";
}
 

/* Not too exciting main parser with superblock on screen */
void interactive_main(void)
{
  int c, next_cmd = CMD_DISPLAY_LOG;

  current_inode = fsc->ROOT_INODE;
  current_block = 0UL;

  /* Curses initialization junk */
  if (!(newterm(NULL,stdout,stdin))) {
    printf("It seems you have not set up ncurses correctly -- See INSTALL for assistance.\n");
    return;
  }
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  scrollok(stdscr, FALSE);
  if (has_colors()) {
    start_color();
    init_pair(1,COLOR_WHITE,COLOR_RED);
    init_pair(2,COLOR_WHITE,COLOR_BLUE);
    init_pair(3,COLOR_RED,COLOR_BLACK);
    WHITE_ON_RED = COLOR_PAIR(1);
    WHITE_ON_BLUE = COLOR_PAIR(2);
    RED_ON_BLACK = COLOR_PAIR(3);
  } else {
    WHITE_ON_BLUE = A_REVERSE;
    RED_ON_BLACK = A_NORMAL;
    WHITE_ON_RED = A_UNDERLINE;
  }

  /* First check to see that our screen is big enough */
  if ((LINES<(4+HEADER_SIZE+TRAILER_SIZE))||(COLS<WIN_COL)) {
    endwin();
    printf("Screen is too small: need %d x %d, have %d x %d\n",
	   WIN_COL,(4+HEADER_SIZE+TRAILER_SIZE),
	   COLS,LINES);
    return;
  }

  /* Clear out restore buffer */
  bzero(fake_inode_zones,sizeof(long)*MAX_BLOCK_POINTER);
 
  /* Our three curses windows */
#if HEADER_SIZE>0
  header = newwin(HEADER_SIZE,COLS,0,0);
#endif
  workspace = newwin(1,0,0,0); /* Gets clobbered before it gets used */
#if TRAILER_SIZE>0
  trailer = newwin(TRAILER_SIZE,COLS,LINES-TRAILER_SIZE,0);
#endif
  
  restore_header();
  
  while ( (c=next_cmd)||(c=lookup_key(mgetch(),main_keymap)) ) {
    next_cmd = 0;

    switch (c) {
      case CMD_BLOCK_MODE:
        next_cmd = block_mode();
        continue;
      case CMD_FLAG_ADJUST:
	flag_popup();
	break;
      case CMD_INODE_MODE:
	next_cmd = inode_mode();
	continue;
      case CMD_EXIT_PROG:
	endwin();
	return;
	break;
      case CMD_RECOVERY_MODE:
	next_cmd = recover_mode();
	continue;
      case CMD_DISPLAY_LOG:
	error_popup();
	break;
      case CMD_CALL_MENU:
	next_cmd = do_popup_menu(ncmain_menu,main_keymap);	
	break;
      case CMD_HELP:
	do_new_scroll_help(ncmain_help,main_keymap,FANCY);
      case CMD_REFRESH:
	refresh_all();
	break;
      case CMD_VIEW_SUPER:
	break;
      default:
	continue;
      }

    show_super();
    update_header();
    display_trailer("F)lags, I)node, B)locks, R)ecover File");

  }
  
  /* No way out here -- have to use Q key */
}


