/*
 *  lde/nc_block.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_block.c,v 1.4 1994/04/24 20:37:05 sdh Exp $
 */

#include "nc_lde.h"
#include "nc_block_help.h"

/* Dump as much of a block as we can to the screen and format it in a nice 
 * hex mode with ASCII printables off to the right. */
void cdump_block(unsigned long nr, unsigned char *dind, int win_start, int win_size)
{
  int i,j;
  unsigned char c;
  char *block_status, block_not_used[10]=":NOT:USED:", block_is_used[10] = "::::::::::";
 
  clobber_window(workspace); 
  workspace = newwin(VERT,(COLS-HOFF),HEADER_SIZE,HOFF);
  
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
void cwrite_block(unsigned long block_nr, void *data_buffer, int *mod_yes)
{
  int c;
  char *warning;

  if (*mod_yes) {
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

/* Highlight current cursor position, turn off last highlight */
void full_highlight_block(int cur_row,int cur_col, int *prev_row, int *prev_col, 
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
void update_block_help(void)
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
  int c,flag,redraw;
  int win_start = 0;
  int cur_row = 0, cur_col = 0;
  int prev_row = 0, prev_col = 0;
  static unsigned char *copy_buffer = NULL;
  unsigned char block_buffer[MAX_BLOCK_SIZE];
  unsigned long temp_ptr;

  int edit_block, ascii_mode, highlight, val, v1, icount, modified = 0;
  char *HEX_PTR, *HEX_NOS = "0123456789ABCDEF";

  unsigned long inode_ptr[2] = { 0UL, 0UL };

  update_block_help();

  display_trailer("PG_UP/DOWN = previous/next block, or '#' to enter block number",
		  "H for help.  Q to quit");

  flag = 1; c = ' ';
  edit_block = 0; icount=0; ascii_mode=0; v1=0; modified = 0;
  cur_row = cur_col = 0;
  while (flag||(c = mgetch())) {
    redraw = highlight = 0;
    if (!flag && edit_block) {
      if (!ascii_mode) {
	HEX_PTR = strchr(HEX_NOS, toupper(c));
	if (HEX_PTR != NULL) {
	  val = HEX_PTR - HEX_NOS;
	  if (icount == 0) {
	    icount++;
	    v1 = val;
	    wattron(workspace,WHITE_ON_RED);
	    waddch(workspace,HEX_NOS[val]);
	    wattroff(workspace,WHITE_ON_RED);
	    c = 0;
	  } else {
	    icount = 0;
	    v1 = v1*16 + val;
	    block_buffer[cur_row*16+cur_col+win_start] = v1;
	    modified = 1;
	    c = KEY_RIGHT; /* Advance the cursor */
	    highlight = 1;
	  }
	}
      } else { /* ASCII MODE */
	if ((c>31)&&(c<127)) {
	  block_buffer[cur_row*16+cur_col+win_start] = c;
	  modified = 1;
	  c = KEY_RIGHT; /* Advance the cursor */
	  highlight = 1;
	}
      }
    }

    flag = 0;
    
    /* These keys are valid in both modes */
    switch(c) {
      case CTRL('I'):
	ascii_mode = 1 - ascii_mode;
	highlight = 1;
        break;
      case CTRL('F'):
      case KEY_RIGHT:
      case 'l':
      case 'L':
	highlight = 1;
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
	redraw = 1;
	icount = 0;
	break;
      case CTRL('B'):
      case KEY_BACKSPACE:
      case KEY_DC:
      case KEY_LEFT:
      case 'h':
      case 'H':
	highlight = 1;
	icount = 0;
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
	redraw = 1;
	break;
      case CTRL('N'):
      case KEY_DOWN:
      case 'J':
      case 'j':
	highlight = 1;
	if (++cur_row >= VERT) {
	  redraw = 1;
	  if ( win_start + VERT*16 < sb->blocksize) win_start += VERT*16;
	  prev_row = prev_col = cur_row = 0;
	}
	icount = 0;
	break;
      case CTRL('V'):
      case CTRL('D'):
      case KEY_NPAGE:
        cwrite_block(current_block, block_buffer, &modified);
	current_block++;
	edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	redraw = 1;
	break;
      case CTRL('P'):
      case KEY_UP:
      case 'k':
      case 'K':
	highlight = 1;
	icount = 0;
	if (--cur_row<0) {
	  redraw = 1;
	  if (win_start - VERT*16 >= 0) {
	    win_start -= VERT*16;
	    cur_row = VERT - 1;
	  } else
	    cur_row = redraw = 0;
	  prev_col = prev_row = 0;
	}
	break;
      case CTRL('U'):
      case META('v'):
      case META('V'):
      case KEY_PPAGE:
        cwrite_block(current_block, block_buffer, &modified);
	if (current_block!=0) current_block--;
	edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	redraw = 1;
	break;
      case( CTRL('W')):
	edit_block = 0;
	cwrite_block(current_block, block_buffer, &modified);
	break;
      case CTRL('A'):
	modified = ascii_mode = edit_block = 0;
	memcpy(block_buffer, cache_read_block(current_block,CACHEABLE), sb->blocksize);
	redraw = highlight = 1;
	break;
      case CTRL('R'):
	if ( (temp_ptr = find_inode(current_block)) )
	  warn("Block is indexed under inode 0x%lX.",temp_ptr);
	else
	  if (rec_flags.search_all)
	    warn("Unable to find inode referenece.");
	  else
	    warn("Unable to find inode referenece try activating the --all option.\n");
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
	warn("Block (%lu) copied into copy buffer.",current_block);
	break;
      case 'p':
      case 'P':
	if (copy_buffer) {
	  modified = 1;
	  memcpy(block_buffer, copy_buffer, sb->blocksize);
	  if (!write_ok) warn("Turn on write permissions before saving this block");
	}
	break;
      case 'E':
      case 'e':
	edit_block = 1;
	icount = 0;
	if (!write_ok) warn("Disk not writeable, change status flags with (F)");
	break;
      case 'D':
      case 'd':
	if ( (c = directory_popup(current_block)) == ' ')
	  redraw = 1;
	else
	  flag = 1;
	break;
      case 'B':
	temp_ptr = block_pointer(&block_buffer[cur_row*16+cur_col+win_start],(unsigned long) 0, fsc->ZONE_ENTRY_SIZE);
	if (temp_ptr <= sb->nzones) {
	  current_block = temp_ptr;
	  edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	  redraw = 1;
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
	  warn("Inode (%lu) out of range in block_mode().",temp_ptr);
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
      case 'V':
      case 'v':
	c = flag = error_popup();
	break;
      case 'W':
      case 'w':
	inode_ptr[0] = current_block;
	crecover_file(inode_ptr);
	break;
      case '#':
        cwrite_block(current_block, block_buffer, &modified);
        if (cread_num("Enter block number (leading 0x or $ indicates hex):",&current_block)) {
	  edit_block = win_start = prev_col = prev_row = cur_col = cur_row = 0;
	  redraw = 1;
	}
	break;
      case '?':
      case KEY_F(1):
      case CTRL('H'):
      case META('h'):
      case META('H'):
	do_scroll_help(block_help, FANCY);
	refresh_all();
	break;
      case 'z':
      case KEY_F(2):
      case CTRL('O'):
	if ( (c = flag = do_popup_menu(block_options,block_map)) == '*')
	  c = flag = do_popup_menu(edit_options,edit_map);
	break;
      case CTRL('L'):
	icount = 0;  /* We want to see actual values? */
	refresh_ht();
      case ' ':
	redraw = 1;
      default:
	break;
    }

    if (current_block > sb->nzones)
      current_block=sb->nzones;

    /* More room on screen, but have we gone past the end of the block? */
    if (cur_row*16+win_start>=sb->blocksize)
      cur_row = (sb->blocksize-win_start)/16 - 1;

    if (redraw) {
      if (!edit_block)
	memcpy(block_buffer, cache_read_block(current_block,CACHEABLE), sb->blocksize);
      cdump_block(current_block,block_buffer,win_start,VERT);
      highlight = 1;
    }

    /* Highlight current cursor position */
    if (highlight)
      full_highlight_block(cur_row,cur_col,&prev_row,&prev_col,win_start,block_buffer,ascii_mode);

    wrefresh(workspace);
  }
  return 0;
}


