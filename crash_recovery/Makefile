#################################################################
# YOU SHOULD NOT EDIT THIS FILE, EDIT THE MAIN MAKEFILE INSTEAD #
#################################################################

SRCS = find_ext2_fs.c

OBJS = find_ext2_fs.o

dummy:
	( cd .. ; ${MAKE} )

all:	find_ext2_fs

find_ext2_fs:	.depend $(OBJS)
	$(CC) -s -o find_ext2_fs $(OBJS)

clean:
	$(RM) *.o .depend

clear:	clean
	$(RM) find_ext2_fs

checkout:

checkin:

.depend:
	@echo Doing \"make depend\"
	$(CPP) $(CPPFLAGS) -M $(SRCS) > .depend

depend:
	@$(RM) .depend
	@$(MAKE) .depend

dep:	depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

.c.o:	.depend
	$(CC) -c $(CFLAGS) -o $*.o $*.c


