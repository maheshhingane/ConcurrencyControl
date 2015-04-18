#
# Makefile for TX Manager project.  Needs GNU make.
#
# Define DEBUGFLAGS for debugging output
#
# Warning: make depend overwrites this file.

.PHONY: depend clean backup setup

MAIN=zgt_test

#set HOME to your home dir
# Change the following line depending on where you have copied and unzipped the files
#solutions dir should have src, includes, a

TXMGR=..

CYGWIN=/usr

CC=$(CYGWIN)/bin/g++

# EXAMPLE: In the next line only TX_DEBUG is enabled
#DEBUGFLAGS =  -DTX_DEBUG # -DTM_DEBUG -DHT_DEBUG

#Below, all are disabled
DEBUGFLAGS = -DTX_DEBUG -DTM_DEBUG -DHT_DEBUG

INCLUDES = -I${TXMGR}/include -I.

LINCLUDES = -L$(CYGWIN)/lib

SRCS = zgt_test.C zgt_tm.C zgt_tx.C zgt_ht.C zgt_semaphore.C

OBJS = $(SRCS:.C=.o)

$(MAIN):  $(OBJS) Makefile
	 $(CC) -lpthread $(CFLAGS) $(DEBUGFLAGS) $(INCLUDES) $(OBJS) -o $(MAIN) $(LFLAGS)

.C.o:
	$(CC) $(CFLAGS) $(INCLUDES) $(LINCLUDES) $(DEBUGFLAGS) -c $<

depend: $(SRCS) Makefile
	makedepend $(INCLUDES)  $^

clean:
	rm -f *.o *~ $(MAIN)

# Grab the sources for a user who has only the makefile
setup:
#	/bin/mkdir srcbkup$$
#	/bin/mv -f * srcbkup$$/
	/bin/cp -f $(TXMGR)/src/*.[C] .
	/bin/cp -f $(TXMGR)/src/*.txt .
	/bin/cp -f $(TXMGR)/src/* .

# DO NOT DELETE THIS LINE -- make depend needs it
