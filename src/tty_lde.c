/*
 *  lde/tty_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: tty_lde.c,v 1.3 1994/03/21 06:01:00 sdh Exp $
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

static char cache[MAX_BLOCK_SIZE];
char * cache_read_block (unsigned long block_nr)
{
  static unsigned long cache_block_nr = 0;
  
  if (block_nr != cache_block_nr)
    {
      cache_block_nr = block_nr;
      if (!block_nr)
	memset(cache,0,sb->blocksize);
      else if (lseek (CURR_DEVICE, cache_block_nr * sb->blocksize, SEEK_SET) !=
	       cache_block_nr * sb->blocksize) {                          
	warn("Read error: unable to seek to block in cache_read_block ");
	memset(cache,0,sb->blocksize);
      } else if (read (CURR_DEVICE, cache, sb->blocksize) != sb->blocksize) {
	warn("read failed in cache_read_block");
	memset(cache,0,sb->blocksize);
      }
    }
  return cache;
}

int write_block (unsigned long block_nr, char *data_buffer)
{
#ifndef PARANOID
  if (lseek (CURR_DEVICE, block_nr * sb->blocksize, SEEK_SET) !=
      block_nr * sb->blocksize) {
    warn("Write error: unable to seek to block in write_block ");
    return -1;
  } else if (write (CURR_DEVICE, data_buffer, sb->blocksize) != sb->blocksize) {
    warn("write failed in write_block");
    return -1;
  }
#endif
  return 0;
}


void ddump_block(nr)
unsigned long nr;
{
  int i;
  unsigned char *dind;

  dind = cache_read_block(nr);
  for (i=0;i<BLOCK_SIZE;i++) printf("%c",dind[i]);
}

/* Dumps a full block to STDOUT -- could merge with curses version someday */
void dump_block(nr)
unsigned short nr;
{
  int i,j;
  unsigned char *dind,	 c;

  dind = cache_read_block(nr);

  printf("\nDATA FOR BLOCK %d (%#x):\n",nr,nr);

  j = 0;

  while (j*16<BLOCK_SIZE) {
    printf("\n0x%04x = ",j*16);
    for (i=0;i<8;i++)
      printf("%2.2x ",dind[j*16+i]);
    printf(" : ");
    for (i=0;i<8;i++)
      printf("%2.2x ",dind[j*16+i+8]);
    
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
  unsigned long atime;
  char f_mode[12];
  struct passwd *NC_PASS;
  struct group *NC_GROUP;

  /* Print inode number and file type */
  printf("\nINODE: %-6d (0x%5.5x) TYPE: ",nr,nr);
  printf("%14s",entry_type(DInode.i_mode(nr)));

  if (FS_cmd.inode_in_use(nr)) 
    printf("\n");
  else
    printf("(NOT USED)\n");
  
  /* Print it like a directory entry */
  mode_string((unsigned short)DInode.i_mode(nr),f_mode);
  f_mode[10] = 0; /* Junk from canned mode_string */
  printf("%10.10s	 ",f_mode);
  /* printf("%2d ",inode->i_nlinks); */
  if ((NC_PASS = getpwuid(DInode.i_uid(nr)))!=NULL)
    printf("%-8s ",NC_PASS->pw_name);
  else
    printf("%-8d ",DInode.i_uid(nr));
  if ((NC_GROUP = getgrgid(DInode.i_gid(nr)))!=NULL)
    printf("%-8s ",NC_GROUP->gr_name);
  else
    printf("%-8d ",DInode.i_gid(nr));
  printf("%9ld ",DInode.i_size(nr));
  atime = DInode.i_atime(nr);
  printf("%24s",ctime(&atime));

  /* Display used blocks */
  j=-1;
  if (DInode.i_zone(nr, (unsigned long) 0)) {
    printf("BLOCKS= ");
    while ((++j<9)&&(DInode.i_zone(nr,j))) {
      if ((j < fsc->N_DIRECT)&&(j==7)) {
	printf("\n        ");
      } else if ((fsc->INDIRECT)&&(j == fsc->INDIRECT)) {
	printf("\nINDIRECT BLOCK: ");
      } else if ((fsc->X2_INDIRECT)&&(j == fsc->X2_INDIRECT )) {
	printf("\nDOUBLE INDIRECT BLOCK: ");
      } else if ((fsc->X3_INDIRECT)&&(j == fsc->X3_INDIRECT )) {
	printf("\nTRIPLE INDIRECT BLOCK: ");
      }
      printf("0x%7.7lx ",DInode.i_zone(nr,j));
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

