/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef MEASUREGUI_PRECOMPILED_H
#define MEASUREGUI_PRECOMPILED_H

#include <FCConfig.h>

#include <Mod/Measure/MeasureGlobal.h>

// point at which warnings of overly long specifiers disabled (needed for VC6)
#ifdef _MSC_VER
# pragma warning( disable : 4251 )
# pragma warning( disable : 4503 )
# pragma warning( disable : 4786 )  // specifier longer then 255 chars
# pragma warning( disable : 4273 )
#endif

#ifdef FC_OS_WIN32
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <windows.h>
#endif

#ifdef _PreComp_

// standard
#include <cfloat>
#include <cmath>

// STL
#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// OpenCasCade
#include <Mod/Part/App/OpenCascadeAll.h>

// Boost
#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>

// GL
// Include glext before QtAll/InventorAll
#ifdef FC_OS_WIN32
# include <GL/gl.h>
# include <GL/glext.h>
#else
# ifdef FC_OS_MACOSX
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
# else
#  ifndef GL_GLEXT_PROTOTYPES
#   define GL_GLEXT_PROTOTYPES 1
#  endif
#  include <GL/gl.h>
#  include <GL/glext.h>
# endif //FC_OS_MACOSX
#endif //FC_OS_WIN32
// Should come after glext.h to avoid warnings
#include <Inventor/C/glue/gl.h>

// Qt Toolkit
#ifndef __QtAll__
# include <Gui/QtAll.h>
#endif

// Inventor includes OpenGL
#ifndef __InventorAll__
# include <Gui/InventorAll.h>
#endif

#endif //_PreComp_

#endif

