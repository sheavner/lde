/*
 *  lde/nc_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_lde.c,v 1.1 1994/03/19 17:12:32 sdh Exp $
 */

#include "lde.h"

#ifdef NC_HEADER
#include <ncurses.h>
#else
#include <curses.h>
#endif

/* From mode_string.c */
void mode_string();

/* Curses variables */
int DATA_START, DATA_END, HOFF;
int VERT, VERT_CENT, VERT_SUPER, VERT_INODE, HOFFPLUS;
int HIGHLIGHT_1, HIGHLIGHT_2, END_HIGHLIGHT;
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
  sprintf(echo_string,"Inode: %ld (0x%5.5lx)",current_inode,current_inode);
  mvwprintw(header,HEADER_SIZE-1,HOFF,"%24s",echo_string);
  sprintf(echo_string,"Block: %ld (0x%5.5lx)",current_block,current_block);
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
  wattron(header,HIGHLIGHT_2);
  werase(header);
  for (i=0;i<HEADER_SIZE;i++) /* A cute way to RVS video the header window */
    for (j=0;j<COLS;j++)
      mvwaddch(header,i,j,' ');
  sprintf(echo_string,"%s v%s : %s : %s",program_name,VERSION,text_names[fsc->FS],device_name);
  mvwaddstr(header,0, (COLS-strlen(echo_string))/2-1,echo_string);
  wrefresh(header);
#endif
}

void nc_warn(char * echo_string)
{
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

  win = newwin(13,80,(VERT-13)/2,HOFF);
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
  mvwprintw(win,11,20,"Press any key to continue.");
  wrefresh(win);
  getch();
  delwin(win);
  touchwin(stdscr);
  refresh();

  return;
}
/*
 * Display some help in a separate window
 */
void do_block_help(void)
{
  WINDOW *win;

  win = newwin(17,80,(VERT-17)/2,HOFF);
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
  mvwprintw(win,16,25,"Press any key to continue.");
  wrefresh(win);
  getch();
  delwin(win);
  touchwin(stdscr);
  refresh();

  return;
}
void do_inode_help(void)
{
  WINDOW *win;

  win = newwin(16,80,(VERT-16)/2,HOFF);
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
  mvwprintw(win,15,25,"Press any key to continue.");
  wrefresh(win);
  getch();
  delwin(win);
  touchwin(stdscr);
  refresh();

  return;
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

/* Format the super block for curses */
void show_super()
{
  clobber_workspace();
  workspace = newwin(VERT_SUPER,COLS/2,VERT_CENT,HOFFPLUS);
  
  mvwprintw(workspace,0,0,"Inodes:       %10d (0x%8.8x)",sb->ninodes, sb->ninodes);
  mvwprintw(workspace,1,0,"Blocks:       %10d (0x%8.8x)",sb->nzones, sb->nzones);
  mvwprintw(workspace,2,0,"Firstdatazone:%10d (N=%d)",sb->first_data_zone,sb->norm_first_data_zone);
  mvwprintw(workspace,3,0,"Zonesize:     %10d (0x%4.4x)",sb->blocksize, sb->blocksize);
  mvwprintw(workspace,4,0,"Maximum size: %10d (0x%8.8x)",sb->max_size,sb->max_size);
  mvwprintw(workspace,6,0,"* Directory entries are %d characters.",sb->namelen);
  mvwprintw(workspace,7,0,"* Inode map occupies %d blocks.",sb->imap_blocks);
  mvwprintw(workspace,8,0,"* Zone map occupies %d blocks.",sb->zmap_blocks);
  mvwprintw(workspace,9,0,"* Inode table occupies %d blocks.",INODE_BLOCKS);
  wrefresh(workspace);
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
  
  dind = cache_read_block(nr);
  
  block_status = (zone_in_use(nr)) ? block_is_used : block_not_used; 
  j = 0;

  while ((j<win_size)&&(j*16+win_start<sb->blocksize)) {
    mvwprintw(workspace,j,0,"0x%04x = ",j*16+win_start);
    for (i=0;i<8;i++)
      mvwprintw(workspace,j,9+i*3,"%2.2x",dind[j*16+i+win_start]);
    mvwprintw(workspace,j,34,"%c",block_status[j%10]);
    for (i=0;i<8;i++)
      mvwprintw(workspace,j,37+i*3,"%2.2x",dind[j*16+i+8+win_start]);
    
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

void highlight_block(int cur_row,int cur_col,int win_start,unsigned char *block_buffer)
/* int cur_row, cur_col, win_start; */
{
  unsigned char c;
  int s_col;

  if (cur_col < 8)
    s_col = 9 + cur_col*3;
  else
    s_col = 13 + cur_col*3;
  mvwprintw(workspace,cur_row,s_col,"%2.2x",block_buffer[cur_row*16+cur_col+win_start]);
  c = block_buffer[cur_row*16+cur_col+win_start];
  c = ((c>31)&&(c<127)) ? c : '.';
  mvwprintw(workspace,cur_row,cur_col+63,"%c",c);
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
  unsigned char *block_buffer = NULL;
  unsigned long temp_ptr;

#if TRAILER_SIZE>0
  werase(trailer);
  strncpy(echo_string,"PG_UP/DOWN = previous/next block, or '#' to enter block number",150);
  mvwaddstr(trailer,0,(COLS-strlen(echo_string))/2-1,echo_string);
  strncpy(echo_string,"H for help.  Q to quit",150);
  mvwaddstr(trailer,1,(COLS-strlen(echo_string))/2-1,echo_string);
  wrefresh(trailer);
#endif

  flag = 1; c = ' ';
  cur_row = cur_col = 0;
  while (flag||(c = getch())) {
    flag = 0;
    redraw = 1;
    switch(c) {
      case 'F'-'A'+1: /* ^F */
      case KEY_RIGHT:
	redraw = 0;
	if (++cur_col > 15) {
	  cur_col = 0;
	  if (++cur_row >= VERT) {
	    if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	    prev_row = prev_col = cur_row = 0;
	    redraw = 1;
	  }
	}
	break;
      case '+':
      case KEY_SRIGHT:
        if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	break;
      case 'B'-'A'+1: /* ^B */
      case KEY_LEFT:
	if (--cur_col < 0) {
	  cur_col = 15;
	  if (--cur_row < 0) {
	    cur_row = cur_col = 0;
	  }
	}
	redraw = 0;
	break;
      case '-':
      case KEY_SLEFT:
	if (win_start - VERT*16 >= 0)  win_start -= VERT*16;
	break;
      case 'N'-'A'+1: /* ^N */
      case KEY_DOWN:
	if (++cur_row >= VERT) {
	  if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	  prev_row = prev_col = cur_row = 0;
	} else
	  redraw = 0;
	break;
      case 'V'-'A'+1: /* ^V */
      case KEY_NPAGE:
	current_block++;
	win_start = prev_col = prev_row = cur_col = cur_row = 0;
	break;
      case 'P'-'A'+1: /* ^P */
      case KEY_UP:
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
      case KEY_PPAGE:
	if (current_block!=0) current_block--;
	win_start = prev_col = prev_row = cur_col = cur_row = 0;
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
	fake_inode_zones[c-'0'] = current_block;
	break;
      case 'l':
      case 'L':
	if ( (temp_ptr = find_inode(current_block)) )
	  sprintf(echo_string, "Block is indexed under inode 0x%lx.\n",temp_ptr);
	else
	  if (rec_flags.search_all)
	    sprintf(echo_string, "Unable to find inode referenece.\n");
	  else
	    sprintf(echo_string, "Unable to find inode referenece try activating the --all option.\n");
	warn(echo_string);
	break;
      case 'B':
	temp_ptr = block_pointer(&block_buffer[cur_row*16+cur_col+win_start],(unsigned long) 0, fsc->ZONE_ENTRY_SIZE);
	if (temp_ptr <= sb->nzones) {
	  current_block = temp_ptr;
	  return c;
	}
	break;
      case 'I':
	temp_ptr = block_pointer(&block_buffer[cur_row*16+cur_col+win_start],(unsigned long) 0, fsc->INODE_ENTRY_SIZE);
	if (temp_ptr <= sb->ninodes) {
	  current_inode = temp_ptr;
	  return c;
	}
	break;
      case 'q':
      case 'Q':
      case 'S':
      case 's':
      case 'i':
      case 'R':
      case 'r':
	return c;
	break;
      case '#':
	current_block = cread_num("Enter block number (leading 0x or $ indicates hex):");
	prev_col = prev_row = cur_col = cur_row = 0;
	break;
      case 'h':
      case 'H':
      case '?':
      case 'H'-'A'+1: /* ^H */
        do_block_help();
      case 'L'-'A'+1: /* ^L */
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

    if (redraw)
      block_buffer = cdump_block(current_block,win_start,VERT);

    /* Highlight current cursor position */
    /* First turn off last position */
    highlight_block(prev_row,prev_col,win_start,block_buffer);
    /* Now highlight current */
    wattron(workspace,HIGHLIGHT_1);
    prev_col = cur_col;
    prev_row = cur_row;
    highlight_block(cur_row,cur_col,win_start,block_buffer);
    wattroff(workspace,HIGHLIGHT_1);

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
  workspace = newwin(9,(COLS-HOFF),(VERT-9)/2,HOFF);
  
  imode = DInode.i_mode(nr);
  mode_string((unsigned short)imode,f_mode);
  f_mode[10] = 0; /* Junk from canned mode_string */
  mvwprintw(workspace,0,0,"%10s",f_mode);
  mvwprintw(workspace,0,11,"%3d",DInode.i_links_count(nr));
  if ((NC_PASS = getpwuid(DInode.i_uid(nr)))!=NULL)
    mvwprintw(workspace,0,15,"%8-s",NC_PASS->pw_name);
  else
    mvwprintw(workspace,0,15,"%8-d",DInode.i_uid(nr));
  if ((NC_GROUP = getgrgid(DInode.i_gid(nr)))!=NULL)
    mvwprintw(workspace,0,24,"%8-s",NC_GROUP->gr_name);
  else
    mvwprintw(workspace,0,24,"%8-d",DInode.i_gid(nr));
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
  
  /* This covers blocks which would give Minix mode not cleared errors */
  if (!inode_in_use(nr)) mvwprintw(workspace,3,25,"(NOT USED)");

  mvwprintw(workspace,4,0,"MODE: \\%4.4s FLAGS: \\%3.3s\n",&f_mode[3],f_mode);
  mvwprintw(workspace,5,0,"UID: %05d(%s)",DInode.i_uid(nr), (NC_PASS != NULL) ? NC_PASS->pw_name : "");
  mvwprintw(workspace,5,20,"GID: %05d(%s)",DInode.i_gid(nr), (NC_GROUP != NULL) ? NC_GROUP->gr_name : "");
  mvwprintw(workspace,6,0,"TIME: %24s",ctime(&atime));
  mvwprintw(workspace,6,0,"LINKS: %3d SIZE: %-8d \n",DInode.i_links_count(nr),DInode.i_size(nr));
 
  if (DInode.i_zone(nr,(unsigned long) 0)) {
    j=-1;
    mvwprintw(workspace,7,0,"BLOCKS=");
    while ((++j<fsc->N_BLOCKS)&&(DInode.i_zone(nr,j))) {
      max_blocks_this_inode = j;
      if (j==highlight_izone) wattron(workspace,HIGHLIGHT_1);
      mvwprintw(workspace,7+j/7,(j%7+1)*10,"0x%7.7x",DInode.i_zone(nr,j));
      wattroff(workspace,HIGHLIGHT_1);
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
      case 'N'-'A'+1: /* ^N */
      case KEY_NPAGE:
      case KEY_DOWN:
        current_inode++;
	highlight_izone = 0;
	break;
      case 'P'-'A'+1: /* ^P */
      case KEY_PPAGE:
      case KEY_UP:
	current_inode--;
	highlight_izone = 0;
	break;
      case 'F'-'A'+1: /* ^F */
      case KEY_RIGHT:
	if (++highlight_izone > max_blocks_this_inode) 
	  highlight_izone = max_blocks_this_inode;
	break;
      case 'B'-'A'+1: /* ^B */
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
      case '#':
	current_inode = cread_num("Enter inode number (leading 0x or $ indicates hex):");	
	break;
      case 'h':
      case 'H':
      case '?':
      case 'H'-'A'+1: /* ^H */
        do_inode_help();
      case 'L'-'A'+1: /* ^L */
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
  workspace = newwin(fsc->N_BLOCKS+1,(COLS-HOFF),(LINES-HEADER_SIZE-TRAILER_SIZE-fsc->N_BLOCKS)/2,HOFFPLUS);
  
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
      case 'h':
      case 'H':
      case '?':
      case 'H'-'A'+1: /* ^H */
        do_help();
      case 'L'-'A'+1: /* ^L */
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
      mvwprintw(workspace,j,20," %2d : 0x%8.8x",j,fake_inode_zones[j]);
    if (fsc->INDIRECT) {
      mvwprintw(workspace,j,0,"INDIRECT BLOCK:" );
      mvwprintw(workspace,j,20," %2d : 0x%8.8x",j,fake_inode_zones[fsc->INDIRECT]);
      j++;
    }
    if (fsc->X2_INDIRECT) {
      mvwprintw(workspace,j,0,"2x INDIRECT BLOCK:" );
      mvwprintw(workspace,j,20," %2d : 0x%8.8x",j,fake_inode_zones[fsc->X2_INDIRECT]);
      j++;
    }
    if (fsc->X3_INDIRECT) {
      mvwprintw(workspace,j,0,"3x INDIRECT BLOCK:" );
      mvwprintw(workspace,j,20," %2d : 0x%8.8x",j,fake_inode_zones[fsc->X3_INDIRECT]);
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
  HIGHLIGHT_1 = COLOR_PAIR(1);
  HIGHLIGHT_2 = COLOR_PAIR(2);
}
else {
  HIGHLIGHT_1 = HIGHLIGHT_2 = A_REVERSE;
}
HOFF = ( COLS - 80 ) / 2;
HOFFPLUS = COLS>40 ? (COLS-50)/2 : 0;
VERT_INODE = VERT>7  ? 7 : VERT;
VERT_SUPER = VERT>10 ? 10 : VERT;
VERT_CENT = (VERT - VERT_SUPER)/2 + HEADER_SIZE;

/* These are the three curses windows */
#if HEADER_SIZE>0
header = newwin(HEADER_SIZE,COLS,0,0);
#endif
workspace = newwin(VERT_SUPER,COLS/2,VERT_CENT,HOFFPLUS);
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
    case 'L'-'A'+1: /* ^L */
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
  strncpy(echo_string,"I)node, B)locks, R)ecover File",150);
  mvwaddstr(trailer,0, (COLS-strlen(echo_string))/2-1,echo_string);
  wrefresh(trailer);
#endif
}

/* No way out here -- have to use Q key */
}

