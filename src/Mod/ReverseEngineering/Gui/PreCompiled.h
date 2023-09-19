/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#pragma warning(disable : 4181)
#pragma warning(disable : 4267)
#pragma warning(disable : 4275)
#pragma warning(disable : 4305)
#pragma warning(disable : 4522)
#endif

#ifdef _PreComp_

// standard
#include <algorithm>
#include <sstream>

// OpenCasCade
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRep_Builder.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Plane.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>

// Qt Toolkit
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>

#endif  //_PreComp_

#endif  // __PRECOMPILED_GUI__
