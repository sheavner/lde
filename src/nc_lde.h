/*
 *  lde/nc_lde.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_lde.h,v 1.6 1995/06/02 14:25:04 sdh Exp $
 */

#include "keymap.h"		/* For lde_keymap struct definition */

#define ESC 27
#define CTRL(x) (x-'A'+1)
#define META(x) (x|1024)

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

/* Curses variables */
int    WHITE_ON_BLUE, WHITE_ON_RED, RED_ON_BLACK;
WINDOW *header, *workspace, *trailer;

unsigned long current_inode;
unsigned long current_block;
unsigned long fake_inode_zones[MAX_BLOCK_POINTER];

/* nc_lde.c */
extern int mgetch(void);
extern void redraw_win(WINDOW *win);
extern int cquery(char *data_string, char *data_options, char *warn_string);
extern int cread_num(char *coutput, unsigned long *a);
extern void display_trailer(char *line1, char *line2);
extern void update_header(void);
extern void clobber_window(WINDOW *win);
extern void restore_header(void);
extern void refresh_ht(void);
extern void refresh_all(void);
extern void nc_warn(char *fmt, ...);
extern int error_popup(void);
extern int do_popup_menu(lde_menu menu[]);
extern void do_scroll_help(char **help_text, int fancy);
extern void show_super(void);
extern void flag_popup(void);
extern void crecover_file(unsigned long inode_zones[]);
extern int recover_mode(void);
extern int lookup_key(int c, lde_keymap *kmap);
extern void interactive_main(void);
extern void dump_scroll(WINDOW *win, int i, int window_offset, int win_col,
		int banner_size, char **banner, char **help_text, int fancy);


