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
#include <TopoDS.hxx>
#endif

#include "FeatureCut.h"


using namespace Surface;

PROPERTY_SOURCE(Surface::Cut, Part::Feature)

Cut::Cut()
{
    ADD_PROPERTY(ShapeList, (nullptr, "TopoDS_Shape"));
    ShapeList.setScope(App::LinkScope::Global);
}

// Check if any components of the surface have been modified

short Cut::mustExecute() const
{
    if (ShapeList.isTouched()) {
        return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn* Cut::execute()
{
    // Perform error checking

    try {
        std::vector<App::DocumentObject*> shapes = ShapeList.getValues();
        if (shapes.size() != 2) {
            return new App::DocumentObjectExecReturn(
                "Two shapes must be entered at a time for a cut operation");
        }

        Part::TopoShape ts1;
        Part::TopoShape ts2;

        // Get first toposhape
        if (shapes[0]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            ts1 = static_cast<Part::Feature*>(shapes[0])->Shape.getShape();  // Part::TopoShape 1
        }
        else {
            return new App::DocumentObjectExecReturn("Shape1 not from Part::Feature");
        }

        // Get second toposhape
        if (shapes[1]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            ts2 = static_cast<Part::Feature*>(shapes[1])->Shape.getShape();
        }
        else {
            return new App::DocumentObjectExecReturn("Shape2 not from Part::Feature");
        }

        // Cut Shape1 by Shape2
        TopoDS_Shape aCutShape;
        aCutShape = ts1.cut(ts2.getShape());

        // Check if resulting shell is null
        if (aCutShape.IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }

        this->Shape.setValue(aCutShape);
        return nullptr;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
