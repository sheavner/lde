/*
 *  lde/nc_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_lde.c,v 1.5 1994/03/23 05:57:59 sdh Exp $
 */

#include "lde.h"

#ifdef NC_HEADER
#include <ncurses.h>
#else
#include <curses.h>
#endif

#define CTRL(x) (x-'A'+1)

/* From mode_string.c */
void mode_string();

/* Curses variables */
int DATA_START, DATA_END, HOFF;
int VERT, HOFFPLUS;
int RED_ON_WHITE, BLUE_ON_WHITE, END_HIGHLIGHT, RED_ON_BLACK;
WINDOW *header, *workspace, *trailer;

unsigned long current_inode = 1;
unsigned long current_block = 0;
int highlight_izone = 0;
int max_blocks_this_inode = 0;
char echo_string[150];
unsigned long fake_inode_zones[MAX_BLOCK_POINTER];

#define WIN_COL 80
#define HEADER_SIZE 2
#define TRAILER_SIZE 3

#define VERT (LINES-HEADER_SIZE-TRAILER_SIZE)

/* This moves to the bottom of the screen, writes whatever is in coutput,
 * and reads a long int from the user.  It recognizes indicators as to the 
 * type of input for decimal, hex, or octal.  For example if we wanted to
 * input 15, we could enter as hex (0xf, xf, $f), decimal (15), or octal
 * (\017).  sscanf is probably smart enough to recognize these, but 
 * what the hell.  If coutput begins with a space, cinput is returned in
 * coutput and the number processing does not occurr.
 */
long cread_num(char *coutput)
{
  char cinput[80];

#if TRAILER_SIZE>0
#define WINDOW_AVAILABLE trailer
#define LINE_NUMBER TRAILER_SIZE-1
#define HORIZ_WINDOW (COLS-strlen(coutput))/2-5
#else
#define WINDOW_AVAILABLE workspace
#define LINE_NUMBER 0
#define HORIZ_WINDOW (COLS-2*HOFF-strlen(coutput))/2-5
#endif

  wmove(WINDOW_AVAILABLE,LINE_NUMBER,0);
  wclrtoeol(WINDOW_AVAILABLE);
  mvwprintw(WINDOW_AVAILABLE,LINE_NUMBER,
    HORIZ_WINDOW,"%s",coutput);
  wrefresh(WINDOW_AVAILABLE);
  echo();
  wgetstr(WINDOW_AVAILABLE,cinput);
  noecho();
  wmove(WINDOW_AVAILABLE,LINE_NUMBER,0);
  wclrtoeol(WINDOW_AVAILABLE);
  wrefresh(WINDOW_AVAILABLE);

  if (coutput[0]!=' ') {
    return read_num(cinput);
  } else {
    strncpy(coutput, cinput, 80);
  }

 return 1; /* This should be a safe number to return if something went wrong */
}

/* This fills in the header window -- 
 * things like the program name, current inode, tagged recovered blocks
 */
void update_header()
{
#if HEADER_SIZE>0
  int(j);
  sprintf(echo_string,"Inode: %ld (0x%5.5lX)",current_inode,current_inode);
  mvwprintw(header,HEADER_SIZE-1,HOFF,"%24s",echo_string);
  sprintf(echo_string,"Block: %ld (0x%5.5lX)",current_block,current_block);
  mvwprintw(header,HEADER_SIZE-1,HOFF+25,"%24s",echo_string);
  for (j=0;j<fsc->N_BLOCKS;j++)
    if (fake_inode_zones[j]) 
      mvwaddch(header,HEADER_SIZE-1,HOFF+60+j,'-');
    else
      mvwaddch(header,HEADER_SIZE-1,HOFF+60+j,j+'0');
  touchwin(header);
  wrefresh(header);
#endif
}

/* This should clear out the old workspace */
void clobber_workspace()
{
  werase(workspace);
  wrefresh(workspace);
  delwin(workspace);
}

void restore_header()
{
  int i,j;

#if HEADER_SIZE>0
  wattron(header,BLUE_ON_WHITE);
  werase(header);
  for (i=0;i<HEADER_SIZE;i++) /* A cute way to RVS video the header window */
    for (j=0;j<COLS;j++)
      mvwaddch(header,i,j,' ');
  sprintf(echo_string,"%s v%s : %s : %s",program_name,VERSION,text_names[fsc->FS],device_name);
  mvwaddstr(header,0, (COLS-strlen(echo_string))/2-1,echo_string);
  wrefresh(header);
#endif
}

/* Redraw everything -- ^L */
void refresh_all()
{
  touchwin(stdscr);
  refresh();
#if TRAILER_SIZE>0
  touchwin(trailer);
  wrefresh(trailer);
#endif
  touchwin(workspace);
  wrefresh(workspace);
#if HEADER_SIZE>0
  restore_header();
  update_header();
#endif
}

void nc_warn(char * echo_string)
{
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

/*
 * Display some help in a separate window
 */
void do_help(void)
{
  WINDOW *win;

  win = newwin(13,80,((VERT-13)/2+HEADER_SIZE),HOFF);
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

/*
 * Display some help in a separate window
 */
void do_block_help(void)
{
  WINDOW *win;

  win = newwin(17,80,((VERT-17)/2+HEADER_SIZE),HOFF);
  wclear(win);
  box(win,0,0);
  mvwprintw(win,1,16,"Program name : Filesystem type : Device name");
  mvwprintw(win,2,7,"Inode: dec. (hex)     Block: dec. (hex)    0123456789:;<=>");
  mvwprintw(win,4,7,"h,H,?   : Calls up this help.");
  mvwprintw(win,5,7,"B       : View block under cursor (%s block ptr is %d bytes).",text_names[fsc->FS],fsc->ZONE_ENTRY_SIZE);
  mvwprintw(win,6,7,"i       : Enter inode mode.");
  mvwprintw(win,7,7,"I       : View inode under cursor (%s inode ptr is %d bytes).",text_names[fsc->FS],fsc->INODE_ENTRY_SIZE);
  mvwprintw(win,8,7,"r,R     : Enter recovery mode.");
  mvwprintw(win,9,7,"q,Q     : Quit.");
  mvwprintw(win,10,7,"arrows  : Move cursor (also ^P, ^N, ^F, ^B)");
  mvwprintw(win,11,7,"+/-     : View next/previous part of current block.");
  mvwprintw(win,12,7,"PGUP/DN : View next/previous block.");
  mvwprintw(win,13,7,"0123... : Add current block to recovery list at position.");
  mvwprintw(win,14,7,"#       : Enter block number and view it.");
  mvwprintw(win,15,7,"l,L     : Find an inode which references this block.");
  mvwprintw(win,16,25," Press any key to continue. ");
  wrefresh(win);
  getch();
  delwin(win);
  refresh_all();

  return;
}

void do_inode_help(void)
{
  WINDOW *win;

  win = newwin(16,80,((VERT-16)/2+HEADER_SIZE),HOFF);
  wclear(win);
  box(win,0,0);
  mvwprintw(win,1,16,"Program name : Filesystem type : Device name");
  mvwprintw(win,2,7,"Inode: dec. (hex)     Block: dec. (hex)    0123456789:;<=>");
  mvwprintw(win,4,7,"h,H,?   : Calls up this help.");
  mvwprintw(win,5,7,"B       : View block under cursor.");
  mvwprintw(win,6,7,"b       : Enter block mode.");
  mvwprintw(win,7,7,"r       : Enter recovery mode.");
  mvwprintw(win,8,7,"R       : Enter recovery mode, copy inode block ptrs to recovery list.");
  mvwprintw(win,9,7,"q,Q     : Quit.");
  mvwprintw(win,10,7,"LT/RT   : Move cursor (also ^F, ^B)");
  mvwprintw(win,11,7,"UP/DN   : View next/previous inode (also PG_UP/DN, ^N, ^P, ^V).");
  mvwprintw(win,12,7,"0123... : Add block under cursor to recovery list at position.");
  mvwprintw(win,13,7,"#       : Enter inode number and view it.");
  mvwprintw(win,15,25," Press any key to continue. ");
  wrefresh(win);
  getch();
  delwin(win);
  refresh_all();

  return;
}

/* Format the super block for curses */
void show_super()
{
  clobber_workspace();
  workspace = newwin(10,(COLS/2),((VERT-10)/2+HEADER_SIZE),HOFFPLUS);
  
  mvwprintw(workspace,0,0,"Inodes:       %10ld (0x%8.8lX)",sb->ninodes, sb->ninodes);
  mvwprintw(workspace,1,0,"Blocks:       %10ld (0x%8.8lX)",sb->nzones, sb->nzones);
  mvwprintw(workspace,2,0,"Firstdatazone:%10ld (N=%ld)",sb->first_data_zone,sb->norm_first_data_zone);
  mvwprintw(workspace,3,0,"Zonesize:     %10ld (0x%4.4lX)",sb->blocksize, sb->blocksize);
  mvwprintw(workspace,4,0,"Maximum size: %10ld (0x%8.8lX)",sb->max_size,sb->max_size);
  mvwprintw(workspace,6,0,"* Directory entries are %d characters.",sb->namelen);
  mvwprintw(workspace,7,0,"* Inode map occupies %ld blocks.",sb->imap_blocks);
  mvwprintw(workspace,8,0,"* Zone map occupies %ld blocks.",sb->zmap_blocks);
  mvwprintw(workspace,9,0,"* Inode table occupies %ld blocks.",INODE_BLOCKS);
  wrefresh(workspace);
}

void flag_popup()
{
  WINDOW *win;
  int c, redraw, flag;

  win = newwin(7,80,((VERT-7)/2+HEADER_SIZE),HOFF);

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
        write_ok = 1 - write_ok;
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
  return;
}

int directory_popup(unsigned long bnr)
{
#ifdef ALPHA_CODE
  int c, redraw;
  static int flag;
  char f_mode[12];
  char *block_buffer;

  WINDOW *win;
  int i;
  char *fname = NULL;
  unsigned long inode_nr;

  block_buffer = cache_read_block(bnr,0);
  win = newwin(VERT,COLS,HEADER_SIZE,0);

  flag=1; c = ' ';
  while (flag||(c = getch())) {
    flag = 0;
    redraw = 0;
    switch (c) {
      case 'Q':
      case 'q':
        delwin(win);
        refresh_all();
        return c;
	break;
      case CTRL('L'):
        refresh_all();
	redraw = 1;
        break;
      case ' ':
	redraw = 1;
	break;
    }

    if (redraw) {
      wclear(win);
      for (i=0;i<VERT;i++) {
	fname = FS_cmd.dir_entry(i, block_buffer, &inode_nr);
	if (fname[0]) {
	  if (inode_nr > sb->ninodes) inode_nr = 0;
	  mvwprintw(win,i,1,"0x%8.8lX:", inode_nr);
	  mode_string( (unsigned short)DInode.i_mode(inode_nr), f_mode);
	  mvwprintw(win,i,1,"0x%8.8lX: %9s %3d %9ld %s", inode_nr, 
		    f_mode, DInode.i_links_count(inode_nr),
		    DInode.i_size(inode_nr), fname);
	  /* Doing inode ops pulls in a different block via read(), so re-read 
	     current block to keep block_mode happy when we return.  If we did not
	     read in a new block (I don't see how that's possible though) the cache
	     read won't really waste any time */
	  block_buffer = cache_read_block(bnr,0);
	}
      }
      wrefresh(win);
    }
  }
#endif ALPHA_CODE
  return 0;
}

/* Dump as much of a block as we can to the screen and format it in a nice 
 * hex mode with ASCII printables off to the right.
 */
unsigned char *cdump_block(nr,win_start,win_size)
unsigned long nr;
int win_start, win_size;
{
  int i,j;
  unsigned char *dind,  c;
  char block_not_used[10]=":NOT:USED:", block_is_used[10] = "::::::::::";
  char *block_status;
 
  clobber_workspace(); 
  workspace = newwin(VERT,(COLS-HOFF),HEADER_SIZE,HOFF);
  
  dind = cache_read_block(nr,0);
  
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
  return dind;
}

void cwrite_block(unsigned long block_nr, char *data_buffer, int *mod_yes)
{
  WINDOW *win;
  int c, flag;

  flag = 1;
  if (*mod_yes) {
    win = newwin(5,80,((VERT-5)/2+HEADER_SIZE),HOFF);
    wclear(win);
    box(win,0,0);
    mvwprintw(win,2,20,"WRITE OUT BLOCK DATA TO DISK [Y/N]? ");
    if (!write_ok) {
      if (!quiet) beep();
      mvwprintw(win,3,3,"(NOTE: write permission not set on disk, use 'F' to set flags before 'Y') ");
    }
    wrefresh(win);
    while (flag) {
      c = tolower(getch());
      if (c=='f') {
	flag_popup();
	touchwin(win);
	wrefresh(win);
      } else if ((c == 'y')||(c =='n')) {
	flag = 0;
	delwin(win);
	refresh_all(); /* Have to refresh screen before write or errors will be lost */
	if (c == 'y') 
	  if (write_ok) write_block(block_nr, data_buffer);
      }
    }
  }
  *mod_yes = 0;
}

void highlight_block(int cur_row,int cur_col,int win_start,unsigned char *block_buffer, int ascii_flag)
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

/* This is the curses menu-type system for block mode, displaying a block
 * on the screen, next/previous block, paging this block, etc., etc.
 */
int block_mode() {
  int c,flag,redraw;
  static int win_start = 0;
  static int cur_row = 0, cur_col = 0;
  static int prev_row = 0, prev_col = 0;
  unsigned char *block_buffer = NULL, *copy_buffer = NULL;
  unsigned long temp_ptr;

  int edit_block, ascii_mode, highlight, val, v1, icount, modified = 0;
  char *HEX_PTR, *HEX_NOS = "0123456789ABCDEF";

#if TRAILER_SIZE>0
  werase(trailer);
  strncpy(echo_string,"PG_UP/DOWN = previous/next block, or '#' to enter block number",150);
  mvwaddstr(trailer,0,(COLS-strlen(echo_string))/2-1,echo_string);
  strncpy(echo_string,"H for help.  Q to quit",150);
  mvwaddstr(trailer,1,(COLS-strlen(echo_string))/2-1,echo_string);
  wrefresh(trailer);
#endif

  flag = 1; c = ' ';
  edit_block = 0; icount=0; ascii_mode=0; v1=0; modified = 0;
  cur_row = cur_col = 0;
  while (flag||(c = getch())) {
    flag = 0;
    redraw = 1;
    highlight = 1;
    if (edit_block) {
      if (c == CTRL('I')) {
	ascii_mode = 1 - ascii_mode;
      } else if (!ascii_mode) {
	HEX_PTR = strchr(HEX_NOS, toupper(c));
	if (HEX_PTR != NULL) {
	  val = HEX_PTR - HEX_NOS;
	  if (icount == 0) {
	    icount++;
	    v1 = val;
	    wattron(workspace,RED_ON_WHITE);
	    waddch(workspace,HEX_NOS[val]);
	    wattroff(workspace,RED_ON_WHITE);
	    highlight = 0;
	  } else {
	    icount = 0;
	    v1 = v1*16 + val;
	    block_buffer[cur_row*16+cur_col+win_start] = v1;
	    modified = 1;
	    c = KEY_RIGHT; /* Advance the cursor */
	  }
	}
      } else { /* ASCII MODE */
	if ((c>31)&&(c<127)) {
	  block_buffer[cur_row*16+cur_col+win_start] = c;
	  modified = 1;
	  c = KEY_RIGHT; /* Advance the cursor */
	}
      }
    } else {
      /* These keys will be deactivated in edit mode */
      switch(c) {
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
	  fake_inode_zones[c-'0'] = current_block;
	  update_header();
	  beep();
	  break;
	case 'c':
	case 'C':
	  if (!copy_buffer) copy_buffer = malloc(sb->blocksize);
	  memcpy(copy_buffer,block_buffer,sb->blocksize);
	  warn("Block copied into copy buffer.");
	  break;
	case 'p':
	case 'P':
	  modified = 1;
	  /* Right now it makes more sense to write to disk, rather
	     rather than copying it to the screen.  redraw block pulls in 
	     a value from inode cache and is reset. */
	  /* block_buffer = copy_block; */
	  /* cwrite_block(current_block, copy_buffer, &modified); */
	  memcpy(block_buffer, copy_buffer, sb->blocksize);
	  if (!write_ok) warn("Turn on write permissions before saving this block");
	  c = ' ';
	  break;
        case 'E':
	case 'e':
	  edit_block = 1;
	  icount = 0;
	  if (!write_ok) warn("Disk not writeable, change status flags with (F)");
	  break;
	case 'D':
	case 'd':
	  directory_popup(current_block);
	  break;
        case 'B':
	  temp_ptr = block_pointer(&block_buffer[cur_row*16+cur_col+win_start],(unsigned long) 0, fsc->ZONE_ENTRY_SIZE);
	  if (temp_ptr <= sb->nzones) {
	    current_block = temp_ptr;
	    return c;
	  }
	  break;
	case 'F':
	case 'f':
	  flag_popup();
	  break;
      }
    }
    
    /* These keys are valid in both modes */
    switch(c) {
      case CTRL('F'):
      case KEY_RIGHT:
	redraw = 0;
	icount = 0;
	if (++cur_col > 15) {
	  cur_col = 0;
	  if ((++cur_row)*16+win_start>=sb->blocksize) {
	    cur_col = 15;
	    cur_row--;
	  } else if (cur_row >= VERT) {
	    if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	    prev_row = prev_col = cur_row = 0;
	    redraw = 1;
	  }
	}
	break;
      case '+':
      case KEY_SRIGHT:
        if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	icount = 0;
	break;
      case CTRL('B'):
      case KEY_BACKSPACE:
      case KEY_DC:
      case KEY_LEFT:
	icount = 0;
	redraw = 0;
	if (--cur_col < 0) {
	  cur_col = 15;
	  if (--cur_row < 0) {
	    if (win_start - VERT*16 >= 0) {
	      win_start -= VERT*16;
	      cur_row = VERT - 1;
	      redraw = 1;
	    } else
	      cur_row = cur_col = 0;
	  }
	}
	break;
      case '-':
      case KEY_SLEFT:
	if (win_start - VERT*16 >= 0)  win_start -= VERT*16;
	icount = 0;
	break;
      case CTRL('N'):
      case KEY_DOWN:
	if (++cur_row >= VERT) {
	  if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	  prev_row = prev_col = cur_row = 0;
	} else
	  redraw = 0;
	icount = 0;
	break;
      case CTRL('V'):
      case CTRL('D'):
      case KEY_NPAGE:
        cwrite_block(current_block, block_buffer, &modified);
	current_block++;
	edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	break;
      case CTRL('P'):
      case KEY_UP:
	icount = 0;
	if (--cur_row<0) {
	  if (win_start - VERT*16 >= 0) {
	    win_start -= VERT*16;
	    cur_row = VERT - 1;
	  } else
	    cur_row = 0;
	  prev_col = prev_row = 0;
	} else
	  redraw = 0;
	break;
      case CTRL('U'):
      case KEY_PPAGE:
        cwrite_block(current_block, block_buffer, &modified);
	if (current_block!=0) current_block--;
	edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	break;
      /* These next two were pulled out of the if edit loop for block pasting */
      case( CTRL('W')):
	edit_block = 0;
	cwrite_block(current_block, block_buffer, &modified);
	break;
      case CTRL('A'):
	modified = ascii_mode = edit_block = 0;
	block_buffer = cache_read_block(current_block,1);
	c = ' ';
	break;
      case 'l':
      case 'L':
	if ( (temp_ptr = find_inode(current_block)) )
	  sprintf(echo_string, "Block is indexed under inode 0x%lX.\n",temp_ptr);
	else
	  if (rec_flags.search_all)
	    sprintf(echo_string, "Unable to find inode referenece.\n");
	  else
	    sprintf(echo_string, "Unable to find inode referenece try activating the --all option.\n");
	warn(echo_string);
	break;
      case 'I':
	cwrite_block(current_block, block_buffer, &modified);
	temp_ptr = block_pointer(&block_buffer[cur_row*16+cur_col+win_start],(unsigned long) 0, fsc->INODE_ENTRY_SIZE);
	if (temp_ptr <= sb->ninodes) {
	  current_inode = temp_ptr;
	  return c;
	} else
	  warn("Inode out of range.");
	break;
      case 'q':
      case 'Q':
      case 'S':
      case 's':
      case 'i':
      case 'R':
      case 'r':
        cwrite_block(current_block, block_buffer, &modified);
	return c;
	break;
      case '#':
        cwrite_block(current_block, block_buffer, &modified);
	current_block = cread_num("Enter block number (leading 0x or $ indicates hex):");
	win_start = prev_col = prev_row = cur_col = cur_row = 0;
	edit_block = 0;
	break;
      case 'h':
      case 'H':
      case '?':
      case CTRL('H'):
        do_block_help();
	redraw = 0;
	break;
      case CTRL('L'):
	icount = 0;  /* We want to see actual values? */
	refresh_all();
	break;
      case ' ':
	break;
      default:
	redraw = 0;
	break;
    }

    if (current_block > sb->nzones)
      current_block=sb->nzones;

    /* More room on screen, but have we gone past the end of the block? */
    if (cur_row*16+win_start>=sb->blocksize) {
      cur_row = (sb->blocksize-win_start)/16 - 1;
    }

    if (highlight) {
      if (redraw)
	block_buffer = cdump_block(current_block,win_start,VERT);

      /* Highlight current cursor position */
      /* First turn off last position */
      highlight_block(prev_row,prev_col,win_start,block_buffer,ascii_mode);
      /* Now highlight current */
      wattron(workspace,RED_ON_WHITE);
      prev_col = cur_col;
      prev_row = cur_row;
      highlight_block(cur_row,cur_col,win_start,block_buffer,ascii_mode);
      wattroff(workspace,RED_ON_WHITE);
    }

    wrefresh(workspace);
  }
  return 0;
}


/* Display current inode */
void cdump_inode(unsigned long nr)
{
  unsigned long imode, atime, j;
  char f_mode[12];
  struct passwd *NC_PASS;
  struct group *NC_GROUP;

  clobber_workspace(); 
  workspace = newwin(9,80,((VERT-9)/2+HEADER_SIZE),HOFF);
  
  imode = DInode.i_mode(nr);
  mode_string((unsigned short)imode,f_mode);
  f_mode[10] = 0; /* Junk from canned mode_string */
  mvwprintw(workspace,0,0,"%10s",f_mode);
  mvwprintw(workspace,0,11,"%3d",DInode.i_links_count(nr));
  if ((NC_PASS = getpwuid(DInode.i_uid(nr)))!=NULL)
    mvwprintw(workspace,0,15,"%-8s",NC_PASS->pw_name);
  else
    mvwprintw(workspace,0,15,"%-8d",DInode.i_uid(nr));
  if ((NC_GROUP = getgrgid(DInode.i_gid(nr)))!=NULL)
    mvwprintw(workspace,0,24,"%-8s",NC_GROUP->gr_name);
  else
    mvwprintw(workspace,0,24,"%-8d",DInode.i_gid(nr));
  mvwprintw(workspace,0,32,"%9d",DInode.i_size(nr));
  atime = DInode.i_atime(nr);
  mvwprintw(workspace,0,42,"ACCESS:%24s",ctime(&atime));
  atime = DInode.i_ctime(nr);
  mvwprintw(workspace,1,42,"CREATE:%24s",ctime(&atime));
  atime = DInode.i_mtime(nr);
  mvwprintw(workspace,2,42,"MOD:   %24s",ctime(&atime));

  sprintf(f_mode,"%07lo",imode);
	
  mvwprintw(workspace,3,0,"TYPE: ");
  mvwprintw(workspace,3,6,entry_type(imode));
  
  if (!FS_cmd.inode_in_use(nr)) mvwprintw(workspace,3,25,"(NOT USED)");

  mvwprintw(workspace,4,0,"MODE: \\%4.4s FLAGS: \\%3.3s\n",&f_mode[3],f_mode);
  mvwprintw(workspace,5,0,"UID: %05d(%s)",DInode.i_uid(nr), (NC_PASS != NULL) ? NC_PASS->pw_name : "");
  mvwprintw(workspace,5,20,"GID: %05d(%s)",DInode.i_gid(nr), (NC_GROUP != NULL) ? NC_GROUP->gr_name : "");
  mvwprintw(workspace,6,0,"TIME: %24s",ctime(&atime));
  mvwprintw(workspace,6,0,"LINKS: %3d SIZE: %-8ld \n",DInode.i_links_count(nr),DInode.i_size(nr));
 
  if (DInode.i_zone(nr,(unsigned long) 0)) {
    j=-1;
    mvwprintw(workspace,7,0,"BLOCKS=");
    while ((++j<fsc->N_BLOCKS)&&(DInode.i_zone(nr,j))) {
      max_blocks_this_inode = j;
      if (j==highlight_izone) wattron(workspace,RED_ON_WHITE);
      mvwprintw(workspace,7+j/7,(j%7+1)*10,"0x%7.7lX",DInode.i_zone(nr,j));
      wattroff(workspace,RED_ON_WHITE);
    }
  }
 
  update_header();
  wmove(workspace,7+highlight_izone/7,(highlight_izone%7+1)*10);
  wrefresh(workspace);
}

/* This is the parser for inode_mode: previous/next inode, etc. */
int inode_mode() {
  int c, redraw;
  unsigned long flag;
  
  highlight_izone = 0;

#if TRAILER_SIZE>0
  werase(trailer);
  strncpy(echo_string,"PG_UP/DOWN = previous/next inode, or '#' to enter inode number",150);
  mvwaddstr(trailer,0,(COLS-strlen(echo_string))/2-1,echo_string);
  strncpy(echo_string,"H for help. Q to quit",150);
  mvwaddstr(trailer,1,(COLS-strlen(echo_string))/2-1,echo_string);
  wrefresh(trailer);
#endif
  
  flag = 1; c = ' ';
  while (flag||(c = getch())) {
    flag = 0;
    redraw = 1;
    switch (c) {
      case CTRL('N'):
      case CTRL('D'):
      case KEY_NPAGE:
      case KEY_DOWN:
        current_inode++;
	highlight_izone = 0;
	break;
      case CTRL('P'):
      case CTRL('U'):
      case KEY_PPAGE:
      case KEY_UP:
	current_inode--;
	highlight_izone = 0;
	break;
      case CTRL('F'):
      case KEY_RIGHT:
	if (++highlight_izone > max_blocks_this_inode) 
	  highlight_izone = max_blocks_this_inode;
	break;
      case CTRL('B'):
      case KEY_BACKSPACE:
      case KEY_DC:
      case KEY_LEFT:
	if (--highlight_izone < 0 ) highlight_izone=0;
	break;
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
	if (DInode.i_zone(current_inode, (unsigned long) (c-'0') )) {
	  current_block = DInode.i_zone(current_inode, (unsigned long) (c-'0') );
	  return 'b';
	}
	break;
      case 'R':
	for (flag=0;(flag<fsc->N_BLOCKS);flag++)
	  fake_inode_zones[flag] = DInode.i_zone(current_inode,flag);
	return c;
	break;
      case 'B':
	if (DInode.i_zone(current_inode,highlight_izone))
	  current_block = DInode.i_zone(current_inode,highlight_izone);
      case 'b':
      case 'q':
      case 'Q':
      case 'S':
      case 's':
      case 'r':
	return c;
	break;
      case 'F':
      case 'f':
	flag_popup();
	break;
      case '#':
	current_inode = cread_num("Enter inode number (leading 0x or $ indicates hex):");	
	break;
      case 'h':
      case 'H':
      case '?':
      case CTRL('H'):
        do_inode_help();
      case CTRL('L'):
	refresh_all();
	break;
      case ' ':
	break;
      default:
	redraw = 0;
	break;
    }
    if (current_inode > sb->ninodes) 
      current_inode = sb->ninodes;
    else if (current_inode < 1 ) 
      current_inode = 1;
    
    if (redraw) {
      cdump_inode(current_inode);
    }
  }
  return 0;
}

int recover_mode()
{
  FILE *fp;
  int j,c,flag;
  char recover_file_name[80];
 
  clobber_workspace(); 
  workspace = newwin(9,80,((VERT-9)/2+HEADER_SIZE),HOFF);
  workspace = newwin(fsc->N_BLOCKS+1,(COLS-HOFF),((VERT-fsc->N_BLOCKS-1)/2+HEADER_SIZE),HOFFPLUS);
  
#if TRAILER_SIZE>0
  werase(trailer);
  strncpy(echo_string,"Enter number corresponding to block to modify values",150);
  mvwaddstr(trailer,0,(COLS-strlen(echo_string))/2-1,echo_string);
  strncpy(echo_string,"Q to quit, R to dump to file",150);
  mvwaddstr(trailer,1,(COLS-strlen(echo_string))/2-1,echo_string);
  wrefresh(trailer);
#endif

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
        fake_inode_zones[c-'0'] = 
	  cread_num("Enter block number (leading 0x or $ indicates hex):");
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
	sprintf(recover_file_name," Write data to file:");
	(void) cread_num(recover_file_name);
	if (strlen(recover_file_name)<1)
	  strcpy(recover_file_name,"RECOVERED.file");
	if ((fp = fopen(recover_file_name,"w")) != NULL) {
	  recover_file(fp, fake_inode_zones);
	  fclose(fp);
	}
#if TRAILER_SIZE>0
	sprintf(echo_string,"Recovered data written to '%s'",
		recover_file_name);
	if (fp==NULL)
	  sprintf(echo_string,"Cannot open file '%s'\n",recover_file_name);
	nc_warn(echo_string);
#endif
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
  return 0;
}

/* Not too exciting main parser with superblock on screen */
void interactive_main()
{
int c;
static int flag;

current_inode = fsc->ROOT_INODE;

/* Curses junk */
initscr();
cbreak();
noecho();
keypad(stdscr, TRUE);
scrollok(stdscr, TRUE);
if (has_colors()) {
  start_color();
  init_pair(1,COLOR_WHITE,COLOR_RED);
  init_pair(2,COLOR_WHITE,COLOR_BLUE);
  init_pair(3,COLOR_RED,COLOR_BLACK);
  RED_ON_WHITE = COLOR_PAIR(1);
  BLUE_ON_WHITE = COLOR_PAIR(2);
  RED_ON_BLACK = COLOR_PAIR(3);
}
else {
  BLUE_ON_WHITE = A_REVERSE;
  RED_ON_BLACK = A_NORMAL;
  RED_ON_WHITE = A_UNDERLINE;
}
HOFF = ( COLS - 80 ) / 2;
HOFFPLUS = COLS>40 ? (COLS-50)/2 : 0;

/* these are the three curses windows */
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
  switch (c|flag) {
    case 'B':
    case 'b':
      c = flag = block_mode();
      break;
    case 'I':
    case 'i':
      c = flag = inode_mode();
      break;
    case 'R':
    case 'r':
      c = flag = recover_mode();
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
    case 'q':
    case 'Q':
      endwin();
      return;
      break;
    case CTRL('L'):
      refresh_all();
      break;
    default:
      flag=0;
      break;
    }

  show_super();
  update_header();
#if TRAILER_SIZE>0
  werase(trailer);
  strncpy(echo_string,"F)lags, I)node, B)locks, R)ecover File",150);
  mvwaddstr(trailer,0, (COLS-strlen(echo_string))/2-1,echo_string);
  wrefresh(trailer);
#endif
}

/* No way out here -- have to use Q key */
}


