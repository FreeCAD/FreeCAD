// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


// Sketcher
#ifndef SketcherExport
# ifdef Sketcher_EXPORTS
#  define SketcherExport FREECAD_DECL_EXPORT
# else
#  define SketcherExport FREECAD_DECL_IMPORT
# endif
#endif

// SketcherGui
#ifndef SketcherGuiExport
# ifdef SketcherGui_EXPORTS
#  define SketcherGuiExport FREECAD_DECL_EXPORT
# else
#  define SketcherGuiExport FREECAD_DECL_IMPORT
# endif
#endif
