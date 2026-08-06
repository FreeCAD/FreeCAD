// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <FCGlobal.h>

#pragma once

// Sketcher3D
#ifndef Sketcher3DExport
# ifdef Sketcher3D_EXPORTS
#  define Sketcher3DExport FREECAD_DECL_EXPORT
# else
#  define Sketcher3DExport FREECAD_DECL_IMPORT
# endif
#endif

// Sketcher3DGui
#ifndef Sketcher3DGuiExport
# ifdef Sketcher3DGui_EXPORTS
#  define Sketcher3DGuiExport FREECAD_DECL_EXPORT
# else
#  define Sketcher3DGuiExport FREECAD_DECL_IMPORT
# endif
#endif
