/*
 *  lde/main_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: main_lde.c,v 1.19 1996/10/12 21:10:34 sdh Exp $
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>

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
  int search_off;
  int fs_type;
  int grep_mode;
  int scrubxiafs;
  int dump_all;
  unsigned dump_start;
  unsigned dump_end;
  void (*dumper)(unsigned long nr);
  char *search_string;
};

struct _search_types {
  char *name;
  char *string;
  int length; 
  int offset;
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
volatile struct _lde_flags lde_flags = 
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;


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
    lde_warn("Out of memory reading /etc/mtab");
  read(fd, mtab, statbuf.st_size);
  close(fd);

  mtab[statbuf.st_size] = 0;
  if (strstr(mtab, device_name)) {
    free(mtab);
    lde_flags.mounted = 1;
    return 1;
  } else {
    free(mtab);
    lde_flags.mounted = 0;
    return 0;
  }
}

/* Define a handler for Interrupt signals: Ctrl-C */
static void handle_sigint(void)
{
  lde_flags.quit_now = 1;
}

int check_root(void)
{
  struct Generic_Inode *GInode;

  GInode = FS_cmd.read_inode(fsc->ROOT_INODE);

  if (!S_ISDIR(GInode->i_mode)) {
    lde_warn("root inode isn't a directory");
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
  lde_warn("User requested %s filesystem. Checking device . . .",text_names[fs_type]);
  if ( ((fs_type==AUTODETECT)&&(MINIX_test(super_block_buffer))) || (fs_type==MINIX) ) {
    MINIX_init(super_block_buffer);
  } else if ( ((fs_type==AUTODETECT)&&(EXT2_test(super_block_buffer))) || (fs_type==EXT2) ) {
    EXT2_init(super_block_buffer);
  } else if ( ((fs_type==AUTODETECT)&&(XIAFS_test(super_block_buffer))) || (fs_type==XIAFS) ) {
    XIAFS_init(super_block_buffer);
  } else if ( ((fs_type==AUTODETECT)&&(DOS_test(super_block_buffer))) || (fs_type==DOS) ) {
    DOS_init(super_block_buffer);
  } else {
    lde_warn("No file system found on device");
    NOFS_init(super_block_buffer);
  }
}

void die(char *msg)
{
  fprintf(stderr,"%s: %s\n",program_name,msg);
  exit(1);
}

#define USAGE_STRING "[-VvIibBdcCStTLOhH?] /dev/name"
static void usage(void)
{
  fprintf(stderr,"Usage: %s %s\n",program_name,USAGE_STRING);
  exit(1);
}

static void long_usage(void)
{
  printf("This is %s (version %s), Usage %s %s\n",program_name,VERSION,program_name,USAGE_STRING);
  printf("   -i ##:       Dump inode number # to stdout (-I all inodes after #) \n");
  printf("   -b ##:       Dump block number # to stdout (-B all blocks after #)\n");
  printf("   -N ##:       Number of blocks to dump (using -I or -B option)\n");
  printf("   -d ##:       Dump block's data to stdout (binary format)\n");
  printf("   -S string:   Search disk for data (questionable)\n");
  printf("   -T type:     Search disk for data. type = {gz, tgz, script, or use filename}\n");
  printf("   -L ##:             Search length (when using specified filename)\n");
  printf("   -O ##:             Search offset (when using specified filename)\n");
  printf("   -N ##:             Starting search block (defaults to first data zone)\n");
  printf("   --indirects:       Search filesystem for things that look like indirect blocks.\n");
  printf("   --ilookup:         Lookup inodes for all matches when searching (also use with -b).\n");
  printf("   --recoverable:     Check to see if inode is recoverable (requires --ilookup or -i/-I).\n");
  printf("   -t fstype:   Overide the autodetect. fstype = {no, minix, xiafs, ext2fs, msdos}\n");
  printf("   --help:      Output this screen\n");
  printf("   --paranoid:  Open the device read only.\n");
  printf("   --append:    Always append to recovery file (if file exists).\n");
  printf("   --quiet:     Turn off warning beeps.\n");
  printf("   --version:   Print version information\n");
  printf("   --write:     Allow writes to the device.\n");
}

static void parse_cmdline(int argc, char ** argv, struct _main_opts *opts)
{
  int option_index = 0, i;
  char c;
  static char gzip_tar_type[] = { 31, 138, 8, 0 };
  static char gzip_type[] = { 31, 139 };
  struct _search_types search_types[] = {
      { "tgz", gzip_tar_type, 4, 0 },
      { "gz", gzip_type, 2, 0},
      { "script", "#!/", 3, 0 },
      { "", NULL, 0, 0 }
  };
  struct option long_options[] =
    {
      {"scrubxiafs", 0, 0, 0},
      {"unscrubxiafs", 0, 0, 0},
      {"version", 0, 0, 'v'},
      {"help", 0, 0, 'h'},
      {"inode", 1, 0, 'i'},
      {"block", 1, 0, 'b'},
      {"limit", 1, 0, 'l'},
      {"all", 0, 0, 'a'},
      {"grep", 0, 0, 'g'},
      {"write", 0, 0, 'w'},
      {"paranoid", 0, 0, 'p'},
      {"read-only", 0, 0, 'p'},
      {"safe", 0, 0, 'p'},
      {"quiet", 0, 0, 'q'},
      {"offset",1,0,'O'},
      {"length",1,0,'L'},
      {"indirects",0,0,'!'},
      {"ilookup",0,0,'@'},
      {"recoverable",0,0,'#'},
      {"append",0,0,'%'},
      {0, 0, 0, 0}
    };


  if (argc && *argv)
    program_name = *argv;

  while (1) {
    option_index = 0;

    c = getopt_long (argc, argv, "avI:i:n:N:B:b:D:d:gpqS:t:T:whH?O:L:",
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
	lde_warn("This is %s (version %s).",program_name,VERSION);
	exit(0);
	break;
      case 'a': /* Search disk space marked in use as well as unused */
	lde_flags.search_all = 1;
	break;
      case 'g': /* Search for an inode which contains the specified block */
	opts->grep_mode = 1;
	break;
      case 'I':
	opts->dump_all=1;
      case 'i': /* dump a formatted inode to stdout */
	opts->dump_start = read_num(optarg);
	opts->dumper = dump_inode;
	break;
      case 'B': 
	opts->dump_all=1;
      case 'b': /* dump a block to stdout (Hex/ASCII) */
	opts->dump_start = read_num(optarg);
	opts->dumper = dump_block;
	break;
      case 'n': /* limit number of inodes/blocks dumped */
      case 'N':
	opts->dump_end = read_num(optarg);
	break;
      case 'D': 
	opts->dump_all=1;
      case 'd': /* dump a block to stdout -- binary format -- why not use dd?? */
	opts->dump_start = read_num(optarg);
	opts->dumper = ddump_block;
	break;
      case 'r':
      case 'p': /* open FS read only */
	lde_flags.paranoid = 1;
	break;
      case 'q': /* no audio -- well nop beeps */
	lde_flags.quiet = 1;
	break;
      case 'S': /* Search for a string of data */
	opts->search_string = optarg;
	opts->search_len = strlen(opts->search_string);
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
	  lde_warn("`%s' type not recognized.",optarg);
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
	for (i=0; strcmp(search_types[i].name,""); i++) {
	  if (!strncmp(optarg, search_types[i].name, search_types[i].length)) {
	      opts->search_string = search_types[i].string;
	      opts->search_len = search_types[i].length;
	      opts->search_off = search_types[i].offset;
	      break;
	  }
	}

	if (opts->search_string==NULL) {
	  i = open(optarg,O_RDONLY);
	  if (i) {
	    opts->search_string = malloc(MAX_BLOCK_SIZE);
	    if (read(i,opts->search_string,MAX_BLOCK_SIZE)<=0) {
	      free(opts->search_string);
	      opts->search_string=NULL;
	    }
	    close(i);
	  } else {
	    lde_warn("Can't open search file: %s",opts->search_string);
	  }
	}
	break;
      case 'O': /* Set offset for search string */
	opts->search_off = read_num(optarg);
	break;
      case 'L': /* Set length for search string */
	opts->search_len = read_num(optarg);
        if (opts->search_len>MAX_BLOCK_SIZE) {
	  lde_warn("Search length reset to %d blocks.",MAX_BLOCK_SIZE);
	  opts->search_len = MAX_BLOCK_SIZE;
	}
	break;
      case 'w': /* Set FS writable */
	lde_flags.write_ok = 1;
	break;
      case '!': /* Search for indirect blocks. */
	lde_flags.indirect_search = 1;
	break;
      case '@': /* Lookup inodes on search matches. */
	lde_flags.inode_lookup = 1;
	break;
      case '#': /* Check for recoverablilty on search matches. */
	lde_flags.check_recover = 1;
	break;
      case '%': /* Always append data when recovery file exists */
	lde_flags.always_append = 1;
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
    lde_warn("Illegal device name specified.");
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
  int i, hasdata;
  unsigned long nr, inode_nr;
  char *thispointer;

  struct Generic_Inode *GInode = NULL;

  sigset_t sa_mask;
  struct sigaction intaction = { (void *)handle_sigint, sa_mask, SA_RESTART, NULL };

  struct _main_opts main_opts = { 0, 0, 0, AUTODETECT, 0, 0, 0, 0UL, 1UL, NULL, NULL };

  /* Set things up to handle control-c:  just sets lde_flags.quit_now to 1 */
  sigemptyset(&sa_mask);
  sigaction(SIGINT,&intaction,NULL);

  lde_warn = tty_warn;
  mgetch = tty_mgetch;

  parse_cmdline(argc, argv, &main_opts);

  if (main_opts.dumper) /* Supress warnings when dumping to stdio */
    lde_warn = no_warn;

  if (check_mount(device_name)&&!lde_flags.paranoid)
    lde_warn("Device \"%s\" is mounted, be careful",device_name);

#ifndef PARANOID
  if (!lde_flags.paranoid) {
    CURR_DEVICE = open(device_name,O_RDWR);
    if (CURR_DEVICE < 0) {
      lde_warn("No write access to \"%s\",  attempting to open read-only.",device_name);
      CURR_DEVICE = open(device_name,O_RDONLY);
      lde_flags.write_ok = 0;
    }
  } else
#endif
  {
    lde_warn("Paranoid flag set.  Opening \"%s\" read-only.",device_name);
    CURR_DEVICE = open(device_name,O_RDONLY);
  }
  
  if (CURR_DEVICE < 0) {
    lde_warn("Unable to open '%s'",device_name);
    exit(1);
  }

  for (i=0 ; i<3 ; i++)
    sync();

  NOFS_init(NULL);

  if (main_opts.scrubxiafs) {
    XIAFS_scrub(main_opts.scrubxiafs);
    exit(0);
  }

  read_tables(main_opts.fs_type);

  if (main_opts.dumper) {
    main_opts.dump_end += main_opts.dump_start;
    if (main_opts.dumper==dump_inode) {
      if ((main_opts.dump_start>sb->ninodes)||(main_opts.dump_end>sb->ninodes)||
	  (!main_opts.dump_start)||(!main_opts.dump_end)) {
	tty_warn("Inode out of range:  Start = 0x%lX (%lu), \n\tEnd = 0x%lX (%lu), Min = 0x1, Max = 0x%lX (%lu)",
		 main_opts.dump_start,main_opts.dump_start,
		 main_opts.dump_end,main_opts.dump_end,sb->ninodes,sb->ninodes);
	exit(-1);
      } else if ((main_opts.dump_end==(main_opts.dump_start+1UL))&&(main_opts.dump_all)) {
	main_opts.dump_end = sb->ninodes;
      }

      /* Looks for recoverable inodes */
      if (lde_flags.check_recover) {
	lde_warn = no_warn;  /* Suppress output */
	for (nr=main_opts.dump_start; nr<main_opts.dump_end; nr++) {
	  if (lde_flags.quit_now) {
	    printf("Search aborted at inode 0x%lX\n",nr);
	    exit(0);
	  }
	  if ((!FS_cmd.inode_in_use(nr))||(lde_flags.search_all)) {
	    GInode = FS_cmd.read_inode(nr);
	    /* Make sure there's some data here */
	    hasdata = 0;
	    for (i=0; i<INODE_BLKS ; i++)
	      if (GInode->i_zone[i])
		hasdata = 1;
	    if ((hasdata)&&(check_recover_file( GInode->i_zone ))) {
		printf("Inode 0x%lX recovery possible",nr);
		if (fsc->inode->i_dtime) {
                  thispointer = ctime(&(GInode->i_dtime));
                  thispointer[24] = 0;
		  printf(" (deleted %24s)",thispointer);
                }
		printf("\n");
	    }
	  }
	}
	exit(0);
      }

    } else {
      if ((main_opts.dump_start>sb->nzones)||(main_opts.dump_end>sb->nzones)) {
	tty_warn("Block out of range:  Start = 0x%lX (%lu), \n\tEnd = 0x%lX (%lu), Max = 0x%lX (%lu)",
		 main_opts.dump_start,main_opts.dump_start,main_opts.dump_end,
		 main_opts.dump_end,sb->nzones,sb->nzones);
	exit(-1);
      } else if ((main_opts.dump_end==(main_opts.dump_start+1UL))&&(main_opts.dump_all)) {
	main_opts.dump_end = sb->nzones;
      }

      /* Lookup blocks inode reference and exit */
      if (lde_flags.inode_lookup) {
	for (nr=main_opts.dump_start; nr<main_opts.dump_end; nr++) {
	  if (lde_flags.quit_now) {
	    printf("Search aborted at block 0x%lX\n",nr);
	    exit(0);
	  }
	  printf("Block 0x%lX ",nr);
	  if ( (inode_nr = find_inode(nr, 0UL)) ) {
	    printf("found in inode 0x%lX\n",inode_nr);
	  } else {
	    printf("not found in any %sinode\n",((lde_flags.search_all)?"":"unused ") );
	  }
	}
	exit(0);
      }

    }
    for (i=main_opts.dump_start; i<main_opts.dump_end; i++)
      main_opts.dumper(i);
    exit(0);
  } else if (main_opts.grep_mode) {
    parse_grep();
    exit(0);
  } else if (main_opts.search_string!=NULL) {
    search_fs(main_opts.search_string, main_opts.search_len, main_opts.search_off,main_opts.dump_end);
    exit(0);
  }

#ifdef LDE_CURSES
  lde_warn = nc_warn;
  mgetch = nc_mgetch;
  interactive_main();
#endif

  exit(0);
}
