/*
 *  lde/nc_lde.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_lde.h,v 1.12 2001/02/23 23:40:04 scottheavner Exp $
 */
#ifndef LDE_NC_LDE_H
#define LDE_NC_LDE_H

#include "keymap.h"		/* For lde_keymap struct definition */

#define LDE_ESC 27
#define LDE_CTRL(x) ((x)-'A'+1)
#define LDE_META(x) ((x)|1024)
#define IS_LDE_CTRL(x) ( ((x)<32)?1:0 )
#define IS_LDE_META(x) ( (LDE_META(x)==(x))?1:0 )
#define INV_LDE_CTRL(x) ( (x)+'A'-1 )
#define INV_LDE_META(x) ( (x)&255 )

#define IS_LDE_MOUSE(x) ((x)&2048)
#define LDE_MOUSEX(x)  (((x)&0xFF000000)>>24)
#define LDE_MOUSEY(x)  (((x)&0xFF0000)>>16)

#define HELP_NO_BANNER 1
#define HELP_BOXED 2
#define HELP_WIDE 4
#define FANCY HELP_BOXED
#define PLAIN ( HELP_NO_BANNER | HELP_WIDE )

#define WIN_COL 80
#define HEADER_SIZE 2
#define TRAILER_SIZE 1
#define VERT (LINES-HEADER_SIZE-TRAILER_SIZE)
#define HOFF ((COLS-WIN_COL)/2)

#define HELP_KEY_SIZE 12

/* Curses variables */
extern int    WHITE_ON_BLUE, WHITE_ON_RED, RED_ON_BLACK;
extern WINDOW *header, *workspace, *trailer;

extern unsigned long current_inode;
extern unsigned long current_block;
extern unsigned long fake_inode_zones[];

/* nc_lde.c */
extern int nc_mgetch(void);
extern void redraw_win(WINDOW *win);
extern int cquery(char *data_string, char *data_options, char *warn_string);
extern int ncread(char *coutput, unsigned long *a, char **string);
extern void display_trailer(char *line1);
extern void update_header(void);
extern void clobber_window(WINDOW *win);
extern void restore_header(void);
extern void refresh_ht(void);
extern void refresh_all(void);
extern void nc_warn(char *fmt, ...);
extern int error_popup(void);
extern int do_popup_menu(lde_menu menu[], lde_keymap keys[]);
extern void do_scroll_help(char **help_text, int fancy);
extern void do_new_scroll_help(lde_menu help_text[], lde_keymap *kmap, int fancy);
extern char *three_text_keys(int c, lde_keymap *kmap);
extern void show_super(void);
extern void flag_popup(void);
extern void crecover_file(unsigned long inode_zones[], unsigned long filesize);
extern int recover_mode(void);
extern int lookup_key(int c, lde_keymap *kmap);
extern char *check_special(int c);
extern char *text_key(int c, lde_keymap *kmap, int skip);
extern void interactive_main(void);

#endif /* LDE_NC_LDE_H */
