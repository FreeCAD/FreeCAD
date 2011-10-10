#-----------------------------------------------------------------------------
# Makefile for ann2fig
#
# ANN: Approximate Nearest Neighbors
# Version: 1.1.1 08/04/06
#-----------------------------------------------------------------------------
# Copyright (c) 1997-2005 University of Maryland and Sunil Arya and
# David Mount.  All Rights Reserved.
# 
# This software and related documentation is part of the Approximate
# Nearest Neighbor Library (ANN).  This software is provided under
# the provisions of the Lesser GNU Public License (LGPL).  See the
# file ../ReadMe.txt for further information.
# 
# The University of Maryland (U.M.) and the authors make no
# representations about the suitability or fitness of this software for
# any purpose.  It is provided "as is" without express or implied
# warranty.
#-----------------------------------------------------------------------------
# Revision 0.1  03/04/98
#	Initial release
# Revision 1.1.1  08/04/06
#	Added copyright/license
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Basic definitions
#		BASEDIR		where include, src, lib, ... are
#		INCDIR		include directory
#		LIBDIR		library directory
#		BINDIR		bin directory
#		LDFLAGS		loader flags
#		ANNLIB		ANN library
#		OTHERLIB	other libraries
#-----------------------------------------------------------------------------

BASEDIR	= ..
INCDIR	= $(BASEDIR)/include
LIBDIR	= $(BASEDIR)/lib
BINDIR	= $(BASEDIR)/bin
LDFLAGS	= -L$(LIBDIR)
ANNLIBS	= -lANN
OTHERLIBS = -lm

#-----------------------------------------------------------------------------
# Some more definitions
#	ANN2FIG		name of executable
#-----------------------------------------------------------------------------

ANN2FIG = ann2fig
SOURCES = ann2fig.cpp
OBJECTS = $(SOURCES:.cpp=.o)

#-----------------------------------------------------------------------------
# Make the program
#-----------------------------------------------------------------------------

default: 
	@echo "Specify a target configuration"

targets: $(BINDIR)/$(ANN2FIG)

$(BINDIR)/$(ANN2FIG): $(OBJECTS)
	$(C++) $(OBJECTS) -o $(ANN2FIG) $(LDFLAGS) $(ANNLIBS) $(OTHERLIBS)
	mv $(ANN2FIG) $(BINDIR)

#-----------------------------------------------------------------------------
# configuration definitions
#-----------------------------------------------------------------------------

include ../Make-config

#-----------------------------------------------------------------------------
# Objects
#-----------------------------------------------------------------------------

ann2fig.o: ann2fig.cpp
	$(C++) -c -I$(INCDIR) ann2fig.cpp

#-----------------------------------------------------------------------------
# Cleaning
#-----------------------------------------------------------------------------

clean:
	-rm -f *.o core

realclean: clean
