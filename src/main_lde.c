/*
 *  lde/main_lde.c -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: main_lde.c,v 1.53 2003/12/07 05:55:47 scottheavner Exp $
 */

#include "lde_config.h"

#include <signal.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_IO_H
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>

#if HAVE_IO_H
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include "lde.h"
#include "allfs.h"
#include "recover.h"
#include "tty_lde.h"

#ifdef HAVE_CURSES
#include "nc_lde.h"
#endif

/* Some internal structures */
struct _main_opts
{
  int search_len;
  int search_all;
  int search_off;
  int fs_type;
  int grep_mode;
  int dump_all;
  unsigned long dump_start;
  unsigned long dump_end;
  void (*dumper)(unsigned long nr);
  char *search_string;
  char *recover_file_name;
  int log_to_file;
  int blocksize;
  int superscan;
  int skiptableread;
};

struct _search_types
{
  char *name;
  unsigned char *string;
  int length;
  int offset;
};

/* Initialize some global variables */
char *program_name = "lde";
char *device_name = NULL;
char *inode_map = NULL;
char *zone_map = NULL;
char *bad_map = NULL;
char *inode_buffer = NULL;
unsigned char *inode_count = NULL;
unsigned char *zone_count = NULL;

static struct sbinfo sb2;
struct sbinfo *sb = &sb2;
struct fs_constants *fsc = NULL;
lde_fs_cmd FS_cmd;

char *badblocks_directory = NULL;

int CURR_DEVICE = 0;
volatile struct _lde_flags
  lde_flags = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void (*lde_warn)(char *fmt, ...) = tty_warn;
int (*mgetch)(void) = tty_mgetch;

struct _lde_typedata lde_typedata[] = LDE_ALLTYPES;

/* Check if device is mounted, return 1 if is mounted else 0 */
static int check_mount(char *name)
{
  static const char *MTAB_FILENAME = "/etc/mtab";
  struct stat statbuf;
  int fd = open(MTAB_FILENAME, O_RDONLY | O_BINARY);
  if (-1 != fd && -1 != stat(MTAB_FILENAME, &statbuf)) {
    char *mtab = malloc(statbuf.st_size + 1);
    if (mtab == NULL) {
      lde_warn("Out of memory reading /etc/mtab");
      close(fd);
      exit(-1);
    }
    int bytesRead = read(fd, mtab, statbuf.st_size);
    close(fd);
    if (-1 == bytesRead) {
      lde_warn("Cannot read from /etc/mtab cannot check if mounted, errno=%d [%s]", errno, strerror(errno));
      lde_flags.mounted = 0;
    } else {
      /* Set last character to 0 (we've allocated a space for the 0) */
      mtab[statbuf.st_size] = 0;
      if (strstr(mtab, name))
        lde_flags.mounted = 1;
      else
        lde_flags.mounted = 0;
    }
    free(mtab);
  } else {
    /* lde_warn("Cannot open /etc/mtab cannot check if mounted, errno=%d [%s]", errno, strerror(errno)); */
    lde_flags.mounted = 0;
  }
  return lde_flags.mounted;
}

/* Define a handler for Interrupt signals: Ctrl-C */
static void handle_sigint(int a)
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

void read_tables(int fs_type, unsigned long blocksize)
{
  char super_block_buffer[17 * 2048];
  sb->blocksize = 17 * 2048; /* Want to read in two blocks */

  /* Pull in first two blocks from the file system.  Xiafs info is
   * in the first block.  Minix, ext2fs is in second block (to leave
   * room for LILO.
   */

  nocache_read_block(0UL, super_block_buffer, sb->blocksize);
  lde_warn("User requested %s filesystem. Checking device . . .",
           lde_typedata[fs_type].name);

  if (fs_type == AUTODETECT) {
    for (++fs_type; fs_type < LAST_AUTO_FSTYPE; fs_type++) {
      if (lde_typedata[fs_type].test &&
          lde_typedata[fs_type].test(super_block_buffer, 1)) {
        break;
      }
    }
  }

  if (fs_type < LAST_AUTO_FSTYPE) {
    lde_typedata[fs_type].init(super_block_buffer);
  } else {
    lde_warn("No file system found on device");
    NOFS_init(super_block_buffer, blocksize);
  }
}

void die(char *msg)
{
  fprintf(stderr, "%s: %s\n", program_name, msg);
  exit(1);
}

#define USAGE_STRING "[-vIibBdfcCStTLOhH?] {/dev/name}"
static void usage(void)
{
  fprintf(stderr, "Usage: %s %s\n", program_name, USAGE_STRING);
  exit(1);
}

static void long_usage(void)
{
  fprintf(stderr,
          "This is %s (version %s), Usage %s %s\n",
          program_name,
          LDE_VERSION,
          program_name,
          USAGE_STRING);
  /* 12345678900123456789012345678901234567890123456789012345678901234567890123456789% */
  fprintf(stderr,
          "   -i {number}          Dump inode to stdout (-I all inodes after {number})\n"
          "   -b {number}          Dump block to stdout (-B all blocks after {number})\n"
          "   -N {number}          Number of blocks to dump (using -I or -B option)\n"
          "   -d {number}          Dump block's data to stdout (binary format)\n"
          "   -s {number}          Override blocksize\n"
          "   -S {string}          Search disk for data (questionable usefulness, try grep)\n"
          "   -T {type}            Search disk for data, type={gz, tgz, script, {filename}}\n"
          "   -L {number}          Search length (when using specified filename)\n"
          "   -O {number}          Search offset (when using specified filename)\n"
          "   -N {number}          Starting search block (defaults to first data zone)\n"
          "   --indirects          Search for things that look like indirect blocks\n"
          "   --ilookup            Lookup inodes for all matches when searching\n"
          "                            (also use with -b)\n"
          "   --recoverable        Check to see if inode is recoverable\n"
          "                            (requires --ilookup or -i/-I)\n"
          "   --all                Search entire disk (else just unused portions)\n"
          "   --blanked-indirects  Work around indirects with useless info (Linux 2.0 bug)\n"
          "   -t fstype            Overide the autodetect (-t help lists supported fs's)\n"
          "   --help               Output this screen\n"
          "   --paranoid           Open the device read only\n"
          "   --append             Always append to recovery file (if file exists)\n"
          "   --quiet              Turn off warning beeps\n"
          "   --version            Print version information\n"
          "   --write              Allow writes to the device\n"
          "   --file name          Specify filename to save recovered inodes to\n"
          "   --superscan          Search device for superblocks, may be used with -t\n"
          "   --badblocks dir      Override badblocks using files in dir\n");
  exit(0);
}

static void parse_cmdline(int argc, char **argv, struct _main_opts *opts)
{
  int option_index = 0, i;
  int c;
  static unsigned char gzip_tar_type[4] = { 31, 138, 8, 0 };
  static unsigned char gzip_type[] = { 31, 139 };
  struct _search_types search_types[] = { { "tgz", gzip_tar_type, 4, 0 },
    { "gz", gzip_type, 2, 0 },
    { "script", "#!/", 3, 0 },
    { "", NULL, 0, 0 }
  };
#if HAVE_GETOPT_LONG
  struct option long_options[] = { { "version", 0, 0, 'v' },
    { "help", 0, 0, 'h' },
    { "inode", 1, 0, 'i' },
    { "block", 1, 0, 'b' },
    { "blocksize", 1, 0, 's' },
    { "bs", 1, 0, 's' },
    { "limit", 1, 0, 'l' },
    { "all", 0, 0, 'a' },
    { "grep", 0, 0, 'g' },
    { "write", 0, 0, 'w' },
    { "paranoid", 0, 0, 'p' },
    { "read-only", 0, 0, 'p' },
    { "safe", 0, 0, 'p' },
    { "quiet", 0, 0, 'q' },
    { "offset", 1, 0, 'O' },
    { "length", 1, 0, 'L' },
    { "indirects", 0, 0, 'j' },
    { "blanked-indirects", 0, 0, 'J' },
    { "ilookup", 0, 0, 'k' },
    { "recoverable", 0, 0, 'R' },
    { "append", 0, 0, 'A' },
    { "nosymbolic", 0, 0, 'y' },
    { "file", 1, 0, 'f' },
    { "logtofile", 0, 0, 'F' },
    { "superscan", 0, 0, 'P' },
    { "badblocks", 0, 0, 'X' },
    { 0, 0, 0, 0 }
  };
#endif

  /* if (argc && *argv)
    program_name = *argv; */

  while (1) {
    option_index = 0;

#define ALLOPTS "AaB:b:D:d:Ff:gHhI:i:JjkL:l:N:n:O:PpqrRS:s:T:t:vwX:y?"
#if HAVE_GETOPT_LONG
    c = getopt_long(argc, argv, ALLOPTS, long_options, &option_index);
#else
    c = getopt(argc, argv, ALLOPTS);
#endif

    if (c == -1)
      break;

    switch (c) {

    case 0: /* long options only */
      switch (option_index) {
      case 0:
      case 1:
        break;
      }
      break;

    case 'V': /* Display version */
    case 'v':
      lde_warn("This is %s (version %s).", program_name, LDE_VERSION);
      exit(0);
      break;
    case 'a': /* Search disk space marked in use as well as unused */
      lde_flags.search_all = 1;
      break;
    case 'J': /* Linux 2.0 blanked indirect workaround */
      lde_flags.blanked_indirects = 1;
      break;
    case 'g': /* Search for an inode which contains the specified block */
      opts->grep_mode = 1;
      break;
    case 'I':
      opts->dump_all = 1;
    case 'i': /* dump a formatted inode to stdout */
      opts->dump_start = read_num(optarg);
      opts->dumper = dump_inode;
      break;
    case 'B':
      opts->dump_all = 1;
    case 'b': /* dump a block to stdout (Hex/ASCII) */
      opts->dump_start = read_num(optarg);
      opts->dumper = dump_block;
      break;
    case 'n': /* limit number of inodes/blocks dumped */
    case 'N':
      opts->dump_end = read_num(optarg);
      break;
    case 'D':
      opts->dump_all = 1;
    case 'd': /* dump a block to stdout -- binary format -- why not use dd?? */
      opts->dump_start = read_num(optarg);
      opts->dumper = ddump_block;
      break;
    case 'r':
    case 'p': /* open FS read only */
      lde_flags.paranoid = 1;
      break;
    case 'P': /* Superblock scanner */
      opts->superscan = 1;
      opts->skiptableread = 1;
      break;
    case 'q': /* no audio -- well no beeps */
      lde_flags.quiet = 1;
      break;
    case 's': /* Override blocksize */
      opts->blocksize = read_num(optarg);
      break;
    case 'S': /* Search for a string of data */
      opts->search_string = optarg;
      opts->search_len = strlen(opts->search_string);
      break;
    case 't': /* Specify the FS on the disk */
      i = AUTODETECT;
      while (lde_typedata[i].name) {
        if (!strncmp(optarg, lde_typedata[i].name, strlen(optarg))) {
          opts->fs_type = i;
          break;
        }
        i++;
      }
      if (!lde_typedata[i].name) {
        lde_warn("`%s' type not recognized.", optarg);
        i = AUTODETECT;
        fprintf(stderr, "Supported file systems include: ");
        while (lde_typedata[i].name) {
          fprintf(stderr, "\"%s\" ", lde_typedata[i].name);
          i++;
        }
        fprintf(stderr, "\n");
        exit(0);
      }
      break;
    case 'T': /* Search for a file by type */
      for (i = 0; strcmp(search_types[i].name, ""); i++) {
        if (!strncmp(optarg, search_types[i].name, search_types[i].length)) {
          opts->search_string = search_types[i].string;
          opts->search_len = search_types[i].length;
          opts->search_off = search_types[i].offset;
          break;
        }
      }

      if (opts->search_string == NULL) {
        i = open(optarg, O_RDONLY | O_BINARY);
        if (-1 != i) {
          opts->search_string = malloc(MAX_BLOCK_SIZE);
          if (-1 == read(i, opts->search_string, MAX_BLOCK_SIZE)) {
            free(opts->search_string);
            opts->search_string = NULL;
          }
          close(i);
        } else {
          lde_warn("Can't open search file: %s", opts->search_string);
        }
      }
      break;
    case 'X': /* Where we'll look for data on disk read errors */
      badblocks_directory = optarg;
      break;
    case 'O': /* Set offset for search string */
      opts->search_off = read_num(optarg);
      break;
    case 'F': /* Log all errors/messages to /tmp/ldeerrors */
      lde_flags.logtofile = 1;
      break;
    case 'L': /* Set length for search string */
      opts->search_len = read_num(optarg);
      if (opts->search_len > MAX_BLOCK_SIZE) {
        lde_warn("Search length reset to %d blocks.", MAX_BLOCK_SIZE);
        opts->search_len = MAX_BLOCK_SIZE;
      }
      break;
    case 'w': /* Set FS writable */
      lde_flags.write_ok = 1;
      break;
    case 'j': /* Search for indirect blocks. */
      lde_flags.indirect_search = 1;
      opts->search_string = "";
      opts->search_len = 0;
      break;
    case 'k': /* Lookup inodes on search matches. */
      lde_flags.inode_lookup = 1;
      break;
    case 'R': /* Check for recoverablilty on search matches. */
      lde_flags.check_recover = 1;
      break;
    case 'A': /* Always append data when recovery file exists */
      lde_flags.always_append = 1;
      break;
    case 'y': /* Don't do symbolic uid/gid lookups */
      lde_flags.nosymbolic_guid = 1;
      break;
    case 'f': /* Specify name of recovery file */
      opts->recover_file_name = optarg;
      break;
    case 'h': /* HELP */
    case 'H':
    case '?':
      long_usage();
      break;
    }
  }

  if ((optind != argc - 1) || (!(device_name = argv[optind]))) {
    lde_warn("Illegal device name specified.");
    usage();
  }

  if (optind < argc - 1) {
    printf("Unknown options: ");
    while (optind < argc - 1)
      printf("%s ", argv[optind++]);
    printf("\n");
    usage();
  }
}

int main(int argc, char **argv)
{
  int i, hasdata, fp;
  unsigned long nr, inode_nr;
  volatile long endian = 0L;

  struct Generic_Inode *GInode = NULL;

  struct _main_opts main_opts = {
    0, 0, 0, AUTODETECT, 0, 0, 0UL, 0UL, NULL, NULL, NULL, 0, 0, 0, 0
  };

#if HAVE_SIGACTION
  sigset_t sa_mask;
  struct sigaction intaction;

  /* Set things up to handle control-c:  just sets lde_flags.quit_now to 1 */
  sigemptyset(&sa_mask);
  intaction.sa_handler = handle_sigint;
  intaction.sa_mask = sa_mask;
  intaction.sa_flags = SA_RESTART;
  sigaction(SIGINT, &intaction, NULL);
#endif

#ifdef _MSC_VER
  _tzset();
#endif

  /* quick test for endianness, blindly assuming sizeof(char) != sizeof(long) */
  *(char *)&endian = 1;
  if (endian != 1L) {
    /* lde_warn("Detected big endian system, assuming fs is little endian"); */
    lde_flags.byteswap = 1;
  }

  init_disk_cache();

  parse_cmdline(argc, argv, &main_opts);

  if (check_mount(device_name) && !lde_flags.paranoid)
    lde_warn("Device \"%s\" is mounted, be careful", device_name);

#ifndef PARANOID
  if (!lde_flags.paranoid) {
    CURR_DEVICE = open(device_name, O_RDWR | O_BINARY);
    if (CURR_DEVICE < 0) {
      lde_warn("No write access to \"%s\",  attempting to open read-only.",
               device_name);
      CURR_DEVICE = open(device_name, O_RDONLY | O_BINARY);
      lde_flags.write_ok = 0;
    }
  } else
#endif
  {
    lde_warn("Paranoid flag set.  Opening \"%s\" read-only.", device_name);
    CURR_DEVICE = open(device_name, O_RDONLY | O_BINARY);
  }

  if (CURR_DEVICE < 0) {
    lde_warn("Unable to open '%s'", device_name);
    exit(1);
  }

#if HAVE_SYNC
  for (i = 0; i < 3; i++)
    sync();
#endif

  NOFS_init(NULL, 0);

  if (main_opts.skiptableread == 0) {
    read_tables(main_opts.fs_type, main_opts.blocksize);

    /* Override blocksize, if desired */
  } else if (main_opts.blocksize) {
    sb->blocksize = main_opts.blocksize;
  }

  /* Process requests handled by tty based lde */
  if (main_opts.recover_file_name != NULL) {
    /* Check if file exists, if so, check if append flag is set and open accordingly */
    if (((fp = open(main_opts.recover_file_name, O_RDONLY | O_BINARY)) > 0) &&
        lde_flags.always_append) {
      close(fp);
      fp = open(main_opts.recover_file_name, O_WRONLY | O_APPEND | O_BINARY);
    } else { /* It's ok to create a new file */
      fp =
        open(main_opts.recover_file_name, O_WRONLY | O_CREAT | O_BINARY, 0644);
    }

    /* Make sure we got a valid file number, if so look up inode and recover it, else print warning */
    if (fp > 0) {
      GInode = FS_cmd.read_inode(main_opts.dump_start);
      if (!recover_file(fp, GInode->i_zone, GInode->i_size))
        lde_warn("Recovered data written to '%s'", main_opts.recover_file_name);
      close(fp);
      exit(0);
    } else {
      lde_warn("Cannot open file '%s'", main_opts.recover_file_name);
      exit(-1);
    }
  } else if (main_opts.dumper) {

    /* End comes in here as a count, convert it to an absolute end */
    main_opts.dump_end += main_opts.dump_start;

    /* Check if dump_end was 0 or unspecified:
     * - for -I, -B, -D set to end of device
     * - for all others, set to 1 */
    if (main_opts.dump_end == main_opts.dump_start)
      if (main_opts.dump_all ==
          0) /* For dump_all==1, must set end to max inode or max block */
        main_opts.dump_end = main_opts.dump_start + 1UL;

    /* Asked to dump an inode? */
    if (main_opts.dumper == dump_inode) {
      if ((main_opts.dump_start > sb->ninodes) ||
          (main_opts.dump_end > sb->ninodes) || (!main_opts.dump_start) ||
          (!main_opts.dump_end)) {
        tty_warn(
          "Inode out of range:  Start = 0x%lX (%lu), \n\tEnd = 0x%lX (%lu), Min = 0x1, Max = 0x%lX (%lu)",
          main_opts.dump_start,
          main_opts.dump_start,
          main_opts.dump_end,
          main_opts.dump_end,
          sb->ninodes,
          sb->ninodes);
        exit(-1);
      } else if (main_opts.dump_end ==
                 main_opts.dump_start) { /* must have specified dump_all */
        main_opts.dump_end = sb->ninodes;
      }

      /* Looks for recoverable inodes */
      if (lde_flags.check_recover) {
        lde_warn = no_warn; /* Suppress output */
        for (nr = main_opts.dump_start; nr < main_opts.dump_end; nr++) {
          if (lde_flags.quit_now) {
            fprintf(stderr, "Search aborted at inode 0x%lX\n", nr);
            exit(0);
          }
          if ((!FS_cmd.inode_in_use(nr)) || (lde_flags.search_all)) {
            GInode = FS_cmd.read_inode(nr);
            /* Make sure there's some data here */
            hasdata = 0;
            for (i = 0; i < INODE_BLKS; i++)
              if (GInode->i_zone[i])
                hasdata = 1;
            if ((hasdata) &&
                (check_recover_file(GInode->i_zone, GInode->i_size))) {
              printf("Inode 0x%lX recovery possible", nr);
              if (fsc->inode->i_dtime) {
                printf(" (deleted %24s)", lde_ctime((time_t *)&(GInode->i_dtime)));
              }
              if (fsc->inode->i_size) {
                if (GInode->i_size > 1024 * 1024)
                  printf(" %5.0fMB", (double)GInode->i_size / 1024.0 / 1024.0);
                else if (GInode->i_size > 1024)
                  printf(" %5.0fKB", (double)GInode->i_size / 1024.0);
                else
                  printf(" %5.0fbytes", (double)GInode->i_size);
              }
              printf("\n");
            }
          }
        }
        exit(0);
      } /* if (lde_flags.check_recover) */

    } else {
      if ((main_opts.dump_start > sb->nzones) ||
          (main_opts.dump_end > sb->nzones)) {
        tty_warn(
          "Block out of range:  Start = 0x%lX (%lu), \n\tEnd = 0x%lX (%lu), Max = 0x%lX (%lu)",
          main_opts.dump_start,
          main_opts.dump_start,
          main_opts.dump_end,
          main_opts.dump_end,
          sb->nzones,
          sb->nzones);
        exit(-1);
      } else if (main_opts.dump_end ==
                 main_opts.dump_start) { /* Must have specified dump_all */
        main_opts.dump_end = sb->nzones;
      }

      /* Lookup blocks inode reference and exit */
      if (lde_flags.inode_lookup) {
        for (nr = main_opts.dump_start; nr < main_opts.dump_end; nr++) {
          if (lde_flags.quit_now) {
            fprintf(stderr, "Search aborted at block 0x%lX\n", nr);
            exit(0);
          }
          printf("Block 0x%lX ", nr);
          if ((inode_nr = find_inode(nr, 0UL))) {
            printf("found in inode 0x%lX\n", inode_nr);
          } else {
            printf("not found in any %sinode\n",
                   ((lde_flags.search_all) ? "" : "unused "));
          }
        }
        exit(0);
      }
    }
    for (nr = main_opts.dump_start; nr < main_opts.dump_end; nr++) {
      if (lde_flags.quit_now) {
        fprintf(stderr, "\nDump aborted at 0x%lX\n", nr);
        exit(0);
      }
      main_opts.dumper(nr);
    }
    if (main_opts.dumper != ddump_block)
      printf("\n"); /* Even up the command line for exit */
    exit(0);
  } else if (main_opts.grep_mode) {
    parse_grep();
    exit(0);
  } else if (main_opts.search_string != NULL) {
    search_fs((unsigned char *)main_opts.search_string,
              main_opts.search_len,
              main_opts.search_off,
              main_opts.dump_end);
    exit(0);
  } else if (main_opts.superscan) {
    search_for_superblocks(main_opts.fs_type);
    exit(0);
  }

#ifdef HAVE_CURSES
  interactive_main();
#endif

  exit(0);

  return 0; /* egcs complains about void main() */
}
