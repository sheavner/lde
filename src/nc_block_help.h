/*
 *  lde/nc_block_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994,1995  Scott D. Heavner
 *
 *  $Id: nc_block_help.h,v 1.8 2001/02/23 23:40:04 scottheavner Exp $
 */
#ifndef LDE_NC_BLOCK_HELP_H
#define LDE_NC_BLOCK_HELP_H

#include "keymap.h"

/* Help for block_mode() */

#define BHELP_BLOCK 1
#define BHELP_INODE 8

static lde_menu block_help[] = {
  { CMD_CALL_MENU,"Popup menu of commands"},
  { CMD_BLOCK_MODE_MC,"" /* This will be filled in at runtime */ },
  { CMD_COPY, "Copy block into copy buffer"},
  { CMD_VIEW_AS_DIR, "View block as a directory"},
  { CMD_EDIT, "Edit block"},
  { CMD_FLAG_ADJUST, "Menu of toggle flags"},
  { CMD_HELP, "Calls up this help"},
  { CMD_INODE_MODE, "Enter inode mode"},
  { CMD_INODE_MODE_MC, NULL /* This will be filled in at runtime */ },
  { CMD_PASTE, "Paste block from copy buffer"},
  { CMD_EXIT_PROG, "Quit"},
  { CMD_RECOVERY_MODE, "Enter recovery mode"},
  { CMD_VIEW_SUPER,"View superblock"},
  { CMD_DISPLAY_LOG, "View error/warning log"},
  { CMD_DO_RECOVER,"Write this block's data to the recovery file"},
  { CMD_NO_ACTION, "Move cursor (arrows)"},
  { CMD_NEXT_BLOCK,"View next block"},
  { CMD_PREV_BLOCK,"View previous block"},
  { CMD_NEXT_IND_BLOCK,"View next block indexed by the current inode"},
  { CMD_PREV_IND_BLOCK,"View previous block indexed by the current inode"},
  { CMD_NEXT_SCREEN,"View next part of current block"},
  { CMD_PREV_SCREEN,"View previous part of current block"},
  { CMD_NO_ACTION, "Add block to recovery list at position (keys at top of screen)"},
  { CMD_NUMERIC_REF, "Enter block number and view it"},
  { CMD_ABORT_EDIT, "Abort edit.  Reread original block from disk"},
  { CMD_REFRESH, "Refresh screen"},
  { CMD_TOGGLE_ASCII, "Toggle hex/ascii edit. (in edit mode)"},
  { CMD_ALL_ASCII, "Supress hex output."},
  { CMD_FIND_STRING,"Search disk for occurances of a string."},
  { CMD_FIND_INODE,"Find an inode which references this block"},
  { CMD_FIND_INODE_MC,"Find an inode which references this block and view it"},
  { CMD_WRITE_CHANGES, "Write changes to disk"},
  { 0, NULL } 
};

/* Popup menu in block_mode() */
static lde_menu block_menu[] = {
  { CMD_CALL_MENU, "MORE EDITING COMMANDS" },
  { CMD_BLOCK_MODE_MC, "View block under cursor" },
  { CMD_VIEW_AS_DIR, "View block as a directory" },
  { CMD_EDIT, "Edit block" },
  { CMD_FIND_STRING, "Search disk for string data" },
  { CMD_FIND_INODE, "Find inode which references this block" },
  { CMD_INODE_MODE, "Inode mode" },
  { CMD_INODE_MODE_MC, "Inode mode, viewing inode under cursor" },
  { CMD_ALL_ASCII, "Supress HEX output" },
  { CMD_HELP, "Help" },
  { CMD_EXIT, "Quit" },
  { CMD_RECOVERY_MODE, "Recovery mode" },
  { CMD_DO_RECOVER, "Append block to recovery file" },
  { CMD_VIEW_SUPER, "View superblock" },
  { CMD_FLAG_ADJUST, "Toggle some flags" },
  { CMD_DISPLAY_LOG, "View error/warning log" },
  { 0, NULL }
};

/* Sub-menu in block_mode() */
static lde_menu edit_menu[] = {
  { CMD_ABORT_EDIT, "Abort edit" },
  { CMD_COPY, "Copy block" },
  { CMD_PASTE, "Paste block"},
  { CMD_TOGGLE_ASCII, "Toggle Hex/ASCII mode" },
  { CMD_WRITE_CHANGES, "Write changes to disk"},
  { 0, NULL }
};

/* default keymap for block mode */
static lde_keymap blockmode_keymap[] = {
  { '#', CMD_NUMERIC_REF },
  { 'q', CMD_EXIT_PROG },
  { 'I', CMD_INODE_MODE_MC },
  { 'B', CMD_BLOCK_MODE_MC },
  { 'd', CMD_VIEW_AS_DIR },
  { 'D', CMD_VIEW_AS_DIR },
  { 'e', CMD_EDIT },
  { 'E', CMD_EDIT },
  { 'c', CMD_COPY },
  { 'C', CMD_COPY },
  { 'p', CMD_PASTE },
  { 'P', CMD_PASTE },
  { 'w', CMD_DO_RECOVER },
  { 'W', CMD_DO_RECOVER },
  { LDE_CTRL('W'), CMD_WRITE_CHANGES },
  { LDE_CTRL('A'), CMD_ABORT_EDIT },
  { LDE_CTRL('V'), CMD_NEXT_SCREEN },
  { LDE_CTRL('D'), CMD_NEXT_SCREEN },
  { KEY_NPAGE, CMD_NEXT_SCREEN },
  { LDE_CTRL('U'), CMD_PREV_SCREEN },
  { LDE_META('v'), CMD_PREV_SCREEN },
  { KEY_PPAGE, CMD_PREV_SCREEN },
  { 'h', CMD_PREV_FIELD },
  { 'H', CMD_PREV_FIELD },
  { LDE_CTRL('B'), CMD_PREV_FIELD },
  { KEY_BACKSPACE, CMD_PREV_FIELD },
  { KEY_DC, CMD_PREV_FIELD },
  { KEY_LEFT, CMD_PREV_FIELD },
  { 'l', CMD_NEXT_FIELD },
  { 'L', CMD_NEXT_FIELD },
  { LDE_CTRL('F'), CMD_NEXT_FIELD },
  { KEY_RIGHT, CMD_NEXT_FIELD },
  { '+', CMD_NEXT_BLOCK },
  { KEY_SRIGHT, CMD_NEXT_BLOCK },
  { '-', CMD_PREV_BLOCK },
  { KEY_SLEFT, CMD_PREV_BLOCK },
  { 'f',CMD_FLAG_ADJUST },
  { 'F',CMD_FLAG_ADJUST },
  { LDE_CTRL('I'),CMD_TOGGLE_ASCII },
  { 'A',CMD_ALL_ASCII },
  { LDE_CTRL('R'),CMD_FIND_INODE },
  { LDE_META('r'),CMD_FIND_INODE_MC },
  { '/',CMD_FIND_STRING },
  { 0, 0 }
};

#endif

