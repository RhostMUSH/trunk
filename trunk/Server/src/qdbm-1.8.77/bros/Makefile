# Makefile for writing tests of DBM brothers



#================================================================
# Setting variables
#================================================================


# Generic settings
SHELL = /bin/sh

# Targets
MYBINS = qdbmtest ndbmtest sdbmtest gdbmtest tdbtest cdbtest bdbtest



#================================================================
# Suffix rules
#================================================================


.SUFFIXES :



#================================================================
# Actions
#================================================================


mesg :
	@echo "Here are writing tests of DBM brothers." 1>&2
	@echo "select a target of {all clean $(MYBINS)}." 1>&2


all : $(MYBINS)


clean :
	rm -rf $(MYBINS) *.o *.exe a.out casket* *~


distclean : clean



#================================================================
# Building binaries
#================================================================


qdbmtest : qdbmtest.c
	gcc -I.. -I/usr/local/include -D_GNU_SOURCE=1 \
	  -Wall -ansi -pedantic -O3 -o $@ qdbmtest.c \
	  -static -L.. -L/usr/local/lib -lqdbm -lz -lc


ndbmtest : ndbmtest.c
	gcc -I/usr/local/include -D_GNU_SOURCE=1 \
	  -Wall -ansi -pedantic -O3 -o $@ ndbmtest.c \
	  -static -L/usr/local/lib -lndbm -lc


sdbmtest : sdbmtest.c
	gcc -I/usr/local/include -D_GNU_SOURCE=1 \
	  -Wall -ansi -pedantic -O3 -o $@ sdbmtest.c \
	  -static -L/usr/local/lib -lsdbm -lc


gdbmtest : gdbmtest.c
	gcc -I/usr/local/include -D_GNU_SOURCE=1 \
	  -Wall -ansi -pedantic -O3 -o $@ gdbmtest.c \
	  -static -L/usr/local/lib -lgdbm -lc


tdbtest : tdbtest.c
	gcc -I/usr/local/include -D_GNU_SOURCE=1 \
	  -Wall -ansi -pedantic -O3 -o $@ tdbtest.c \
	  -static -L/usr/local/lib -ltdb -lc


cdbtest : cdbtest.c
	gcc -I/usr/local/include -D_GNU_SOURCE=1 \
	  -Wall -ansi -pedantic -O3 -o $@ cdbtest.c \
	  -static -L/usr/local/lib -lcdb -lc


bdbtest : bdbtest.c
	gcc -I/usr/local/bdb/include -D_GNU_SOURCE=1 \
	  -Wall -O3 -o $@ bdbtest.c \
	  -static -L/usr/local/bdb/lib -ldb-4.4 -lpthread -lc



# END OF FILE
