# Generated automatically from Makefile.in by configure.
###############################################################################
# TARGETS
###############################################################################
# make install:
#	does a full make and installs the lde binary and man page
###############################################################################
srcdir = /share/usr/local/src/lde/macros/..
VPATH = /share/usr/local/src/lde/macros/..

VERSION=2.4.pre1

.EXPORT_ALL_VARIABLES:

###############################################################################
# Variables for binary installation
###############################################################################
# sbindir       = where to install binary (/sbin)
# mandir        = where to install man page (/usr/local/man/man1)
# program_name  = name of installed binary (lde)
###############################################################################
prefix = /usr/local
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib
sbindir = /sbin
mandir = $(prefix)/man/man8
manext = 8
program_name = lde

###############################################################################
# Curses support (include only when defined as LDE_CURSES_SRC)
###############################################################################
LDE_CURSES_SRC=nc_lde.c nc_block.c nc_inode.c nc_dir.c
LDE_CURSES_OBJ=nc_lde.o nc_block.o nc_inode.o nc_dir.o

###############################################################################
# Other defines which might be useful
#
# -DREAD_PART_TABLES -- for ext2fs currently, can read in the full
#                       disk tables if undefined, or just one group at
#                       a time when defined (memory savings should really be on the
#                       order of kb, maybe 10's of kb)
# -DNO_WRITE_INODE   -- turn off inode writes
# -DPARANOID         -- open device read only at all times
# -DNO_KERNEL_BITOPS -- use the C bitop replacement code instead of the ones
#			in <asm/bitops.h>
# -DALPHA_CODE       -- Activate some of the code that is still Alpha
# -DBETA_CODE        -- Activate some of the code that is still Beta
# -DERRORS_SAVED=30  -- define the number of errors which are saved in the
#			error history log (default to 30 if not set below).
# -DHAVE_LIBGPM      -- use GPM Mouse support
#
###############################################################################
QUIET_CFLAGS=-w
CPPFLAGS=  -DMSDOS_BS_NAMED_FAT=1 -DUNISTD_LLSEEK_PROTO=1 -DHAVE_LLSEEK=1 -DSTDC_HEADERS=1 -DHAVE_MINIXFS=1 -DHAVE_MSDOSFS=1 -DHAVE_XIAFS=1 -DHAVE_EXT2FS=1 -DBETA_CODE=1 -DHAS_CURSES=1 -DUSE_NCURSES=1 -DLDE_CURSES=1 -DHAVE_LIBGPM=1 -DHAVE_UNAME=1  
CFLAGS=-DVERSION=\"$(VERSION)\" $(CPPFLAGS) -O2
LDFLAGS=
LIBS= -lgpm -lncurses

###############################################################################
# Define filesystems
###############################################################################
FS_SRCS=ext2fs.c xiafs.c msdos_fs.c minix.c 
FS_OBJS=ext2fs.o xiafs.o msdos_fs.o minix.o 

###############################################################################
# Define compilers
###############################################################################
CC=gcc

YACC=bison -y
AR=/usr/bin/ar
RM=/bin/rm -f
INSTALL = /usr/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL} -m 0500
INSTALL_DATA = ${INSTALL} -m 644

###############################################################################
#                          STOP EDITING HERE
###############################################################################

$(program_name):	all

all:
	( cd src ; $(MAKE) -f Makefile all )

clean:
	( cd src ; $(MAKE) -f Makefile clean )

clear:
	( cd src ; $(MAKE) -f Makefile clear )
	$(RM) macros/config.*
	$(RM) lde

dep:	depend

depend:
	( cd src ; $(MAKE) -f Makefile depend )

install:	$(program_name)
	${INSTALL_PROGRAM} $(program_name) $(sbindir)/$(program_name)
	${INSTALL_DATA} doc/lde.man $(mandir)/$(program_name).$(manext)

###############################################################################
#                     YOU SHOULD NEVER EVER NEED THESE
###############################################################################

doc/lde.man.text:	doc/lde.man
	groff -Tascii -P-u -P-b -man doc/lde.man > doc/lde.man.text

dist:	clear doc/lde.man.text all clean

tar:
	@if [ -f lde*tar.gz ] ; then mv -f lde*tar.gz old ; fi
	( cd ..; tar -cvz -f lde/lde-$(VERSION).tar.gz lde \
	   --exclude lde-$(VERSION).tar.gz --exclude RCS \
	   --exclude lde/test\* --exclude \*~ \
           --exclude .depend --exclude old \
           --exclude lde-$(VERSION).zip --exclude lde/Makefile \
           --exclude lde/macros/config.\* )

diff:
	( cd ..; diff -u --recursive --new-file \
	  -x 'lde-$(VERSION).zip' -x 'lde/Makefile' \
	  -x lde/old/untar/lde/Makefile  \
	  -x 'lde-$(VERSION).tar.gz' -x 'RCS' \
	  -x 'test*' -x '*~' \
	  -x '.depend' -x 'old' \
	  -x 'config.*' lde/old/untar/lde lde )
	  echo
	  echo

autoconf:
	cd macros
	autoconf ; $(RM) config.cache
