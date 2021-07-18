# Test data for the Linux Disk Editor.

Scripts
=======
* test.sh = test script to verify builds

Sample small images
===================
* test.ext2  = small iamge of extfs
* test.minix = small image of MINIX file system
* test.xiafs = small image of XIAFS file system
* test.data  = some binary test data, no associated file system

Folders
=======
* expected/ = normal output from test scripts
* results/  = temporary directory for storing results, set RETAIN=1 to prevent autocleanup after each test
* search/   = files with some binary signatures
