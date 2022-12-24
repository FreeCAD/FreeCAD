/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#ifndef PATHGUI_PRECOMPILED_H
#define PATHGUI_PRECOMPILED_H

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
# define PartExport    __declspec(dllimport)
# define PathExport    __declspec(dllimport)
# define PartGuiExport __declspec(dllexport)
# define PathGuiExport __declspec(dllexport)
#else // for Linux
# define PartExport
# define PathExport
# define PartGuiExport
# define PathGuiExport
#endif

#include <Standard_math.hxx>

#ifdef _MSC_VER
# pragma warning( disable : 4273 )
#endif

#ifdef _PreComp_

// boost
#include <boost/algorithm/string/replace.hpp>

// OCC
#include <TopExp_Explorer.hxx>

// Qt Toolkit
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

// all of Inventor
#include <Inventor/SbVec3f.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>

#endif //_PreComp_

#endif // PATHGUI_PRECOMPILED_H
