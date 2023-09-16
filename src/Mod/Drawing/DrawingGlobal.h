/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef DRAWING_GLOBAL_H
#define DRAWING_GLOBAL_H


// Drawing
#ifndef DrawingExport
#ifdef Drawing_EXPORTS
#define DrawingExport FREECAD_DECL_EXPORT
#else
#define DrawingExport FREECAD_DECL_IMPORT
#endif
#endif

// DrawingGui
#ifndef DrawingGuiExport
#ifdef DrawingGui_EXPORTS
#define DrawingGuiExport FREECAD_DECL_EXPORT
#else
#define DrawingGuiExport FREECAD_DECL_IMPORT
#endif
#endif

#endif  // DRAWING_GLOBAL_H
