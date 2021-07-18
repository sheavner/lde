[![CMake](https://github.com/sheavner/lde/actions/workflows/cmake.yml/badge.svg)](https://github.com/sheavner/lde/actions/workflows/cmake.yml)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/23407/badge.svg)](https://scan.coverity.com/projects/sheavner-lde)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/sheavner/lde.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/sheavner/lde/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/sheavner/lde.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/sheavner/lde/context:cpp)

This is lde, the Linux disk editor, for Minix/Linux
partitions.  It supports what were once the three most
popular file systems under Linux: ext2fs, minix, and xiafs (there is
also a "nofs" system under which lde will function as a binary
editor).  There is also minimal support for msdos/fat filesytems and
very minimal support for ISO9660 cdrom based filesystems.
lde allows you to view and edit disk blocks as hex and/or
ASCII, view/navigate directory entries, and view and edit formatted
inodes.  Most of the functions can be accessed using the program's
curses interface or from the command line so that you can automate
things with your own scripts.

lde can also be used to recover files which may have been
accidentally erased or just to poke around the file system to see what
it's made of.  I've included a short introduction to the Minix file
system (just enough to get the ideas of blocks and inodes across to
most people) and some docs on the ext2fs (mostly just data out of
<linux/ext2_fs.h> formatted as tables to make wading through the disk
blocks a little bit easier).  If you aren't familiar with inode based
file systems, you should have a look at those files before proceeding
to the doc/UNERASE file, which details what I think you might try to
recover a file. 

There is no tutorial, but there is a detailed man page.  Look
at docs/lde.man (or docs/lde.man.text) for information on running lde
from the command line and using its ncurses interface.  For more
information on compiling, installing, or running lde for the first
time, see INSTALL and INSTALL.LDE.

This project started as a major hack to fsck.  In the first
few months of the project, most of the fsck code evaporated as I added
support for the xiafs and ext2fs file systems, but there is still some
code which should be credited to Linus Torvalds.  Also, some of the
code in ext2fs.c has worked its way out of Remy Card's e2fsprogs.

Development has been moved to github.  The project homepage
is https://sheavner.github.io/lde or check the project page at
https://github.com/sheavner/lde.  They both contain links back to
the other.  Older versions of the project can be found at
http://lde.sourceforge.net

If you find lde useful, send me email and a postcard -- email
for instant gratification and a postcard, so I'll have some tangible
evidence of a userbase.  I won't turn away cash or gifts if you feel
any urges to bestow them upon me.
