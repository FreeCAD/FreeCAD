/***************************************************************************
 *   Copyright (c) 2013 WandererFan <wandererfan (at) gmail.com>           *
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
/***************************************************************************
 *  FreeType License (FTL) credit:                                         *
 *  Portions of this software are copyright (c) <1996-2011> The FreeType   *
 *  Project (www.freetype.org).  All rights reserved.                      *
 ***************************************************************************/


// Public header for FT2FC.cpp
#ifndef FT2FC_H
#define FT2FC_H
// public functions
PyObject* FT2FC(const Py_UNICODE *unichars,
                const size_t length,
                const char *FontPath,
                const char *FontName,
                const double stringheight,
                const double tracking);

PyObject* FT2FC(const Py_UNICODE *unichars,
                const size_t length,
                const char *FontSpec,
                const double stringheight,
                const double tracking);

#endif // FT2FC_H
