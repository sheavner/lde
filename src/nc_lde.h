/*
 *  lde/nc_lde.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_lde.h,v 1.2 1994/04/04 04:23:03 sdh Exp $
 */

#include "lde.h"

#ifdef NC_HEADER
#include <ncurses.h>
#else
#include <curses.h>
#endif

#define CTRL(x) (x-'A'+1)

#define WIN_COL 80
#define HEADER_SIZE 2
#define TRAILER_SIZE 3
#define VERT (LINES-HEADER_SIZE-TRAILER_SIZE)
#define HOFF ((COLS-WIN_COL)/2)

/* Curses variables */
int    RED_ON_WHITE, BLUE_ON_WHITE, END_HIGHLIGHT, RED_ON_BLACK;
WINDOW *header, *workspace, *trailer;

unsigned long current_inode;
unsigned long current_block;
int highlight_field;
int max_blocks_this_inode;
char echo_string[150];
unsigned long fake_inode_zones[MAX_BLOCK_POINTER];

/* nc_lde.c */
int cquery(char *data_string, char *data_options, char *warn_string);
int cread_num(char *coutput, long *a);
void display_trailer(char *line1, char *line2);
void update_header();
void clobber_window(WINDOW *win);
void restore_header();
void refresh_ht();
void refresh_all();
void nc_warn(char *fmt, ...);
int error_popup();
void do_help();
void show_super();
void flag_popup();
void crecover_file(unsigned long inode_zones[]);
int recover_mode();
void interactive_main();

/* nc_block.c */
int directory_popup();
void do_block_help();
unsigned char *cdump_block();
void cwrite_block();
void highlight_block();
int block_mode();

/* nc_inode.c */
void do_inode_help();
void cdump_inode();
int inode_mode();



