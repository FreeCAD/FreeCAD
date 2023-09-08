/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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


#ifndef GUI_PRECOMPILED_H
#define GUI_PRECOMPILED_H

#include <FCConfig.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// Importing of App classes
#ifdef FC_OS_WIN32
# define SandboxAppExport __declspec(dllimport)
# define SandboxGuiExport __declspec(dllexport)
# define MeshExport       __declspec(dllimport)
# define PartExport       __declspec(dllimport)
# define AppPartExport    __declspec(dllimport)
#else // for Linux
# define SandboxAppExport
# define SandboxGuiExport
# define MeshExport
# define PartExport
# define AppPartExport
#endif

#ifdef _PreComp_

// standard
#include <cstdio>
#include <cassert>

// STL
#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

// Xerces
#include <xercesc/util/XercesDefs.hpp>

// Qt Toolkit
#include <qaction.h>
#include <qapplication.h>
#include <qmainwindow.h>
#include <qworkspace.h>

#endif  //_PreComp_

#endif // GUI_PRECOMPILED_H 
