##############################################################################
#   MAKEFILE.INC - this file is part of Binary vIEW project (BIEW)           #
##############################################################################
#   Copyrights:           1998, 2001 Nick Kurshev                            #
#   License:              See below                                          #
#   Author and developer: Nick Kurshev                                       #
#   Requirement:          GNU make                                           #
#   Original file name:   makefile                                           #
####################### [ D e s c r i p t i o n ] ############################
#  This include file is script for make utility of GNU development system.   #
########################### [ L i c e n c e ] ################################
# The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.                 #
# All rights reserved. This software is redistributable under the licence    #
# given in the file "Licence.en" ("Licence.ru" in russian translation)       #
# distributed in the BIEW archive.                                           #
##############################################################################

##############################################################################
# ATTENTION!!! I use manual method configuring of BIEW project rather than   #
# 'configure' script technology, because 'configure' script does not work    #
# propertly on non *NIX systems (like Cygwin, EMX,...).                      #
##############################################################################
# If you do not have GNU development system, modify RM CC LD variables       #
# Also, you will be able redefine it in system depended part                 #
##############################################################################
CONFIG_SHELL=/bin/sh -c
NUL=/dev/null
RM=rm -f
CC=gcc
LD=gcc
AR=ar -rcu
RANLIB=ranlib
LIBEXT=a
LIBPREFIX=lib

##############################################################################
# HOST determines host operating system                                      #
##############################################################################
# Please write name of any desired operating system if this name is present  #
# in biewlib/sysdep/$(MACHINE) as subfolder, or port BIEW for your OS        #
# manully and please bring to author your version (with changes list)        #
# NOTE: I use file name model: 8.3 + all character in lowercase +            #
#       no symlinks (project must be compatible with DOS compilers)          #
##############################################################################
bad_os = no
bad_machine = no

ifeq ($(TARGET_OS),dos)
 HOST=dos
 CDEFOS=-D__MSDOS__ -D__OS_NAME__='"DOS"' -D__OS__="dos" -Udos
 OSLDEF=--force-exe-suffix
 OS_LIBS=
endif
ifeq ($(TARGET_OS),os2)
 HOST=os2
 CDEFOS=-D__OS2__ -D__OS_NAME__='"OS/2"' -Zomf -D__OS__="os2" -Uos2
 # here I want to have static link of glibc instead emx.dll
 OSLDEF=-Zexe -Zsmall-conv -Zsys -Zomf -Zstack 1024
 OS_LIBS=
 AR=emxomfar -p32 rc
 RANLIB=echo
 LIBEXT=lib
 LIBPREFIX=
 CONFIG_SHELL=bash -c
 NUL=$nul
endif
ifeq ($(TARGET_OS),win32)
 HOST=win32
 # I named this as win32 because of exists Win386 by Watcom.
 # here I link crtdll.dll instead cygwin1.dll (it's safe)
 CDEFOS= -D__WIN32__ -D__OS_NAME__='"Win32"' -mno-cygwin -D__OS__="win32" -Uwin32
 OSLDEF= -mno-cygwin --force-exe-suffix
 OS_LIBS= -lwinmm
endif
ifeq ($(TARGET_OS),beos)
 HOST=beos
 CDEFOS=-D__BEOS__ -D__UNIX__ -D__OS_NAME__='"BeOS"' -D_VT100_ -D__OS__="beos" -Ubeos
 CDEFOS+=-DHAVE_TERMINAL_OUT=1 -DDATADIR='"/boot/home/config/lib"' -D__DISABLE_MMF -D__DISABLE_ASM
 OSLDEF=
 OS_LIBS=
endif
ifeq ($(TARGET_OS),linux)
 HOST=linux
 CDEFOS=-D__LINUX__ -D__UNIX__ -D__OS_NAME__='"Linux"' -D_VT100_ -D__OS__="linux" -Ulinux
 CDEFOS+=-DHAVE_TERMINAL_OUT=1
 OSLDEF=
 OS_LIBS=
 ifeq ($(USE_MOUSE),n)
 else
  CDEFOS += -DHAVE_MOUSE
  OS_LIBS += -lgpm
 endif
endif
ifeq ($(TARGET_OS),unix)
 HOST=unix
 CDEFOS = -D__UNIX__ -D__OS__="unix" -Uunix
 CDEFOS+=-DHAVE_TERMINAL_OUT=1
 OSLDEF=
 ifeq ($(TARGET_SCREEN_LIB),vt100)
  CDEFOS += -D_VT100_
  OSNAME=Unix/VT100
  ifeq ($(USE_MOUSE),y)
   OS_LIBS += -lgpm
  endif
 else
   ifeq ($(TARGET_SCREEN_LIB),slang)
     CDEFOS += -D_SLANG_
     OS_LIBS += -lslang
     OSNAME=Unix/Slang
   else
     CDEFOS += -D_CURSES_
     OS_LIBS += -lncurses	#-lcurses
     OSNAME=Unix/Curses
   endif
 endif

 ifeq ($(USE_MOUSE),y)
  CDEFOS += -DHAVE_MOUSE
 endif
  CDEFOS += -D__OS_NAME__='"$(OSNAME)"'
endif
ifndef HOST
bad_os = yes
endif
##############################################################################
# MACHINE determines hardware platform                                       #
##############################################################################
# WARNING!!!                                                                 #
# BIEW is designed for ia-16/ia-32 platforms and was not tested on non       #
# LITTLE ENDIAN platforms. Project also was not tested on non 16/32 bits     #
# systems. If you want you may port project by hand and please bring to      #
# author your version (with changes list)                                    #
##############################################################################
ifeq ($(TARGET_PLATFORM),i386)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32 -D__CPU_NAME__='"i386"' -D__CPU__=386\
         -D__DEFAULT_DISASM=0 -m386 -fno-builtin
endif
ifeq ($(TARGET_PLATFORM),i486)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i486"' -D__CPU__=486\
         -D__DEFAULT_DISASM=0 -m486
endif
ifeq ($(TARGET_PLATFORM),i586)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i586"' -D__CPU__=586\
         -D__DEFAULT_DISASM=0
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
endif
ifeq ($(TARGET_PLATFORM),i586mmx)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i586mmx"' -D__CPU__=586\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),i686)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i686"' -D__CPU__=686\
         -D__DEFAULT_DISASM=0
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=pentiumpro -mcpu=pentiumpro -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentiumpro -mcpu=pentiumpro"; else echo "-m486"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
endif
ifeq ($(TARGET_PLATFORM),i686mmx)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i686mmx"' -D__CPU__=686\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=pentiumpro -mcpu=pentiumpro -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentiumpro -mcpu=pentiumpro"; else echo "-m486"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),p3)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"p3"' -D__CPU__=686\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1 -DHAVE_MMX2=1 -DHAVE_SSE=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=pentiumpro -mcpu=pentiumpro -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentiumpro -mcpu=pentiumpro"; else echo "-m486"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
endif
ifeq ($(TARGET_PLATFORM),p3mmx)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"p3mmx"' -D__CPU__=686\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1 -DHAVE_MMX2=1 -DHAVE_SSE=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=pentiumpro -mcpu=pentiumpro -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentiumpro -mcpu=pentiumpro"; else echo "-m486"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),p4)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"p4"' -D__CPU__=686\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1 -DHAVE_MMX2=1 -DHAVE_SSE=1\
         -DHAVE_SSE2=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=pentiumpro -mcpu=pentiumpro -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentiumpro -mcpu=pentiumpro"; else echo "-m486"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
endif
ifeq ($(TARGET_PLATFORM),p4mmx)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"p4mmx"' -D__CPU__=686\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1 -DHAVE_MMX2=1 -DHAVE_SSE=1\
         -DHAVE_SSE2=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=pentiumpro -mcpu=pentiumpro -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentiumpro -mcpu=pentiumpro"; else echo "-m486"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),k5)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"amdk5"' -D__CPU__=586\
         -D__DEFAULT_DISASM=0
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=k5 -mcpu=k5 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=k5 -mcpu=k5"; else if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
endif
ifeq ($(TARGET_PLATFORM),k6)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"amdk6"' -D__CPU__=586\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=k6 -mcpu=k6 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=k6 -mcpu=k6"; else if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
endif
ifeq ($(TARGET_PLATFORM),k6mmx)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"k6mmx"' -D__CPU__=586\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=k6 -mcpu=k6 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=k6 -mcpu=k6"; else if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),k6_2)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"amdk6-2"' -D__CPU__=586\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1 -DHAVE_3DNOW=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=k6 -mcpu=k6 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=k6 -mcpu=k6"; else if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
endif
ifeq ($(TARGET_PLATFORM),k6_2mmx)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"k6-2mmx"' -D__CPU__=586\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1 -DHAVE_3DNOW=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=k6 -mcpu=k6 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=k6 -mcpu=k6"; else if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),athlon)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"athlon"' -D__CPU__=686\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1 -DHAVE_3DNOW=1 -DHAVE_3DNOWEX=1\
         -DHAVE_MMX2
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=athlon -mcpu=athlon -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=athlon -mcpu=athlon"; else if $(CC) -march=pentiumpro -mcpu=pentiumpro -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentiumpro -mcpu=pentiumpro"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
endif
ifeq ($(TARGET_PLATFORM),athlon_mmx)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"athlon-mmx"' -D__CPU__=686\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1 -DHAVE_3DNOW=1 -DHAVE_MMX2\
         -DHAVE_3DNOWEX=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=athlon -mcpu=athlon -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=athlon -mcpu=athlon"; else if $(CC) -march=pentiumpro -mcpu=pentiumpro -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentiumpro -mcpu=pentiumpro"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),6x86)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"cyrix6x86"' -D_CPU__=586\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=6x86 -mcpu=6x86 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=6x86 -mcpu=6x86"; else if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),6x86mx)
 MACHINE=ia32
 CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"cyrix6x86mx"' -D__CPU__=586\
         -D__DEFAULT_DISASM=0 -DHAVE_MMX=1
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -march=6x86mx -mcpu=6x86mx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=6x86mx -mcpu=6x86mx"; else if $(CC) -march=pentium -mcpu=pentium -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-march=pentium -mcpu=pentium"; else echo "-m486"; fi fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -fschedule-insns2 -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fschedule-insns2"; fi')
CDEFSYS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmmx -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmmx"; fi')
endif
ifeq ($(TARGET_PLATFORM),i86)
 MACHINE=ia16
 CDEFSYS=-D__MACHINE__=ia16  -D__CPU_NAME__='"i86"' -D__CPU__=86\
         -D__DEFAULT_DISASM=0
endif
ifeq ($(TARGET_PLATFORM),generic)
 MACHINE=generic
 CDEFSYS=-D__MACHINE__=generic  -D__CPU_NAME__='"generic"'\
         -D__DEFAULT_DISASM=1 -fno-builtin -D__DISABLE_ASM
endif
ifndef MACHINE
bad_machine = yes
endif
ifeq ($(MACHINE),ia32)
CDEFSYS += -fshort-enums
ifeq ($(compilation),advance)
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -mschedule-prologue -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mschedule-prologue"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -momit-leaf-frame-pointer -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-momit-leaf-frame-pointer"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -mmove -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-mmove"; fi')
endif
endif
###########################################################################
#  CFLAGS set it for optimization                                         #
#  LDFLAGS set it for link method                                         #
###########################################################################
CFLAGS = -Wall -D__HAVE_PRAGMA_PACK__=1 $(CDEFOS) $(CDEFSYS) $(HOST_CFLAGS)
###########################################################################
#  Autoconfigure section ;)                                               #
# This part was taken from Linux-2.4                                      #
###########################################################################
ifeq ($(compilation),advance)
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -fmove-all-movables -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fmove-all-movables"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -freduce-all-givs -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-freduce-all-givs"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -fexpensive-optimizations -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fexpensive-optimizations"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -fregmove -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-fregmove"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -foptimize-register-move -S -o $(NUL) -xc $(NUL); then echo "-foptimize-register-move"; fi')
endif
ifeq ($(findstring debug,$(compilation)),debug)
CFLAGS += -g3
# Was detected problem with pgcc so temporary disable those keys
#ifeq ($(findstring unix,$(TARGET_OS)),unix)
#CFLAGS += -p -ax
#endif
ifeq ($(compilation),max_debug)
CFLAGS += -fcheck-memory-usage -fstack-check
CDEFOS += -D__DISABLE_ASM -DDEBUG -UNDEBUG
endif
else
CFLAGS += -O2 -fomit-frame-pointer -DNDEBUG=1
endif
ifeq ($(compilation),advance)
CFLAGS += -W
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Winline -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Winline"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wuninitialized -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wuninitialized"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wcast-align -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wcast-align"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wswitch -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wswitch"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wshadow -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wshadow"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wbad-function-cast -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wbad-function-cast"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wmissing-noreturn -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wmissing-noreturn"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wcast-qual -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wcast-qual"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wchar-subscripts -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wchar-subscripts"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wcomment -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wcomment"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wformat -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wformat"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wimplicit-function-declaration -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wimplicit-function-declaration"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wimplicit-int -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wimplicit-int"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wmain -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wmain"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wmissing-braces -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wmissing-braces"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wmissing-prototypes -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wmissing-prototypes"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wmultichar -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wmultichar"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wnested-externs -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wnested-externs"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wparentheses -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wparentheses"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wpointer-arith -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wpointer-arith"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wsign-compare -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wsign-compare"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wunknown-pragmas -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wunknown-pragmas"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wstrict-prototypes -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wstrict-prototypes"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wtrigraphs -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wtrigraphs"; fi')
CFLAGS += $(shell $(CONFIG_SHELL) 'if $(CC) -Wwrite-strings -S -o $(NUL) -xc $(NUL) >$(NUL) 2>&1; then echo "-Wwrite-strings"; fi')
endif
LDFLAGS = $(OSLDEF) $(HOST_LDFLAGS)
ifeq ($(findstring debug,$(compilation)),debug)
LDFLAGS += -g
else
LDFLAGS += -s
endif
ifeq ($(compilation),advance)
LDFLAGS += --sort-common
endif
