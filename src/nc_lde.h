/*
 *  lde/nc_lde.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_lde.h,v 1.3 1994/04/24 20:32:30 sdh Exp $
 */

#include "lde.h"

#ifdef NC_HEADER
#include <ncurses.h>
#else
#include <curses.h>
#endif

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
#define TRAILER_SIZE 3
#define VERT (LINES-HEADER_SIZE-TRAILER_SIZE)
#define HOFF ((COLS-WIN_COL)/2)

/* Curses variables */
int    WHITE_ON_BLUE, WHITE_ON_RED, RED_ON_BLACK;
WINDOW *header, *workspace, *trailer;

unsigned long current_inode;
unsigned long current_block;
unsigned long fake_inode_zones[MAX_BLOCK_POINTER];

/* nc_lde.c */
int mgetch(void);
void redraw_win(WINDOW *win);
int cquery(char *data_string, char *data_options, char *warn_string);
int cread_num(char *coutput, long *a);
void display_trailer(char *line1, char *line2);
void update_header(void);
void clobber_window(WINDOW *win);
void restore_header(void);
void refresh_ht(void);
void refresh_all(void);
void nc_warn(char *fmt, ...);
int error_popup(void);
int do_popup_menu(char **menu_choices, char *valid_choices);
void do_scroll_help(char **help_text, int fancy);
void show_super(void);
void flag_popup(void);
void crecover_file(unsigned long inode_zones[]);
int recover_mode(void);
void interactive_main(void);

/* nc_dir.c */
int directory_popup(unsigned long bnr);

/* nc_block.c */
void cdump_block(unsigned long nr, unsigned char *dind, int win_start, int win_size);
void cwrite_block(unsigned long block_nr, void *data_buffer, int *mod_yes);
int block_mode(void);

/* nc_inode.c */
int inode_mode(void);



