/*
 *  lde/lde.h -- The Linux Disk Editor
 *
 *  Copyright (C) 1994  Scott D. Heavner
 *
 *  $Id: lde.h,v 1.16 1996/09/14 02:34:09 sdh Exp $
 */

#ifndef VERSION
#define VERSION "2.3"
#endif

extern char *program_name;
extern char *device_name;

/* main_lde.c */
void die(char *msg);
void read_tables(int fs_type);
int check_root(void);
void (*warn)(char *fmt, ...);
int  (*mgetch)(void);

/* filemode.c  */
void mode_string(unsigned short mode, char *str);

#define MAX_NAME_LEN      30
#define MAX_BLOCK_POINTER 200
#define MAX_BLOCK_SIZE    4096  /* must be at least EXT2_MAX_BLOCK_SIZE or whatever the biggest FS we are using */

#define INODE_BLKS 15 /* EXT2_N_BLOCKS or higher -- can't use EXT2 references after
		       * mulitiple architecture support was added to ext2.
		       */


enum lde_fstypes { AUTODETECT, NONE, MINIX, XIAFS, EXT2, DOS };
extern char *text_names[]; /* defined in main.c */

/*
 * Structure of the super block (mostly from ext2, with name 
 *   name changes and some additions.
 */
struct sbinfo {
	unsigned long  ninodes;	            /* Inodes count */
	unsigned long  nzones;	            /* Blocks count */
	unsigned long  s_r_blocks_count;    /* Reserved blocks count */
	unsigned long  s_free_blocks_count; /* Free blocks count */
	unsigned long  s_free_inodes_count; /* Free inodes count */
	unsigned long  first_data_zone;     /* First Data Block */
	unsigned long  s_log_block_size;    /* Block size */
	long           s_log_frag_size;     /* Fragment size */
	unsigned long  s_blocks_per_group;  /* # Blocks per group */
	unsigned long  s_frags_per_group;   /* # Fragments per group */
	unsigned long  s_inodes_per_group;  /* # Inodes per group */
	unsigned long  s_mtime;		    /* Mount time */
	unsigned long  s_wtime;		    /* Write time */
	unsigned short s_mnt_count;	    /* Mount count */
	short          s_max_mnt_count;	    /* Maximal mount count */
	unsigned short s_magic;		    /* Magic signature */
	unsigned short s_state;		    /* File system state */
	unsigned short s_errors;	    /* Behaviour when detecting errors */
	unsigned short s_pad;
	unsigned long  s_lastcheck;	    /* time of last check */
	unsigned long  s_checkinterval;	    /* max. time between checks */
	unsigned long max_size;
	unsigned long zonesize;
	unsigned long magic;
	unsigned long blocksize;
	unsigned long norm_first_data_zone;     /* First Data Block */
	int namelen;
	int dirsize;
	unsigned long imap_blocks;
	unsigned long zmap_blocks;
	int I_MAP_SLOTS;
	int Z_MAP_SLOTS;
	int INODES_PER_BLOCK;
	unsigned long last_block_size;
};

/* The generic inode, in actuality it is an ext2fs inode,
 * if you wish to add more fields for a new fs, please add
 * them at the end, so that the ext2fs inode remains intact.
 */
struct Generic_Inode {
  unsigned short i_mode;                   /* File mode */
  unsigned short i_uid;                    /* Owner Uid */
  unsigned long  i_size;                   /* Size in bytes */
  unsigned long  i_atime;                  /* Access time */
  unsigned long  i_ctime;                  /* Creation time */
  unsigned long  i_mtime;                  /* Modification time */
  unsigned long  i_dtime;                  /* Deletion Time */
  unsigned short i_gid;                    /* Group Id */
  unsigned short i_links_count;            /* Links count */
  unsigned long  i_blocks;                 /* Blocks count */
  unsigned long  i_flags;                  /* File flags */
  unsigned long  i_reserved1;
  unsigned long  i_zone[INODE_BLKS];       /* Pointers to blocks */
  unsigned long  i_version;                /* File version (for NFS) */
  unsigned long  i_file_acl;               /* File ACL */
  unsigned long  i_dir_acl;                /* Directory ACL */
  unsigned long  i_faddr;                  /* Fragment address */
  unsigned char  i_frag;                   /* Fragment number */
  unsigned char  i_fsize;                  /* Fragment size */
  unsigned short i_pad1;
  unsigned long  i_reserved2[2];
};

/* These are the fields which might someday be recognized by inode
 * mode, if both the file system and inode mode support the field you
 * may define the field to be one, otherwise it should be zero. */
struct inode_fields {
  char i_mode;
  char i_uid;
  char i_size;
  char i_links_count;
  char i_mode_flags;
  char i_gid;
  char i_blocks;
  char i_atime;
  char i_ctime;
  char i_mtime;
  char i_dtime;
  char i_flags;
  char i_reserved1;
  char i_zone[INODE_BLKS];
  char i_version;
  char i_file_acl;
  char i_dir_acl;
  char i_faddr;
  char i_frag;
  char i_fsize;
  char i_pad1;
  char i_reserved2;
};

/* These have to be in the same order as inode_fields */
enum {
  I_BEGIN = -1,
  I_MODE,
  I_UID,
  I_SIZE,
  I_LINKS_COUNT,
  I_MODE_FLAGS,
  I_GID,
  I_BLOCKS,
  I_ATIME,
  I_CTIME,
  I_MTIME,
  I_DTIME,
  I_FLAGS,
  I_RESERVED1,
  I_ZONE_0,
  I_ZONE_1,
  I_ZONE_2,
  I_ZONE_3,
  I_ZONE_4,
  I_ZONE_5,
  I_ZONE_6,
  I_ZONE_7,
  I_ZONE_8,
  I_ZONE_9,
  I_ZONE_10,
  I_ZONE_11,
  I_ZONE_12,
  I_ZONE_13,
  I_ZONE_LAST,
  I_VERSION,
  I_FILE_ACL,
  I_DIR_ACL,
  I_FADDR,
  I_FRAG,
  I_FSIZE,
  I_PAD1,
  I_RESERVED2,
  I_END
};

/* Constants for each defined file system */
struct fs_constants {
  int FS;
  int ROOT_INODE;
  int INODE_SIZE;
  unsigned short N_DIRECT;
  unsigned short INDIRECT;
  unsigned short X2_INDIRECT;
  unsigned short X3_INDIRECT;
  unsigned short N_BLOCKS;
  unsigned long  FIRST_MAP_BLOCK;
  int ZONE_ENTRY_SIZE;
  int INODE_ENTRY_SIZE;
  struct inode_fields * inode;
};

/* File system specific commands */
struct {
  /* Check if inode is marked in use */
  int (*inode_in_use)(unsigned long n);
  /* Check if data zone/block is marked in use */
  int (*zone_in_use)(unsigned long n);
  /* Check if data zone/block is marked in bad -- not implemented in v2.2 yet */
  int (*zone_is_bad)(unsigned long n);
  /* Get dir name and inode number */
  char* (*dir_entry)(int i, void *block_buffer, unsigned long *inode_nr);
  /* Copies the FS specific inode into a generic inode structure */
  struct Generic_Inode* (*read_inode)(unsigned long inode_nr);
  /* Copies the generic inode to a FS specific one, then write it to disk */
  int (*write_inode)(unsigned long inode_nr, struct Generic_Inode *GInode);
  /* Map inode to block containing inode */
  unsigned long (*map_inode)(unsigned long n);
} FS_cmd;

/* Flags */
volatile struct _lde_flags {
  unsigned search_all:      1;
  unsigned quiet:           1;
  unsigned write_ok:        1;
  unsigned paranoid:        1;
  unsigned inode_lookup:    1;
  unsigned indirect_search: 1;
  unsigned quit_now:        1;
  unsigned mounted:         1;
} lde_flags;

extern struct sbinfo *sb;
extern struct fs_constants *fsc;

#define INODE_BLOCKS ( (unsigned long) ((sb->ninodes+((sb->INODES_PER_BLOCK)-1))/(sb->INODES_PER_BLOCK)) )
#define INODE_BUFFER_SIZE ( (unsigned long) ( INODE_BLOCKS * sb->blocksize ) )
#define ZONES_PER_BLOCK (sb->blocksize/fsc->ZONE_ENTRY_SIZE)

/* Pull some maps off the disk into memory */
extern char *inode_map;
extern char *zone_map;
extern char *bad_map;
extern char *inode_buffer;
extern unsigned char *inode_count;
extern unsigned char *zone_count;

/* The current device file descriptor */
extern int CURR_DEVICE;

/* Error logging functionality */
#define ERRORS_SAVED 30
extern char *error_save[ERRORS_SAVED];
extern int current_error;

/* #define block_is_bad(x) (bit(bad_map, (x) - FIRSTBLOCK)) */
/* #define mark_inode(x) (setbit(inode_map,(x)),changed=1) */
/* #define unmark_inode(x) (clrbit(inode_map,(x)),changed=1) */
/* #define mark_zone(x) (setbit(zone_map,(x)-sb->first_dat_zone+1),changed=1) */
/* #define unmark_zone(x) (clrbit(zone_map,(x)-sb->first_data_zone+1),changed=1) */
