#################################################################
# YOU SHOULD NOT EDIT THIS FILE, EDIT THE MAIN MAKEFILE INSTEAD #
#################################################################

SRCS = fileutils-3.12/filemode.c 
OBJS = getdate.o filemode.o 

SWIPED_LIB = ../swiped.a

dummy:
	( cd ../.. ; $(MAKE) )

all:	$(SWIPED_LIB)

$(SWIPED_LIB):	$(OBJS)
	$(RM) $(SWIPED_LIB)
	$(AR) $(EXTRA_ARFLAFS) rcs $(SWIPED_LIB) $(OBJS)

# Special modules stolen from freely distributable code
getdate.o:	cnews/getdate.y
	$(YACC) cnews/getdate.y
	$(CC) -c $(CFLAGS) $(QUIET_CFLAGS) -o getdate.o y.tab.c
	$(RM) y.tab.c

filemode.o:	fileutils-3.12/filemode.c
	$(CC) -c $(CFLAGS) $(QUIET_CFLAGS) -o filemode.o fileutils-3.12/filemode.c

getsize.o:	e2fsprogs-1.02/getsize.c
	$(CC) -c $(CFLAGS) $(QUIET_CFLAGS) -o getsize.o e2fsprogs-1.02/getsize.c

ismounted.o:	e2fsprogs-1.02/ismounted.c
	$(CC) -c $(CFLAGS) $(QUIET_CFLAGS) -o ismounted.o e2fsprogs-1.02/ismounted.c

clean:
	$(RM) *.o $(SWIPED_LIB) .depend

clear:	clean

.depend:
	@for i in $(SRCS);do $(CPP) -M $$i;done >> .depend

depend:
	@$(RM) .depend
	@$(MAKE) .depend

dep:	depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

.c.o:	.depend
	$(CC) -c $(CFLAGS) -o $*.o $*.c


