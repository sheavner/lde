/*
 *  lde/main_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: main_lde.c,v 1.9 1995/06/01 06:02:59 sdh Exp $
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/stat.h>

#include "lde.h"
#include "ext2fs.h"
#include "minix.h"
#include "msdos_fs.h"
#include "no_fs.h"
#include "recover.h"
#include "tty_lde.h"
#include "xiafs.h"

#ifdef LDE_CURSES
#  include "ncurses.h"
#  include "nc_lde.h"
#endif

#ifndef volatile
#define volatile
#endif

/* Some internal structures */
struct _main_opts {
  int search_len;
  int search_all;
  int fs_type;
  int idump_all;
  int bdump_all;
  int grep_mode;
  int scrubxiafs;
  unsigned long idump;
  unsigned long bdump;
  unsigned long ddump;
  char *search_string;
};

struct _search_types {
  char *name;
  char *string;
  int length; 
};

/* Initialize some global variables */
char *program_name = "lde";
char *device_name = NULL;
char *text_names[] = { "autodetect", "no file system" , "minix", "xiafs", "ext2fs", "msdos" };

char *inode_map;
char *zone_map;
char *inode_buffer;
unsigned char *inode_count = NULL;
unsigned char *zone_count = NULL;

struct sbinfo sb2, *sb = &sb2;
struct fs_constants *fsc = NULL;

int CURR_DEVICE = 0;
int paranoid = 0, list = 0;
int write_ok = 0, quiet = 0;

struct _rec_flags rec_flags = 
  { 0 } ;


/* Check if device is mounted, return 1 if is mounted else 0 */
static int check_mount(char *device_name)
{
  int fd;
  char *mtab;
  struct stat statbuf;

  fd = open("/etc/mtab",O_RDONLY);
  fstat(fd, &statbuf);

  mtab = malloc(statbuf.st_size+1);
  if (mtab==NULL)
    warn("Out of memory reading /etc/mtab");
  read(fd, mtab, statbuf.st_size);
  close(fd);

  mtab[statbuf.st_size] = 0;
  if (strstr(mtab, device_name)) {
    free(mtab);
    return 1;
  } else {
    free(mtab);
    return 0;
  }
}

int check_root(void)
{
  struct Generic_Inode *GInode;

  GInode = FS_cmd.read_inode(fsc->ROOT_INODE);

  if (!S_ISDIR(GInode->i_mode)) {
    warn("root inode isn't a directory");
    return 1;
  }
  return 0;
}
  
void read_tables(int fs_type)
{
  char *super_block_buffer;
  sb->blocksize = 2048; /* Want to read in two blocks */

  /* Pull in first two blocks from the file system.  Xiafs info is
   * in the first block.  Minix, ext2fs is in second block (to leave
   * room for LILO.
   */
 
  super_block_buffer = cache_read_block(0UL, FORCE_READ);
  warn("FSTYPE = %2d:%20s\n",fs_type,text_names[fs_type]);
  if ( ((fs_type==AUTODETECT)&&(MINIX_test(super_block_buffer))) || (fs_type==MINIX) ) {
    MINIX_init(super_block_buffer);
  } else if ( ((fs_type==AUTODETECT)&&(EXT2_test(super_block_buffer))) || (fs_type==EXT2) ) {
    EXT2_init(super_block_buffer);
  } else if ( ((fs_type==AUTODETECT)&&(XIAFS_test(super_block_buffer))) || (fs_type==XIAFS) ) {
    XIAFS_init(super_block_buffer);
  } else if ( ((fs_type==AUTODETECT)&&(DOS_test(super_block_buffer))) || (fs_type==DOS) ) {
    DOS_init(super_block_buffer);
  } else {
    warn("No file system found on device");
    NOFS_init(super_block_buffer);
  }
}

void die(char *msg)
{
  fprintf(stderr,"%s: %s\n",program_name,msg);
  exit(1);
}

#define USAGE_STRING "[-VvIibBdcCStThH?] /dev/name"
static void usage(void)
{
  fprintf(stderr,"Usage: %s %s\n",program_name,USAGE_STRING);
  exit(1);
}

static void long_usage(void)
{
  printf("This is %s (version %s), Usage %s %s\n",program_name,VERSION,program_name,USAGE_STRING);
  printf("   -i ##:      dump inode number # to stdout (-I all inodes after #) \n");
  printf("   -b ##:      dump block number # to stdout (-B all blocks after #)\n");
  printf("   -d ##:      dump block's data to stdout (binary format)\n");
  printf("   -S string:  search disk for data (questionable)\n");
  printf("   -T type:    search disk for data. type = {gz, tgz, script}\n");
  printf("   -t fstype:  Overide the autodetect. fstype = {no, minix, xiafs, ext2fs, msdos}\n");
  printf("   --help:     output this screen\n");
  printf("   --paranoid: Open the device read only.\n");
  printf("   --quiet:    Turn off warning beeps.\n");
  printf("   --version:  print version information\n");
  printf("   --write:    Allow writes to the device.\n");
}

static void parse_cmdline(int argc, char ** argv, struct _main_opts *opts)
{
  int option_index = 0, i;
  char c;
  static char gzip_tar_type[] = { 31, 138, 8, 0 };
  static char gzip_type[] = { 31, 139 };
  struct _search_types search_types[] = {
      { "tgz", gzip_tar_type, 4 },
      { "gz", gzip_type, 2 },
      { "script", "#!/b", 4 }
  };
  struct option long_options[] =
    {
      {"scrubxiafs", 0, 0, 0},
      {"unscrubxiafs", 0, 0, 0},
      {"version", 0, 0, 'v'},
      {"help", 0, 0, 'h'},
      {"inode", 1, 0, 'i'},
      {"block", 1, 0, 'b'},
      {"all", 0, 0, 'a'},
      {"grep", 0, 0, 'g'},
      {"write", 0, 0, 'w'},
      {"paranoid", 0, 0, 'p'},
      {"read-only", 0, 0, 'p'},
      {"safe", 0, 0, 'p'},
      {"quiet", 0, 0, 'q'},
      {0, 0, 0, 0}
    };


  if (argc && *argv)
    program_name = *argv;

  while (1) {
    option_index = 0;

    c = getopt_long (argc, argv, "avI:i:b:B:d:cCgpqS:t:T:whH?",
		     long_options, &option_index);

    if (c == -1)
      break;

    switch(c)
      {
      case 0: /* Some XIAFS utils of limited usefulness */
	switch (option_index)
	  {
	  case 0:
	     opts->scrubxiafs = 1;
	     break;
	   case 1:
	     opts->scrubxiafs = -1;
	     break;
	   }

      case 'V': /* Display version */
      case 'v':
	warn("This is %s (version %s).\n",program_name,VERSION);
	exit(0);
	break;
      case 'a': /* Search disk space marked in use as well as unused */
	rec_flags.search_all = 1;
	break;
      case 'g': /* Search for an inode which contains the specified block */
	opts->grep_mode = 1;
	break;
      case 'I':
	opts->idump_all=1;
      case 'i': /* dump a formatted inode to stdout */
	opts->idump = read_num(optarg);
	break;
      case 'B': 
	opts->bdump_all=1;
      case 'b': /* dump a block to stdout (Hex/ASCII) */
	opts->bdump = read_num(optarg);
	break;
      case 'd': /* dump a block to stdout -- binary format -- why not use dd?? */
	opts->ddump = read_num(optarg);
	break;
      case 'r':
      case 'p': /* open FS read only */
	paranoid = 1;
	break;
      case 'q': /* no audio -- well nop beeps */
	quiet = 1;
	break;
      case 'S': /* Seatch for a string of data */
	opts->search_all = 1;
	opts->search_string = optarg;
	break;
      case 't': /* Specify the FS on the disk */
	i = NONE;
	while (text_names[i]) {
	  if (!strncmp(optarg, text_names[i], strlen(optarg))) {
	    opts->fs_type = i;
	    break;
	  }
	  i++;
	}
	if (opts->fs_type==AUTODETECT) {
	  warn("`%s' type not recognized.",optarg);
	  i = NONE;
	  printf("Supported file systems include: ");
	  while (text_names[i]) {
	    printf("\"%s\" ",text_names[i]);
	    i++;
	  }
	  printf("\n");
	  exit(0);
	}
	break;
      case 'T': /* Search for a file by type */
	opts->search_all = 1;
	i = -1;
	while (strcmp(search_types[++i].name,"")) {
	  if (!strncmp(optarg, search_types[i].name, search_types[i].length)) {
	    opts->search_string = search_types[i].string;
	    opts->search_len = search_types[i].length;
	    break;
	  }
	}
	if (!opts->search_len) {
	  warn("`%s' type not recognized.",optarg);
	  i = -1;
	  printf("Supported types include: ");
	  while (strcmp(search_types[++i].name,"")) {
	    printf("%s ",search_types[i].name);
	  }
	  printf("\n");
	  exit(0);
	}
	break;
      case 'w': /* Set FS writable */
	write_ok = 1;
	break;
      case 'h': /* HELP */
      case 'H':
      case '?':
	long_usage();
	exit(0);
	break;
      }
  }

  if ( (optind != argc - 1) || ( !(device_name = argv[optind]) ) ) {
    warn("Illegal device name specified.");
    usage ();
  }

  if (optind < argc - 1)
    {
      printf ("Unknown options: ");
      while (optind < argc - 1)
	printf ("%s ", argv[optind++]);
      printf ("\n");
      usage();
    }
}

void main(int argc, char ** argv)
{
  int i;
  char search_string[10];
  
  struct _main_opts main_opts = { 0, 0, AUTODETECT, 0, 0, 0, 0, 0UL, 0UL, 0UL, search_string };

  warn = tty_warn;

  parse_cmdline(argc, argv, &main_opts);

  if (check_mount(device_name)&&!paranoid) warn("DEVICE: %s is mounted, be careful",device_name);

#ifndef PARANOID
  if (!paranoid)
    CURR_DEVICE = open(device_name,O_RDWR);
  else
#endif
    CURR_DEVICE = open(device_name,O_RDONLY);
  
  if (CURR_DEVICE < 0)
    die("unable to open '%s'");
  for (i=0 ; i<3 ; i++)
    sync();

  NOFS_init(NULL);

  if (main_opts.scrubxiafs) {
    XIAFS_scrub(main_opts.scrubxiafs);
    exit(0);
  }

  read_tables(main_opts.fs_type);

  if (main_opts.ddump) {
    if (main_opts.ddump<sb->nzones) 
      ddump_block(main_opts.ddump);
    exit(0);
  } else if (main_opts.bdump||main_opts.bdump_all) {
    list=1;
    if (main_opts.bdump<sb->nzones) 
      if (main_opts.bdump_all)
	for (i=main_opts.bdump;i<sb->nzones;i++) dump_block(i);
      else
	dump_block(main_opts.bdump);
    else
      warn("Zone %d out of range.",main_opts.bdump);
    exit(0);
  } else if (main_opts.idump||main_opts.idump_all) {
    list=1;
    if (main_opts.idump==0) main_opts.idump=1;
    if (main_opts.idump<sb->ninodes) 
      if (main_opts.idump_all)
	for (i=main_opts.idump;i<sb->ninodes;i++) dump_inode(i);
      else
      	dump_inode(main_opts.idump);
    else
      warn("Inode %d out of range.",main_opts.idump);
    exit(0);
  } else if (main_opts.grep_mode) {
    parse_grep();
    exit(0);
  } else if (main_opts.search_all) {
    search_fs(main_opts.search_string, main_opts.search_len);
    exit(0);
  }

#ifdef LDE_CURSES
  warn = nc_warn;
  interactive_main();
#endif

  exit(0);
}
