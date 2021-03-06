##############################################################################
#   MAKEFILE - this file is part of Binary vIEW project (BIEW)               #
##############################################################################
#   Copyrights:           1998, 2000 Nickols_K                               #
#   License:              See below                                          #
#   Author and developer: Nickols_K                                          #
#   Requirement:          GNU make                                           #
#   Original file name:   makefile                                           #
####################### [ D e s c r i p t i o n ] ############################
#  This file is script for make utility of GNU development system.           #
########################### [ L i c e n c e ] ################################
# The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.                    #
# All rights reserved.                                                       #
# This software is redistributable under the licence given in the file       #
# "Licence" distributed in the BIEW archive.                                 #
##############################################################################
HOST_CFLAGS=
HOST_LDFLAGS=

include ./config.mak

CFLAGS = $(CDEFOS) $(CDEFSYS) $(HOST_CFLAGS)
LDFLAGS = $(OSLDEF) $(HOST_LDFLAGS)

###########################################################################
# TARGET: put name of executable image here                               #
###########################################################################
TARGET = biew
BIEWLIB = ./biewlib/$(LIBPREFIX)$(TARGET).$(LIBEXT)
##########################################################################
#                Please not modify contents below                        #
##########################################################################

INCS = -I.
LIBS = -L./biewlib -l$(TARGET) $(OS_LIBS)
OBJS = \
addendum.o\
bconsole.o\
biew.o\
biewhelp.o\
biewutil.o\
bin_util.o\
bmfile.o\
codeguid.o\
colorset.o\
dialogs.o\
editors.o\
events.o\
fileutil.o\
info_win.o\
mainloop.o\
refs.o\
search.o\
setup.o\
sysinfo.o\
tstrings.o\
addons/sys/ascii.o\
addons/sys/consinfo.o\
addons/sys/cpu_perf.o\
addons/sys/inview.o\
addons/tools/dig_conv.o\
addons/tools/eval.o\
plugins/bin/aout.o\
plugins/bin/arch.o\
plugins/bin/asf.o\
plugins/bin/avi.o\
plugins/bin/bin.o\
plugins/bin/bmp.o\
plugins/bin/coff386.o\
plugins/bin/dos_sys.o\
plugins/bin/elf386.o\
plugins/bin/jpeg.o\
plugins/bin/jvmclass.o\
plugins/bin/le.o\
plugins/bin/lmf.o\
plugins/bin/lx.o\
plugins/bin/mov.o\
plugins/bin/mp3.o\
plugins/bin/mpeg.o\
plugins/bin/mz.o\
plugins/bin/ne.o\
plugins/bin/nlm386.o\
plugins/bin/opharlap.o\
plugins/bin/pe.o\
plugins/bin/pharlap.o\
plugins/bin/rdoff.o\
plugins/bin/rdoff2.o\
plugins/bin/realmedia.o\
plugins/bin/sis.o\
plugins/bin/sisx.o\
plugins/bin/wav.o\
plugins/binmode.o\
plugins/disasm.o\
plugins/hexmode.o\
plugins/textmode.o\
plugins/nls/russian.o\
plugins/disasm/null_da.o\
plugins/disasm/arm/arm.o\
plugins/disasm/arm/arm16.o\
plugins/disasm/arm/arm32.o\
plugins/disasm/avr/avr.o\
plugins/disasm/ppc/ppc.o\
plugins/disasm/java/java.o\
plugins/disasm/ix86/ix86.o\
plugins/disasm/ix86/ix86_fpu.o\
plugins/disasm/ix86/ix86_fun.o
BIEWLIB_OBJS =\
biewlib/bbio.o\
biewlib/biewlib.o\
biewlib/file_ini.o\
biewlib/pmalloc.o\
biewlib/twin.o\
biewlib/tw_class.o\
biewlib/sysdep/$(MACHINE)/aclib.o\
biewlib/sysdep/$(MACHINE)/cpu_info.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/fileio.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/keyboard.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/mmfio.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/mouse.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/misc.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/nls.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/os_dep.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/timer.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/vio.o

ifeq ($(HOST),qnx)
OBJS += \
biewlib/sysdep/$(MACHINE)/$(HOST)/3rto3s.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/3sto3r.o
endif

HLP_SUBDIRS=tools/biewhlp tools/lzss
DO_HELP = @ for i in $(HLP_SUBDIRS); do $(MAKE) -C $$i $@ || exit; done

all: $(BIEWLIB) $(TARGET)
biewlib: $(BIEWLIB)

clean:
	$(DO_HELP)
	$(RM) $(OBJS)
	$(RM) $(BIEWLIB_OBJS)
	$(RM) $(TARGET)
	$(RM) $(BIEWLIB)
	$(RM) biew.map
	$(RM) *.err

distclean: clean
	$(DO_HELP)
	$(RM) config.log config.mak
	$(RM) -f ./hlp/biew.hlp
	$(RM) -f ./hlp/biewhlp
	$(RM) -f ./hlp/lzss

cleansys:
	$(RM) biewlib/sysdep/$(MACHINE)/{*.o,$(HOST)/*.o}
cleanlib:
	$(RM) $(BIEWLIB_OBJS)
	$(RM) $(BIEWLIB)

$(BIEWLIB): $(BIEWLIB_OBJS)
	$(AR) $@ $(BIEWLIB_OBJS)
	$(RANLIB) $@

$(TARGET): $(OBJS) $(BIEWLIB)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
%.o : %.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

addendum.o:                   addendum.c
bconsole.o:                   bconsole.c
biew.o:                       biew.c
biewhelp.o:                   biewhelp.c
biewutil.o:                   biewutil.c
bin_util.o:                   bin_util.c
bmfile.o:                     bmfile.c
codeguid.o:                   codeguid.c
colorset.o:                   colorset.c
dialogs.o:                    dialogs.c
editors.o:                    editors.c
events.o:                     events.c
fileutil.o:                   fileutil.c
info_win.o:                   info_win.c
mainloop.o:                   mainloop.c
refs.o:                       refs.c
search.o:                     search.c
setup.o:                      setup.c
sysinfo.o:                    sysinfo.c
tstrings.o:                   tstrings.c
addons/sys/ascii.o:           addons/sys/ascii.c
addons/sys/consinfo.o:        addons/sys/consinfo.c
addons/sys/cpu_perf.o:        addons/sys/cpu_perf.c
addons/sys/inview.o:          addons/sys/inview.c
addons/tools/dig_conv.o:      addons/tools/dig_conv.c
addons/tools/eval.o:          addons/tools/eval.c
plugins/bin/aout.o:           plugins/bin/aout.c
plugins/bin/arch.o:           plugins/bin/arch.c
plugins/bin/asf.o:            plugins/bin/asf.c
plugins/bin/avi.o:            plugins/bin/avi.c
plugins/bin/bin.o:            plugins/bin/bin.c
plugins/bin/bmp.o:            plugins/bin/bmp.c
plugins/bin/coff386.o:        plugins/bin/coff386.c
plugins/bin/dos_sys.o:        plugins/bin/dos_sys.c
plugins/bin/elf386.o:         plugins/bin/elf386.c
plugins/bin/jpeg.o:           plugins/bin/jpeg.c
plugins/bin/jvmclass.o:       plugins/bin/jvmclass.c
plugins/bin/le.o:             plugins/bin/le.c
plugins/bin/lmf.o:            plugins/bin/lmf.c
plugins/bin/lx.o:             plugins/bin/lx.c
plugins/bin/mov.o:            plugins/bin/mov.c
plugins/bin/mp3.o:            plugins/bin/mp3.c
plugins/bin/mpeg.o:           plugins/bin/mpeg.c
plugins/bin/mz.o:             plugins/bin/mz.c
plugins/bin/ne.o:             plugins/bin/ne.c
plugins/bin/nlm386.o:         plugins/bin/nlm386.c
plugins/bin/opharlap.o:       plugins/bin/opharlap.c
plugins/bin/pe.o:             plugins/bin/pe.c
plugins/bin/pharlap.o:        plugins/bin/pharlap.c
plugins/bin/rdoff.o:          plugins/bin/rdoff.c
plugins/bin/rdoff2.o:         plugins/bin/rdoff2.c
plugins/bin/realmedia.o:      plugins/bin/realmedia.c
plugins/bin/sis.o:            plugins/bin/sis.c
plugins/bin/sisx.o:           plugins/bin/sisx.c
plugins/bin/wav.o:            plugins/bin/wav.c
plugins/binmode.o:            plugins/binmode.c
plugins/disasm.o:             plugins/disasm.c
plugins/hexmode.o:            plugins/hexmode.c
plugins/textmode.o:           plugins/textmode.c
plugins/nls/russian.o:        plugins/nls/russian.c
plugins/disasm/null_da.o:     plugins/disasm/null_da.c
plugins/disasm/java/java.o:   plugins/disasm/java/java.c
plugins/disasm/arm/arm.o:     plugins/disasm/arm/arm.c
plugins/disasm/arm/arm16.o:   plugins/disasm/arm/arm16.c
plugins/disasm/arm/arm32.o:   plugins/disasm/arm/arm32.c
plugins/disasm/avr/avr.o:     plugins/disasm/avr/avr.c
plugins/disasm/ppc/ppc.o:     plugins/disasm/ppc/ppc.c
plugins/disasm/ix86/ix86.o:   plugins/disasm/ix86/ix86.c
plugins/disasm/ix86/ix86_fpu.o: plugins/disasm/ix86/ix86_fpu.c
plugins/disasm/ix86/ix86_fun.o: plugins/disasm/ix86/ix86_fun.c
biewlib/bbio.o:               biewlib/bbio.c
biewlib/biewlib.o:            biewlib/biewlib.c
biewlib/file_ini.o:           biewlib/file_ini.c
biewlib/pmalloc.o:            biewlib/pmalloc.c
biewlib/twin.o:               biewlib/twin.c
biewlib/tw_class.o:           biewlib/tw_class.c
biewlib/sysdep/$(MACHINE)/aclib.o:            biewlib/sysdep/$(MACHINE)/aclib.c
biewlib/sysdep/$(MACHINE)/cpu_info.o:         biewlib/sysdep/$(MACHINE)/cpu_info.c
biewlib/sysdep/$(MACHINE)/$(HOST)/fileio.o:   biewlib/sysdep/$(MACHINE)/$(HOST)/fileio.c
biewlib/sysdep/$(MACHINE)/$(HOST)/keyboard.o: biewlib/sysdep/$(MACHINE)/$(HOST)/keyboard.c
biewlib/sysdep/$(MACHINE)/$(HOST)/mmfio.o:    biewlib/sysdep/$(MACHINE)/$(HOST)/mmfio.c
biewlib/sysdep/$(MACHINE)/$(HOST)/mouse.o:    biewlib/sysdep/$(MACHINE)/$(HOST)/mouse.c
biewlib/sysdep/$(MACHINE)/$(HOST)/misc.o:     biewlib/sysdep/$(MACHINE)/$(HOST)/misc.c
biewlib/sysdep/$(MACHINE)/$(HOST)/nls.o:      biewlib/sysdep/$(MACHINE)/$(HOST)/nls.c
biewlib/sysdep/$(MACHINE)/$(HOST)/os_dep.o:   biewlib/sysdep/$(MACHINE)/$(HOST)/os_dep.c
biewlib/sysdep/$(MACHINE)/$(HOST)/timer.o:    biewlib/sysdep/$(MACHINE)/$(HOST)/timer.c
biewlib/sysdep/$(MACHINE)/$(HOST)/vio.o:      biewlib/sysdep/$(MACHINE)/$(HOST)/vio.c

ifeq ($(HOST),qnx)
biewlib/sysdep/ia32/qnx/cpu_info.o:           biewlib/sysdep/ia32/qnx/cpu_info.asm
	cc -c $< -o $@
biewlib/sysdep/ia32/qnx/3rto3s.o:             biewlib/sysdep/ia32/qnx/3rto3s.asm
	cc -c $< -o $@
biewlib/sysdep/ia32/qnx/3sto3r.o:             biewlib/sysdep/ia32/qnx/3sto3r.asm
	cc -c $< -o $@
endif

install:
ifeq ($(INSTALL),)
	@echo "*** 'install' utility was not found and you can't run automatic"
	@echo "*** installation. Please download 'fileutils' from ftp://ftp.gnu.org and"
	@echo "*** install them to have possibility perform autiomatic installation"
	@echo "*** of this project" 
	@exit 1
endif
	$(INSTALL) -D -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	$(INSTALL) -D -c -m 644 doc/biew.1 $(DESTDIR)$(PREFIX)/man/man1/biew.1
	$(INSTALL) -D -c -m 644 bin_rc/biew.hlp $(DESTDIR)$(DATADIR)/biew.hlp
	mkdir --parents $(DESTDIR)$(DATADIR)/skn
	$(INSTALL) -D -c -m 644 bin_rc/skn/*.skn $(DESTDIR)$(DATADIR)/skn
	mkdir --parents $(DESTDIR)$(DATADIR)/syntax
	$(INSTALL) -D -c -m 644 bin_rc/syntax/*.stx $(DESTDIR)$(DATADIR)/syntax
	mkdir --parents $(DESTDIR)$(DATADIR)/xlt/russian
	$(INSTALL) -D -c -m 644 bin_rc/xlt/russian/*.xlt $(DESTDIR)$(DATADIR)/xlt/russian
	$(INSTALL) -D -c -m 644 bin_rc/xlt/*.xlt bin_rc/xlt/readme $(DESTDIR)$(DATADIR)/xlt
uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	$(RM) $(DESTDIR)$(DATADIR)/skn/*
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(DATADIR)/skn
	$(RM) $(DESTDIR)$(DATADIR)/syntax/*
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(DATADIR)/syntax
	$(RM) $(DESTDIR)$(DATADIR)/xlt/russian/*
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(DATADIR)/xlt/russian
	$(RM) $(DESTDIR)$(DATADIR)/xlt/*
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(DATADIR)/xlt
	$(RM) $(DESTDIR)$(DATADIR)/*
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(DATADIR)

help:
	$(DO_HELP)
	$(RM) -f hlp/biewhlp
	$(RM) -f hlp/lzss
	$(LN) ../tools/biewhlp/biewhlp hlp/biewhlp
	$(LN) ../tools/lzss/lzss hlp/lzss
	($(CD) ./hlp && ./biewhlp biewhlp.prj && $(CD) .. )
	$(RM) -f ./bin_rc/biew.hlp
	$(CP) ./hlp/biew.hlp ./bin_rc
