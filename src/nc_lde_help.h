/*
 *  lde/nc_lde_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994,1995  Scott D. Heavner
 *
 */

#include "keymap.h"

static char *recover_help[] = {
  "F2, ^O  : Popup menu of commands",
  "b       : Enter block mode.",
  "f       : Menu of toggle flags",
  "h,?,^H  : Calls up this help.",
  "i       : Enter inode mode.",
  "q       : Quit.",
  "r       : Write recover file to disk.",
  "s       : View superblock.",
  "v       : View error/warning log.",
  "^L      : Refresh screen.",
  NULL
};

static lde_menu recover_menu[] = {
  { CMD_BLOCK_MODE_MC, "Block mode" },
  { CMD_HELP, "Help" },
  { CMD_INODE_MODE, "Inode mode" },
  { CMD_EXIT, "Quit" },
  { CMD_FLAG_ADJUST, "Toggle some flags" },
  { CMD_DISPLAY_LOG, "View error/warning log" },
  { CMD_DO_RECOVER, "Write recover file to disk" },
  { 0, NULL }
};

static char *ncmain_help[] = {
  "F2, ^O  : Popup menu of commands",
  "b,B     : Enter block mode.",
  "f       : Menu of toggle flags",
  "h,H,?,^H: Calls up this help.",
  "i,I     : Enter inode mode.",
  "q,Q     : Quit.",
  "r,R     : Enter recovery mode.",
  "v       : View error/warning log.",
  "^L      : Refresh screen.",
  NULL
};

static lde_menu ncmain_menu[] = {
  { CMD_BLOCK_MODE_MC, "Block mode" },
  { CMD_HELP, "Help" },
  { CMD_INODE_MODE, "Inode mode" },
  { CMD_RECOVERY_MODE, "Recover mode" },
  { CMD_EXIT, "Quit" },
  { CMD_FLAG_ADJUST, "Toggle some flags" },
  { CMD_DISPLAY_LOG, "View error/warning log" },
  { 0, NULL }
};
