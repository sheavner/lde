/*
 *  lde/nc_inode.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_inode.c,v 1.3 1994/04/24 20:36:43 sdh Exp $
 */

#include "nc_lde.h"
#include "nc_inode_help.h"

static int park_x = 0, park_y = 0;

void cwrite_inode(unsigned long inode_nr, struct Generic_Inode *GInode, int *mod_yes)
{
  int c;
  char *warning;

#ifdef ALPHA_CODE
  if (*mod_yes) {
    if (!write_ok)
      warning = "(NOTE: write permission not set on disk, use 'F' to set flags before 'Y') ";
    else
      warning = "";
    
    while ( (c = cquery("WRITE OUT INODE DATA TO DISK [Y/N]? ","ynfq",warning)) == 'f') {
      flag_popup();
      if (write_ok) warning = "";
    }

    refresh_all(); /* Have to refresh screen before write or error messages will be lost */
    if (c == 'y')
      FS_cmd.write_inode(inode_nr, GInode);
  }
#endif
  
  *mod_yes = 0;
}

/* Display current labels */
void cdump_inode_labels()
{
  clobber_window(workspace); 
  workspace = newwin(17,WIN_COL,((VERT-17)/2+HEADER_SIZE),HOFF);

  /* Now display it again in a longer format */
  if (fsc->inode->i_mode)
    mvwprintw(workspace,2,0,"TYPE: ");

  if (fsc->inode->i_links_count)
    mvwprintw(workspace,2,20,"LINKS: ");

  if (fsc->inode->i_mode)
    mvwprintw(workspace,3,0,"MODE: ");

  if (fsc->inode->i_mode_flags)
    mvwprintw(workspace,3,20,"FLAGS: ");

  if (fsc->inode->i_uid)
    mvwprintw(workspace,4,0,"UID: ");

  if (fsc->inode->i_gid)
    mvwprintw(workspace,4,20,"GID: ");

  if (fsc->inode->i_size)
    mvwprintw(workspace,5,0,"SIZE: ");

  if (fsc->inode->i_blocks)
    mvwprintw(workspace,5,20,"SIZE(BLKS): ");

  if (fsc->inode->i_atime)
    mvwprintw(workspace,7,0,"ACCESS TIME:        ");

  if (fsc->inode->i_ctime)
    mvwprintw(workspace,8,0,"CREATION TIME:      ");

  if (fsc->inode->i_mtime)
    mvwprintw(workspace,9,0,"MODIFICATION TIME:  ");

  if (fsc->inode->i_dtime)
    mvwprintw(workspace,10,0,"DELETION TIME:      ");
 
  if (fsc->inode->i_zone[0]) {
    mvwprintw(workspace,2,47,"DIRECT BLOCKS=");
    if (fsc->INDIRECT)
      mvwprintw(workspace,fsc->INDIRECT+2,47,"INDIRECT BLOCK=");
    if (fsc->X2_INDIRECT)
      mvwprintw(workspace,fsc->X2_INDIRECT+2,47,"2x INDIRECT BLOCK=");
    if (fsc->X3_INDIRECT)
      mvwprintw(workspace,fsc->X3_INDIRECT+2,47,"3x INDIRECT BLOCK=");
  }
}

/* Display current inode */
void cdump_inode_values(unsigned long nr, struct Generic_Inode *GInode, int highlight_field)
{
  unsigned long imode = 0UL, j = 0UL;
  char f_mode[12];
  struct passwd *NC_PASS = NULL;
  struct group *NC_GROUP = NULL;

  /* Line 0 looks like a directory entry */
  if (fsc->inode->i_links_count) {
    imode = GInode->i_mode;
    mode_string((unsigned short)imode,f_mode);
    f_mode[10] = 0; /* Junk from canned mode_string */
    mvwprintw(workspace,0,0,"%10s",f_mode);
  }

  if (fsc->inode->i_links_count)
    mvwprintw(workspace,0,11,"%3d",GInode->i_links_count);
  if (fsc->inode->i_uid)
    if ((NC_PASS = getpwuid(GInode->i_uid))!=NULL)
      mvwprintw(workspace,0,15,"%-8s",NC_PASS->pw_name);
    else
      mvwprintw(workspace,0,15,"%-8d",GInode->i_uid);
  if (fsc->inode->i_gid)
    if ((NC_GROUP = getgrgid(GInode->i_gid))!=NULL)
      mvwprintw(workspace,0,24,"%-8s",NC_GROUP->gr_name);
    else
      mvwprintw(workspace,0,24,"%-8d",GInode->i_gid);
  if (fsc->inode->i_size)
    mvwprintw(workspace,0,32,"%9ld",GInode->i_size);
  if (fsc->inode->i_mtime)
    mvwprintw(workspace,0,43,"%24s",ctime(&GInode->i_mtime));

  if (!FS_cmd.inode_in_use(nr)) mvwprintw(workspace,1,30,"(NOT USED)");

  /* Now display it again in a longer format */
  if (fsc->inode->i_mode) {
    sprintf(f_mode,"%07lo",imode);
    mvwprintw(workspace,2,6,entry_type(imode));
  }

  if (fsc->inode->i_links_count) {
    if (highlight_field == I_LINKS_COUNT) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 2;
      park_x = 27;
    }
    mvwprintw(workspace,2,27,"%3d",GInode->i_links_count);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_mode) {
    if (highlight_field == I_MODE) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 3;
      park_x = 6;
    }
    mvwprintw(workspace,3,6,"\\%4.4s",&f_mode[3]);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_mode_flags) {
    if (highlight_field == I_MODE_FLAGS) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 3;
      park_x = 27;
    }
    mvwprintw(workspace,3,27,"\\%2.2s\n",&f_mode[1]);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_uid) {
    mvwprintw(workspace,4,10,"(%s)",(NC_PASS != NULL) ? NC_PASS->pw_name : "");
    if (highlight_field == I_UID) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 4;
      park_x = 5;
    }
    mvwprintw(workspace,4,5,"%05d",GInode->i_uid);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_gid) {
    mvwprintw(workspace,4,30,"(%s)",(NC_GROUP != NULL) ? NC_GROUP->gr_name : "");
    if (highlight_field == I_GID) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 4;
      park_x = 25;
    }
    mvwprintw(workspace,4,25,"%05d",GInode->i_gid);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_size) {
    if (highlight_field == I_SIZE) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 5;
      park_x = 6;
    }
    mvwprintw(workspace,5,6,"%-8ld",GInode->i_size);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_blocks) {
    if (highlight_field == I_BLOCKS) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 5;
      park_x = 32;
    }
    mvwprintw(workspace,5,32,"%-8ld \n",GInode->i_blocks);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_atime) {
    if (highlight_field == I_ATIME) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 7;
      park_x = 20;
    }
    mvwprintw(workspace,7,20,"%24s",ctime(&GInode->i_atime));
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_ctime) {
    if (highlight_field == I_CTIME) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 8;
      park_x = 20;
    }
    mvwprintw(workspace,8,20,"%24s",ctime(&GInode->i_ctime));
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_mtime) {
    if (highlight_field == I_MTIME) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 9;
      park_x = 20;
    }
    mvwprintw(workspace,9,20,"%24s",ctime(&GInode->i_mtime));
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_dtime) {
    if (highlight_field == I_DTIME) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 10;
      park_x = 20;
    }
    mvwprintw(workspace,10,20,"%24s",ctime(&GInode->i_dtime));
    wattroff(workspace,WHITE_ON_RED);
  }
 
  if (fsc->inode->i_zone[0]) {
    j=-1;
    while (++j<fsc->N_BLOCKS) {
      if (j==(highlight_field-I_ZONE_0)) {
	wattron(workspace,WHITE_ON_RED);
	park_y = 2+j;
	park_x = 65;
      }
      if (GInode->i_zone[j]) {
	mvwprintw(workspace,2+j,65,"0x%7.7lX",GInode->i_zone[j]);
      } else {
	mvwprintw(workspace,2+j,65,"         ");
      }
      wattroff(workspace,WHITE_ON_RED);
    }
  }

  update_header();
  wmove(workspace,park_y,park_x);
  wrefresh(workspace);
}

/* Not terribly ugly, but it does all the type casting we might require */
void set_inode_field(int curr_field, unsigned long a, struct Generic_Inode *GInode)
{
  switch (curr_field) {
    case I_MODE:
      GInode->i_mode = (GInode->i_mode & 07770000 ) + ( (unsigned short) a & 07777 );
      break;
    case I_MODE_FLAGS:
      beep();
      GInode->i_mode = (GInode->i_mode & 07777 ) +
	                 ( ((unsigned short) a & 017 ) * (07777+1) ) ;
      break;
    case I_UID:
      GInode->i_uid = (unsigned short) a;
      break;
    case I_SIZE:
      GInode->i_size = a;
      break;
    case I_ATIME:
      GInode->i_atime = a;
      break;
    case I_CTIME:
      GInode->i_ctime = a;
      break;
    case I_MTIME:
      GInode->i_mtime = a;
      break;
    case I_DTIME:
      GInode->i_dtime = a;
      break;
    case I_GID:
      GInode->i_gid = (unsigned short) a;
      break;
    case I_LINKS_COUNT:
      GInode->i_links_count = (unsigned short) a;
      break;
    case I_BLOCKS:
      GInode->i_blocks = a;
      break;
    case I_FLAGS:
      GInode->i_flags = a;
      break;
    case I_VERSION:
      GInode->i_version = a;
      break;
    case I_FILE_ACL:
      GInode->i_file_acl = a;
      break;
    case I_DIR_ACL:
      GInode->i_dir_acl = a;
      break;
    case I_FADDR:
      GInode->i_faddr = a;
      break;
    case I_FRAG:
      GInode->i_frag = (unsigned char) a;
      break;
    case I_FSIZE:
      GInode->i_fsize = (unsigned short) a;
      break;
    case I_PAD1:
    case I_RESERVED1:
    case I_RESERVED2:
      break;
    case I_ZONE_0:
    case I_ZONE_1:
    case I_ZONE_2:
    case I_ZONE_3:
    case I_ZONE_4:
    case I_ZONE_5:
    case I_ZONE_6:
    case I_ZONE_7:
    case I_ZONE_8:
    case I_ZONE_9:
    case I_ZONE_10:
    case I_ZONE_11:
    case I_ZONE_12:
    case I_ZONE_13:
    case I_ZONE_LAST:
      GInode->i_zone[curr_field-I_ZONE_0] = a;
      break;
    }
}

/* This is the parser for inode_mode: previous/next inode, etc. */
int inode_mode() {
  int c, redraw, full_redraw;
  unsigned long flag;
  long a;
  struct Generic_Inode *GInode = NULL;
  static unsigned char *copy_buffer = NULL;
  int edit_inode, modified, re_read_inode, highlight_field;

#ifdef NCURSES_IS_COOL
  WINDOW *win;
  char cinput[10], *HEX_PTR, *HEX_NOS = "0123456789ABCDEFX\\$";
#endif
  
  highlight_field = I_ZONE_0;

  display_trailer("PG_UP/DOWN = previous/next inode, or '#' to enter inode number",
		  "H for help. Q to quit");

  cdump_inode_labels();
  
  flag = 1; c = ' ';
  modified = edit_inode = 0;
  GInode = FS_cmd.read_inode(current_inode);

  while (flag||(c = mgetch())) {
    flag = full_redraw = re_read_inode = 0;
    redraw = 1;

#ifdef ALPHA_CODE
    if (edit_inode) {
#ifndef NCURSES_IS_COOL
      if ( (c==KEY_ENTER) || (c==CTRL('M')) || (c==CTRL('J')) || (toupper(c)=='E') ) {
	if (cread_num("Enter new value: ",&a)) {
	  set_inode_field(highlight_field, (unsigned long) a, GInode);
	  modified = 1;
	}
	c = ' ';
      }
#else
      HEX_PTR = strchr(HEX_NOS, toupper(c));
      if (HEX_PTR != NULL) {
	ungetch(c);
	echo();
	nodelay(workspace,TRUE);
	wmove(workspace,park_y,park_x);
	wgetnstr(workspace, cinput, 10);
	nodelay(workspace,FALSE);
	noecho();
	set_inode_field(highlight_field, read_num(cinput), GInode);
	c = ' ';
      }
#endif /* NCURSES_IS_COOL -- NOT */
    }
#endif /* ALPHA_CODE */

    switch (c) {
      case CTRL('D'):
      case CTRL('F'):
      case 'l':
      case 'L':
      case META('V'):
      case META('v'):
      case KEY_RIGHT:
      case KEY_NPAGE:
	cwrite_inode(current_inode, GInode, &modified);
	edit_inode = modified = 0;
        current_inode++;
	highlight_field = I_ZONE_0;
	re_read_inode = full_redraw = 1;
	break;
      case CTRL('B'):
      case KEY_BACKSPACE:
      case KEY_DC:
      case KEY_LEFT:
      case 'h':
      case 'H':
      case CTRL('U'):
      case CTRL('V'):
      case KEY_PPAGE:
	cwrite_inode(current_inode, GInode, &modified);
	edit_inode = modified = 0;
	current_inode--;
	highlight_field = I_ZONE_0;
	re_read_inode = full_redraw = 1;
	break;
      case KEY_DOWN:
      case 'j':
      case 'J':
      case CTRL('N'):
      case CTRL('I'):
        while ( (! *(&fsc->inode->i_mode+(++highlight_field))) || (highlight_field >= I_END) )
          if (highlight_field >= (I_END-1) ) highlight_field = I_BEGIN;
	break;
      case KEY_UP:
      case CTRL('P'):
      case 'k':
      case 'K':
      case KEY_BTAB:
        while ( (! *(&fsc->inode->i_mode+(--highlight_field))) || (highlight_field <= I_BEGIN)) 
          if (highlight_field <= (I_BEGIN+1) ) highlight_field = I_END;
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
	if (GInode->i_zone[(unsigned long) (c-'0')]) {
	  current_block = GInode->i_zone[(unsigned long) (c-'0')];
	  return 'b';
	}
	break;
      case 'R':
	cwrite_inode(current_inode, GInode, &modified);
	for (flag=0;(flag<fsc->N_BLOCKS);flag++)
	  fake_inode_zones[flag] = GInode->i_zone[flag];
	return c;
	break;
      case 'B':
	if (GInode->i_zone[highlight_field-I_ZONE_0])
	  current_block = GInode->i_zone[highlight_field-I_ZONE_0];
      case 'b':
      case 'q':
      case 'Q':
      case 'S':
      case 's':
      case 'r':
	cwrite_inode(current_inode, GInode, &modified);
	return c;
	break;
      case 'F':
      case 'f':
	flag_popup();
	break;
      case 'E':
      case 'e':
	edit_inode = 1;
	if (!write_ok) warn("Disk not writeable, change status flags with (F)");
	break;
      case 'D':
      case 'd':
	full_redraw = 1;
	if (S_ISDIR(GInode->i_mode)&&((highlight_field>=I_ZONE_0)&&(highlight_field<=I_ZONE_LAST)))
	  if (GInode->i_zone[highlight_field-I_ZONE_0]) {
	    if (tolower(directory_popup(GInode->i_zone[highlight_field-I_ZONE_0]))=='n') {
	      while (! *(&fsc->inode->i_mode+(++highlight_field)) )
		if (highlight_field >= (I_END-1) ) highlight_field = I_BEGIN;
	      flag = 1;
	      full_redraw = redraw = 0;
	    }
	    re_read_inode = 1;
	  }
	break;
      case 'c':
      case 'C':
	if (!copy_buffer) copy_buffer = malloc(sizeof(struct Generic_Inode));
	memcpy(copy_buffer,GInode,sizeof(struct Generic_Inode));
	warn("Inode (%lu) copied into copy buffer.",current_inode);
	break;
      case 'p':
      case 'P':
	if (copy_buffer) {
	  full_redraw = modified = 1;
	  memcpy(GInode,copy_buffer,sizeof(struct Generic_Inode));
	  if (!write_ok) warn("Turn on write permissions before saving this inode");
	} else {
	  warn("Nothing in copy buffer.");
	}
	break;
      case 'V':
      case 'v':
	c = flag = error_popup();
	break;
      case( CTRL('W')):
	edit_inode = 0;
	cwrite_inode(current_inode, GInode, &modified);
	break;
      case CTRL('A'):
	modified = edit_inode = 0;
	re_read_inode = 1;
	break;
      case '#':
	if (cread_num("Enter inode number (leading 0x or $ indicates hex):",&a)) {
	  current_inode = (unsigned long) a;
	  full_redraw = 1;
	}
	break;
      case 'z':
      case KEY_F(2):
      case CTRL('O'):
	c = flag = do_popup_menu(inode_menu_options, inode_menu_map);	
	if (c == '*') 
	  c = flag = do_popup_menu(edit_menu_options, edit_menu_map);
	break;
      case '?':
      case KEY_F(1):
      case CTRL('H'):
      case META('H'):
        do_scroll_help(inode_help, FANCY);
      case CTRL('L'):
	refresh_all();
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

    if (re_read_inode)
	GInode = FS_cmd.read_inode(current_inode);

    if (full_redraw)
      cdump_inode_labels();

    if (redraw||full_redraw)
      cdump_inode_values(current_inode, GInode, highlight_field);
  }
  return 0;
}
 
