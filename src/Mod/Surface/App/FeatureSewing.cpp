/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller <Nathan.A.Mill[at]gmail.com>         *
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
#include <BRepBuilderAPI_Sewing.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#endif

#include "FeatureSewing.h"


using namespace Surface;

PROPERTY_SOURCE(Surface::Sewing, Part::Feature)

// Initial values

Sewing::Sewing()
{
    ADD_PROPERTY_TYPE(ShapeList, (nullptr, ""), "Sewing", App::Prop_None, "Input shapes");
    ADD_PROPERTY_TYPE(Tolerance,
                      (Precision::Confusion()),
                      "Sewing",
                      App::Prop_None,
                      "Sewing tolerance");
    ADD_PROPERTY_TYPE(SewingOption, (true), "Sewing", App::Prop_None, "Sewing option");
    ADD_PROPERTY_TYPE(DegenerateShape,
                      (true),
                      "Sewing",
                      App::Prop_None,
                      "Analysis of degenerated shapes");
    ADD_PROPERTY_TYPE(CutFreeEdges, (true), "Sewing", App::Prop_None, "Cutting of free edges");
    ADD_PROPERTY_TYPE(Nonmanifold, (false), "Sewing", App::Prop_None, "Non-manifold processing");

    ShapeList.setScope(App::LinkScope::Global);
}

short Sewing::mustExecute() const
{
    if (ShapeList.isTouched() || Tolerance.isTouched() || SewingOption.isTouched()
        || DegenerateShape.isTouched() || CutFreeEdges.isTouched() || Nonmanifold.isTouched()) {
        return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn* Sewing::execute()
{
    // Assign Variables
    double atol = Tolerance.getValue();
    bool opt1 = SewingOption.getValue();
    bool opt2 = DegenerateShape.getValue();
    bool opt3 = CutFreeEdges.getValue();
    bool opt4 = Nonmanifold.getValue();

    try {
        BRepBuilderAPI_Sewing builder(atol, opt1, opt2, opt3, opt4);

        std::vector<App::PropertyLinkSubList::SubSet> subset = ShapeList.getSubListValues();
        for (const auto& it : subset) {
            // the subset has the documentobject and the element name which belongs to it,
            // in our case for example the cube object and the "Edge1" string
            if (it.first->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                // we get the shape of the document object which resemble the whole box
                Part::TopoShape ts = static_cast<Part::Feature*>(it.first)->Shape.getShape();

                // we want only the subshape which is linked
                for (const auto& jt : it.second) {
                    TopoDS_Shape sub = ts.getSubShape(jt.c_str());
                    builder.Add(sub);
                }
            }
            else {
                Standard_Failure::Raise("Shape item not from Part::Feature");
            }
        }

        builder.Perform();  // Perform Sewing

        TopoDS_Shape aShape = builder.SewedShape();  // Get Shape
        if (aShape.IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }
        this->Shape.setValue(aShape);
        return StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
