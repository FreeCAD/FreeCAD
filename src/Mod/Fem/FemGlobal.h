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

#ifndef FEM_GLOBAL_H
#define FEM_GLOBAL_H


// Fem
#ifndef FemExport
#ifdef Fem_EXPORTS
#  define FemExport      FREECAD_DECL_EXPORT
#else
#  define FemExport      FREECAD_DECL_IMPORT
#endif
#endif

// FemGui
#ifndef FemGuiExport
#ifdef FemGui_EXPORTS
#  define FemGuiExport   FREECAD_DECL_EXPORT
#else
#  define FemGuiExport   FREECAD_DECL_IMPORT
#endif
#endif

#endif //FEM_GLOBAL_H
