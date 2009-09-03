##############################################################################
#   MAKE_QNX.INC - this file is part of Binary vIEW project (BIEW)           #
##############################################################################
#   Copyrights:           1998, 2000 Nickols_K                               #
#   License:              See below                                          #
#   Author and developer: Andrew Golovnia                                    #
#   Requirement:          GNU make                                           #
#   Original file name:   makefile                                           #
####################### [ D e s c r i p t i o n ] ############################
#  This include file is script for make utility of GNU development system.   #
########################### [ L i c e n c e ] ################################
# The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.                    #
# All rights reserved. This software is redistributable under the licence    #
# given in the file "Licence.en" ("Licence.ru" in russian translation)       #
# distributed in the BIEW archive.                                           #
##############################################################################

##############################################################################
# ATTENTION!!! I use manual method configuring of BIEW project rather than   #
# 'configure' script technology, because 'configure' script does not work    #
# propertly on non *NIX systems (like Cygwin, EMX,...).                      #
##############################################################################
# Select QNX4 compiler. Valid values are:
# watcomc, gcc(developing in progress)
#-----------------------------------------------------------------------------
COMPILER=gcc

# Select Watcom C 10.6 patch. Valid values are:
# none, a, b
#-----------------------------------------------------------------------------
WATCOM_PATCH=none

# Happy hacking :)
##########################################################################
bad_os = no
bad_machine = no

ifeq ($(TARGET_OS),qnx4)
 HOST=qnx
 CDEFOS=-D__QNX4__ -D__OS_NAME__='"QNX4"' -D__OS__="qnx" -U__QNX__
 CDEFOS += -DHAVE_TERMINAL_OUT=1
 ifeq ($(WATCOM_PATCH),a)
  CDEFOS+=-D__WATCOMC_A__
 endif
 ifeq ($(WATCOM_PATCH),b)
  CDEFOS+=-D__WATCOMC_A__
  CDEFOS+=-D__WATCOMC_B__
 endif
 OSLDEF=
 OS_LIBS= -ltermlib -lunix -lphoton3r.lib
 ifeq ($(USE_MOUSE),n)
 else
  CDEFOS += -DHAVE_MOUSE
  OS_LIBS +=
 endif
endif
ifeq ($(TARGET_OS),qnx6)
 HOST=qnxnto
 CDEFOS=-D__QNX6__ -D_CURSES_ -D__OS_NAME__='"QNX6"' -D__OS__="qnxnto"\
  -U__NTO__ -U__QNX__
 OSLDEF=
 OS_LIBS= -lncurses
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
ifeq ($(TARGET_OS),qnx4)
 ifeq ($(COMPILER),watcomc)
  ifeq ($(TARGET_PLATFORM),i386)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32 -D__CPU_NAME__='"i386"' -D__CPU__=386\
           -D__DEFAULT_DISASM=0 -3 -fp3 -zp4
   OSLDEF=-mh
  endif
  ifeq ($(TARGET_PLATFORM),i486)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i486"' -D__CPU__=486\
           -D__DEFAULT_DISASM=0 -4 -fp3 -zp4
  endif
  ifeq ($(TARGET_PLATFORM),i586)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i586"' -D__CPU__=586\
           -D__DEFAULT_DISASM=0 -5 -fp5 -zp4
  endif
  ifeq ($(TARGET_PLATFORM),generic)
   MACHINE=generic
   CDEFSYS=-D__MACHINE__=generic  -D__CPU_NAME__='"generic"'\
           -D__DEFAULT_DISASM=1 -3 -fp3 -zp4
  endif
  ifndef MACHINE
  bad_machine = yes
  endif
  ifeq ($(MACHINE),ia32)
  CDEFSYS += -Osax 
   ifeq ($(compilation),advance)
    CDEFSYS += 
   endif
  endif

  RM=rm -f
  CC=cc
  LD=cc
  AR=ar -rcu
  RANLIB=touch
  LIBEXT=lib
  LIBPREFIX=
 endif

 ifeq ($(COMPILER),gcc)
  ifeq ($(TARGET_PLATFORM),i386)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32 -D__CPU_NAME__='"i386"' -D__CPU__=386\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -m386 -fno-builtin
  endif
  ifeq ($(TARGET_PLATFORM),i486)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i486"' -D__CPU__=486\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -m486
  endif
  ifeq ($(TARGET_PLATFORM),i586)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i586"' -D__CPU__=586\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=pentium -mcpu=pentium -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),i586mmx)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i586mmx"' -D__CPU__=586\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=pentium -mcpu=pentium -mmmx\
           -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),i686)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i686"' -D__CPU__=686\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=pentiumpro -mcpu=pentiumpro\
           -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),i686mmx)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i686mmx"' -D__CPU__=686\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=pentiumpro -mcpu=pentiumpro -mmmx\
           -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),k5)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"amdk5"' -D__CPU__=586\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=k5 -mcpu=k5 -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),k6)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"amdk6"' -D__CPU__=586\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=k6 -mcpu=k6 -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),athlon)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"athlon"' -D__CPU__=686\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=athlon -mcpu=athlon\
           -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),athlon_mmx)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"athlon"' -D__CPU__=686\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=athlon -mcpu=athlon -mmmx\
           -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),k6mmx)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"k6mmx"' -D__CPU__=586\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=k6 -mcpu=k6 -mmmx -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),6x86)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"cyrix6x86"' -D_CPU__=586\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=6x86 -mcpu=6x86 -mmmx -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),6x86mx)
   MACHINE=ia32
   CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"cyrix6x86mx"' -D__CPU__=586\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -march=6x86mx -mcpu=6x86mx -mmmx\
           -fschedule-insns2
  endif
  ifeq ($(TARGET_PLATFORM),i86)
   MACHINE=ia16
   CDEFSYS=-D__MACHINE__=ia16  -D__CPU_NAME__='"i86"' -D__CPU__=86\
           -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1
  endif
  ifeq ($(TARGET_PLATFORM),generic)
   MACHINE=generic
   CDEFSYS=-D__MACHINE__=generic  -D__CPU_NAME__='"generic"'\
           -D__DEFAULT_DISASM=1 -fno-builtin
  endif
  ifndef MACHINE
   bad_machine = yes
  endif
  ifeq ($(MACHINE),ia32)
   CDEFSYS += -fshort-enums -O2
   ifeq ($(compilation),advance)
    CDEFSYS += -mschedule-prologue -momit-leaf-frame-pointer -mmove
   endif
  endif

  RM=rm -f
  CC=gcc
  LD=gcc
  AR=ar -rcu
  RANLIB=touch
  LIBEXT=a
  LIBPREFIX=lib
 endif
endif

ifeq ($(TARGET_OS),qnx6)
 ifeq ($(TARGET_PLATFORM),i386)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32 -D__CPU_NAME__='"i386"' -D__CPU__=386\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-m386,-fno-builtin
 endif
 ifeq ($(TARGET_PLATFORM),i486)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i486"' -D__CPU__=486\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-m486
 endif
 ifeq ($(TARGET_PLATFORM),i586)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i586"' -D__CPU__=586\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=pentium,-mcpu=pentium,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),i586mmx)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i586mmx"' -D__CPU__=586\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=pentium,-mcpu=pentium,-mmmx,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),i686)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i686"' -D__CPU__=686\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=pentiumpro,-mcpu=pentiumpro,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),i686mmx)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"i686mmx"' -D__CPU__=686\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=pentiumpro,-mcpu=pentiumpro,-mmmx,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),k5)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"amdk5"' -D__CPU__=586\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=k5,-mcpu=k5,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),k6)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"amdk6"' -D__CPU__=586\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=k6,-mcpu=k6,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),athlon)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"athlon"' -D__CPU__=686\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=athlon,-mcpu=athlon,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),athlon_mmx)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"athlon"' -D__CPU__=686\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=athlon,-mcpu=athlon,-mmmx,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),k6mmx)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"k6mmx"' -D__CPU__=586\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=k6,-mcpu=k6,-mmmx,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),6x86)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"cyrix6x86"' -D_CPU__=586\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=6x86,-mcpu=6x86,-mmmx,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),6x86mx)
  MACHINE=ia32
  CDEFSYS=-D__MACHINE__=ia32  -D__CPU_NAME__='"cyrix6x86mx"' -D__CPU__=586\
          -D__DEFAULT_DISASM=0 -DCAN_COMPILE_X86_GAS=1 -Wc,-march=6x86mx,-mcpu=6x86mx,-mmmx,-fschedule-insns2
 endif
 ifeq ($(TARGET_PLATFORM),i86)
  MACHINE=ia16
  CDEFSYS=-D__MACHINE__=ia16  -D__CPU_NAME__='"i86"' -D__CPU__=86\
          -D__DEFAULT_DISASM=0 
 endif
 ifeq ($(TARGET_PLATFORM),generic)
  MACHINE=generic
  CDEFSYS=-D__MACHINE__=generic  -D__CPU_NAME__='"generic"'\
          -D__DEFAULT_DISASM=1 -Wc,-fno-builtin
 endif
 ifndef MACHINE
 bad_machine = yes
 endif
 ifeq ($(MACHINE),ia32)
 CDEFSYS += -O
  ifeq ($(compilation),advance)
  CDEFSYS += 
  endif
 endif

 RM=rm -f
 CC=qcc
 LD=qcc
 AR=ar -rcou
 RANLIB=ranlib
 LIBEXT=a
 LIBPREFIX=lib
endif

###########################################################################
#  CFLAGS set it for optimization                                         #
#  LDFLAGS set it for link method                                         #
###########################################################################
CFLAGS = -D__HAVE_PRAGMA_PACK__=1 $(CDEFOS) $(CDEFSYS) $(HOST_CFLAGS)
ifeq ($(findstring debug,$(compilation)),debug)
CFLAGS += -g
ifeq ($(compilation),max_debug)
CFLAGS += 
endif
else
CFLAGS += -DNDEBUG=1
endif
adv_warn =
adv_keys =
ifeq ($(compilation),advance)
CFLAGS += $(adv_keys) $(adv_warn)
endif
ifeq ($(compilation),master)
CFLAGS += $(adv_keys) $(adv_warn)
#-ansi
endif
LDFLAGS = $(OSLDEF) $(HOST_LDFLAGS)
ifeq ($(findstring debug,$(compilation)),debug)
LDFLAGS += -g
else
LDFLAGS +=
endif
ifeq ($(compilation),advance)
LDFLAGS +=
endif
