/*
 *  lde/nc_dir.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *  Sections Copyright (c) 2002 John Quirk
 *
 *  $Id: nc_dir.c,v 1.23 2002/02/01 03:35:20 scottheavner Exp $
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

#ifdef JQDIR
#include <unistd.h>  /* chown, chdir */
#include "ext2fs.h"
static void clip_string(char *fname,char *fname1, int clip);
/* A little routine to keep track of where we are reitive to start point.*/
static void showpath(char *fname, unsigned long inode_nbr );
static void freepath(void);
#endif /* JQDIR */

/* Help for directory_popup() function */
static lde_menu dp_help[] = {
  { CMD_EXPAND_SUBDIR,"expand directory under cursor"},
  { CMD_EXPAND_SUBDIR_MC,"expand directory under cursor and make it the current inode"},
  { CMD_INODE_MODE,"make inode under cursor the current inode"},
  { CMD_INODE_MODE_MC,"make inode under cursor the current inode and view it"},
#ifdef JQDIR
  { CMD_EDIT_DIR,"Edit the directory Block"},
  { CMD_RESTORE_ENT,"Restore or undelete this entry"},
  { CMD_TEST_ENTRY, "Checks entry to see if can be recoverd" },
  { CMD_COPY_TO_FILE, "Copies directory name and files contents to new location " },  
#endif /* JQDIR */
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
#ifdef JQDIR
  { 'e', CMD_EDIT_DIR },
  { 'r', CMD_RESTORE_ENT },
  { 'c', CMD_COPY_TO_FILE },
  { 't', CMD_TEST_ENTRY },
#endif /* JQDIR */
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
#ifndef JQDIR
  char *fname = NULL;
#else
  char fname[40]; /* this is the cliped name string */
  char deleted = ' ';
#endif /* JQDIR */
  struct Generic_Inode *GInode;
  lde_dirent d = { 0 };
  unsigned long inode_nr;
  char f_mode[12] = "----------";

  int  attr = 0;
  
  if (!FS_cmd.dir_entry(i+off, block_buffer, &d)) return 0;
  inode_nr = d.inode_nr;

#ifndef JQDIR
  fname = d.name;
#else
  clip_string(fname,d.name,40);
#endif

  /* Highlight deleted entries */
  if (d.isdel) {
    if(highlight)
      attr = BLACK_ON_CYAN;
    else
      attr = GREEN_ON_BLACK;
  } else if(highlight) {
    attr = WHITE_ON_RED;
  }
  if (attr) {
      wattron(win,attr);
  }
      
  if (inode_nr > sb->ninodes) inode_nr = 0UL;
  if (inode_nr) {
    GInode = FS_cmd.read_inode(inode_nr);
    mode_string( (unsigned short)GInode->i_mode, f_mode);
    f_mode[10] = 0; 
    
    if(GInode->i_links_count == 0)
      deleted = 'D';
    else
      deleted = ' ';

    mvwprintw(win,i,0,"%c 0x%8.8lX: %9s %3d %9ld %-*s",deleted, inode_nr,
	      f_mode, GInode->i_links_count,
	      GInode->i_size, COLS-40, fname);
  } else {

    mvwprintw(win,i,0,"  0x%8.8lX: %24s %-*s", inode_nr,
	      " ", COLS-40, fname);

  }

  /* Turn off highlighting */
  if (attr) {
      wattroff(win,attr);
  }
  
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

  /* show which mode we are in */
  strcpy(ldemode,"Directory");
  update_header();

  werase(win);
  for (i=0;((i<VERT)&&(i<=max_entries));i++)
    (void) dump_dir_entry(win, i, screen_off, buffer, 0);
  redraw_win(win);
}

/* Read the block buffer from disk, return the number of dir entries contained in this block */
static int reread_dir(lde_buffer *block_buffer, unsigned long bnr)
{
  int i;
  lde_dirent d;

  if (bnr)
    memcpy(block_buffer->start, cache_read_block(bnr,NULL,CACHEABLE),block_buffer->size);
  for (i=0; FS_cmd.dir_entry(i, block_buffer, &d); i++);
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
#ifdef JQDIR
  /* Code to recreate i_size if we have block count
   * This code has only been tested on ext2fs most likely will break on non
   * ext2 systems
   */
  if( GInode->i_size == 0 && GInode->i_blocks !=0) {
    /*Possible deleted dir inode */
    GInode->i_size = GInode->i_blocks * 512;
  }
#endif /* JQDIR */
  buffer->size = GInode->i_size+sb->blocksize;
  buffer->start = malloc(buffer->size);
  bzero(buffer->start, (int)buffer->size);
  c = count = 0UL;
	  
  while (count < GInode->i_size) {
    FS_cmd.map_block(GInode->i_zone,c,&bnr);
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
  lde_dirent d;
  struct Generic_Inode *GInode;
  WINDOW *win;
  char *fname = NULL;
#ifdef JQDIR
  FILE *fp;
#endif /* JQDIR */

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
	(void) FS_cmd.dir_entry(current+screen_off, block_buffer, &d);
	inode_nr = d.inode_nr;
	if (inode_nr) {
	  current_inode = inode_nr;
	  update_header();
	  if (c==CMD_INODE_MODE_MC) {
#ifdef JQDIR
	    freepath(); 
	    update_header();
#endif /* JQDIR */
	    clobber_window(win);
	    free(block_buffer->start);
	    return CMD_INODE_MODE_MC;
	  }
	}
	break;

      case CMD_EXIT: /* Exit popup */
#ifdef JQDIR
	freepath();       /* clear relpath freeing any memory */
	update_header();
#endif /* JQDIR */
      	clobber_window(win);
	free(block_buffer->start);
        return CMD_NO_ACTION;
	break;

      case CMD_EXPAND_SUBDIR: /* Expand this subdirectory - 'D' also sets current inode to be this subdir */
      case CMD_EXPAND_SUBDIR_MC:
	(void) FS_cmd.dir_entry(current+screen_off, block_buffer, &d);
	fname = d.name;
	inode_nr = d.inode_nr;
	if (inode_nr) {
	  GInode = FS_cmd.read_inode(inode_nr);
	  if (S_ISDIR(GInode->i_mode)) {
#ifdef JQDIR
	    showpath(fname,inode_nr);  
#endif /* JQDIR */
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
#ifdef JQDIR
      case CMD_EDIT_DIR: 
      case CMD_RESTORE_ENT:
	if(!strcmp(fsc->text_name,"ext2")) {
	  EXT2_dir_undelete(current+screen_off,block_buffer);
	} else {
	  log_error("Not ready for non ext2 file systems");
	  error_popup();
	  touchwin(win);
	}
	break;
      case CMD_TEST_ENTRY:
	if(!strcmp(fsc->text_name,"ext2")) {
	  EXT2_dir_check_entry(current+screen_off,block_buffer);
	} else {
	  log_error("Not ready for non ext2 file systems");
	  error_popup();
	  touchwin(win);
	}
	break;
      case CMD_COPY_TO_FILE:
	if(!strcmp(fsc->text_name,"ext2")) {
	  if(chdir("/lost+found") ) {
	    log_error("Unable to find lost+found directory on /");
	    error_popup();
	    touchwin(win);
	    break;
	  }

	  (void) FS_cmd.dir_entry(current+screen_off, block_buffer, &d);
	  fname = d.name;
	  inode_nr = d.inode_nr;
	  if((fp = fopen(fname,"w+")) == NULL) {
	    lde_warn("Unable to open file %s",fname);
	    error_popup();
	    touchwin(win);
	    break;
	  }

	  /* file open get inode so we can set some data */
	  GInode = FS_cmd.read_inode(inode_nr);
	  fchown(fileno(fp),GInode->i_uid,GInode->i_gid);
	  fchmod(fileno(fp),GInode->i_mode);
	  recover_file(fileno(fp),GInode->i_zone,GInode->i_size);
	  fclose(fp);
	  
	} else {
	  log_error("Not ready for non ext2 file systems");
	  error_popup();
	  touchwin(win);
	}
	break;
#endif /* JQDIR */

      default:
        continue;
    }

    highlight_dir_entry(win,current,&last,screen_off,block_buffer);
  }

  return 0;
}

#ifdef JQDIR
/* This routine tracks where are in the relitve directory structure on 
 * the disk it also tracks the inode numbers assocated with these items
 */

struct {
		unsigned long inr; /*inode*/
		char *fname;       /* point to filename */
} dir_list[30];

static int where=0;


static void freepath() /* clears data structure in readness for new path */
{
	int i;
	for (i=where; i > 0 ; i--)
		{
		if(dir_list[i-1].inr != 0)
			{
			free(dir_list[i-1].fname);
			dir_list[i-1].inr = 0;
			}
		}
		
	where = 0;
	*rel_path='\0';  /* clear path */
}

/* this keeps track of where we are */

static void showpath( char * fname, unsigned long inode_nbr)
{
	int len, i;

	*rel_path='\0';

	if(strcmp(fname,".") == 0){
		
		/* do nothing */
	}
	else if( strcmp(fname,"..") == 0){	
		if(where != 0 && dir_list[where-1].inr != 0) 
		{
			free(dir_list[where-1].fname);
			dir_list[where-1].inr = 0;  /* mark as removed */
			where--;
		}
	}
	else{
		len = strlen(fname);
		if ((dir_list[where].fname=(char *)malloc(len+1)) != NULL)
		{
			strcpy(dir_list[where].fname,fname);
			dir_list[where].inr=inode_nbr;
			where++;
		}
		else
		{
			perror("fatal error unable to allocate memory");
			exit(-1); /* not clean but will do for now */
		}

	}
	
	for( i=0; i < where; i++)
	{
		strcat(rel_path,"/");
		strcat(rel_path,dir_list[i].fname);
	}
}

static void clip_string(char *fname,char *fname1, int clip)
{
        strncpy(fname,fname1,clip);
        fname[clip-1]= 0;
}
#endif /* JQDIR */
