/*
 *  lde/nc_inode_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994,1995  Scott D. Heavner
 *
 */
#ifndef LDE_NC_INODE_HELP_H
#define LDE_NC_INODE_HELP_H

#include "keymap.h"

#ifndef USE_OLD_HELP_FORMAT
static lde_menu inode_help[] = {
  { CMD_CALL_MENU,"Popup menu of commands"},
  { CMD_BLOCK_MODE, "Enter block mode"},
  { CMD_BLOCK_MODE_MC, "View block under cursor"},
  { CMD_COPY, "Copy inode into copy buffer"},
  { CMD_VIEW_AS_DIR, "View inode as a directory"},
  { CMD_EDIT, "Edit inode"},
  { CMD_FLAG_ADJUST, "Menu of toggle flags"},
  { CMD_HELP, "Calls up this help"},
  { CMD_INODE_MODE_MC, "View inode under cursor"},
  { CMD_PASTE, "Paste inode from copy buffer"},
  { CMD_EXIT_PROG, "Quit"},
  { CMD_RECOVERY_MODE, "Enter recovery mode"},
  { CMD_RECOVERY_MODE_MC,"Enter recovery mode, copy inode block ptrs to recovery list"},
  { CMD_VIEW_SUPER,"View superblock"},
  { CMD_DISPLAY_LOG, "View error/warning log"},
  { CMD_NO_ACTION, "Move cursor (arrows)"},
  { CMD_NEXT_INODE,"View next inode"},
  { CMD_PREV_INODE,"View previous inode"},
  { REC_FILE0, "Add block under cursor to recovery list at position"},
  { CMD_NUMERIC_REF, "Enter inode number and view it"},
  { CMD_ABORT_EDIT, "Abort edit.  Reread inode from disk"},
  { CMD_REFRESH, "Refresh screen"},
  { CMD_WRITE_CHANGES, "Write inode to disk"},
  { CMD_BIN_INODE, "View inode as raw block"},
  { 0, NULL} 
};
#else
static lde_menu inode_help[] = {
  "F2, ^O  : Popup menu of commands",
  "b       : Enter block mode.",
  "B       : View block under cursor.",
  "c       : Copy inode into copy buffer.",
  "d       : View inode as a directory, 'n' to view next block in directory",
  "e       : Edit inode.",
  "f       : Menu of toggle flags",
  "h,?,^H  : Calls up this help.",
  "I       : View inode under cursor.",
  "p       : Paste inode from copy buffer.",
  "q       : Quit.",
  "r       : Enter recovery mode.",
  "R       : Enter recovery mode, copy inode block ptrs to recovery list.",
  "s       : View superblock.",
  "v       : View error/warning log.",
  "UP/DN   : Move cursor (also ^P, ^N)",
  "LT/RT   : View next/previous inode (also PG_UP/DN, ^B, ^F, ^V).",
  "0123... : Add block under cursor to recovery list at position.",
  "#       : Enter inode number and view it.",
  "^A      : Abort edit.  Reread inode from disk.",
  "^L      : Refresh screen.",
  "^W      : Write inode to disk.",
  "M-b     : View inode as raw block.",
  NULL
};
#endif

static lde_menu inode_menu[] = {
  { CMD_CALL_MENU, "MORE EDITING COMMANDS" },
  { CMD_BLOCK_MODE, "Block mode" },
  { CMD_BLOCK_MODE_MC, "Block mode, viewing block under cursor" },
  { CMD_INODE_MODE_MC, "View inode under cursor" },
  { CMD_VIEW_AS_DIR, "View inode as a directory" },
  { CMD_EDIT, "Edit inode"},
  { CMD_HELP, "Help" },
  { CMD_EXIT, "Quit" },
  { CMD_RECOVERY_MODE, "Recovery mode" },
  { CMD_RECOVERY_MODE_MC, "Recovery mode, recover this inode" },
  { CMD_VIEW_SUPER, "View superblock" },
  { CMD_FLAG_ADJUST, "Toggle some flags" },
  { CMD_DISPLAY_LOG, "View error/warning log" },
  { 0, NULL }
};

static lde_menu edit_menu[] = {
  { CMD_ABORT_EDIT, "Abort edit" },
  { CMD_COPY, "Copy inode" },
  { CMD_PASTE, "Paste inode"},
  { CMD_WRITE_CHANGES, "Write changes to disk"},
  { 0, NULL }
};

/* default keymap for directory mode -- are all these really necessary ??? */
static lde_keymap inodemode_keymap[] = {
  { 'q', CMD_EXIT_PROG },
  { 'D', CMD_EXPAND_SUBDIR_MC },
  { '#', CMD_NUMERIC_REF },
  { LDE_CTRL('A'), CMD_ABORT_EDIT },
  { LDE_CTRL('W'), CMD_WRITE_CHANGES },
  { 'p', CMD_PASTE },
  { 'P', CMD_PASTE },
  { 'c', CMD_COPY },
  { 'C', CMD_COPY },
  { 'd', CMD_VIEW_AS_DIR },
  { 'D', CMD_VIEW_AS_DIR },
  { 'e', CMD_EDIT },
  { 'E', CMD_EDIT },
  { LDE_CTRL('M'), CMD_EDIT },
  { LDE_CTRL('J'), CMD_EDIT },
  { 'B', CMD_BLOCK_MODE_MC },
  { 'R', CMD_RECOVERY_MODE_MC },
  { LDE_CTRL('U'), CMD_PREV_INODE },
  { LDE_META('v'), CMD_PREV_INODE },
  { KEY_PPAGE, CMD_PREV_INODE },
  { KEY_NPAGE, CMD_NEXT_INODE },
  { LDE_CTRL('V'), CMD_NEXT_INODE },
  { LDE_CTRL('D'), CMD_NEXT_INODE },
  { 'h', CMD_PREV_FIELD },
  { 'H', CMD_PREV_FIELD },
  { KEY_BTAB, CMD_PREV_FIELD },
  { LDE_CTRL('B'), CMD_PREV_FIELD },
  { KEY_BACKSPACE, CMD_PREV_FIELD },
  { KEY_DC, CMD_PREV_FIELD },
  { KEY_LEFT, CMD_PREV_FIELD },
  { LDE_CTRL('P'), CMD_PREV_FIELD },
  { KEY_UP, CMD_PREV_FIELD },
  { 'K', CMD_PREV_FIELD },
  { 'k', CMD_PREV_FIELD },
  { 'l', CMD_NEXT_FIELD },
  { 'L', CMD_NEXT_FIELD },
  { LDE_CTRL('F'), CMD_NEXT_FIELD },
  { KEY_RIGHT, CMD_NEXT_FIELD },
  { LDE_CTRL('I'), CMD_NEXT_FIELD },
  { LDE_CTRL('N'), CMD_NEXT_FIELD },
  { KEY_DOWN, CMD_NEXT_FIELD },
  { LDE_META('b'), CMD_BIN_INODE },
  { 'J', CMD_NEXT_FIELD },
  { 'j', CMD_NEXT_FIELD },
  { 0, 0 }
};

#endif
