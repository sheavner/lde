/*
 *  lde/nc_inode_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994,1995  Scott D. Heavner
 *
 */

#include "keymap.h"

static char *inode_help[] = {
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
  NULL
};

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
