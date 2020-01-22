# Makefile for HSpace

CFLAGS=$(CCFLAGS) -g -I.. -I../../hdrs -Wall

CC=gcc -DRHOST

# Used for protoizing
.cpp.o:
	$(CC) $(CFLAGS) -c $<

SPACE_O=hscelestial.o hsclass.o hscmds.o hsconf.o \
	hsconsole.o hsconsolecmds.o hsdb.o hseng.o hsengcmds.o \
	hsengines.o hsfuel.o hsfuncs.o hsinterface.o hsmain.o \
	hsmissile.o hsnavcmds.o hsobjects.o hspace.o \
	hsplanet.o hsship.o hsshipnav.o hsshipeng.o hstrig.o \
	hsuniverse.o hsutils.o hsweapon.o hscomm.o \
	hsterritory.o

hspace: $(SPACE_O)
#	ar rcs libhspace.a *.o
#	ranlib libhspace.a
	@echo "Done with HSpace."

hscmds.o: hscmds.h hsuniverse.h hspace.h hsdb.h hsinterface.h hsutils.h hsconf.h
hscelestial.o: hscelestial.h hsinterface.h hsutils.h hsobjects.h
hsclass.o: hsclass.h hsengines.h hsutils.h hsconf.h hstypes.h hseng.h hsfuel.h
hsconf.o: hsconf.h hsutils.h hstypes.h
hsconsole.o: hsobjects.h hsutils.h hsinterface.h
hsconsolecmds.o: hstypes.h hsdb.h hscmds.h hsobjects.h hsutils.h hsinterface.h
hsdb.o: hsdb.h hsweapon.h hsinterface.h hsutils.h hsuniverse.h hstypes.h hsobjects.h hscelestial.h
hseng.o: hseng.h hsutils.h hsuniverse.h hsengines.h hsconf.h hspace.h hstypes.h
hsengcmds.o: hscmds.h hsdb.h hseng.h hsinterface.h hsutils.h
hsengines.o: hsengines.h hsobjects.h hspace.h hsconf.h hsfuel.h
hsfuel.o: hsfuel.h hstypes.h hseng.h
hsfuncs.o: hstypes.h hsfuncs.h hsinterface.h hsobjects.h hsdb.h hsutils.h hsuniverse.h hspace.h hscomm.h
hsinterface.o: hstypes.h hsinterface.h
hsmain.o: hspace.h
hsmissile.o: hsmissile.h hsuniverse.h hsutils.h hsinterface.h hspace.h hsweapon.h hsobjects.h
hsnavcmds.o: hscmds.h hsdb.h hseng.h hsinterface.h hsutils.h hsconf.h
hsobjects.o: hsobjects.h hsinterface.h hsutils.h hsuniverse.h hstypes.h hsclass.h hsweapon.h hsfuel.h
hspace.o: hspace.h hsuniverse.h hscmds.h hsfuncs.h hsconf.h hsdb.h hstypes.h
hsplanet.o: hsobjects.h hsuniverse.h hsinterface.h hsutils.h
hsship.o: hsobjects.h hsinterface.h hsuniverse.h hsutils.h hsconf.h hspace.h hsengines.h hscomm.h
hsshipnav.o: hsobjects.h hscelestial.h hsinterface.h hsutils.h hsconf.h hsengines.h hsdb.h hspace.h
hsshipnav.o: hsuniverse.h
hsshipeng.o: hsobjects.h hsinterface.h hsutils.h hsconf.h hspace.h
hstrig.o: hscopyright.h
hsuniverse.o: hsuniverse.h hsutils.h hspace.h hseng.h hsconf.h hsinterface.h hsobjects.h hscelestial.h
hsutils.o: hstypes.h hsutils.h
hsweapon.o: hsweapon.h hsobjects.h hsinterface.h hspace.h hsutils.h hsmissile.h hstypes.h
hscomm.o: hsobjects.h hsuniverse.h
hsterritory.o: hsterritory.h hsobjects.h hsconf.h hsuniverse.h hsinterface.h hsutils.h

clean: 
	-rm -f *.o

