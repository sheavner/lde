/*
 *  lde/nc_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_lde.c,v 1.7 1994/04/04 04:22:28 sdh Exp $
 */

#include "nc_lde.h"

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
  wclear(win);
  box(win,0,0);
  if (strlen(warn_string)) {
    if (!quiet) beep();
    mvwprintw(win,3,(WIN_COL-strlen(warn_string))/2-1,"%s",warn_string);
  }
  mvwprintw(win,2,(WIN_COL-strlen(data_string))/2-1,"%s",data_string);

  wrefresh(win);
  while (!strchr(data_options,c))
      c = tolower(getch());

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
int cread_num(char *coutput, long *a)
{
  char cinput[80];

#if TRAILER_SIZE>0
#define window_available trailer
#define LINE_NUMBER TRAILER_SIZE-1
#else
  win *window_avaliable;
  window_avaliable = newwin(5,WIN_COL,((VERT-5)/2+HEADER_SIZE),HOFF);
  wclear(window_avaliable);
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

/* Displays up to two lines in the trailer window */
void display_trailer(char *line1, char *line2)
{
#if TRAILER_SIZE>0
  werase(trailer);
  mvwaddstr(trailer,0,(COLS-strlen(line1))/2-1,line1);
  mvwaddstr(trailer,1,(COLS-strlen(line2))/2-1,line2);
  wrefresh(trailer);
#endif
}

/* This fills in the header window -- 
 * things like the program name, current inode, tagged recovered blocks
 */
void update_header()
{
#if HEADER_SIZE>0
  int j;

  mvwprintw(header,HEADER_SIZE-1,HOFF,"Inode: %lu (0x%5.5lX)",current_inode,current_inode);
  mvwprintw(header,HEADER_SIZE-1,HOFF+25,"Block: %lu (0x%5.5lX)",current_block,current_block);
  for (j=0;j<fsc->N_BLOCKS;j++)
    if (fake_inode_zones[j]) 
      mvwaddch(header,HEADER_SIZE-1,HOFF+60+j,'-');
    else
      mvwaddch(header,HEADER_SIZE-1,HOFF+60+j,j+'0');
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
void restore_header()
{
#if HEADER_SIZE>0
  int i,j;
  char echo_string[132];

  wattron(header,BLUE_ON_WHITE);
  werase(header);

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
void refresh_ht()
{
#if TRAILER_SIZE>0
  redraw_win(trailer);
#endif
  restore_header();
}

/* Redraw everything -- ^L */
void refresh_all()
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
 
  if (!quiet) beep();
#if TRAILER_SIZE>0
  mvwaddstr(trailer,TRAILER_SIZE-1,(COLS-strlen(echo_string))/2-1,
	    echo_string);
  wrefresh(trailer);
  wmove(trailer,TRAILER_SIZE-1,0);
  wclrtoeol(trailer);
  /* Still has to be refreshed by someone */
#endif
}

/* Dump the error log to a window */
int error_popup()
{
  WINDOW *win;
  int c, redraw, flag, present_error, i;

  win = newwin(VERT,COLS,HEADER_SIZE,0);

  flag = 1; c = 0;
  while ((flag)||(c = getch())) {
    flag = 0;
    redraw = 0;
    switch (c) {
      case 'q':
      case 'Q':
        delwin(win);
	refresh_all();
        return ' ';
	break;
      case CTRL('L'):
        refresh_all();
        break;
      case  0:
	redraw = 1;
	break;
      default:
	return c;
	break;
    }

    wclear(win);
    for (i=-1;((++i<VERT)&&(i<ERRORS_SAVED)); ) {
      present_error = current_error  - i;
      if (present_error<0) present_error += ERRORS_SAVED;
      mvwprintw(win,i,0,error_save[present_error]);
    }
    wrefresh(win);
  }

  return 0;
}

/* Display some help in a separate window */
void do_help()
{
  WINDOW *win;

  win = newwin(13,WIN_COL,((VERT-13)/2+HEADER_SIZE),HOFF);
  wclear(win);
  box(win,0,0);
  mvwprintw(win,1,16,"Program name : Filesystem type : Device name");
  mvwprintw(win,2,7,"Inode: dec. (hex)     Block: dec. (hex)    0123456789:;<=>");
  mvwprintw(win,4,20,"h,H,?   : Calls up this help.");
  mvwprintw(win,5,20,"b,B     : Enter block mode.");
  mvwprintw(win,6,20,"i,I     : Enter inode mode.");
  mvwprintw(win,7,20,"r,R     : Enter recovery mode.");
  mvwprintw(win,8,20,"q,Q     : Quit.");
  mvwprintw(win,9,20,"^L      : Refresh screen.");
  mvwprintw(win,11,20," Press any key to continue. ");
  wrefresh(win);
  getch();
  delwin(win);
  refresh_all();

  return;
}

/* Format the super block for curses */
void show_super()
{
  clobber_window(workspace);
  workspace = newwin(10,WIN_COL,((VERT-10)/2+HEADER_SIZE),HOFF);
  
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

/* Throw up a list of flags which the user can toggle */
void flag_popup()
{
  WINDOW *win;
  int c, redraw, flag;

  win = newwin(7,WIN_COL,((VERT-7)/2+HEADER_SIZE),HOFF);

  flag = 1; c = ' ';
  while ((flag)||(c = getch())) {
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
        if (!paranoid)
	  write_ok = 1 - write_ok;
	else
	  warn("Device opened read only, do not specify '--paranoid' on the command line");
	redraw = 1;
        break;
      case 'A':
      case 'a':
        rec_flags.search_all = 1 - rec_flags.search_all;
	redraw = 1;
        break;
      case 'N':
      case 'n':
        quiet = 1 - quiet;
	redraw = 1;
        break;
      case CTRL('L'):
        refresh_all();
        break;
      case ' ':
	redraw = 1;
	break;
    }

    wclear(win);
    wattron(win,RED_ON_BLACK);
    box(win,0,0);
    wattroff(win,RED_ON_BLACK);
    mvwprintw(win,1,15,"A: (%-3s) Search all blocks",rec_flags.search_all ? "YES" : "NO");
    mvwprintw(win,2,15,"N: (%-3s) Noise is off -- i.e. quiet",quiet ? "YES" : "NO");
    mvwprintw(win,3,15,"W: (%-3s) OK to write to file system",write_ok ? "YES" : "NO");
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
    switch (cquery("File exists, append data [Y/N/Q]: ","ynq","")) {
      case 'y':
        fp = open(recover_file_name,O_WRONLY|O_APPEND);
	break;
      case 'n':
	fp = open(recover_file_name,O_WRONLY|O_TRUNC);
	break;
      default:
	fp = 0;
	break;
      }
  } else if ( (fp = open(recover_file_name,O_WRONLY|O_CREAT)) < 0 )
    warn("Cannot open file '%s'\n",recover_file_name);

  if (fp > 0) {
    recover_file(fp, inode_zones);
    close(fp);
    warn("Recovered data written to '%s'", recover_file_name);
  }
}
  
/* This lists all the tagged inodes */
int recover_mode()
{
  int j,c,flag;
  unsigned long a;

  clobber_window(workspace); 
  workspace = newwin(fsc->N_BLOCKS+1,WIN_COL,((VERT-fsc->N_BLOCKS-1)/2+HEADER_SIZE),HOFF);
  
  display_trailer("Enter number corresponding to block to modify values",
		  "Q to quit, R to dump to file");

  flag=1; c = ' ';
  while (flag||(c = getch())) {
    switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case ':':
      case ';':
      case '<':
      case '=':
      case '>':
	if (cread_num("Enter block number (leading 0x or $ indicates hex):", &a))
	    fake_inode_zones[c-'0'] = (unsigned long) a;
        break;
      case 'B':
      case 'b':
      case 'q':
      case 'Q':
      case 'S':
      case 's':
      case 'I':
      case 'i':
	return c;
	break;
      case 'f':
      case 'F':
	flag_popup();
	break;
      case 'h':
      case 'H':
      case '?':
      case CTRL('H'):
        do_help();
      case CTRL('L'):
	refresh_all();
	break;
      case 'R':
      case 'r':
	crecover_file(fake_inode_zones);
	break;
      case 'V':
      case 'v':
	c = flag = error_popup();
	break;
    }

    mvwprintw(workspace,0,0,"DIRECT BLOCKS:" );
    for (j=0;j<fsc->N_DIRECT; j++)
      mvwprintw(workspace,j,20," %2d : 0x%8.8lX",j,fake_inode_zones[j]);
    if (fsc->INDIRECT) {
      mvwprintw(workspace,j,0,"INDIRECT BLOCK:" );
      mvwprintw(workspace,j,20," %2d : 0x%8.8lX",j,fake_inode_zones[fsc->INDIRECT]);
      j++;
    }
    if (fsc->X2_INDIRECT) {
      mvwprintw(workspace,j,0,"2x INDIRECT BLOCK:" );
      mvwprintw(workspace,j,20," %2d : 0x%8.8lX",j,fake_inode_zones[fsc->X2_INDIRECT]);
      j++;
    }
    if (fsc->X3_INDIRECT) {
      mvwprintw(workspace,j,0,"3x INDIRECT BLOCK:" );
      mvwprintw(workspace,j,20," %2d : 0x%8.8lX",j,fake_inode_zones[fsc->X3_INDIRECT]);
      j++;
    }
    wrefresh(workspace);
    flag = 0;
  }	

  return 0; /* Ain't gunna happen */
}

/* Not too exciting main parser with superblock on screen */
void interactive_main()
{
  int c, redraw, flag;

  current_inode = fsc->ROOT_INODE;
  current_block = 0UL;

  /* Curses initialization junk */
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
    RED_ON_WHITE = COLOR_PAIR(1);
    BLUE_ON_WHITE = COLOR_PAIR(2);
    RED_ON_BLACK = COLOR_PAIR(3);
  } else {
    BLUE_ON_WHITE = A_REVERSE;
    RED_ON_BLACK = A_NORMAL;
    RED_ON_WHITE = A_UNDERLINE;
  }
  
  /* Our three curses windows */
#if HEADER_SIZE>0
  header = newwin(HEADER_SIZE,COLS,0,0);
#endif
  workspace = newwin(1,0,0,0); /* Gets clobbered before it gets used */
#if TRAILER_SIZE>0
  trailer = newwin(TRAILER_SIZE,COLS,LINES-TRAILER_SIZE,0);
#endif
  
  restore_header();
  
  /* flag is used so that the user can switch between modes without
   * getting stuck here.  Flag indicates no keypress is required, so
   * getch() is not called.
   */
  flag = 1; c = ' ';
  while (flag || (c = getch())) {
    flag = 0;
    redraw = 0;
    switch (c|flag) {
      case 'B':
      case 'b':
        c = flag = block_mode();
        break;
      case 'F':
      case 'f':
	flag_popup();
	break;
      case 'h':
      case 'H':
      case '?':
	do_help();
	break;
      case 'I':
      case 'i':
	c = flag = inode_mode();
	break;
      case 'q':
      case 'Q':
	endwin();
	return;
	break;
      case 'R':
      case 'r':
	c = flag = recover_mode();
	break;
      case 'V':
      case 'v':
	c = flag = error_popup();
	break;
      case CTRL('L'):
	refresh_all();
	redraw = 1;
	break;
      default:
	redraw=0;
	break;
      case ' ':
      case 's':
      case 'S':
	redraw = 1;
	break;
      }

    if (redraw) {
      show_super();
      update_header();
      display_trailer("", "F)lags, I)node, B)locks, R)ecover File");
    }

  }
  
  /* No way out here -- have to use Q key */
}


