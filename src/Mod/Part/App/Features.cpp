/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepFill.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Shell.hxx>
#endif


#include "Features.h"


using namespace Part;

PROPERTY_SOURCE(Part::RuledSurface, Part::Feature)

RuledSurface::RuledSurface()
{
    ADD_PROPERTY_TYPE(Curve1,(0),"Ruled Surface",App::Prop_None,"Curve of ruled surface");
    ADD_PROPERTY_TYPE(Curve2,(0),"Ruled Surface",App::Prop_None,"Curve of ruled surface");
}

short RuledSurface::mustExecute() const
{
    if (Curve1.isTouched())
        return 1;
    if (Curve2.isTouched())
        return 1;
    return 0;
}

void RuledSurface::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn *RuledSurface::execute(void)
{
    App::DocumentObject* c1 = Curve1.getValue();
    if (!(c1 && c1->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
        return new App::DocumentObjectExecReturn("No shape linked.");
    const std::vector<std::string>& element1 = Curve1.getSubValues();
    if (element1.size() != 1)
        return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");
    App::DocumentObject* c2 = Curve2.getValue();
    if (!(c2 && c2->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
        return new App::DocumentObjectExecReturn("No shape linked.");
    const std::vector<std::string>& element2 = Curve2.getSubValues();
    if (element2.size() != 1)
        return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");

    const Part::TopoShape& shape1 = static_cast<Part::Feature*>(c1)->Shape.getValue();
    TopoDS_Shape curve1 = shape1.getSubShape(element1[0].c_str());
    if (curve1.IsNull()) curve1 = shape1._Shape;

    const Part::TopoShape& shape2 = static_cast<Part::Feature*>(c2)->Shape.getValue();
    TopoDS_Shape curve2 = shape2.getSubShape(element2[0].c_str());
    if (curve2.IsNull()) curve2 = shape2._Shape;

    try {
        if (curve1.IsNull() || curve2.IsNull())
            return new App::DocumentObjectExecReturn("Linked shapes are empty.");
        if (curve1.ShapeType() == TopAbs_EDGE && curve2.ShapeType() == TopAbs_EDGE) {
            TopoDS_Face face = BRepFill::Face(TopoDS::Edge(curve1), TopoDS::Edge(curve2));
            this->Shape.setValue(face);
        }
        else if (curve1.ShapeType() == TopAbs_WIRE && curve2.ShapeType() == TopAbs_WIRE) {
            TopoDS_Shell shell = BRepFill::Shell(TopoDS::Wire(curve1), TopoDS::Wire(curve2));
            this->Shape.setValue(shell);
        }
        else {
            return new App::DocumentObjectExecReturn("Curves must either be edges or wires.");
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}
