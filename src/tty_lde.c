/*
 *  lde/tty_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: tty_lde.c,v 1.6 1994/04/01 09:47:37 sdh Exp $
 */

#include "lde.h"

void tty_warn(char *warn_string)
{
  printf("%s\n",warn_string);
}

/* This reads a long int from the user.  It recognizes indicators as to the 
 * type of input for decimal, hex, or octal.  For example if we wanted to
 * input 15, we could enter as hex (0xf, xf, $f), decimal (15), or octal
 * (\017).  sscanf is probably smart enough to recognize these, but 
 * what the hell.
 */
long read_num(char *cinput)
{
  long i;

  if  (strlen(cinput)>0) {
    if ((cinput[0]=='$')||(cinput[0]=='x'))
      sscanf(cinput,"%*1c%lx", &i);
    else if (cinput[0]=='\\')
      sscanf(cinput,"%*1c%lo", &i);
    else if (cinput[1]=='x')
      sscanf(cinput,"%*2c%lx", &i);
    else
      sscanf(cinput,"%ld", &i);
    return i;
  }

 return 0; /* This should be a safe number to return if something went wrong */
}

char * cache_read_block (unsigned long block_nr, int force)
{
  static char cache[MAX_BLOCK_SIZE];
  static unsigned long cache_block_nr = -1L; /* set to an outrageous number */
  int read_count;
  
  if ((force==FORCE_READ)||(block_nr != cache_block_nr))
    {
      cache_block_nr = block_nr;
      memset(cache,0,sb->blocksize);

      if (lseek (CURR_DEVICE, cache_block_nr * sb->blocksize, SEEK_SET) !=
	       cache_block_nr * sb->blocksize) {                          
	warn("Read error: unable to seek to block in cache_read_block ");
      } else if ( (read_count = read (CURR_DEVICE, cache, sb->blocksize)) != sb->blocksize) {
	if ((sb->nzones != block_nr) && (read_count != sb->last_block_size)) 
	  warn("Unable to read full block in cache_read_block");
      }
    }
  return cache;
}

int write_block (unsigned long block_nr, char *data_buffer)
{
  int write_count;

  if (!write_ok) {
    warn("Disk not writable, block not written");
    return -1;
  }
#ifndef PARANOID
  if (lseek (CURR_DEVICE, block_nr * sb->blocksize, SEEK_SET) !=
      block_nr * sb->blocksize) {
    warn("Write error: unable to seek to block in write_block ");
    return -1;
  } else {
    if (sb->nzones == block_nr) 
      write_count = sb->last_block_size;
    else
      write_count = sb->blocksize;
    if (write (CURR_DEVICE, data_buffer, write_count) != write_count) {
      warn("write failed in write_block");
      return -1;
    }
  }
#endif
  return 0;
}


void ddump_block(nr)
unsigned long nr;
{
  int i;
  unsigned char *dind;

  dind = cache_read_block(nr,CACHEABLE);
  for (i=0;i<BLOCK_SIZE;i++) printf("%c",dind[i]);
}

/* Dumps a full block to STDOUT -- could merge with curses version someday */
void dump_block(nr)
unsigned short nr;
{
  int i,j;
  unsigned char *dind,	 c;

  dind = cache_read_block(nr,CACHEABLE);

  printf("\nDATA FOR BLOCK %d (%#X):\n",nr,nr);

  j = 0;

  while (j*16<BLOCK_SIZE) {
    printf("\n0x%04X = ",j*16);
    for (i=0;i<8;i++)
      printf("%2.2X ",dind[j*16+i]);
    printf(" : ");
    for (i=0;i<8;i++)
      printf("%2.2X ",dind[j*16+i+8]);
    
    printf("   ");
    for (i=0;i<16;i++) {
      c = dind[j*16+i];
      c = ((c>31)&&(c<127)) ? c : '.';
      printf("%c",c);
    }
    j++;
  }

  printf("\n");
}

  
/* Dump inode contents to STDOUT -- could also merge with the curses one someday */
void dump_inode(unsigned int nr)
{
  int j;
  char f_mode[12];
  struct Generic_Inode *GInode;
  struct passwd *NC_PASS;
  struct group *NC_GROUP;

  GInode = FS_cmd.read_inode(nr);

  /* Print inode number and file type */
  printf("\nINODE: %-6d (0x%5.5X) TYPE: ",nr,nr);
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
      printf("0x%7.7lX ",GInode->i_zone[j]);
    }
  }
  
  printf("\n");

}

char *entry_type(unsigned long imode)
{
  if (S_ISREG(imode))
    return "regular file";
  else if (S_ISDIR(imode))
    return "directory";
  else if (S_ISLNK(imode))
    return "symbolic link";
  else if (S_ISCHR(imode))
    return "char device";
  else if (S_ISBLK(imode))
    return "block device";
  else if (S_ISFIFO(imode))
    return "named pipe";
  else if (S_ISSOCK(imode))
    return "socket";
  return "???";
}

