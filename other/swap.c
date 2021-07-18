/* msdos_info - v1.0 94/11/05 - Scott Heavner */

/* This file will print out the information contained in the
 * boot record of any msdos filesystem.  It will also print
 * out the minimum number of sectors to save if you are going to 
 * share a swap partition between windows and linux.
 *
 * Sharing swaps:
 * ***** Linux *****
 * in your rc  (This flat out clobbers the windows swap file)
 *    # mount swap partition specified in /etc/fstab
 *    /etc/swapoff -a >/dev/null 2>&1
 *    /etc/mkswap /dev/swap 41020
 *    /etc/swapon -a
 * 
 * shutdown might contain something like:
 *    #!/bin/sh
 *    swapoff -a
 *    zcat /var/win_swap_header.gz | dd of=/dev/swap
 *    rm -f /dos/windows/spart.par
 *    sync
 *    # reboot -f
 *    if [ .$1 = . ]; then
 *            TIME=now
 *    fi
 *    exec /etc/shutdown -f -r ${TIME} $*
 *
 * If you don't mount your DOS partitions under Linux,
 * you can delete the spart.par file from dos (it is 
 * set read only by windows, so you might need some other
 * program to get at it).
 *
 */

/* Compile with "gcc msdos_info.c -i msdos_info" */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linux/msdos_fs.h>

/* Align values in boot block, converts char[2] values 
 * (defined in msdos_fs.h) to integer
 */
int cvt_c2(char cp[2])
{
  int i = 0;
  memcpy(&i, cp, 2);
  return i;
}

main(int argc, char **argv)
{
  struct msdos_boot_sector boot;
  char *device_name, *program_name, string[9];
  int fd;
  unsigned long total_sect;

  if (argc > 1) {
    program_name = argv[0];
    device_name = argv[1];
  } else {
    printf("Please enter swap device on command line: i.e. \"%s /dev/swap\"\n",
      argv[0]);
    exit(-1);
  }

  fd = open(device_name, O_RDWR);

  if ((!fd) || (read(fd, &boot, sizeof(struct msdos_boot_sector)) !=
                 sizeof(struct msdos_boot_sector))) {
    printf("Error reading from %s\n", device_name);
    exit(-2);
  }

  printf("Summary for device \"%s\", system name \"%s\"\n",
    device_name,
    strncpy(string, boot.system_id, 8));
  printf("Bytes per sector:            %10d\n", cvt_c2(boot.sector_size));
  printf("Sectors per cluster:         %10d\n", boot.cluster_size);
  printf("FAT copies:                  %10d\n", boot.fats);
  printf("Sectors per FAT:             %10d\n", boot.fat_length);
  printf("Reserved sectors:            %10d\n", boot.reserved);
  if (cvt_c2(boot.sectors))
    printf("Total sectors:               %10d\n", cvt_c2(boot.sectors));
  else
    printf("Total sectors:               %10ld\n", boot.total_sect);
  printf("Max. number of root entries: %10d\n", cvt_c2(boot.dir_entries));
  printf("Sectors per track:           %10d\n", boot.secs_track);
  printf("Number of heads:             %10d\n", boot.heads);

  /* We are saving (all sizes in sectors):
   *   boot.reserved - any reserved space on the FS, including the boot record
   *   boot.fats*boot.fat_length - all copies of FAT
   *   (boot.cluster_size*sizeof(struct msdos_dir_entry))/cvt_c2(boot.sector_size)
   *       - the root directory
   */
  printf("** SWAP saver: \"dd if=%s of=/var/win_swap_header bs=%d count=%d\"\n",
    device_name,
    cvt_c2(boot.sector_size),
    boot.reserved + (boot.fats * boot.fat_length) +
      (sizeof(struct msdos_dir_entry) * cvt_c2(boot.dir_entries)) /
        cvt_c2(boot.sector_size));

  close(fd);
}
