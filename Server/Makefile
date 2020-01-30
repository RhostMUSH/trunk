#
#
#
#

SHELL  = /bin/sh
srcdir = ./src
gdbmdir= ./src/gdbm/
qdbmdir= ./src/qdbm/
bindir = ./bin

CHK = $(shell gmake --version 2>/dev/null|grep -ic "GNU")
ifeq "$(CHK)" "0"
   MAKE=make
else
   MAKE=gmake
endif

all: instructions

instructions:
	@echo 
	@echo "Use one of the following options:"
	@echo "make config"
	@echo "   If you don't feel comfortable hacking the Makefile by hand"
	@echo "   then you may use this to customize your compile."
	@echo "make confsource"
	@echo "   This ties 'config' and 'source' together into a single"
	@echo "   command, for ease of use."
	@echo "make default"
	@echo "   This auto-configures the most commonly used default values"
	@echo "   If you don't want to use the menu this can get you running"
	@echo "make source"
	@echo "   If you have a source code included distribution, then this"
	@echo "   will cause rhost to get compiled"
	@echo "   This also will configure and/or compile gdbm if required."
	@echo "   --- MAKE SURE YOU HAVE SETTINGS IN ./src/Makefile RIGHT --"
	@echo "make create_oldsrc"
	@echo "   Creates a directory 'csrc' which contains both hdrs and src"
	@echo "make links"
	@echo "   Causes the symlinks to be created within the game dirs"
	@echo "make gdbm"
	@echo "   This compiles just the gdbm libraries."
	@echo "make qdbm"
	@echo "   This compiles just the qdbm libraries."
	@echo " "
	@echo "make clean"
	@echo "   Removes any old object files and binaries"
	@echo "make realclean"
	@echo "   Removes old object files, temporary files, backup files"
	@echo "   This also removes the gdbm support libraries."
	@echo "make superclean"
	@echo "   Removes old object files, temporary files, backup files"
	@echo "   This also puts gdbm into a 'clean' state."
	@echo "make gdbmclean"
	@echo "   This removes the gdbm libs requiring a fresh compile."
	@echo "make gdbmrclean"
	@echo "   This puts gdbm into a 'clean' state as if just downloaded."
#	@echo "make distrib"
#	@echo "   Creates a tar archive of the rhost directory hierachy"
	@echo
	@echo "make customize"
	@echo "   If you want to run more than one game, use this command to"
	@echo "   setup a new game directory."
	@echo "make bugreport"
	@echo "   Use this to file an automatic bug report - cool heh?"
	@echo
config:
	@(cd $(bindir) ; ./asksource.sh)
default:
	@(cd $(bindir) ; ./asksource.sh default)
	@(cd $(srcdir) ; ./do_compile.sh)
	(cd $(srcdir) ; $(MAKE))
	@echo
	@echo " *******************************************************"
	@echo "  If the build succeeded you might want to 'make links' " 
	@echo " *******************************************************"
	@echo 
confsource: config source
	@echo
source:
	@(cd $(srcdir) ; ./do_compile.sh)
	(cd $(srcdir) ; $(MAKE))
	@echo
	@echo " *******************************************************"
	@echo "  If the build succeeded you might want to 'make links' " 
	@echo " *******************************************************"
	@echo 
qdbm:
	@(cd $(qdbmdir) ; $() clean ; ./configure; $(MAKE); rm libqdbm*.so*)
gdbm:
	@(cd $(gdbmdir) ; $() clean ; ./configure --enable-static --disable-shared; $(MAKE); rm libqdbm*.so*)
create_oldsrc:
	@(./bin/make_csrc.sh)
	@echo ... Done
links:
	@./bin/make_links.sh
clean:
	@(cd $(srcdir) ; $(MAKE) clean)
	@(cd ${srcdir}/hspace ; ${MAKE} clean)
realclean: clean gdbmclean
	@(rm src/*~ hdrs/*~)
superclean: clean gdbmrclean
	@(rm src/*~ hdrs/*~ bin/*~)
gdbmclean:
	@(cd $(gdbmdir) ; $(MAKE) clean)
gdbmrclean:
	@(cd $(gdbmdir) ; $(MAKE) realclean)
customize:
	@./bin/customize.pl
bugreport: 
	@./bin/bugreport.sh
