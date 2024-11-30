/***************************************************************************
 *   Copyright (c) 2019 Jean-Marie Verdun        jmverdun3@gmail.com       *
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

// Importing of App classes
#ifdef FC_OS_WIN32
#define CloudAppExport __declspec(dllimport)
#define CloudGuiExport __declspec(dllexport)
#else  // for Linux
#define CloudAppExport
#define CloudGuiExport
#endif

#ifdef _PreComp_

// standard
#include <cassert>
#include <cstdio>

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

#ifdef FC_OS_WIN32
#include <windows.h>
#endif

// Qt Toolkit
#ifndef __QtAll__
#include <Gui/QtAll.h>
#endif

#endif  //_PreComp_

#endif  // GUI_PRECOMPILED_H
