/*
 *  lde/nc_inode.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_inode.c,v 1.26 2003/12/07 01:35:53 scottheavner Exp $
 */

#include <ctype.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_PWD_H
#include <pwd.h>
#endif
#if HAVE_GRP_H
#include <grp.h>
#endif
#include <time.h>

#include "lde.h"
#include "tty_lde.h"
#include "curses.h"
#include "nc_lde.h"
#include "nc_inode.h"
#include "nc_inode_help.h"
#include "nc_dir.h"
#include "keymap.h"
#include "swiped.h"

#define LDE_DUMP_ILABELS 128


static struct { /* Someday I'll clean up dump_inode() to use something like this */
  int sb_entry;
  int row;
  int col;
  char *text;
  char *fmt;
  int park_row;
  int park_col;
} inode_labels[] = { 
  { I_MODE,        2, 0,"TYPE: ","type",2,6 },
  { I_MODE,        3, 0,"MODE: ","mode",3,6 }, 
  { I_LINKS_COUNT, 2,20,"LINKS: ","%3d",2,27 },
  { I_MODE_FLAGS,  3,20,"FLAGS: ","\\%2.2s\n",3,27 },  /* MODE_FLAGS */
  { I_UID,         4, 0,"UID: ","uid",4,5 },
  { I_GID,         4,20,"GID: ","gid",4,25 },
  { I_SIZE,        5, 0,"SIZE: ","%-8ld",5,6 },
  { I_BLOCKS,      5,20,"SIZE(BLKS): ","%-8ld \n",5,32 },
  { I_ATIME,       7, 0,"ACCESS TIME:        ","time25",7,20 },
  { I_CTIME,       8, 0,"CREATION TIME:      ","time25",8,20 },
  { I_MTIME,       9, 0,"MODIFICATION TIME:  ","time25",9,20 },
  { I_DTIME,      10, 0,"DELETION TIME:      ","time25",10,20 },
  { I_ZONE_0,      2,47,"DIRECT BLOCKS=","",0,0 },
  { -1,            0, 0,"","",0,0 }
};

static void cwrite_inode(unsigned long inode_nr, struct Generic_Inode *GInode, int *mod_yes);
static void cdump_inode_labels(void);
static void cdump_inode_values(unsigned long nr, struct Generic_Inode *GInode, int highlight_field);
static void set_inode_field(int curr_field, unsigned long a, struct Generic_Inode *GInode);
static void limit_inode(unsigned long *inode, struct sbinfo *local_sb);
static void parse_edit(WINDOW *workspace, int c, int *modified, struct Generic_Inode *GInode,
		int highlight_field, int park_x, int park_y );

static int park_x = 0, park_y = 0;

/* Dump a modified inode to disk */
static void cwrite_inode(unsigned long inode_nr, struct Generic_Inode *GInode, int *mod_yes)
{
  int c;

#ifndef NO_WRITE_INODE
  if (*mod_yes) {
    while ( (c = cquery("WRITE OUT INODE DATA TO DISK [Y/N]? ","ynfq",
			lde_flags.write_ok?"":"(NOTE: write permission not set on disk, use 'F' to set flags before 'Y')")
	     ) == 'f') {
      flag_popup();
    }

    refresh_all(); /* Have to refresh screen before write or error messages will be lost */
    if (c == 'y')
      FS_cmd.write_inode(inode_nr, GInode);
  }
#else
  lde_warn("Compiled with NO_WRITE_INODE, write is disallowed");
#endif
  
  *mod_yes = 0;
}

    
/* Display current labels */
static void cdump_inode_labels()
{
  int vstart,vsize=17, i;

  /* Do some bounds checking on our window */
  if (VERT>vsize) {
    vstart = ((VERT-vsize)/2+HEADER_SIZE);
  } else {
    vsize  = LINES-HEADER_SIZE-TRAILER_SIZE;
    vstart = HEADER_SIZE;
  }

  clobber_window(workspace); 
  workspace = newwin(vsize,WIN_COL,vstart,HOFF);
  werase(workspace);

  for (i=0;(inode_labels[i].sb_entry>=0);i++)
    if ( (char *)fsc->inode + i )
      if (inode_labels[i].text != NULL)
	mvwprintw(workspace,inode_labels[i].row,inode_labels[i].col,inode_labels[i].text);
 
  /* Special cases */
  if (fsc->INDIRECT)
    mvwprintw(workspace,fsc->INDIRECT+2,47,"INDIRECT BLOCK=");
  if (fsc->X2_INDIRECT)
    mvwprintw(workspace,fsc->X2_INDIRECT+2,47,"2x INDIRECT BLOCK=");
  if (fsc->X3_INDIRECT)
    mvwprintw(workspace,fsc->X3_INDIRECT+2,47,"3x INDIRECT BLOCK=");
}

/* Display current inode */
static void cdump_inode_values(unsigned long nr, struct Generic_Inode *GInode, int highlight_field)
{
  unsigned long imode = 0UL, j = 0UL;
  char f_mode[12];
#if HAVE_GETPWUID
  struct passwd *NC_PASS = NULL;
#endif
#if HAVE_GETGRGID
  struct group *NC_GROUP = NULL;
#endif

  /* update header to show we are in INODE mode */
  strcpy(ldemode,"Inode       ");
  update_header();

  if (highlight_field&LDE_DUMP_ILABELS) {
    cdump_inode_labels();
    highlight_field = highlight_field^LDE_DUMP_ILABELS;
  }

  /* Line 0 looks like a directory entry */
  if (fsc->inode->i_links_count) {
    imode = GInode->i_mode;
    mode_string((unsigned short)imode,f_mode);
    f_mode[10] = 0; /* Junk from canned mode_string */
    mvwprintw(workspace,0,0,"%10s",f_mode);
  }

  if (fsc->inode->i_links_count)
    mvwprintw(workspace,0,11,"%3d",GInode->i_links_count);

  if (fsc->inode->i_uid) {
#if HAVE_GETPWUID
    if ((NC_PASS = getpwuid(GInode->i_uid))!=NULL)
      mvwprintw(workspace,0,15,"%-8s",NC_PASS->pw_name);
    else
#endif
      mvwprintw(workspace,0,15,"%-8d",GInode->i_uid);
  }
  if (fsc->inode->i_gid) {
#if HAVE_GETGRGID
    if ((NC_GROUP = getgrgid(GInode->i_gid))!=NULL)
      mvwprintw(workspace,0,24,"%-8s",NC_GROUP->gr_name);
    else
#endif
      mvwprintw(workspace,0,24,"%-8d",GInode->i_gid);
  }
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
#if HAVE_GETPWUID
    mvwprintw(workspace,4,10,"(%s)",(NC_PASS != NULL) ? NC_PASS->pw_name : "");
#endif
    if (highlight_field == I_UID) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 4;
      park_x = 5;
    }
    mvwprintw(workspace,4,5,"%05d",GInode->i_uid);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_gid) {
#if HAVE_GETGRGID
    mvwprintw(workspace,4,30,"(%s)",(NC_GROUP != NULL) ? NC_GROUP->gr_name : "");
#endif
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
    mvwaddnstr(workspace,7,20,ctime(&GInode->i_atime),25);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_ctime) {
    if (highlight_field == I_CTIME) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 8;
      park_x = 20;
    }
    mvwaddnstr(workspace,8,20,ctime(&GInode->i_ctime),25);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_mtime) {
    if (highlight_field == I_MTIME) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 9;
      park_x = 20;
    }
    mvwaddnstr(workspace,9,20,ctime(&GInode->i_mtime),24);
    wattroff(workspace,WHITE_ON_RED);
  }

  if (fsc->inode->i_dtime) {
    if (highlight_field == I_DTIME) {
      wattron(workspace,WHITE_ON_RED);
      park_y = 10;
      park_x = 20;
    }
    mvwaddnstr(workspace,10,20,ctime(&GInode->i_dtime),25);
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
	mvwprintw(workspace,2+j,65,"0x%8.8lX",GInode->i_zone[j]);
      } else {
	mvwprintw(workspace,2+j,65,"          ");
      }
      wattroff(workspace,WHITE_ON_RED);
    }
  }

  update_header();
  wmove(workspace,park_y,park_x);
  wrefresh(workspace);
}

/* Not terribly ugly, but it does all the type casting we might require */
static void set_inode_field(int curr_field, unsigned long a, struct Generic_Inode *GInode)
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

/* Checks for inode out of range */
void limit_inode(unsigned long *local_inode, struct sbinfo *local_sb)
{
  if (*local_inode > local_sb->ninodes) 
    *local_inode = local_sb->ninodes;
  else if (*local_inode < 1 ) 
    *local_inode = 1;
}


/* Reads a value from the user and stuffs it into the current inode field */
void parse_edit(WINDOW *workspace, int c, int *modified, struct Generic_Inode *GInode,
		int highlight_field, int park_x, int park_y )
{
  unsigned long a;
  int result = 0;
  char *s;

#ifndef NC_FIXED_UNGETCH
  switch(highlight_field) {
    case I_ATIME:
    case I_DTIME:
    case I_MTIME:
    case I_CTIME:
      if ((result = ncread("Enter new time and/or date:",NULL,&s)))
      {
	a = (unsigned long) lde_getdate(s);
	if ( a == -1 )
        {
	  lde_warn( "Bad date - %s", s );
          result = 0;
        }
      }
      break;
    default:
      result = ncread("Enter new value: ",&a,NULL);
      break;
  }

  if (result) {
    set_inode_field(highlight_field, a, GInode);
    *modified = 1;
  }
#else
  WINDOW *win;
  char cinput[10], *HEX_PTR, *HEX_NOS = "0123456789ABCDEFX\\$";
  
  HEX_PTR = strchr(HEX_NOS, toupper(c));
  if (HEX_PTR != NULL) {
    ungetch(c);
    echo();
    nodelay(workspace,TRUE);
    wmove(workspace,park_y,park_x);
    wgetnstr(workspace, cinput, 10);
    nodelay(workspace,FALSE);
    noecho();
    result = 1;
    switch(highlight_field) {
      case I_ATIME:
      case I_DTIME:
      case I_MTIME:
      case I_CTIME:
	a = (unsigned long) lde_getdate(cinput);
        if ( a == -1 )
	{
           result = 0;
	   lde_warn( "Bad date - %s", cinput );
        }
	break;
      default:
	a = read_num(cinput);
    }
    set_inode_field(highlight_field, a, GInode);
    *modified = 1;
  }
#endif /* NC_FIXED_UNGETCH */
  if (!lde_flags.write_ok && result ) lde_warn("Disk not writeable, change status flags with (F)");
}


/* This is the parser for inode_mode: previous/next inode, etc. */
int inode_mode() {
  int c, next_cmd=0, i, modified = 0, highlight_field = I_ZONE_0;
  struct Generic_Inode *GInode = NULL;
  unsigned long a;
  static unsigned char *copy_buffer = NULL;

  display_trailer("F1/? for help.  F2/^O for menu.  Q to quit");

  GInode = FS_cmd.read_inode(current_inode);
  cdump_inode_values(current_inode, GInode, (highlight_field|LDE_DUMP_ILABELS));

  while ( (c=next_cmd)||(c=lookup_key(mgetch(),inodemode_keymap)) ) {
    next_cmd = 0;

    switch (c) {
      case CMD_NEXT_INODE: /* Forward one inode */
	cwrite_inode(current_inode++, GInode, &modified);
	modified = 0;
	limit_inode(&current_inode, sb);
	highlight_field = I_ZONE_0;
	GInode = FS_cmd.read_inode(current_inode);
	cdump_inode_values(current_inode, GInode, (highlight_field|LDE_DUMP_ILABELS));
	break;

      case CMD_PREV_INODE: /* Backward one inode */
	cwrite_inode(current_inode--, GInode, &modified);
	modified = 0;
	limit_inode(&current_inode, sb);
	highlight_field = I_ZONE_0;
	GInode = FS_cmd.read_inode(current_inode);
	cdump_inode_values(current_inode, GInode, (highlight_field|LDE_DUMP_ILABELS));
	break;

      case CMD_NEXT_FIELD: /* Forward one field */
        while ( (! *(&fsc->inode->i_mode+(++highlight_field))) || (highlight_field >= I_END) )
          if (highlight_field >= (I_END-1) ) highlight_field = I_BEGIN;
	break;

      case CMD_PREV_FIELD: /* Back one field */
        while ( (! *(&fsc->inode->i_mode+(--highlight_field))) || (highlight_field <= I_BEGIN)) 
          if (highlight_field <= (I_BEGIN+1) ) highlight_field = I_END;
	break;

      case REC_FILE0: /* Tag block under cursor as block 'n' of recovery file */
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
	if (GInode->i_zone[c-REC_FILE0])
	  fake_inode_zones[c-REC_FILE0] = GInode->i_zone[c-REC_FILE0];
	update_header();
	beep();
	break;

      case CMD_RECOVERY_MODE_MC: /* Goto recovery mode, set to recover all blocks in this inode */
	cwrite_inode(current_inode, GInode, &modified);
	for (i=0;(i<fsc->N_BLOCKS);i++)
	  fake_inode_zones[i] = GInode->i_zone[i];
	fake_inode_zones[INODE_BLKS] = GInode->i_size;
	return CMD_RECOVERY_MODE;
	break;

      case CMD_BLOCK_MODE_MC: /* Go to block mode, examine the block which is currently highlighted */
	if (GInode->i_zone[highlight_field-I_ZONE_0])
	  current_block = GInode->i_zone[highlight_field-I_ZONE_0];
	cwrite_inode(current_inode, GInode, &modified);
	return CMD_BLOCK_MODE;
	break;

      case CMD_EXIT_PROG:
      case CMD_VIEW_SUPER:
      case CMD_RECOVERY_MODE:
      case CMD_BLOCK_MODE: /* Switch to another mode */
	cwrite_inode(current_inode, GInode, &modified);
	return c;
	break;

      case CMD_FLAG_ADJUST: /* Put up a menu of adjustable flags */
	flag_popup();
	break;

      case CMD_EDIT: /* Edit inode */
	parse_edit(workspace, c, &modified, GInode, highlight_field, park_x, park_y );
	break;

      case CMD_VIEW_AS_DIR: /* View inode as a directory */
	if (S_ISDIR(GInode->i_mode)&&((highlight_field>=I_ZONE_0)&&(highlight_field<=I_ZONE_LAST)))
	  if (GInode->i_zone[highlight_field-I_ZONE_0]) {
	    directory_popup(0UL,current_inode,(unsigned long)(highlight_field-I_ZONE_0));
	    GInode = FS_cmd.read_inode(current_inode);
	    cdump_inode_values(current_inode, GInode, (highlight_field|LDE_DUMP_ILABELS));
	  }
	break;

      case CMD_BIN_INODE: /* View raw inode in block mode */
        if (FS_cmd.map_inode) {
	   current_block = FS_cmd.map_inode(current_inode);
	   cwrite_inode(current_inode, GInode, &modified);
	   return CMD_BLOCK_MODE;
        }
	break;

      case CMD_COPY: /* Copy inode to copy buffer */
	if (!copy_buffer) copy_buffer = malloc(sizeof(struct Generic_Inode));
	memcpy(copy_buffer,GInode,sizeof(struct Generic_Inode));
	lde_warn("Inode (%lu) copied into copy buffer.",current_inode);
	break;

      case CMD_PASTE: /* Paste inode from copy buffer */
	if (copy_buffer) {
	  modified = 1;
	  memcpy(GInode,copy_buffer,sizeof(struct Generic_Inode));
	  cdump_inode_values(current_inode, GInode, (highlight_field|LDE_DUMP_ILABELS));
	  if (!lde_flags.write_ok) lde_warn("Turn on write permissions before saving this inode");
	} else {
	  lde_warn("Nothing in copy buffer.");
	}
	break;

      case CMD_DISPLAY_LOG: /* Show error log */
	error_popup();
	break;

      case CMD_WRITE_CHANGES: /* Write out modifications to this inode */
	cwrite_inode(current_inode, GInode, &modified);
	break;

      case CMD_ABORT_EDIT: /* Abort edit, re-read original inode */
	modified = 0;
	GInode = FS_cmd.read_inode(current_inode);
	break;

      case CMD_NUMERIC_REF: /* Go to an inode specified by number */
	if (ncread("Enter inode number (leading 0x or $ indicates hex):",&a,NULL)) {
	  current_inode = a;
	  limit_inode(&current_inode, sb);
	  GInode = FS_cmd.read_inode(current_inode);
	  cdump_inode_values(current_inode, GInode, (highlight_field|LDE_DUMP_ILABELS));
	}
	break;

      case CMD_CALL_MENU: /* Display popup menu, process submenus here */
	next_cmd = do_popup_menu(inode_menu,inodemode_keymap);	
	if (next_cmd == CMD_CALL_MENU) 
	  next_cmd = do_popup_menu(edit_menu,inodemode_keymap);
	break;

      case CMD_HELP: /* Display help */
        do_new_scroll_help(inode_help, inodemode_keymap, FANCY);
	/* Requires a redraw after return, so let it fall through */

      case CMD_REFRESH: /* Refresh screen */
	refresh_all();
	cdump_inode_values(current_inode, GInode, (highlight_field|LDE_DUMP_ILABELS));
	continue;

      default:
	beep();
	break;
    }
    cdump_inode_values(current_inode, GInode, (highlight_field));
  }
  return 0;
}
