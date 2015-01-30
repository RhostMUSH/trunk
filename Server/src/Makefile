# Generated automatically from Makefile.in by configure.
# Makefile for RhostMUSH 3.2.0
#
# Search for the text 'CONFIGURATION SECTION' and make any changes needed
# there.

include make.defs

SHELL=/bin/sh
srcdir = .
bindir = ../bin
incdir = ../hdrs

ERR = $(shell which gcc >/dev/null; echo $$?)
ifeq "$(ERR)" "0"
   CC = gcc
else
   CC = cc
endif
# CPP = gcc -E	# This is broken in autoconf.  Sigh.
CPP = $(CC) -E

# LIBS is now written to make.defs by asksource.sh

LIBOBJS = 

# CFLAGS (used only to enable low-mem compile) is now written to make.defs by asksource.sh

# If you wish to debug the code, run under this
OPTIM = -g
# Without debug information run this
#OPTIM = -O

# UnterMUD database layer.
UDBA_SRC = udb_acache.c udb_achunk.c udb_attr.c
UDBA_OBJ = udb_acache.o udb_achunk.o udb_attr.o
UDBA_FLG = -DCACHE_ATTRS

UDBO_SRC = udb_ocache.c udb_ochunk.c udb_obj.c
UDBO_OBJ = udb_ocache.o udb_ochunk.o udb_obj.o
UDBO_FLG = -DCACHE_OBJS

UDB_SRC = udb_misc.c
UDB_OBJ = udb_misc.o
UDB_INC	= udb.h udb_defs.h

# Source common to netmush and standalone code
COM_SRC	=
COM_OBJ	=

# Door Configurations -- "Options" now written to make.defs by asksource.sh
DR_SRC = $(DRMUSHSRC) $(DREMPIRESRC) $(DRMAILSRC)
DR_OBJ = $(DRMUSHOBJ) $(DREMPIREOBJ) $(DRMAILOBJ)
DR_HDR = $(DREMPIREHDR)

# Everything needed to use the database in standalone mode.
SA_SRC	= sa-db.c sa-db_rw.c sa-boolexp.c sa-unparse.c sa-compress.c \
	  sa-pred.c sa-stringu.c sa-conf.c sa-log.c sa-object.c \
	  sa-vattr.c sa-htab.c sa-player_c.c sa-flags.c debug.c \
	  sa-tprintf.c
SA_OBJ	= sa-db.o sa-db_rw.o sa-boolexp.o sa-unparse.o sa-compress.o \
	  sa-pred.o sa-stringu.o sa-conf.o sa-log.o sa-object.o \
	  sa-vattr.o sa-htab.o sa-player_c.o sa-flags.o debug.o \
	  sa-tprintf.o

# Distribution source files
D_SRC	= door.c shs.c mushcrypt.c news.c mail.c mailfix.c debug.c senses.c \
	  create.c game.c help.c look.c match.c move.c player.c predicates.c \
	  rob.c set.c speech.c wiz.c walkdb.c timer.c boolexp.c log.c cque.c \
	  unparse.c eval.c command.c wild.c netcommon.c functions.c vattr.c \
	  db.c db_rw.c compress.c stringutil.c object.c conf.c flags.c htab.c \
	  compat.c file_c.c player_c.c bsd.c alloc.c autoreg.c levels.c \
          local.c utils.c pcre.c pcre_extensions.c tprintf.c sha1.c 64btime.c \
          sqlite.c $(DR_SRC)

D_OBJ	= door.o shs.o mushcrypt.o news.o mail.o mailfix.o debug.o senses.o \
	  create.o game.o help.o look.o match.o move.o player.o predicates.o \
	  rob.o set.o speech.o wiz.o walkdb.o timer.o boolexp.o log.o cque.o \
	  unparse.o eval.o command.o wild.o netcommon.o functions.o vattr.o \
	  db.o db_rw.o compress.o stringutil.o object.o conf.o flags.o htab.o \
          compat.o file_c.o player_c.o bsd.o alloc.o autoreg.o levels.o \
          local.o utils.o pcre.o pcre_extensions.o tprintf.o sha1.o 64btime.o \
          sqlite.o $(DR_OBJ)

D_INC	= door.h shs.h news.h mail.h debug.h \
	  copyright.h flags.h help.h htab.h interface.h match.h functions.h \
	  command.h rwho_clilib.h  config.h db.h externs.h mudconf.h misc.h \
	  vattr.h file_c.h fifo.h alloc.h attrs.h levels.h timez.h \
          local.h utils.h pcre.h pcre_extensions.h sha1.h $(DR_HDRS)

D_FLG   = -DMUSH_BUILD_DATE="\"`cat date.txt`\""

# RWHO source files
WHO_SRC	= rwho_clilib.c
WHO_OBJ	= rwho_clilib.o
WHO_FLG	= -DRWHO_IN_USE
RWHO_FL	= -DLOCAL_RWHO_SERVER

# Version number routine
VER_SRC	= version.c
VER_OBJ	= version.o
VER_INC	= patchlevel.h
VER_FLG	= -DMUSH_BUILD_DATE="\"`cat date.txt`\"" \
	  -DMUSH_BUILD_NUM="\"`sh ./buildnum.sh`\""

# ===================== CONFIGURATION SECTION ====================
#
# Select the correct C compiler.  Whatever you choose, it must be able
# to grok ANSI C (function prototypes)
#
#-----CC or GCC (must be able to grok function prototypes)
#
# Following Defs available:
# USE_SIDEEFFECT - enable side effect functions
# TINY_U - Rhost to MUX/MUSH/PENN u()/ufun() functionality
# MUX_INCDEC - Rhost to MUX inc()/dec() functionality
# SOFTCOM - Disable rhost hardcom system (it's archiac... really)
# TINY_SUB - Substitute %c and %x substitutions
# NOEXTSUBS - Disable %c and %x substitutions in its entirity
# USECRYPT - Enable crypt() decrypt() functions
# PLUSHELP - use a hardcoded +help (and plushelp.txt/plushelp.indx)
# PROG_LIKEMUX - makes @program not split up args with comma (work like MUX)
# ENABLE_COMMAND_FLAG - if you want to only have things set $command check for commands
# ATTR_HACK - enables all attribute starting with '_' to be wiz-only.
# -I/usr/ucbinclude -DSOLARIS  (this may be required on some Solaris systems. 
#                               try without this first)
# REALITY_LEVELS - enable if you wish to use REALMS/Reality Levels
# EXPANDED_QREGS - Enable (allow) a-z setq() registers.  This causes memory overhead.
# ZENTY_ANSI - This allows enhanced ansi support.  It comes at some cost as %c codes are seen as 'special'.
# PARIS - Enables an alternate WHO listing that's more Penn/MUSH compatable with display listing.
# OLD_SETQ - This switches setq/setr with setq_old/setr_old for functions less abrasive on braces/brackets.
# SECURE_SIDEEFFECT - This shores up some sideeffects to avoid a double-evaluation.
#
###########################################################################################################
# The following Defs are used for tweaking the build on different Operating Systems
# .. if you get stuck, ask Ashen-Shugar or Lensman to help you out!
# 
# FIX_AIX - use on broken AIX platforms
# BROKEN_ERRNO - use if it can't find errno.h definitions
#                examples: EINTR, EBADF, E<whatever>
#                calls <asm/errno.h>.
# BROKEN_ERRNO_SYS - use if BROKEN_ERRNO returns an error.
#                calls <sys/errno.h>.
# SYSWAIT - use if wait.h undefined.
# BROKEN_NDBM -  ndbm.h file loaded in weird location? This fixes it.
# SYSWAIT - use if wait.h undefined.
# BSD_LIKE   - BSD, Cygwin and Max OSX uses probably want to try setting this.
#              They probably also need to define: -DBROKEN_ERRNO_SYS -DBROKEN_NDBM -DSYSWAIT
# SYS_MALLOC - Used if malloc.h is in a borked spot.
###########################################################################################################

#
# Rhost supports two encryption schemes SHS (SHA-1), and the 
# unix crypt() function (DES).
#
# By default it will try and match the password first to DES
# and then to SHS, however you can override this behavior
# such that it ONLY does DES or ONLY does SHS.
# 
# If moving from one platform to another and the SHS doesn't
# seem to work, try specifying SHS_REVERSE =)
#
# If required, uncomment one of the following.
#CRYPT_SCHEME = -DSHS_REVERSE
#CRYPT_SCHEME = -DCRYPT_DES_ONLY
#CRYPT_SCHEME = -DCRYPT_SHS_ONLY
#CRYPT_SCHEME = -DCRYPT_SHS_ONLY -DSHS_REVERSE
#
#
#-----GCC on some Linux boxes are broken with regards to the below
#
#DEFS = -D__inet_addr=inet_addr -D__inet_ntoa=inet_ntoa
#
#-----CC on a NeXT system, really weird derivative of GCC
#
#DEFS = -DNEXT -DNEED_STRDUP
#
#-----GCC if the libraries were built for a pcc-derived cc compiler
#     (most systems)
#
#DEFS = -fpcc-struct-return -Wall
#
#-----GCC with GCC-compatible libraries if you want verbose error messages
#
#DEFS = -Wall
#
#-----HP-UX C compiler
#
#DEFS = -w +Obb800 -Aa -D_INCLUDE_POSIX_SOURCE -D_INCLUDE_HPUX_SOURCE -D_INCLUDE_XOPEN_SOURCE
#
#-----MIPS C compiler (also DEC 3xxx, Prime EXL7xxx)
#
#DEFS = -signed
#-----Solaris.
#
#DEFS = -I/usr/ucbinclude -DSOLARIS
#
# CYGWIN CONFIGURATION SECTION
# If compiling with CYGWIN, you'll want to uncomment this line =)
# In addition, make sure that none of the system fix defines are set
# in the DEFS line above, and that the crypt schema is SHS only.
#
# IMPORTANT: Make sure you've read CYGWIN.readme before trying this!
#
#CYGWIN = -I/usr/local/include -DCYGWIN -DBROKEN_ERRNO_SYS -DBROKEN_NDBM\
#         -DSYSWAIT -DNODEBUGMONITOR -g 

# IMPORTANT: MORELIBS is now written to make.defs by asksource.sh
# Libraries.  
# The following extra libraries are required based on what system you have.
# #1 - This is used for most libc5 distributions.  This includes Slackware 7.0
#      and earlier, older versions of AIX, and etc.
#
# #2 - This is used for most glibc distributions.  This includes Slackware 7.1
#      and later, Redhat (most distros), BSD, Solaris, etc.
#      This one will work for most situations.
#
# #3 - This should not be a big issues anymore but some systems require MIPS.
#      If you have one of these systems, set this.
#
# #4 - This is for older Solaris systems and some non-comformat platforms.
#      Use if you're running on a SysV-ish system and BSD support isn't built 
#      in to the standard libc.
#
# #5 - This is for cygwin using the cgyipc library.
################################################################  #1
#MORELIBS  = -lm -lrt
################################################################  #2
#MORELIBS = -lm -lrt -lnsl -lresolv -lcrypt 
#-L/usr/lib -lc -lm
################################################################  #3
#MORELIBS  = -lm -lrt -lnsl -lsocket -L/usr/ucblib -lucb
################################################################  #4
#MORELIBS  = -lresolv -lsocket -lnsl -lm -lrt
################################################################  #5
#MORELIBS  = -lm -lrt -L/usr/local/lib -lcygipc

# Set this to the directory where the MUSH game lives.
# This is not really used much
GAME	= /usr/games/lib/mush

# If you want to be able to transmit RWHO data, uncomment these lines:
WHO_S	= $(WHO_SRC)
WHO_O	= $(WHO_OBJ)
WHO_F	= $(WHO_FLG)

# If you want to allow the local RWHO command (only recommended if you are on
# the same machine or subnet as the RWHO server), uncomment these lines:
#RWHO_F	= $(RWHO_FL)

# Select the level at which you want cacheing to be performed.  Prior releases
# of MUSH only supported attribute-level cacheing.
# 
# ----- Attribute-level cacheing.
#UDB_S	= $(UDBA_SRC) $(UDB_SRC)
#UDB_O	= $(UDBA_OBJ) $(UDB_OBJ)
#UDB_F	= $(UDBA_FLG)

# ----- Object-level cacheing.
UDB_S	= $(UDBO_SRC) $(UDB_SRC)
UDB_O	= $(UDBO_OBJ) $(UDB_OBJ)
UDB_F	= $(UDBO_FLG)

# Select the networking support you have.  Currently only BSD networking
# is supported.
#
# ----- BSD networking
NET_S	= bsd.c
NET_O	= bsd.o

# ================== END OF CONFIGURATION SECTION =================


DEBUG	= $(OPTIM)
ALLCFLAGS = $(CFLAGS) $(DEBUG) $(DEFS) $(MOREDEFS) $(D_FLG) $(WHO_F) $(RWHO_F) $(RLIM_F) $(UDB_F) -I$(incdir) $(CRYPT_SCHEME) $(CYGWIN)

# Auxiliary source files: only used by offline utilities.
# This one is for NON-html auto help creation
AUX_SRC	= dbconvert.c mkindx.c unsplit.c
#
# Use this one if you intend to auto create help files via html.
#AUX_SRC	= dbconvert.c mkindx.c mkhtml.c unsplit.c


#Auxiliary files
A_INST	= README NOTES HIGHLIGHTS CREDITS CHANGES
A_TXT	= badsite.txt connect.txt create_reg.txt down.txt guest.txt help.txt \
	  motd.txt news.txt newuser.txt quit.txt register.txt wizhelp.txt \
	  wizmotd.txt full.txt
A_SH	= db_load db_unload db_check buildnum.sh
A_RT	= Startmush mush.config
A_CONF	= netmush.conf alias.conf compat.conf
A_MISC	= buildnum.data
A_VMS	= vms_dbm.c vms_dbm.h startmush.com compile.com link.com
A_AUTO	= configure configure.in autoconf.h.in Makefile.in .depend
AUXIL	= $(A_INST) $(A_TXT) $(A_SH) $(A_CONF) $(A_MISC) $(A_RT) $(A_VMS) \
	  $(A_AUTO)

#Installation files
INSTALL_PROG	= $(bindir)/netrhost $(bindir)/netrhost.debugmon \
                  $(bindir)/murboot $(bindir)/dbconvert \
                  $(bindir)/mkhtml $(bindir)/mkindx $(bindir)/db_load \
                  $(bindir)/db_unload $(bindir)/db_check $(R_SERV)
INSTALL_TARG	= $(INSTALL_PROG) *.txt *.conf
REINSTALL_TARG	= $(INSTALL_PROG) help.txt wizhelp.txt alias.conf compat.conf

#compiliation source files
ALLSRC	= $(D_SRC) $(UDB_SRC) $(UDBA_SRC) $(UDBO_SRC) $(VER_SRC) \
	  $(WHO_SRC) $(COM_SRC) $(SA_SRC) $(AUX_SRC) $(LIB_SRC)
ALLINC	= $(D_INC) $(UDB_INC) $(VER_INC) $(NET_INC) $(LIB_INC)
SRC	= $(D_SRC) $(COM_SRC) $(UDB_S) $(WHO_S)
OBJ	= $(D_OBJ) $(COM_OBJ) $(UDB_O) $(WHO_O)

# Files in the standard distribution
DIST	= $(ALLSRC) $(ALLINC) $(AUXIL)

OUT	= $(bindir)/netrhost $(bindir)/netrhost.debugmon $(bindir)/murboot \
          $(bindir)/mkhtml $(bindir)/mkindx $(bindir)/dbconvert

.c.o:
	$(CC) $(ALLCFLAGS) -c $<

# paths is likely to remain broken
all: rundate $(OUT)

rundate:
	@../bin/dodate.sh

#.PHONY:	all

new-install: $(INSTALL_TARG)
	-mkdir $(GAME)
	cp $(INSTALL_TARG) $(GAME)

install: $(REINSTALL_TARG)
	cp $(REINSTALL_TARG) $(GAME)

TAGS: *.c *.h
	etags *.c *.h

$(bindir)/murboot: murboot.o
	$(CC) $(ALLCFLAGS) -o $(bindir)/murboot murboot.o

$(bindir)/mkhtml: mkhtml.o
	$(CC) $(ALLCFLAGS) -o $(bindir)/mkhtml mkhtml.o

$(bindir)/mkindx: mkindx.o
	$(CC) $(ALLCFLAGS) -o $(bindir)/mkindx mkindx.o

$(bindir)/unsplit: unsplit.o
	$(CC) $(ALLCFLAGS) -o $(bindir)/unsplit unsplit.o

$(bindir)/dbconvert: dbconvert.o $(SA_OBJ) $(UDB_O) $(ALLOC_O)$(LIBOBJS)
	$(CC) $(ALLCFLAGS) -o $(bindir)/dbconvert dbconvert.o $(SA_OBJ) $(COM_OBJ) $(UDB_O) $(ALLOC_O) $(LIBS) $(MORELIBS) $(LIBOBJS)

saber:	$(SRC)
	#load $(SRC)

saber-o: $(OBJ) $(LIBOBJS)
	#load $(OBJ) $(VER_OBJ) $(LIBS) $(MORELIBS)

debugmon.o: debugmon.c $(incdir)/debug.h
	$(CC) -g $(ALLCFLAGS) -c debugmon.c

$(bindir)/netrhost.debugmon: debugmon.o debug.o
	$(CC) -o $(bindir)/netrhost.debugmon debugmon.o debug.o $(MORELIBS)

$(bindir)/netrhost: $(OBJ) $(LIBOBJS)
	$(CC) $(ALLCFLAGS) $(VER_FLG) -c $(VER_SRC)
	- ( test -f $(bindir)/netrhost && mv -f $(bindir)/netrhost netrhost~ )
	$(CC) $(ALLCFLAGS) -o $(bindir)/netrhost $(OBJ) $(LIBOBJS) $(VER_OBJ) $(LIBS) $(MORELIBS)

dist.tar.Z: $(DIST)
	-rm -f buildnum.data
	echo 0 > buildnum.data
	tar cvhf - $(DIST) | compress -c > dist.tar.Z.NEW
	mv dist.tar.Z.NEW dist.tar.Z

depend: $(ALLSRC) unsplit
	for i in $(D_SRC) $(COM_SRC) $(SA_SRC) $(UDBA_SRC) $(UDBO_SRC) $(AUX_SRC) $(WHO_SRC) $(LIB_SRC); do $(CPP) $(ALLCFLAGS) -M $$i; done | sed -e 's:/usr[^ ]* ::g' | ./unsplit > .depend~
	mv .depend~ .depend

index: help.indx news.indx wizhelp.indx error.indx

# Note Uncomment these ONLY if you wish to impliment ALPHA code.
# The door functionality is considered ALPHA code and can and will
# bring down the server until officially released.   It crashes
# the server once every 100 or so attempts on an average but
# with no rhyme or reason.  It is possibly a compile-time error
# and we are currently looking into it.
# Until then, use at your own risk.
#
#index: help.indx news.indx wizhelp.indx error.indx doorconf.indx
#doorconf.indx: doorconf.txt
#	./mkindx doorconf.txt doorconf.indx
#

error.indx: error.txt
	./mkindx error.txt error.indx

help.indx: help.txt
	./mkindx help.txt help.indx

news.indx: news.txt
	./mkindx news.txt news.indx

wizhelp.indx: wizhelp.txt
	./mkindx wizhelp.txt wizhelp.indx

verify:
	runtest --tool mush --src ./testsuite

realclean:
	-rm -f .depend *.o a.out core gmon.out mush.*log mush.*sum $(OUT) netrhost~ 
	echo '' > .depend

clean:
	-cp -f $(bindir)/netrhost $(bindir)/netrhost~
	-rm -f .depend *.o a.out core gmon.out mush.*log mush.*sum $(OUT)
	echo '' > .depend

include .depend

