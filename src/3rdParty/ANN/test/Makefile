#-----------------------------------------------------------------------------
# Makefile for the test and evaluation program
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
# Note: For full performance measurements, it is assumed that the library
# and this program have both been compiled with the -DANN_PERF flag.  See
# the Makefile in the ANN base directory for this flag.
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Basic definitions
#		BASEDIR		where include, src, lib, ... are
#		INCDIR		include directory
#		LIBDIR		library directory
#		BINDIR		bin directory
#		LDFLAGS		loader flags
#		ANNLIBS		ANN library
#		OTHERLIBS	other libraries
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
#		ANNTEST		name of test program
#-----------------------------------------------------------------------------

ANNTEST = ann_test

HEADERS = rand.h
TESTSOURCES = ann_test.cpp rand.cpp
TESTOBJECTS = $(TESTSOURCES:.cpp=.o)

#-----------------------------------------------------------------------------
# Make the program
#-----------------------------------------------------------------------------

default: 
	@echo "Specify a target configuration"

targets: $(BINDIR)/$(ANNTEST)

$(BINDIR)/$(ANNTEST): $(TESTOBJECTS) $(LIBDIR)/$(ANNLIB)
	$(C++) $(TESTOBJECTS) -o $(ANNTEST) $(LDFLAGS) $(ANNLIBS) $(OTHERLIBS)
	mv $(ANNTEST) $(BINDIR)

#-----------------------------------------------------------------------------
# configuration definitions
#-----------------------------------------------------------------------------

include ../Make-config

#-----------------------------------------------------------------------------
# Objects
#-----------------------------------------------------------------------------

ann_test.o: ann_test.cpp
	$(C++) -c -I$(INCDIR) $(CFLAGS) ann_test.cpp

rand.o: rand.cpp
	$(C++) -c -I$(INCDIR) $(CFLAGS) rand.cpp

#-----------------------------------------------------------------------------
# Cleaning
#-----------------------------------------------------------------------------

clean:
	-rm -f *.o *.out core

realclean: clean
