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
HOST_CFLAGS=
HOST_LDFLAGS=

include ../../config.mak

CFLAGS = $(CDEFOS) $(CDEFSYS) $(HOST_CFLAGS)
LDFLAGS = $(OSLDEF) $(HOST_LDFLAGS)
###########################################################################
# TARGET: put name of executable image here                               #
###########################################################################
TARGET = biewhlp

##########################################################################
#                Please not modify contents below                        #
##########################################################################

INCS = -I. -I../../
LIBS = -L../../biewlib -lbiew $(OS_LIBS)
OBJS =\
biewhlp.o
all: $(TARGET)
help: $(TARGET)

distclean:
clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
	$(RM) biew.map

cleansys:
	$(RM) lib/sysdep/$(MACHINE)/$(HOST)/*.o

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

biewhlp.o:                     biewhlp.c
