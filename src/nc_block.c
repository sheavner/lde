/*
 *  lde/nc_block.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_block.c,v 1.1 1994/04/01 09:42:35 sdh Exp $
 */

#include "nc_lde.h"

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
  wattron(win,RED_ON_WHITE);
  mvwprintw(win, nr, 0, str);
  wattroff(win,RED_ON_WHITE);
  
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
  char *block_buffer, *fname = NULL;
  struct Generic_Inode *GInode;
  WINDOW *win;

  win = newwin(VERT,COLS,HEADER_SIZE,0);
  max_entries = screen_off = current = 0;
  scrollok(win, TRUE);
  
  flag=1; c = ' ';
  while (flag||(c = getch())) {
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
	if (current>0)
	  current--;
	else if ((current==0)&&(screen_off>0)) {
	  screen_off--;
	  wscrl(win,-1);
	  dump_dir_entry(win, current, screen_off, bnr, &inode_nr);
	  wrefresh(win);
	}
	break;
      case ' ':
	redraw = 1;
	break;
    }

    if (!max_entries) {
      block_buffer = cache_read_block(bnr,CACHEABLE);
      i = 0;
      do fname = FS_cmd.dir_entry(i++, block_buffer, &inode_nr);
      while (strlen(fname));
      max_entries = i - 2;
    }

    if (redraw) {
      wclear(win);
      for (i=0;((i<VERT)&&(i<max_entries));i++)
	(void) dump_dir_entry(win, i, screen_off, bnr, &inode_nr);
    }

    highlight_dir_entry(win, current);
    wrefresh(win);

  }
#endif ALPHA_CODE
  return 0;
}

/*
 * Display some help in a separate window */
void do_block_help(void)
{
  WINDOW *win;

  win = newwin(19,WIN_COL,((VERT-17)/2+HEADER_SIZE),HOFF);
  wclear(win);
  box(win,0,0);
  mvwprintw(win,1,16,"Program name : Filesystem type : Device name");
  mvwprintw(win,2,7,"Inode: dec. (hex)     Block: dec. (hex)    0123456789:;<=>");
  mvwprintw(win,4,7,"h,H,?,^H: Calls up this help.");
  mvwprintw(win,5,7,"B       : View block under cursor (%s block ptr is %d bytes).",text_names[fsc->FS],fsc->ZONE_ENTRY_SIZE);
  mvwprintw(win,6,7,"i       : Enter inode mode.");
  mvwprintw(win,7,7,"I       : View inode under cursor (%s inode ptr is %d bytes).",text_names[fsc->FS],fsc->INODE_ENTRY_SIZE);
  mvwprintw(win,8,7,"r,R     : Enter recovery mode.");
  mvwprintw(win,9,7,"q,Q     : Quit.");
  mvwprintw(win,10,7,"arrows  : Move cursor (also ^P, ^N, ^F, ^B)");
  mvwprintw(win,11,7,"+/-     : View next/previous part of current block.");
  mvwprintw(win,12,7,"PGUP/DN : View next/previous block (also ^U, ^D)");
  mvwprintw(win,13,7,"0123... : Add current block to recovery list at position.");
  mvwprintw(win,14,7,"#       : Enter block number and view it.");
  mvwprintw(win,15,7,"l,L     : Find an inode which references this block.");
  mvwprintw(win,16,7,"W,w     : Write this block's data to a file.");
  mvwprintw(win,17,7,"E,e     : Edit block. (^H for help in edit mode)");
  mvwprintw(win,18,25," Press any key to continue. ");
  wrefresh(win);
  getch();
  delwin(win);
  refresh_all();

  return;
}

/*
 * Display some help in a separate window */
void do_block_edit_help(void)
{
  WINDOW *win;

  win = newwin(13,WIN_COL,((VERT-17)/2+HEADER_SIZE),HOFF);
  wclear(win);
  box(win,0,0);
  mvwprintw(win,1,16,"Program name : Filesystem type : Device name");
  mvwprintw(win,2,7,"Inode: dec. (hex)     Block: dec. (hex)    0123456789:;<=>");
  mvwprintw(win,4,7,"^H       : Calls up this help.");
  mvwprintw(win,5,7,"arrows   : Move cursor (also ^P, ^N, ^F, ^B)");
  mvwprintw(win,6,7,"+/-      : View next/previous part of current block.");
  mvwprintw(win,7,7,"PGUP/DN  : View next/previous block (also ^U, ^D)");
  mvwprintw(win,8,7,"TAB,^I   : Toggle hex/ascii edit.");
  mvwprintw(win,9,7,"^W       : Write changes to disk.");
  mvwprintw(win,10,7,"^A       : Abort changes, re-read original block from disk.");
  mvwprintw(win,12,25," Press any key to continue. ");
  wrefresh(win);
  getch();
  delwin(win);
  refresh_all();

  return;
}

/* Dump as much of a block as we can to the screen and format it in a nice 
 * hex mode with ASCII printables off to the right. */
unsigned char *cdump_block(nr,win_start,win_size)
unsigned long nr;
int win_start, win_size;
{
  int i,j;
  unsigned char *dind,  c;
  char block_not_used[10]=":NOT:USED:", block_is_used[10] = "::::::::::";
  char *block_status;
 
  clobber_window(workspace); 
  workspace = newwin(VERT,(COLS-HOFF),HEADER_SIZE,HOFF);
  
  dind = cache_read_block(nr,CACHEABLE);
  
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

/* Asks the user if it is ok to write the block to the disk, then goes
 * ahead and does it if the medium is writable.  The user has access to the
 * flags menu from this routine, to toggle the write flag */
void cwrite_block(unsigned long block_nr, char *data_buffer, int *mod_yes)
{
  int c;
  char *warning;

  if (*mod_yes) {
    if (!write_ok)
      warning = "(NOTE: write permission not set on disk, use 'F' to set flags before 'Y') ";
    else
      warning = "";
    
    while ( (c = cquery("WRITE OUT BLOCK DATA TO DISK [Y/N]? ","ynfq",warning)) == 'f')
      flag_popup();

    refresh_all(); /* Have to refresh screen before write or error messages will be lost */
    if (c == 'y')
      write_block(block_nr, data_buffer);
  }
  
  *mod_yes = 0;
}

/* Highlights the current position in the block in both ASCII + hex */
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
 * on the screen, next/previous block, paging this block, etc., etc. */
int block_mode() {
  int c,flag,redraw;
  static int win_start = 0;
  static int cur_row = 0, cur_col = 0;
  static int prev_row = 0, prev_col = 0;
  unsigned char *block_buffer = NULL, *copy_buffer = NULL, warning_string[80];
  unsigned long temp_ptr;
  long a;

  int edit_block, ascii_mode, highlight, val, v1, icount, modified = 0;
  char *HEX_PTR, *HEX_NOS = "0123456789ABCDEF";

  unsigned long inode_ptr[2] = { 0UL, 0UL };

  display_trailer("PG_UP/DOWN = previous/next block, or '#' to enter block number",
		  "H for help.  Q to quit");

  flag = 1; c = ' ';
  edit_block = 0; icount=0; ascii_mode=0; v1=0; modified = 0;
  cur_row = cur_col = 0;
  while (flag||(c = getch())) {
    flag = 0;
    highlight = redraw = 1;
    if (edit_block) {
      if (c == CTRL('I')) {
	ascii_mode = 1 - ascii_mode;
      } else if (c == CTRL('H')) {
	do_block_edit_help();
	c = 0;
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
      case( CTRL('W')):
	edit_block = 0;
	cwrite_block(current_block, block_buffer, &modified);
	break;
      case CTRL('A'):
	modified = ascii_mode = edit_block = 0;
	block_buffer = cache_read_block(current_block,FORCE_READ);
	c = ' ';
	break;
      case 'l':
      case 'L':
	if ( (temp_ptr = find_inode(current_block)) )
	  sprintf(warning_string, "Block is indexed under inode 0x%lX.\n",temp_ptr);
	else
	  if (rec_flags.search_all)
	    sprintf(warning_string, "Unable to find inode referenece.\n");
	  else
	    sprintf(warning_string, "Unable to find inode referenece try activating the --all option.\n");
	warn(warning_string);
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
	/* Need to use memcpy rather than just reassigning pointer because the next 
	   read() will clobber block_buffer. */
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
	c = directory_popup(current_block);
	if (c==' ')
	  redraw = 1;
	else {
	  redraw = highlight = 0;
	  flag = 1;
	}
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
      case 'W':
      case 'w':
	inode_ptr[0] = current_block;
	crecover_file(inode_ptr);
	break;
      case '#':
        cwrite_block(current_block, block_buffer, &modified);
        if (cread_num("Enter block number (leading 0x or $ indicates hex):",&a)) {
	  current_block = a;
	  win_start = prev_col = prev_row = cur_col = cur_row = 0;
	  edit_block = 0;
	}
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
	refresh_ht();
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
