/*
 *  lde/nc_lde_help.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 */

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

static char *recover_menu_options[] = {
  "Block mode",
  "Help",
  "Inode mode",
  "Quit",
  "Toggle some flags",
  "View error/warning log",
    "Write recover file to disk",
  NULL
};
static char recover_menu_map[] = {
  'b', '?', 'i', 'q', 'f', 'v', 'r' 
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

static char *ncmain_menu_options[] = {
  "Block mode",
  "Help",
  "Inode mode",
  "Recover mode",
  "Quit",
  "Toggle some flags",
  "View error/warning log",
  NULL
};
char ncmain_menu_map[] = {
  'b', '?', 'i', 'r', 'q', 'f', 'v' 
};

