/*
 *  lde/nc_block_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 */

/* Help for block_mode() */
static char *block_help[] = {
  "F2, ^O  : Popup menu of commands",
  "",
  "c       : Copy block into copy buffer.",
  "d       : View block as a directory.",
  "e       : Edit block.",
  "f       : Menu of toggle flags",
  "?,^H,F1 : Calls up this help.",
  "i       : Enter inode mode.",
  "",
  "p       : Paste block from copy buffer.",
  "q       : Quit.",
  "r       : Enter recovery mode.",
  "s       : View superblock.",
  "v       : View error/warning log.",
  "w       : Write this block's data to the recovery file.",
  "arrows  : Move cursor (also ^P, ^N, ^F, ^B)",
  "PGUP/DN : View next/previous block (also ^U, ^D)",
  "+/-     : View next/previous part of current block.",
  "0123... : Add current block to recovery list at position.",
  "#       : Enter block number and view it.",
  "^A      : Abort changes, re-read original block from disk.",
  "TAB,^I  : Toggle hex/ascii edit. (in edit mode)",
  "^R      : Find an inode which references this block.",
  "^W      : Write changes to disk.",
  NULL
};

/* Popup menu in block_mode() */
static char *block_options[] = {
  "MORE EDITING COMMANDS",
  "View block under cursor",
  "View block as a directory",
  "Edit block",
  "Find inode which references this block",
  "Inode mode",
  "Inode mode, viewing inode under cursor",
  "Help",
  "Quit program",
  "Recovery mode",
  "View superblock",
  "Toggle some flags",
  "View error/warning log",
  "Write block to recovery file",
  NULL
};
static char block_map[] = {
  '*', 'B', 'd', 'e', CTRL('R'), 'i', 'I', '?', 'q', 'r', 's', 'f', 'v', 'w' 
} ;

/* Sub-menu in block_mode() */
char *edit_options[] = {
  "Abort edit",
  "Copy block",
  "Paste block",
  "Toggle Hex/ASCII mode",
  "Write changes to disk",
  NULL
};
char edit_map[] = {
  CTRL('A'), 'c', 'p', CTRL('I'), CTRL('W')
} ;
