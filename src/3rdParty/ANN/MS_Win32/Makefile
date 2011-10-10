#-----------------------------------------------------------------------------
# Makefile for Windows Versions.
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
# Revision 1.0  05/03/05
#	Initial release
# Revision 1.1.1  08/04/06
#	Added copyright/license
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# This is not used for compiling the dll. It is just used for cleaning
# things up for distribution.  For compilcation, open the Ann.sln
# solution file in Microsoft Windows Visual Studio.NET.
#-----------------------------------------------------------------------------

default:
	@echo "Enter one of the following:"
	@echo "  make clean                remove object files"
	@echo "  make realclean            remove library and executable files"
	@echo " "
	@echo "See file Makefile for other compilation options."

#-----------------------------------------------------------------------------
# Remove .o files and core files
#-----------------------------------------------------------------------------
clean:
	-rm -f -r ann2fig/Debug ann2fig/Release
	-rm -f -r dll/Debug dll/Release
	-rm -f -r sample/Debug sample/Release
	-rm -f -r test/Debug test/Release
	-rm -f Ann.ncb Ann.suo

#-----------------------------------------------------------------------------
# Remove everthing that can be remade
#-----------------------------------------------------------------------------
realclean:  clean
	-rm -f bin/*
