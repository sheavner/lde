/*
 *  lde/nc_block.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: nc_block.h,v 1.3 1998/01/17 17:45:20 sdh Exp $
 */

int block_mode(void);

struct _bm_flags
{
  char edit_block; /* Are we currently editing this block? */
  char ascii_mode; /* Is the cursor on the hex side or the ASCII side? */
  char highlight;  /* Does the highlighted field need to be updated?
		     *  usually a result of a cursor move or data entry */
  char modified;   /* Was this block modified?  If so we better ask
		     * to save it before we move to another block */
  char redraw;     /* Request window redraw */
  char dontwait;   /* Indicates we should not wait for getch() and 
		     * should jump right into the main loop */
};
typedef struct _bm_flags bm_flags;

struct _bm_cursor
{
  int row; /* Cursor row */
  int col; /* Cursor column */
  int sow; /* Window start: offset of start of screen window in data buffer */
  int sob; /* Start of block: offset of start of this block in data buffer */
  int eob; /* End of block:   offset of end of this block in data buffer */
  int eod; /* Data end: size of data buffer */
  int rs;  /* Row size: How big is each row on the screen?  With hex
	     * 16 entries.  All ASCII 80+ entries */
  int wl;  /* Window length: define as VERT */

  int bps; /* Number of blocks we want to buffer to fill a screen */

  unsigned char *data; /* Buffered data for this window */
};
typedef struct _bm_cursor bm_cursor;

struct _bm_irecord
{
  struct Generic_Inode *inode;
  unsigned long inr;
  long increment;
};
typedef struct _bm_irecord bm_irecord;
