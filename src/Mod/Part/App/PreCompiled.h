/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef PART_PRECOMPILED_H
#define PART_PRECOMPILED_H

#include <FCConfig.h>


// Exporting of App classes
#ifdef FC_OS_WIN32
# define PartExport __declspec(dllexport)
#else // for Linux
# define PartExport
#endif

// here get the warnings of too long specifiers disabled (needed for VC6)
#ifdef _MSC_VER
#	pragma warning( disable : 4251 )
#	pragma warning( disable : 4275 )
#	pragma warning( disable : 4503 )
#	pragma warning( disable : 4786 )  // specifier longer then 255 chars
#endif


#ifdef _PreComp_

// standard
#include <list>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <assert.h>

#include <vector>
#include <list>
#include <set>
#include <map>

// Boost
#include <boost/signals.hpp>
#include <boost/bind.hpp>

#include <boost/tuple/tuple.hpp>
#include <boost/utility.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <boost/program_options.hpp>
//namespace po = boost::program_options;

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>

#include "OpenCascadeAll.h"

#elif defined(FC_OS_WIN32)
#include <windows.h>
#endif //_PreComp_

#ifndef _Standard_Version_HeaderFile
#include <Standard_Version.hxx>
#endif

#if defined(OCC_VERSION_MAJOR) && defined(OCC_VERSION_MINOR) && defined(OCC_VERSION_MAINTENANCE)
#   define OCC_HEX_VERSION ((OCC_VERSION_MAJOR<<16)+(OCC_VERSION_MINOR<<8)+(OCC_VERSION_MAINTENANCE))
#else
#   define OCC_HEX_VERSION 0x050000 // use an old version
#endif


#endif
