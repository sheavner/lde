#################################################################
# YOU SHOULD NOT EDIT THIS FILE, EDIT THE MAIN MAKEFILE INSTEAD #
#################################################################

SRCS = ext2fs.c msdos_fs.c minix.c xiafs.c no_fs.c recover.c \
       main_lde.c $(LDE_CURSES_SRC) tty_lde.c bitops.c iso9660.c

OBJS = ext2fs.o msdos_fs.o minix.o xiafs.o no_fs.o recover.o \
       main_lde.o $(LDE_CURSES_OBJ) tty_lde.o bitops.o iso9660.o swiped.a

HEADERS = lde.h nc_lde.h bitops.h nc_block_help.h nc_inode_help.h \
	nc_lde_help.h no_fs.h xiafs.h curses.h minix.h nc_dir.h \
	nc_lde.h recover.h ext2fs.h nc_block.h nc_inode.h tty_lde.h \
	msdos_fs.h keymap.h iso9660.h

dummy:
	( cd .. ; ${MAKE} )

all:	$(program_name)

$(program_name):	../$(program_name)

../$(program_name):	.depend $(OBJS)
	$(CC) $(LDFLAGS) -o ../$(program_name) $(OBJS) $(LIBS)

swiped.a:
	@(cd swiped; $(MAKE) -f Makefile all)

clean:
	$(RM) *.o ChangeLog~ .depend
	@(cd swiped; ${MAKE} -f Makefile clean)

clear:	clean
	$(RM) $(PROG_NAME)
	@(cd swiped; ${MAKE} -f Makefile clear)

checkout:
	RCS/checkout_all

checkin:
	RCS/checkin_all

.depend:
	@echo Doing \"make depend\"
	$(CPP) $(CPPFLAGS) -M $(SRCS) > .depend
	(cd swiped; ${MAKE} depend)

depend:
	@$(RM) .depend
	@$(MAKE) .depend

dep:	depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

.c.o:	.depend
	$(CC) -c $(CFLAGS) -o $*.o $*.c


