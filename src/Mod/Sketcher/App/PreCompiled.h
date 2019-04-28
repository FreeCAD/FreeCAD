/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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

// Exporting of App classes
#ifdef FC_OS_WIN32
# define SketcherExport     __declspec(dllexport)
# define PartExport         __declspec(dllimport)
# define PartDesignExport   __declspec(dllimport)
# define MeshExport         __declspec(dllimport)
#else // for Linux
# define SketcherExport
# define PartExport
# define PartDesignExport
# define MeshExport
#endif

#ifdef _PreComp_

// standard
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <bitset>

#include <Mod/Part/App/OpenCascadeAll.h>
// Other needed opencascade
# include <ShapeFix_Wire.hxx>


#include <Python.h>

#elif defined(FC_OS_WIN32)
#define NOMINMAX
#include <windows.h>
#endif // _PreComp_
#endif

