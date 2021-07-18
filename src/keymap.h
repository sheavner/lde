/*
 *  lde/keymap.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1995  Scott D. Heavner
 *
 *  $Id: keymap.h,v 1.11 2002/02/14 19:44:51 scottheavner Exp $
 *
 */
#ifndef KEYMAP_H
#define KEYMAP_H

/* The keymaps are of this type */
typedef struct _lde_keymap
{
  int key_code;
  int action_code;
} lde_keymap;

/* The keymaps are of this type */
typedef struct _lde_menu
{
  int action_code;
  char *description;
} lde_menu;

/* Action codes */
enum lde_actions
{
  /* Codes valid in all modes */
  CMD_NO_ACTION = 1, /* Do nothing */
  CMD_ANY_ACTION, /* Do more than nothing (CMD_NO_ACTION can abort rest of loop,
			     * CMD_ANY_ACTION must complete entire loop) */
  CMD_EXIT_PROG,  /* Quit program */
  CMD_EXIT,       /* Exit popup or local mode */
  CMD_REFRESH,    /* Refresh screen */
  CMD_HELP,       /* Call up a help window */
  CMD_NEXT_LINE,  /* Goto next line */
  CMD_PREV_LINE,  /* Goto previous line */
  CMD_NEXT_SCREEN,   /* Goto next screen */
  CMD_PREV_SCREEN,   /* Goto previous screen */
  CMD_EXPAND_SUBDIR, /* Expand the current subdirectory */
  CMD_EDIT_DIR,      /* Popup to edit dir */
  CMD_RESTORE_ENT,   /* Recover Directory entry */
  CMD_COPY_TO_FILE,  /* copy this entry to a file on mounted disk */
  CMD_TEST_ENTRY,    /* Test entry to see if valid. */
  CMD_EXPAND_SUBDIR_MC, /* Expand the current subdirectory, set inode to point to new directory */
  CMD_NEXT_IND_BLOCK, /* Find the next block in the directory/file chain */
  CMD_PREV_IND_BLOCK, /* Find the next block in the directory/file chain */
  CMD_CALL_MENU,      /* Display popup menu with submenus of all 
			     * avaliable commands */
  CMD_NUMERIC_REF, /* Prompt user to enter a number, then display that block or inode */
  CMD_ABORT_EDIT, /* Abort edit in progress, lose changes, revert to original */
  CMD_WRITE_CHANGES, /* Save any changes to disk */
  CMD_DISPLAY_LOG,   /* Display error/warning log */
  CMD_PASTE,         /* Paste from copy buffer */
  CMD_COPY,          /* Copy to copy buffer */
  CMD_EDIT,          /* Edit the current selection */
  CMD_FLAG_ADJUST,   /* Display a menu of adjustable flags */
  CMD_VIEW_AS_DIR,   /* View the current selection as a directory */
  CMD_BLOCK_MODE,    /* Switch to block mode */
  CMD_BLOCK_MODE_MC, /* Switch to block mode, set current block accordingly */
  CMD_SET_CURRENT_INODE, /* Sets this inode to be the current inode */
  CMD_INODE_MODE,        /* Switch to inode mode */
  CMD_INODE_MODE_MC, /* Switch to inode mode, set current inode accordingly */
  CMD_VIEW_SUPER,    /* Switch to viewing the super block */
  CMD_RECOVERY_MODE, /* Switch to recovery mode */
  CMD_RECOVERY_MODE_MC, /* Switch to recovery mode, set to recover current inode */
  CMD_DO_RECOVER,       /* Recover file in fake inode to disk */
  CMD_CLR_RECOVER,      /* Clear all entries in fake inode */
  CMD_CHECK_RECOVER,    /* Check recoverability of fake inode */
  CMD_PREV_INODE,       /* Back up one inode */
  CMD_NEXT_INODE,       /* Forward one inode */
  CMD_PREV_BLOCK,       /* Back up one block */
  CMD_NEXT_BLOCK,       /* Forward one block */
  CMD_PREV_FIELD,       /* Back one field */
  CMD_NEXT_FIELD,       /* Forward one field */
  CMD_TOGGLE_ASCII,     /* Switch between ASCII and HEX editing */
  CMD_ALL_ASCII,        /* Only display ascii information in block mode ? */
  CMD_FIND_STRING,      /* Find this string somewhere on the disk */
  CMD_FIND_INODE,       /* Find an inode which references this block */
  CMD_FIND_INODE_MC, /* Find an inode which references this block and view it */
  CMD_BIN_INODE,     /* View this inode with the block editor, i.e the 
                             * raw block on the disk containing this inode */

  REC_FILE0, /* First block in inode's recovery list */
  REC_FILE1,
  REC_FILE2,
  REC_FILE3,
  REC_FILE4,
  REC_FILE5,
  REC_FILE6,
  REC_FILE7,
  REC_FILE8,
  REC_FILE9,
  REC_FILE10,
  REC_FILE11,
  REC_FILE12,
  REC_FILE13,
  REC_FILE14, /* Last block in inode's recovery list */
  REC_FILE_LAST = REC_FILE14 + 1,
  REC_FILE_SIZE, /* Size of file to recover */

  LAST_DEF_COMMAND
};

extern lde_keymap global_keymap[];

#endif
