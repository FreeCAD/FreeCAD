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
# include <BRepBuilderAPI_MakeFace.hxx>
# include <gp_Pln.hxx>
#endif

#include <Mod/Part/App/PartPyCXX.h>
#include <Mod/Part/App/OCCError.h>
#include "DatumCS.h"

using namespace PartDesign;
using namespace Attacher;


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

App::DocumentObject *CoordinateSystem::getSubObject(const char *subname, 
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int) const
{
    if(pmat && transform)
        *pmat *= Placement.getValue().toMatrix();

    if(!pyObj)
        return const_cast<CoordinateSystem*>(this);

    gp_Dir dir(0,0,1);
    if(subname) {
        if(strcmp(subname,"X")==0)
            dir = gp_Dir(1,0,0);
        else if(strcmp(subname,"Y")==0)
            dir = gp_Dir(0,1,0);
    }
    PY_TRY {
        BRepBuilderAPI_MakeFace builder(gp_Pln(gp_Pnt(0,0,0), dir));
        Part::TopoShape ts(builder.Shape());
        if(pmat)
            ts.transformShape(*pmat,false,true);
        *pyObj =  Py::new_reference_to(Part::shape2pyshape(ts));
        return const_cast<CoordinateSystem*>(this);
    } PY_CATCH_OCC
}

