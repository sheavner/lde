/*
 *  lde/nc_inode_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 */

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

static char *inode_menu_options[] = {
  "MORE EDITING COMMANDS",
  "Block mode",
  "Block mode, viewing block under cursor",
  "View inode under cursor",
  "View inode as a directory",
  "Edit inode",
  "Help",
  "Quit program",
  "Recovery mode",
  "Recovery mode, recover this inode",
  "View superblock",
  "Toggle some flags",
  "View error/warning log",
  NULL
};
static char inode_menu_map[] = {
  '*', 'b', 'B', 'I', 'd', 'e', '?', 'q', 'r', 'R', 's', 'f', 'v'
};

static char *edit_menu_options[] = {
  "Abort edit",
  "Copy inode",
  "Paste inode",
  "Write changes to disk",
  NULL
};
static char edit_menu_map[] = {
  CTRL('A'), 'c', 'p', CTRL('W')
} ;
