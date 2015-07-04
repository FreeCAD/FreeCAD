/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "DatumCS.h"
#include "DatumPoint.h"
#include "DatumPlane.h"
#include "DatumLine.h"
#include <App/Plane.h>
#include <App/Part.h>
#include <App/Line.h>
#include <Base/Exception.h>
#include <gp_Pln.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_Plane.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAbs_CurveType.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <gp_Quaternion.hxx>
#include <TopoDS_Vertex.hxx>
#include <QObject>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;


// ============================================================================

    
PROPERTY_SOURCE(PartDesign::CoordinateSystem, Part::Datum)

CoordinateSystem::CoordinateSystem()
{
    this->setAttacher(new AttachEngine3D);
    // Create a shape, which will be used by the Sketcher. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeFace builder(gp_Pln(gp_Pnt(0,0,0), gp_Dir(0,0,1)));
    if (!builder.IsDone())
        return;
    Shape.setValue(builder.Shape());
}

CoordinateSystem::~CoordinateSystem()
{
}

void CoordinateSystem::onChanged(const App::Property *prop)
{
    Part::Datum::onChanged(prop);
}

Base::Vector3d CoordinateSystem::getXAxis()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(1,0,0), normal);
    return normal;
}

Base::Vector3d CoordinateSystem::getYAxis()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(0,1,0), normal);
    return normal;
}
 
Base::Vector3d CoordinateSystem::getZAxis()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(0,0,1), normal);
    return normal;
}
