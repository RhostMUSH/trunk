# Makefile for QDBM



#================================================================
# Setting variables
#================================================================


# Generic settings
SHELL = /bin/bash
srcdir = .

SUBDIRS = plus java perl ruby cgi lab bros

# Packaging
PACKAGE = qdbm
VERSION = 1.8.78
PACKAGEDIR = $(PACKAGE)-$(VERSION)
PACKAGETGZ = $(PACKAGE)-$(VERSION).tar.gz
LIBVER = 14
LIBREV = 14

# Targets
MYHEADS = depot.h curia.h relic.h hovel.h cabin.h villa.h vista.h odeum.h
MYLIBOBJS = depot.o curia.o relic.o hovel.o cabin.o villa.o vista.o odeum.o myconf.o
MYLIBS = libqdbm.a libqdbm.so.$(LIBVER).$(LIBREV).0 libqdbm.so.$(LIBVER) libqdbm.so
MYDOCS = COPYING ChangeLog THANKS
MYPCS = qdbm.pc
MYWINLIBS = libqdbm.a libqdbm.dll.a
MYMACLIBS = libqdbm.a libqdbm.$(LIBVER).$(LIBREV).0.dylib libqdbm.$(LIBVER).dylib libqdbm.dylib
MYHPUXLIBS = libqdbm.a libqdbm.sl

# Install destinations
prefix = /usr/local
exec_prefix = ${prefix}
MYHEADDIR = ${prefix}/include
MYLIBDIR = ${exec_prefix}/lib
MYDATADIR = ${prefix}/share/$(PACKAGE)
MYPCDIR = ${exec_prefix}/lib/pkgconfig
DESTDIR =

# Building binaries
CC = gcc
CPPFLAGS =  -I$(srcdir) -I$(MYHEADDIR) \
  -I$(HOME)/include -I/usr/local/include  \
  -D_XOPEN_SOURCE_EXTENDED=1 -D_GNU_SOURCE=1 -D__EXTENSIONS__=1 -D_HPUX_SOURCE=1 \
  -D_POSIX_MAPPED_FILES=1 -D_POSIX_SYNCHRONIZED_IO=1 \
  -DPIC=1 -D_THREAD_SAFE=1 -D_REENTRANT=1 -DNDEBUG
ERR = $(shell cc --version|grep -ic clang)
ERR2 = $(shell uname -s|grep -ic bsd)
ifeq "$(ERR)$(ERR2)" "11"
   CFLAGS = -Wall -pedantic -fPIC -fsigned-char -O3 -fomit-frame-pointer -minline-all-stringops
else
   CFLAGS = -Wall -pedantic -fPIC -fsigned-char -O3 -fomit-frame-pointer -fforce-addr -minline-all-stringops
endif

LD = /usr/bin/ld
LIBS = -lqdbm -lc 
LIBLDFLAGS =  -L. -L$(MYLIBDIR) -L$(HOME)/lib -L/usr/local/lib -lc 
LDFLAGS =  -L. -L$(MYLIBDIR) -L$(HOME)/lib -L/usr/local/lib $(LIBS)
LDENV = LD_RUN_PATH=/lib:/usr/lib:$(MYLIBDIR):$(HOME)/lib:/usr/local/lib
AR = /usr/bin/ar
ARFLAGS = rcsv
RUNENV = LD_LIBRARY_PATH=.:/lib:/usr/lib:$(MYLIBDIR):$(HOME)/lib:/usr/local/lib



#================================================================
# Suffix rules
#================================================================


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<



#================================================================
# Actions
#================================================================


targets : all


all : $(MYLIBS) $(MYBINS)
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Ready to install.\n'
	@printf '#================================================================\n'


static :
	make MYLIBS="$(MYLIBS)" LDFLAGS="-static $(LDFLAGS)" all


debug :
	make MYLIBS="$(MYLIBS)" CPPFLAGS="$(CPPFLAGS) -UNDEBUG" \
	  CFLAGS="-Wall -ansi -pedantic -fPIC -fsigned-char -O2 -g" \
	  LDFLAGS="-static $(LDFLAGS)" all


devel :
	make MYLIBS="$(MYLIBS)" CPPFLAGS="$(CPPFLAGS) -UNDEBUG" \
	  CFLAGS="-Wall -ansi -pedantic -fPIC -fsigned-char -O2 -g -pipe" all
	sync ; sync


stable :
	make MYLIBS="$(MYLIBS)" CFLAGS="-Wall -ansi -pedantic -fPIC -fsigned-char -O2" all


profile :
	make MYLIBS="$(MYLIBS)" \
	  CFLAGS="-Wall -pedantic -fPIC -fsigned-char -O3 -pg -g -Werror" \
	  LDFLAGS="-static $(LDFLAGS)" all


unsigned :
	make MYLIBS="$(MYLIBS)" CPPFLAGS="$(CPPFLAGS) -UNDEBUG" \
	  CFLAGS="-Wall -ansi -pedantic -fPIC -funsigned-char -g -O2" all


m64 :
	make MYLIBS="$(MYLIBS)" CPPFLAGS="$(CPPFLAGS) -UNDEBUG" \
	  CFLAGS="-Wall -ansi -pedantic -fPIC -fsigned-char -O2 -m64 -g" all


pen4 :
	stdopt="-O3 -fomit-frame-pointer -minline-all-stringops" ; \
	exopt="-march=pentium4 -minline-all-stringops -fprefetch-loop-arrays" ; \
	  make MYLIBS="$(MYLIBS)" \
	    CFLAGS="-Wall -pedantic -fPIC -fsigned-char $$stdopt $$exopt" all


k8 :
	stdopt="-O3 -fomit-frame-pointer -minline-all-stringops" ; \
	exopt="-march=k8 -minline-all-stringops -fprefetch-loop-arrays" ; \
	  make MYLIBS="$(MYLIBS)" \
	    CFLAGS="-Wall -pedantic -fPIC -fsigned-char $$stdopt $$exopt" all


clean :
	rm -rf $(MYLIBS) *.o *.a *.so *.lo *.la a.out .libs \
	  *.exe *.dll.a *.dll *.dylib *.sl TAGS srcdoc gmon.out leak.log \
	  casket casket.* casket-* *~


version :
	vernum=`expr $(LIBVER)00 + $(LIBREV)` ; \
	  sed -e 's/_QDBM_VERSION.*/_QDBM_VERSION  "$(VERSION)"/' \
	    -e "s/_QDBM_LIBVER.*/_QDBM_LIBVER   $$vernum/" depot.h > depot.h~
	[ -f depot.h~ ] && mv -f depot.h~ depot.h


install :
	mkdir -p $(DESTDIR)$(MYHEADDIR)
	cd $(srcdir) && cp -Rf $(MYHEADS) $(DESTDIR)$(MYHEADDIR)
	mkdir -p $(DESTDIR)$(MYLIBDIR)
	cp -Rf $(MYLIBS) $(DESTDIR)$(MYLIBDIR)
	mkdir -p $(DESTDIR)$(MYDATADIR)
	cd $(srcdir) && cp -Rf $(MYDOCS) $(DESTDIR)$(MYDATADIR)
	mkdir -p $(DESTDIR)$(MYPCDIR)
	cd $(srcdir) && cp -Rf $(MYPCS) $(DESTDIR)$(MYPCDIR)
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Thanks for using QDBM.\n'
	@printf '#================================================================\n'


uninstall :
	cd $(DESTDIR)$(MYHEADDIR) && rm -f $(MYHEADS)
	cd $(DESTDIR)$(MYLIBDIR) && rm -f $(MYLIBS)
	rm -rf $(DESTDIR)$(MYDATADIR)
	cd $(DESTDIR)$(MYPCDIR) && rm -f $(MYPCS)


dist :
	sync ; sync
	for dir in $(SUBDIRS) ; \
	  do \
	    if [ -d $$dir ] ; \
	    then \
	      echo Making $@ in $$dir ; \
	      ( cd $$dir && if [ -f configure.in ] ; then autoconf ; ./configure ; \
	        make dist ; fi ) ; \
	    fi ; \
	  done
	make version
	make distclean
	cd .. && tar cvf - $(PACKAGEDIR) | gzip -c > $(PACKAGETGZ)
	sync ; sync


distclean : clean
	for dir in $(SUBDIRS) ; \
	  do \
	    if [ -d $$dir ] ; \
	    then \
	      echo Making $@ in $$dir ; \
	      ( cd $$dir && if [ -f Makefile ] ; then make distclean ; fi ) ; \
	    fi ; \
	  done
	rm -rf Makefile LTmakefile qdbm.spec qdbm.pc config.cache config.log config.status \
	  autom4te.cache rpm-tmp *-win32


TAGS :
	etags -o $@ *.c *.h

win :
	make MYLIBS="$(MYWINLIBS)" CFLAGS="-Wall -ansi -pedantic -fsigned-char -O2"


mingw :
	make CC="gcc -mno-cygwin" MYLIBS="$(MYWINLIBS)" \
	  CFLAGS="-Wall -fsigned-char -O2" LIBLDFLAGS="" LDFLAGS="-L. -lqdbm "


def : libqdbm.a
	./misc/makevcdef libqdbm.a > qdbm.def


mac :
	make MYLIBS="$(MYMACLIBS)" CFLAGS="-Wall -fsigned-char -fno-common -O2"


hpux :
	make MYLIBS="$(MYHPUXLIBS)"


no-so :
	make MYLIBS="libqdbm.a" all


.PHONY : all clean install check



#================================================================
# Building binaries
#================================================================


libqdbm.a : $(MYLIBOBJS)
	$(AR) $(ARFLAGS) $@ $(MYLIBOBJS)


libqdbm.so.$(LIBVER).$(LIBREV).0 : $(MYLIBOBJS)
	if uname -a | egrep -i 'SunOS' > /dev/null ; \
	  then \
	    $(CC) -shared -Wl,-G,-h,libqdbm.so.$(LIBVER) -o $@ $(MYLIBOBJS) $(LIBLDFLAGS) ; \
	  else \
	    $(CC) -shared -Wl,-soname,libqdbm.so.$(LIBVER) -o $@ $(MYLIBOBJS) $(LIBLDFLAGS) ; \
	  fi


libqdbm.so.$(LIBVER) : libqdbm.so.$(LIBVER).$(LIBREV).0
	ln -f -s libqdbm.so.$(LIBVER).$(LIBREV).0 $@


libqdbm.so : libqdbm.so.$(LIBVER).$(LIBREV).0
	ln -f -s libqdbm.so.$(LIBVER).$(LIBREV).0 $@


libqdbm.dll.a : qdbm.dll


qdbm.dll : $(MYLIBOBJS)
	$(CC) -shared -o $@ \
	  -Wl,--out-implib=lib$@.a \
	  -Wl,--export-all-symbols \
	  -Wl,--enable-auto-import \
	  -Wl,--add-stdcall-alias \
	  -Wl,--whole-archive \
	  -Wl,--no-whole-archive $(MYLIBOBJS) $(LIBLDFLAGS)


libqdbm.$(LIBVER).$(LIBREV).0.dylib : $(MYLIBOBJS)
	$(CC) -dynamiclib -o $@ \
	  -install_name $(MYLIBDIR)/libqdbm.$(LIBVER).dylib \
	  -current_version $(LIBVER).$(LIBREV).0 \
	  -compatibility_version $(LIBVER) \
	  $(MYLIBOBJS) $(LIBLDFLAGS)


libqdbm.$(LIBVER).dylib : libqdbm.$(LIBVER).$(LIBREV).0.dylib
	ln -f -s libqdbm.$(LIBVER).$(LIBREV).0.dylib $@


libqdbm.dylib : libqdbm.$(LIBVER).$(LIBREV).0.dylib
	ln -f -s libqdbm.$(LIBVER).$(LIBREV).0.dylib $@


libqdbm.sl : $(MYLIBOBJS)
	$(CC) -shared -Wl,-b -o $@ $(MYLIBOBJS) $(LIBLDFLAGS)

depot.o : depot.h myconf.h

curia.o : depot.h curia.h myconf.h

relic.o : depot.h relic.h myconf.h

hovel.o : depot.h curia.h hovel.h myconf.h

cabin.o : cabin.h myconf.h

villa.o : depot.h cabin.h villa.h myconf.h

vista.o : depot.h curia.h cabin.h villa.h vista.h myconf.h villa.c

odeum.o : depot.h curia.h cabin.h villa.h myconf.h

myconf.o : myconf.h

dpmgr.o dptest.o dptsv.o : depot.h cabin.h

crmgr.o crtest.o crtsv.o : depot.h curia.h cabin.h

rlmgr.o rltest.o : depot.h relic.h cabin.h

hvmgr.o hvtest.o : depot.h curia.h hovel.h cabin.h

cbtest.o cbcodec.o : cabin.h

vlmgr.o vltest.o vltsv.o : depot.h cabin.h villa.h

odmgr.o odtest.o odidx.o : depot.h curia.h cabin.h villa.h odeum.h



# END OF FILE
