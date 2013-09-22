/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <gp_Ax1.hxx>
#endif


#include "FeatureRevolution.h"
#include <Base/Tools.h>

using namespace Part;

App::PropertyFloatConstraint::Constraints Revolution::angleRangeU = {-360.0,360.0,1.0};

PROPERTY_SOURCE(Part::Revolution, Part::Feature)

Revolution::Revolution()
{
    ADD_PROPERTY(Source,(0));
    ADD_PROPERTY(Base,(Base::Vector3d(0.0,0.0,0.0)));
    ADD_PROPERTY(Axis,(Base::Vector3d(0.0,0.0,1.0)));
    ADD_PROPERTY(Angle,(360.0));
    Angle.setConstraints(&angleRangeU);
}

short Revolution::mustExecute() const
{
    if (Base.isTouched() ||
        Axis.isTouched() ||
        Angle.isTouched() ||
        Source.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Revolution::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *base = static_cast<Part::Feature*>(Source.getValue());

    Base::Vector3d b = Base.getValue();
    Base::Vector3d v = Axis.getValue();
    gp_Pnt pnt(b.x,b.y,b.z);
    gp_Dir dir(v.x,v.y,v.z);

    try {
        // Now, let's get the TopoDS_Shape
        TopoDS_Shape revolve = base->Shape.getShape().revolve(gp_Ax1(pnt, dir),
            Angle.getValue()/180.0f*M_PI);
        if (revolve.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        this->Shape.setValue(revolve);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}
