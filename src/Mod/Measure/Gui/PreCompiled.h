// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <FCConfig.h>

#include <Mod/Measure/MeasureGlobal.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// standard
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
# endif  // FC_OS_MACOSX
#endif   // FC_OS_WIN32
// Should come after glext.h to avoid warnings
#include <Inventor/C/glue/gl.h>

// Qt Toolkit
#include <Gui/QtAll.h>

// Inventor includes OpenGL
#include <Gui/InventorAll.h>
