/*
 *  lde/tty_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: tty_lde.c,v 1.15 1996/10/12 21:13:49 sdh Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#include <linux/fs.h>

#include "lde.h"
#include "tty_lde.h"
#include "swiped.h"

/* We don't need no stinkin' linked list */
char *error_save[ERRORS_SAVED];
int current_error = -1;

/* Stores errors/warnings */
void log_error(char *echo_string)
{
  if ((++current_error)>=ERRORS_SAVED) current_error=0;
  error_save[current_error] = realloc(error_save[current_error],strlen(echo_string));
  strcpy(error_save[current_error], echo_string);
}

/* Immediate printing of warnings and errors to standard out */
void tty_warn(char *fmt, ...)
{
  va_list argp;
  char echo_string[132];

  va_start(argp, fmt);
  vsprintf(echo_string, fmt, argp);
  va_end(argp);

  log_error(echo_string);
  printf("%s\n",echo_string);
}

/* Immediate printing of warnings and errors to standard out */
void no_warn(char *fmt, ...)
{
}

/* No input  available in tty mode */
int tty_mgetch()
{
  return 13;
}

/* This looks up the size of the block, the only block which might have
 * an irregular size is the last block
 */
unsigned long lookup_blocksize(unsigned long nr)
{
  return (((sb->nzones-1)==nr) ? sb->last_block_size : sb->blocksize);
}

/* This reads a long int from the user.  It recognizes indicators as to the 
 * type of input for decimal, hex, or octal.  For example if we wanted to
 * input 15, we could enter as hex (0xf, xf, $f), decimal (15), or octal
 * (\017, 017).  sscanf is probably smart enough to recognize these, but 
 * what the hell.
 */
unsigned long read_num(char *cinput)
{
  unsigned long i;

  if  (strlen(cinput)>0) {
    if ((cinput[0]=='$')||(cinput[0]=='x')||(cinput[0]=='X'))
      sscanf(cinput,"%*1c%lx", &i);
    else if (cinput[0]=='\\')
      sscanf(cinput,"%*1c%lo", &i);
    else if (cinput[0]=='0')
      if ((cinput[1]=='x')||(cinput[1]=='X'))
	sscanf(cinput,"%*2c%lx", &i);
      else
	if (cinput[1]!=0)
	  sscanf(cinput,"%*1c%lo", &i);
	else
	  i = 0;
    else
      sscanf(cinput,"%lu", &i);
    return i;
  }

 return 0; /* This should be a safe number to return if something went wrong */
}

char * cache_read_block (unsigned long block_nr, int force)
{
  static char cache[MAX_BLOCK_SIZE];
  static unsigned long cache_block_nr = -1L; /* set to an outrageous number */
  size_t read_size;
  
  if ((force==FORCE_READ)||(block_nr != cache_block_nr))
    {
      cache_block_nr = block_nr;
      memset(cache,0,sb->blocksize);
      
      read_size = (size_t) lookup_blocksize(block_nr);

      if (lseek (CURR_DEVICE, cache_block_nr * sb->blocksize, SEEK_SET) !=
	  cache_block_nr * sb->blocksize)                          
	lde_warn("Read error: unable to seek to block  in cache_read_block", block_nr);
      else if ( read (CURR_DEVICE, cache, read_size) != read_size)
	  lde_warn("Unable to read full block (%lu) in cache_read_block",block_nr);
    }
  return cache;
}

int write_block(unsigned long block_nr, void *data_buffer)
{
  size_t write_count;

  if (!lde_flags.write_ok) {
    lde_warn("Disk not writable, block (%lu) not written",block_nr);
    return -1;
  }
#ifndef PARANOID
  if (lseek (CURR_DEVICE, block_nr * sb->blocksize, SEEK_SET) !=
      block_nr * sb->blocksize) {
    lde_warn("Write error: unable to seek to block (%lu) in write_block", block_nr);
    return -1;
  } else {
    write_count = (size_t) lookup_blocksize(block_nr);
    if (write (CURR_DEVICE, data_buffer, write_count) != write_count) {
      lde_warn("Write error: unable to write block (%d) in write_block",block_nr);
      return -1;
    }
  }
#endif
  return 0;
}


void ddump_block(unsigned long nr)
{
  int i;
  unsigned char *dind;

  dind = cache_read_block(nr,CACHEABLE);
  for (i=0;i<(int)lookup_blocksize(nr);i++) printf("%c",dind[i]);
}

/* Dumps a full block to STDOUT -- could merge with curses version someday */
void dump_block(unsigned long nr)
{
  int i,j=0;
  unsigned char *dind,	 c;
  size_t blocksize;

  dind = cache_read_block(nr,CACHEABLE);
  blocksize = lookup_blocksize(nr);

  while (j*16<blocksize) {
    printf("\n0x%08lX  ",j*16+nr*sb->blocksize);
    for (i=j*16;i<(j*16+8);i++)
      if (i<blocksize)
	printf("%2.2X ",dind[i]);
      else
	printf("   ");
    printf(": ");
    for (i=(j*16+8);i<(j*16+16);i++)
      if (i<blocksize)
	printf("%2.2X ",dind[i]);
      else
	printf("   ");
    
    printf(" ");
    for (i=(j*16);((i<(j*16+16))&&(i<blocksize));i++) {
      c = dind[i];
      c = ((c>31)&&(c<127)) ? c : '.';
      printf("%c",c);
    }
    j++;
  }

  printf("\n");
}

  
/* Dump inode contents to STDOUT -- could also merge with the curses one someday */
void dump_inode(unsigned long nr)
{
  int j;
  char f_mode[12];
  struct Generic_Inode *GInode;
  struct passwd *NC_PASS;
  struct group *NC_GROUP;

  GInode = FS_cmd.read_inode(nr);

  /* Print inode number and file type */
  printf("\nINODE: %-6lu (0x%8.8lX) TYPE: ",nr,nr);
  printf("%14s",entry_type(GInode->i_mode));

  if (FS_cmd.inode_in_use(nr)) 
    printf("\n");
  else
    printf("(NOT USED)\n");
  
  /* Print it like a directory entry */
  mode_string((unsigned short)GInode->i_mode,f_mode);
  f_mode[10] = 0; /* Junk from canned mode_string */
  printf("%10.10s	 ",f_mode);
  /* printf("%2d ",inode->i_nlinks); */
  if ((NC_PASS = getpwuid(GInode->i_uid))!=NULL)
    printf("%-8s ",NC_PASS->pw_name);
  else
    printf("%-8d ",GInode->i_uid);
  if ((NC_GROUP = getgrgid(GInode->i_gid))!=NULL)
    printf("%-8s ",NC_GROUP->gr_name);
  else
    printf("%-8d ",GInode->i_gid);
  printf("%9ld ",GInode->i_size);
  printf("%24s",ctime(&GInode->i_atime));

  /* Display used blocks */
  j=-1;
  if (GInode->i_zone[0]) {
    printf("BLOCKS= ");
    while ((++j<9)&&(GInode->i_zone[j])) {
      if ((j < fsc->N_DIRECT)&&(j==7)) {
	printf("\n        ");
      } else if ((fsc->INDIRECT)&&(j == fsc->INDIRECT)) {
	printf("\nINDIRECT BLOCK: ");
      } else if ((fsc->X2_INDIRECT)&&(j == fsc->X2_INDIRECT )) {
	printf("\nDOUBLE INDIRECT BLOCK: ");
      } else if ((fsc->X3_INDIRECT)&&(j == fsc->X3_INDIRECT )) {
	printf("\nTRIPLE INDIRECT BLOCK: ");
      }
      printf("0x%8.8lX ",GInode->i_zone[j]);
    }
  }
  
  printf("\n");

}

char *entry_type(unsigned long imode)
{
  if (S_ISREG(imode))
    return "regular file  ";
  else if (S_ISDIR(imode))
    return "directory     ";
  else if (S_ISLNK(imode))
    return "symbolic link ";
  else if (S_ISCHR(imode))
    return "char device   ";
  else if (S_ISBLK(imode))
    return "block device  ";
  else if (S_ISFIFO(imode))
    return "named pipe    ";
  else if (S_ISSOCK(imode))
    return "socket        ";
  return "???           ";
}

