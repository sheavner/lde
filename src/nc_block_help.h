/*
 *  lde/nc_block_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994,1995  Scott D. Heavner
 *
 */

#include "keymap.h"

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
static lde_menu block_menu[] = {
  { CMD_CALL_MENU, "MORE EDITING COMMANDS" },
  { CMD_BLOCK_MODE_MC, "View block under cursor" },
  { CMD_VIEW_AS_DIR, "View block as a directory" },
  { CMD_EDIT, "Edit block" },
  { CMD_FIND_INODE, "Find inode which references this block" },
  { CMD_INODE_MODE, "Inode mode" },
  { CMD_INODE_MODE_MC, "Inode mode, viewing inode under cursor" },
  { CMD_HELP, "Help" },
  { CMD_EXIT, "Quit" },
  { CMD_RECOVERY_MODE, "Recovery mode" },
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
