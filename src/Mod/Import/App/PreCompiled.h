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


#ifndef __PRECOMPILED__
#define __PRECOMPILED__

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
# define ImportExport  __declspec(dllexport)
# define PartExport    __declspec(dllimport)
# define AppPartExport __declspec(dllimport)
#else // for Linux
# define ImportExport
# define PartExport
# define AppPartExport
#endif


/// here get the warnings of to long specifieres disabled (needet for VC6)
#ifdef _MSC_VER
# pragma warning( disable : 4251 )
# pragma warning( disable : 4503 )
# pragma warning( disable : 4786 )  // specifier longer then 255 chars
#endif


#ifdef _PreComp_

// Python
#include <Python.h>

// standard
#include <list>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <io.h>
#include <fcntl.h>
#include <vector>
#include <map>

// Xerces
#include <xercesc/util/XercesDefs.hpp>

// OpenCasCade =====================================================================================
// Base
#include "OpenCascadeAll.h"

#endif //_PreComp_

#endif
