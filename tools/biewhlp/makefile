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
# All rights reserved. This software is redistributable under the licence    #
# This software is redistributable under the licence given in the file       #
# "Licence.en" ("Licence.ru" in russian translation) distributed in the BIEW #
##############################################################################

##############################################################################
#  Main configure section of this makefile                                   #
##############################################################################

# Please select target platform. For detail see ../../makefile
TARGET_PLATFORM=i386

# Please select target operation system. For detail see ../../ makefile
TARGET_OS=unix

# For detail see ../../makefile
HOST_CFLAGS=

# For detail see ../../makefile
HOST_LDFLAGS=

# For detail see ../../makefile
TARGET_SCREEN_LIB = curses

# For detail see ../../makefile
USE_MOUSE=n

# For detail see ../../makefile
USE_SELECT=y

# For detail see ../../makefile
compilation=normal

ifeq ($(findstring qnx,$(TARGET_OS)),qnx)
include ../../make_qnx.inc
else
include ../../makefile.inc
endif

###########################################################################
# TARGET: put name of executable image here                               #
###########################################################################
TARGET = biewhlp

##########################################################################
#                Please not modify contents below                        #
##########################################################################

INCS = -I.
LIBS = -L./biewlib -lbiew $(OS_LIBS)
OBJS =\
biewhlp.o
all: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
	$(RM) biew.map

cleansys:
	$(RM) lib/sysdep/$(MACHINE)/$(HOST)/*.o

$(TARGET): $(OBJS)
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

biewhlp.o:                     biewhlp.c
