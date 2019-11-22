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


#ifndef __PRECOMPILED_GUI__
#define __PRECOMPILED_GUI__

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
# define ReenExport    __declspec(dllimport)
# define ReenGuiExport __declspec(dllexport)
# define PartExport __declspec(dllimport)
# define MeshExport    __declspec(dllimport)
# define PointsExport __declspec(dllimport)
# define AppExport __declspec(dllimport)
#else // for Linux
# define ReenExport
# define ReenGuiExport
# define PartExport
# define MeshExport
# define PointsExport
# define AppExport
#endif

#ifdef _MSC_VER
#   pragma warning(disable : 4275)
#endif


#ifdef _PreComp_

// Python
#include <Python.h>

// standard
#include <iostream>
#include <assert.h>
#include <cmath>

// STL
#include <vector>
#include <map>
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <stack>
#include <queue>
#include <bitset>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif


// Qt Toolkit
#ifndef __Qt4All__
# include <Gui/Qt4All.h>
#endif

#endif //_PreComp_

#endif // __PRECOMPILED_GUI__
