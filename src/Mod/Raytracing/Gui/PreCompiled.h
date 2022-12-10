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

#ifndef __PRECOMPILED_GUI__
#define __PRECOMPILED_GUI__

#include <FCConfig.h>

#ifdef _MSC_VER
#   pragma warning(disable : 4005)
#endif

#ifdef _PreComp_

// STL
#include <sstream>
#include <vector>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// OpenCascade
#include <gp_Vec.hxx>
#include <Standard_Failure.hxx>

// Qt Toolkit
# include <QAction>
# include <QApplication>
# include <QDir>
# include <QFileInfo>
# include <QInputDialog>
# include <QMenu>
# include <QRegularExpression>
# include <QRegularExpressionMatch>

// Inventor
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoNode.h>

#endif //_PreComp_

#endif // __PRECOMPILED_GUI__ 
