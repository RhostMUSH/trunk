# Makefile for developer's utility



#================================================================
# Setting variables
#================================================================


# Generic settings
SHELL = /bin/sh

# Targets
MYBINS = hashtest sorttest fmtcnv031127

# Building binaries
CC = gcc
CPPFLAGS = -I.. -I/usr/local/include -DNDEBUG
CFLAGS = -Wall -ansi -pedantic -g
LDFLAGS =  -L.. -L/usr/local/lib -lqdbm



#================================================================
# Suffix rules
#================================================================


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<



#================================================================
# Actions
#================================================================


all : $(MYBINS)


clean :
	rm -rf $(MYBINS) *.o *.exe a.out gmon.out leak.log *~


distclean : clean



#================================================================
# Building binaries
#================================================================


hashtest : hashtest.o
	$(CC) $(CFLAGS) -o $@ hashtest.o $(LDFLAGS)


sorttest : sorttest.o
	$(CC) $(CFLAGS) -o $@ sorttest.o $(LDFLAGS)


fmtcnv031127 : fmtcnv031127.o
	$(CC) $(CFLAGS) -o $@ fmtcnv031127.o $(LDFLAGS)



# END OF FILE
