/*
 *  lde/main_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: main_lde.c,v 1.2 1994/03/21 06:00:16 sdh Exp $
 */

#include <unistd.h>
#include <getopt.h>
#include "lde.h"

#ifndef __GNUC__
#error "needs gcc for the bitop-__asm__'s"
#endif

#ifndef __linux__
#define volatile
#endif

char *program_name = "lde";
char *device_name = NULL;
char *text_names[4] = { "no file system detected" , "minix", "xiafs", "ext2fs" };

char *inode_map;
char *zone_map;
char *inode_buffer;
unsigned char *inode_count = NULL;
unsigned char *zone_count = NULL;

void nc_warn();
void tty_warn();

struct sbinfo sb2, *sb = &sb2;
struct fs_constants *fsc = NULL;

int CURR_DEVICE = 0;
int verbose = 0, list = 0;
int write_ok = 0, quiet = 0;

#define USAGE_STRING "[-VvIibBdcCStThH?] /dev/name\n"
#define usage() fatal_error("Usage: %s " USAGE_STRING)

struct _rec_flags rec_flags = 
  { 0 } ;

/* Volatiles to let gcc know that this doesn't return. */
volatile void bye(int i)
{
  if (inode_map) free(inode_map);
  if (zone_map) free(zone_map);
  exit(i);
}

volatile void fatal_error(const char * fmt_string)
{
	fprintf(stderr,fmt_string,program_name,device_name);
	bye(1);
}

int check_root(void)
{
  if (!S_ISDIR(DInode.i_mode(fsc->ROOT_INODE))) {
    warn("root inode isn't a directory");
    return 1;
  }
  return 0;
}
  
#undef ONE_BLOCK
#define ONE_BLOCK 1024
void read_tables(int fs_type)
{
  char super_block_buffer[ONE_BLOCK];

  /* Read and parse super block -- xiafs stuff is in 1st block,
   * 
   * xiafs and any other filesystems which use block 0 may cause
   * autodetect problems, as if a new fs which uses block two for
   * the superblock will leave block 0 alone -- anyhow, sometimes
   * the autodect will return xiafs even though the fs blatently
   * isn't.
   */
  if (ONE_BLOCK != read(CURR_DEVICE, super_block_buffer, ONE_BLOCK))
    die("unable to read super block");
  if ( ((!fs_type)||(fs_type==XIAFS)) && (!XIAFS_test(super_block_buffer)) )
    (void) XIAFS_init(super_block_buffer);
  else { 
    /* Pull in second block for other fs's */
    if (ONE_BLOCK != read(CURR_DEVICE, super_block_buffer, ONE_BLOCK))
      die("unable to read super block");
    if ( ((!fs_type)||(fs_type==MINIX)) && (!MINIX_test(super_block_buffer)) )
      MINIX_init(super_block_buffer);
    else if ( ((!fs_type)||(fs_type==EXT2)) && (!EXT2_test(super_block_buffer)) )
      EXT2_init(super_block_buffer);
    else
      die("bad magic number in super-block");
  }

}
#undef ONE_BLOCK

void long_usage()
{
  printf("This is %s (version %s), Usage %s %s\n",program_name,VERSION,program_name,USAGE_STRING);
  printf("   -i ##:      dump inode number # to stdout (-I all inodes after #) \n");
  printf("   -b ##:      dump block number # to stdout (-B all blocks after #)\n");
  printf("   -d ##:      dump block's data to stdout (binary format)\n");
  printf("   -S string:  search disk for data (questionable)\n");
  printf("   -T type:    search disk for data. type = {gz, tgz, script}\n");
  printf("   -t fstype:  Overide the autodetect. fstype = {minix, xiafs, ext2fs}\n");
  printf("   --help:     output this screen\n");
  printf("   --paranoid: Open the device read only.\n");
  printf("   --quiet:    Turn off warning beeps.\n");
  printf("   --version:  print version information\n");
  printf("   --write:    Allow writes to the device.\n");
}


int main(int argc, char ** argv)
{
  char search_type[10], *search_string = search_type;
  static char gzip_tar_type[] = { 31, 138, 8, 0 };
  static char gzip_type[] = { 31, 139 };
  struct _search_types {
    char *name;
    char *string;
    int length; } search_types[] = {
      { "tgz", gzip_tar_type, 4 },
      { "gz", gzip_type, 2 },
      { "script", "#!/b", 4 }
  };
  
  
  int search_len = 0, fs_type = 0;
  int count,idump_all=0,bdump_all=0;
  int grep_mode = 0, paranoid = 0;
  unsigned int idump=0,bdump=0,i,ddump=0;
  unsigned int search_all=0;
  char c;

  static struct option long_options[] =
    {
      {"version", 0, 0, 'v'},
      {"help", 0, 0, 'h'},
      {"inode", 1, 0, 'i'},
      {"block", 1, 0, 'b'},
      {"all", 0, 0, 'a'},
      {"grep", 0, 0, 'g'},
      {"write", 0, 0, 'w'},
      {"paranoid", 0, 0, 'p'},
      {"safe", 0, 0, 'p'},
      {"quiet", 0, 0, 'q'},
      {0, 0, 0, 0}
    };
      
  warn = tty_warn;

  if (argc && *argv)
    program_name = *argv;
  while (1) {
    int option_index = 0;

    c = getopt_long (argc, argv, "avIi:bB:d:cCgpqS:t:T:whH?",
		     long_options, &option_index);

    if (c == -1)
      break;

    switch(c)
      {
      case 'V': 
      case 'v':
	printf("This is %s (version %s).\n",program_name,VERSION);
	exit(0);
	break;
      case 'a':
	rec_flags.search_all = 1;
	break;
      case 'g':
	grep_mode = 1;
	break;
      case 'I':
	idump_all=1;
      case 'i':
	idump = read_num(optarg);
	break;
      case 'B': bdump_all=1;
      case 'b': 
	bdump = read_num(optarg);
	break;
      case 'd':
	ddump = read_num(optarg);
	break;
      case 'p':
	paranoid = 1;
	break;
      case 'q':
	quiet = 1;
	break;
      case 'S': 
	search_all = 1;
	search_string = optarg;
	break;
      case 't':
	i = 0;
	while (strcmp(text_names[++i],"")) {
	  if (!strcmp(optarg, text_names[i])) {
	    fs_type = i;
	    break;
	  }
	}
	if (!fs_type) {
	  printf("`%s' type not recognized.\n",optarg);
	  i = 0;
	  printf("Supported file systems include: ");
	  while (strcmp(text_names[++i],"")) {
	    printf("%s ",text_names[i]);
	  }
	  printf("\n");
	  exit(0);
	}
	break;
      case 'T':
	search_all = 1;
	i = -1;
	while (strcmp(search_types[++i].name,"")) {
	  if (!strncmp(optarg, search_types[i].name, search_types[i].length)) {
	    search_string = search_types[i].string;
	    search_len = search_types[i].length;
	    break;
	  }
	}
	if (!search_len) {
	  printf("`%s' type not recognized.\n",optarg);
	  i = -1;
	  printf("Supported types include: ");
	  while (strcmp(search_types[++i].name,"")) {
	    printf("%s ",search_types[i].name);
	  }
	  printf("\n");
	  exit(0);
	}
	break;
      case 'w':
	write_ok = 1;
	break;
      case 'h':
      case 'H':
      case '?':
	long_usage();
	exit(0);
	break;
      }
  }

  if ( (optind != argc - 1) || ( !(device_name = argv[optind]) ) ) {
    printf("Illegal device name specified.\n");
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

#ifndef PARANOID
  if (!paranoid)
    CURR_DEVICE = open(device_name,O_RDWR);
  else
#endif
    CURR_DEVICE = open(device_name,O_RDONLY);
  
  if (CURR_DEVICE < 0)
    die("unable to open '%s'");
  for (count=0 ; count<3 ; count++)
    sync();
  read_tables(fs_type);
   if (ddump) {
    verbose=0;
    if (ddump<sb->nzones) 
      ddump_block(ddump);
    bye(0);
  }
  if (bdump||bdump_all) {
    list=1;
    verbose=1;
    if (bdump<sb->nzones) 
      if (bdump_all)
	for (i=bdump;i<sb->nzones;i++) dump_block(i);
      else
	dump_block(bdump);
    else
      printf("Zone %d out of range.\n",bdump);
    bye(0);
  }
  if (idump||idump_all) {
    list=1;
    verbose=1;
    if (idump==0) idump=1;
    if (idump<sb->ninodes) 
      if (idump_all)
	for (i=idump;i<sb->ninodes;i++) dump_inode(i);
      else
      	dump_inode(idump);
    else
      printf("Inode %d out of range.\n",idump);
    bye(0);
  }
  if (grep_mode) {
    parse_grep();
    bye(0);
  }
  if (search_all) {
#ifdef EMERGENCY
    search_fs(search_string, search_len);
    bye(0);
#else
    printf("Search function not implemented, recompile source with -DEMERGENCY\n");
    bye(1);
#endif
  }
#ifdef LDE_CURSES
  warn = nc_warn;
  interactive_main();
#endif
  bye(0);

  return 0; /* Gcc complains without this */
}


