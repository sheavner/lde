/*
 *  lde/nc_lde_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994,1995  Scott D. Heavner
 *
 *  $Id: nc_lde_help.h,v 1.8 1998/06/05 21:07:14 sdh Exp $
 *
 */
#ifndef LDE_NC_LDE_HELP_H
#define LDE_NC_LDE_HELP_H

#include "keymap.h"

static lde_menu recover_help[] = {
  { CMD_CALL_MENU, "Popup menu of commands" } ,
  { CMD_BLOCK_MODE, "Enter block mode." },
  { CMD_FLAG_ADJUST, "Menu of toggle flags"  },
  { CMD_HELP, "Calls up this help." },
  { CMD_INODE_MODE, "Enter inode mode." },
  { CMD_EXIT, "Quit." },
  { CMD_DO_RECOVER, "Write recover file to disk." },
  { CMD_CHECK_RECOVER, "Check if file can be recovered." },
  { CMD_VIEW_SUPER, "View superblock." },
  { CMD_DISPLAY_LOG, "View error/warning log." },
  { CMD_REFRESH, "Refresh screen." },
  { 0, NULL }
};

static  lde_menu ncmain_help[] = {
  { CMD_CALL_MENU,"Popup menu of commands"} ,
  { CMD_BLOCK_MODE, "Enter block mode." },
  { CMD_FLAG_ADJUST, "Menu of toggle flags"} ,
  { CMD_HELP, "Calls up this help"} ,
  { CMD_INODE_MODE, "Enter inode mode"} ,
  { CMD_EXIT_PROG, "Quit"},
  { CMD_RECOVERY_MODE, "Enter recovery mode"},
  { CMD_DISPLAY_LOG, "View error/warning log"},
  { CMD_REFRESH,"Refresh screen."} ,
  { 0, NULL }
};

static lde_menu recover_menu[] = {
  { CMD_BLOCK_MODE,    "Block mode" },
  { CMD_HELP,          "Help" },
  { CMD_INODE_MODE,    "Inode mode" },
  { CMD_EXIT_PROG,     "Quit" },
  { CMD_FLAG_ADJUST,   "Toggle some flags" },
  { CMD_DISPLAY_LOG,   "View error/warning log" },
  { CMD_CHECK_RECOVER, "Check recoverability of file" },
  { CMD_CLR_RECOVER,   "Clear all entries in recovery inode" },
  { CMD_DO_RECOVER,    "Write recover file to disk" },
  { 0, NULL }
};

static lde_menu ncmain_menu[] = {
  { CMD_BLOCK_MODE, "Block mode" },
  { CMD_HELP, "Help" },
  { CMD_INODE_MODE, "Inode mode" },
  { CMD_RECOVERY_MODE, "Recover mode" },
  { CMD_EXIT_PROG, "Quit" },
  { CMD_FLAG_ADJUST, "Toggle some flags" },
  { CMD_DISPLAY_LOG, "View error/warning log" },
  { 0, NULL }
};

/* Keys which should be displayed as text */
static lde_menu special_keys[] = { 
  { KEY_BREAK, "BRK" },
  { KEY_DOWN, "DN" },
  { KEY_UP, "UP" },
  { KEY_LEFT, "LFT" },
  { KEY_RIGHT, "RGT" },
  { KEY_HOME, "HOM" },
  { KEY_BACKSPACE, "BCK" },
  { KEY_F(1), "F1 "},
  { KEY_F(2), "F2 "},
  { KEY_F(3), "F3 "},
  { KEY_F(4), "F4 "},
  { KEY_F(5), "F5 "},
  { KEY_F(6), "F6 "},
  { KEY_F(7), "F7 "},
  { KEY_F(8), "F8 "},
  { KEY_F(9), "F9 "},
  { KEY_F(10), "F10"},
  { KEY_F(11), "F11"},
  { KEY_F(12), "F12"},
  { KEY_NPAGE, "PDN" },
  { KEY_PPAGE, "PUP" },
  { KEY_ENTER, "ENT" },
  { LDE_CTRL('M'), "CR" },
  { LDE_CTRL('J'), "LF" },
  { LDE_CTRL('I'), "TAB" },
  { LDE_CTRL('['), "ESC" },
  { 0, NULL }
};

/* Global keymap, overriden by local mode keymaps */
lde_keymap global_keymap[] = {
  { 'Q', CMD_EXIT_PROG },
  { 'q', CMD_EXIT },
  { LDE_CTRL('L'), CMD_REFRESH },
  { '?', CMD_HELP },
  { KEY_F(1), CMD_HELP },
  { LDE_META('h'), CMD_HELP },
  { KEY_F(2), CMD_CALL_MENU },
  { 'z', CMD_CALL_MENU },
  { LDE_CTRL('O'), CMD_CALL_MENU },
  { 'f', CMD_FLAG_ADJUST },
  { 'F', CMD_FLAG_ADJUST },
  { 'v', CMD_DISPLAY_LOG },
  { 'V', CMD_DISPLAY_LOG },
  { 'b', CMD_BLOCK_MODE },
  { 'B', CMD_BLOCK_MODE },
  { 'i', CMD_INODE_MODE },
  { 'I', CMD_INODE_MODE },
  { 's', CMD_VIEW_SUPER },
  { 'S', CMD_VIEW_SUPER },
  { 'r', CMD_RECOVERY_MODE },
  { 'R', CMD_RECOVERY_MODE },
  { '0', REC_FILE0 },
  { '1', REC_FILE1 },
  { '2', REC_FILE2 },
  { '3', REC_FILE3 },
  { '4', REC_FILE4 },
  { '5', REC_FILE5 },
  { '6', REC_FILE6 },
  { '7', REC_FILE7 },
  { '8', REC_FILE8 },
  { '9', REC_FILE9 },
  { '!', REC_FILE10 },
  { '@', REC_FILE11 },
  { '$', REC_FILE12 },
  { '%', REC_FILE13 },
  { '^', REC_FILE14 },
  { KEY_DOWN, CMD_NEXT_LINE },
  { LDE_CTRL('N'), CMD_NEXT_LINE },
  { 'J', CMD_NEXT_LINE },
  { 'j', CMD_NEXT_LINE },
  { KEY_UP, CMD_PREV_LINE },
  { LDE_CTRL('P'), CMD_PREV_LINE },
  { 'K', CMD_PREV_LINE },
  { 'k', CMD_PREV_LINE },
  { 'N', CMD_NEXT_IND_BLOCK },
  { 'n', CMD_NEXT_IND_BLOCK },
  { 'M', CMD_PREV_IND_BLOCK },
  { 'm', CMD_PREV_IND_BLOCK },
  { 0, 0 }
};


static lde_keymap recover_keymap[] = {
  { 'r', CMD_DO_RECOVER },
  { 'R', CMD_DO_RECOVER },
  { 'u', CMD_CLR_RECOVER },
  { 'U', CMD_CLR_RECOVER },
  { 'n', REC_FILE_SIZE },
  { 'N', REC_FILE_SIZE },
  { 'c', CMD_CHECK_RECOVER },
  { 'C', CMD_CHECK_RECOVER },
  { 0, 0 }
};

static lde_keymap main_keymap[] = {
  { 'q', CMD_EXIT_PROG },
  { 0, 0 }
};

static lde_keymap help_keymap[] = {
  { 0, 0 }
};

#endif


