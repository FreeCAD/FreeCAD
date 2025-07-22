/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef _TEMPLATE__GLOBAL_H
#define _TEMPLATE__GLOBAL_H


// _TEMPLATE_
#ifndef _TEMPLATE_Export
#ifdef _TEMPLATE__EXPORTS
#define _TEMPLATE_Export FREECAD_DECL_EXPORT
#else
#define _TEMPLATE_Export FREECAD_DECL_IMPORT
#endif
#endif

// _TEMPLATE_Gui
#ifndef _TEMPLATE_GuiExport
#ifdef _TEMPLATE_Gui_EXPORTS
#define _TEMPLATE_GuiExport FREECAD_DECL_EXPORT
#else
#define _TEMPLATE_GuiExport FREECAD_DECL_IMPORT
#endif
#endif

#endif  //_TEMPLATE__GLOBAL_H
