/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepFilletAPI_MakeFillet.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include <Mod/Part/App/TopoShape.h>

#include "FeatureFillet.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Fillet, PartDesign::DressUp)

const App::PropertyFloatConstraint::Constraints floatRadius = {0.0,FLT_MAX,0.1};

Fillet::Fillet()
{
    ADD_PROPERTY(Radius,(1.0));
    Radius.setConstraints(&floatRadius);
}

short Fillet::mustExecute() const
{
    if (Placement.isTouched() || Radius.isTouched())
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Fillet::execute(void)
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *base = static_cast<Part::Feature*>(Base.getValue());
    const Part::TopoShape& TopShape = base->Shape.getShape();
    if (TopShape._Shape.IsNull())
        return new App::DocumentObjectExecReturn("Cannot fillet invalid shape");

    const std::vector<std::string>& SubVals = Base.getSubValuesStartsWith("Edge");
    if (SubVals.size() == 0)
        return new App::DocumentObjectExecReturn("No edges specified");

    double radius = Radius.getValue();

    this->positionByBase();
    // create an untransformed copy of the base shape
    Part::TopoShape baseShape(TopShape);
    baseShape.setTransform(Base::Matrix4D());
    try {
        BRepFilletAPI_MakeFillet mkFillet(baseShape._Shape);

        for (std::vector<std::string>::const_iterator it=SubVals.begin(); it != SubVals.end(); ++it) {
            TopoDS_Edge edge = TopoDS::Edge(baseShape.getSubShape(it->c_str()));
            mkFillet.Add(radius, edge);
        }

        mkFillet.Build();
        if (!mkFillet.IsDone())
            return new App::DocumentObjectExecReturn("Failed to create fillet");

        TopoDS_Shape shape = mkFillet.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        this->Shape.setValue(shape);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}
