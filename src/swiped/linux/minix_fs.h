#ifndef _LINUX_MINIX_FS_H
#define _LINUX_MINIX_FS_H

/* FOR LDE, BLOCK_SIZE lifted from <linux/fs.h> */

#define BLOCK_SIZE_BITS 10
#define BLOCK_SIZE (1 << BLOCK_SIZE_BITS)

/* END LDE */

/*
 * The minix filesystem constants/structures
 */

/*
 * Thanks to Kees J Bot for sending me the definitions of the new
 * minix filesystem (aka V2) with bigger inodes and 32-bit block
 * pointers.
 */

#define MINIX_ROOT_INO 1

/* Not the same as the bogus LINK_MAX in <linux/limits.h>. Oh well. */
#define MINIX_LINK_MAX 250
#define MINIX2_LINK_MAX 65530

#define MINIX_I_MAP_SLOTS 8
#define MINIX_Z_MAP_SLOTS 64
#define MINIX_SUPER_MAGIC 0x137F   /* original minix fs */
#define MINIX_SUPER_MAGIC2 0x138F  /* minix fs, 30 char names */
#define MINIX2_SUPER_MAGIC 0x2468  /* minix V2 fs */
#define MINIX2_SUPER_MAGIC2 0x2478 /* minix V2 fs, 30 char names */
#define MINIX_VALID_FS 0x0001      /* Clean fs. */
#define MINIX_ERROR_FS 0x0002      /* fs has errors. */

#define MINIX_INODES_PER_BLOCK ((BLOCK_SIZE) / (sizeof(struct minix_inode)))
#define MINIX2_INODES_PER_BLOCK ((BLOCK_SIZE) / (sizeof(struct minix2_inode)))

#define MINIX_V1 0x0001 /* original minix fs */
#define MINIX_V2 0x0002 /* minix V2 fs */

#define INODE_VERSION(inode) inode->i_sb->u.minix_sb.s_version

/*
 * This is the original minix inode layout on disk.
 * Note the 8-bit gid and atime and ctime.
 */
struct minix_inode
{
  uint16_t i_mode;
  uint16_t i_uid;
  uint32_t i_size;
  uint32_t i_time;
  uint8_t i_gid;
  uint8_t i_nlinks;
  uint16_t i_zone[9];
};

/*
 * The new minix inode has all the time entries, as well as
 * long block numbers and a third indirect block (7+1+1+1
 * instead of 7+1+1). Also, some previously 8-bit values are
 * now 16-bit. The inode is now 64 bytes instead of 32.
 */
struct minix2_inode
{
  uint16_t i_mode;
  uint16_t i_nlinks;
  uint16_t i_uid;
  uint16_t i_gid;
  uint32_t i_size;
  uint32_t i_atime;
  uint32_t i_mtime;
  uint32_t i_ctime;
  uint32_t i_zone[10];
};

/*
 * minix super-block data on disk
 */
struct minix_super_block
{
  uint16_t s_ninodes;
  uint16_t s_nzones;
  uint16_t s_imap_blocks;
  uint16_t s_zmap_blocks;
  uint16_t s_firstdatazone;
  uint16_t s_log_zone_size;
  uint32_t s_max_size;
  uint16_t s_magic;
  uint16_t s_state;
  uint32_t s_zones;
};

struct minix_dir_entry
{
  uint16_t inode;
  char name[0];
};

#ifdef __KERNEL__

extern struct dentry *minix_lookup(struct inode *dir, struct dentry *dentry);
extern int minix_create(struct inode *dir, struct dentry *dentry, int mode);
extern int minix_mkdir(struct inode *dir, struct dentry *dentry, int mode);
extern int minix_rmdir(struct inode *dir, struct dentry *dentry);
extern int minix_unlink(struct inode *dir, struct dentry *dentry);
extern int minix_symlink(struct inode *inode,
  struct dentry *dentry,
  const char *symname);
extern int minix_link(struct dentry *old_dentry,
  struct inode *dir,
  struct dentry *dentry);
extern int minix_mknod(struct inode *dir,
  struct dentry *dentry,
  int mode,
  int rdev);
extern int minix_rename(struct inode *old_dir,
  struct dentry *old_dentry,
  struct inode *new_dir,
  struct dentry *new_dentry);
extern struct inode *minix_new_inode(const struct inode *dir);
extern void minix_free_inode(struct inode *inode);
extern unsigned long minix_count_free_inodes(struct super_block *sb);
extern int minix_new_block(struct super_block *sb);
extern void minix_free_block(struct super_block *sb, int block);
extern unsigned long minix_count_free_blocks(struct super_block *sb);

extern int minix_bmap(struct inode *, int);

extern struct buffer_head *minix_getblk(struct inode *, int, int);
extern struct buffer_head *minix_bread(struct inode *, int, int);

extern void minix_truncate(struct inode *);
extern int init_minix_fs(void);
extern int minix_sync_inode(struct inode *);
extern int minix_sync_file(struct file *, struct dentry *);

extern struct inode_operations minix_file_inode_operations;
extern struct inode_operations minix_dir_inode_operations;
extern struct inode_operations minix_symlink_inode_operations;
extern struct dentry_operations minix_dentry_operations;

#endif /* __KERNEL__ */

#endif
