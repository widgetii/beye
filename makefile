##############################################################################
#   MAKEFILE - this file is part of Binary vIEW project (BIEW)               #
##############################################################################
#   Copyrights:           1998, 2000 Nick Kurshev                            #
#   License:              See below                                          #
#   Author and developer: Nick Kurshev                                       #
#   Requirement:          GNU make                                           #
#   Original file name:   makefile                                           #
####################### [ D e s c r i p t i o n ] ############################
#  This file is script for make utility of GNU development system.           #
########################### [ L i c e n c e ] ################################
# The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.                 #
# All rights reserved.                                                       #
# This software is redistributable under the licence given in the file       #
# "Licence" distributed in the BIEW archive.                                 #
##############################################################################

##############################################################################
#  Main configure section of this makefile                                   #
##############################################################################
#
# Please select target platform. Valid values are:
# For 16-bit Intel: i86 i286 ( still is not supported by gcc )
# For 32-bit Intel
#        basic    : i386 i486
#        gcc-2.9x : i586 i686 p3 p4 k6 k6_2 athlon
#        pgcc     : i586mmx i686mmx p3mmx p4mmx k5 k6mmx k6_2mmx 6x86 6x86mmx
#                   athlon_mmx
# Other platform  : generic
#-----------------------------------------------------------------------------
TARGET_PLATFORM=i386

# Please select target operation system. Valid values are:
# dos, os2, win32, linux, unix, beos, qnx4, qnx6
#---------------------------------------------------------
TARGET_OS=unix

# Please add any host specific flags here
# (like -fcall-used-R -fcall-saved-R -mrtd -mregparm=3 -mreg-alloc=  e.t.c ;-):
#------------------------------------------------------------------------------
# Notes: You can also define -D__EXPERIMENTAL_VERSION flag, if you want to
# build experimental version with fastcall technology.
# *****************************************************************************
# You can also define:
# -DHAVE_MMX     mostly common for all cpu since Pentium-MMX and compatible
# -DHAVE_MMX2    exists on K7+ and P3+
# -DHAVE_SSE     exists only on P3+
# -DHAVE_SSE2    exists only on P4+
# -DHAVE_3DNOW   exists only on AMD's K6-2+
# -DHAVE_3DNOWEX exists only on AMD's K7+
# These flags are implicitly defined when you select pgcc mmx optimization
# for corresponded CPUs. But you may want to disable mmx optimization for pgcc
# (for example if you don't trust pgcc or if you don't have pgcc), but
# explicitly enable it for inline assembler. Also for K6 CPU (not for K6-2)
# it would be better to disable pgcc mmx optimization and enable it for
# inline assembler (if you are interested in speed but not size).
# *****************************************************************************
# -D__DISABLE_ASM disables all inline assembly code.
# Try it if you have problems with compilation due to assembler errors.
# Note that it is not the same as specifying TARGET_PLATFORM=generic.
#------------------------------------------------------------------------------
HOST_CFLAGS=

# Please add any host specific linker flags here
#------------------------------------------------------------------------------
HOST_LDFLAGS=

###########################################################################
# Here comes Unix-specific configuration, see unix.txt for details.
# Please select screen library, valid values are:
# vt100, slang, curses (default)
#--------------------------------------------------------------------------
TARGET_SCREEN_LIB=curses

# Please select if you want to use mouse. Valid values are:
# n(default), y
#----------------------------------------------------------
USE_MOUSE=n

# You can set compilation level:
# max_debug  - To enable debugging, profiling, checking memory usage and more
# debug      - To enable debugging and profiling only
# normal     - Default for most platforms and gcc versions.
# advance    - Use it only with the latest gcc version.
#----------------------------------------------------------------------
compilation=normal

# Happy hacking :)
##########################################################################

ifeq ($(findstring qnx,$(TARGET_OS)),qnx)
include ./make_qnx.inc
else
include ./makefile.inc
endif

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
plugins/bin/bin.o\
plugins/bin/coff386.o\
plugins/bin/dos_sys.o\
plugins/bin/elf386.o\
plugins/bin/le.o\
plugins/bin/lmf.o\
plugins/bin/lx.o\
plugins/bin/mz.o\
plugins/bin/ne.o\
plugins/bin/nlm386.o\
plugins/bin/opharlap.o\
plugins/bin/pe.o\
plugins/bin/pharlap.o\
plugins/bin/rdoff.o\
plugins/bin/rdoff2.o\
plugins/binmode.o\
plugins/disasm.o\
plugins/hexmode.o\
plugins/textmode.o\
plugins/nls/russian.o\
plugins/disasm/null_da.o\
plugins/disasm/avr/avr.o\
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

ifeq ($(TARGET_OS),qnx4)
 ifeq ($(COMPILER),watcomc)
OBJS += \
biewlib/sysdep/$(MACHINE)/$(HOST)/cpu_info.o
 endif
 ifeq ($(COMPILER),gcc)
OBJS += \
biewlib/sysdep/$(MACHINE)/$(HOST)/3rto3s.o\
biewlib/sysdep/$(MACHINE)/$(HOST)/3sto3r.o
 endif
endif

all: $(BIEWLIB) $(TARGET)
biewlib: $(BIEWLIB)

clean:
	$(RM) $(OBJS)
	$(RM) $(BIEWLIB_OBJS)
	$(RM) $(TARGET)
	$(RM) $(BIEWLIB)
	$(RM) biew.map
	$(RM) *.err

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
ifeq ($(bad_os),yes)
	@echo Please select valid TARGET_OS
	@exit
endif
ifeq ($(bad_machine),yes)
	@echo Please select valid TARGET_MACHINE
	@exit
endif
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
plugins/bin/bin.o:            plugins/bin/bin.c
plugins/bin/coff386.o:        plugins/bin/coff386.c
plugins/bin/dos_sys.o:        plugins/bin/dos_sys.c
plugins/bin/elf386.o:         plugins/bin/elf386.c
plugins/bin/le.o:             plugins/bin/le.c
plugins/bin/lmf.o:            plugins/bin/lmf.c
plugins/bin/lx.o:             plugins/bin/lx.c
plugins/bin/mz.o:             plugins/bin/mz.c
plugins/bin/ne.o:             plugins/bin/ne.c
plugins/bin/nlm386.o:         plugins/bin/nlm386.c
plugins/bin/opharlap.o:       plugins/bin/opharlap.c
plugins/bin/pe.o:             plugins/bin/pe.c
plugins/bin/pharlap.o:        plugins/bin/pharlap.c
plugins/bin/rdoff.o:          plugins/bin/rdoff.c
plugins/bin/rdoff2.o:         plugins/bin/rdoff2.c
plugins/binmode.o:            plugins/binmode.c
plugins/disasm.o:             plugins/disasm.c
plugins/hexmode.o:            plugins/hexmode.c
plugins/textmode.o:           plugins/textmode.c
plugins/nls/russian.o:        plugins/nls/russian.c
plugins/disasm/null_da.o:     plugins/disasm/null_da.c
plugins/disasm/avr/avr.o:     plugins/disasm/avr/avr.c
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

ifeq ($(findstring qnx,$(TARGET_OS)),qnx)
biewlib/sysdep/ia32/qnx/cpu_info.o:           biewlib/sysdep/ia32/qnx/cpu_info.asm
	cc -c $< -o $@
biewlib/sysdep/ia32/qnx/3rto3s.o:             biewlib/sysdep/ia32/qnx/3rto3s.asm
	cc -c $< -o $@
biewlib/sysdep/ia32/qnx/3sto3r.o:             biewlib/sysdep/ia32/qnx/3sto3r.asm
	cc -c $< -o $@
endif

install:
	@echo Sorry! This operation should be performed manually for now!
	@exit
