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

#ifndef __PRECOMPILED_GUI__
#define __PRECOMPILED_GUI__

#include <FCConfig.h>

// here get the warnings of too long specifiers disabled (needed for VC6)
#ifdef _MSC_VER
# pragma warning(disable : 4005)
# pragma warning(disable : 4251)
# pragma warning(disable : 4503)
# pragma warning(disable : 4786)  // specifier longer then 255 chars
#endif

#ifdef _PreComp_

// standard
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

// STL
#include <algorithm>
#include <bitset>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <vector>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// Inventor
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTexture2.h>
# include <Inventor/nodes/SoTextureCoordinate2.h>

// Qt Toolkit
#ifndef __QtAll__
# include <Gui/QtAll.h>
#endif

#endif //_PreComp_

#endif // __PRECOMPILED_GUI__
