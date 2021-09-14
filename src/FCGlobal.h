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
/** \file FCGlobal.h
 *  \brief Include export or import macros.
 */


#ifndef FC_GLOBAL_H
#define FC_GLOBAL_H


#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(__CYGWIN__)
#  define FREECAD_DECL_EXPORT __declspec(dllexport)
#  define FREECAD_DECL_IMPORT __declspec(dllimport)
#else
#  define FREECAD_DECL_EXPORT
#  define FREECAD_DECL_IMPORT
#endif

// FreeCADBase
#ifdef FreeCADBase_EXPORTS
#  define BaseExport  FREECAD_DECL_EXPORT
#else
#  define BaseExport  FREECAD_DECL_IMPORT
#endif

// FreeCADApp
#ifdef FreeCADApp_EXPORTS
#       define AppExport   FREECAD_DECL_EXPORT
#       define DataExport  FREECAD_DECL_EXPORT
#else
#       define AppExport   FREECAD_DECL_IMPORT
#       define DataExport  FREECAD_DECL_IMPORT
#endif

// FreeCADGui
#ifdef FreeCADGui_EXPORTS
#  define GuiExport   FREECAD_DECL_EXPORT
#else
#  define GuiExport   FREECAD_DECL_IMPORT
#endif

#endif //FC_GLOBAL_H
