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

#ifndef WEB_GLOBAL_H
#define WEB_GLOBAL_H


// Web
#ifndef WebExport
#ifdef Web_EXPORTS
#define WebExport FREECAD_DECL_EXPORT
#else
#define WebExport FREECAD_DECL_IMPORT
#endif
#endif

// WebGui
#ifndef WebGuiExport
#ifdef WebGui_EXPORTS
#define WebGuiExport FREECAD_DECL_EXPORT
#else
#define WebGuiExport FREECAD_DECL_IMPORT
#endif
#endif

#endif  // WEB_GLOBAL_H
