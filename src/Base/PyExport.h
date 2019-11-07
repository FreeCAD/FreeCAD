/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


/** \file PyExport.h
 *  \brief the python object export base class
 *  \author Juergen Riegel
 *  \version 0.1
 *  \date    5.2001
 */

#ifndef BASE_PYEXPORT_H
#define BASE_PYEXPORT_H

// (re-)defined in pyconfig.h
#if defined (_POSIX_C_SOURCE)
#   undef    _POSIX_C_SOURCE
#endif
#if defined (_XOPEN_SOURCE)
#   undef    _XOPEN_SOURCE
#endif

#include <Python.h>
#ifdef FC_OS_MACOSX
#undef toupper
#undef tolower
#undef isupper
#undef islower
#undef isspace
#undef isalpha
#undef isalnum
#endif

#endif // BASE_PYEXPORT_H
