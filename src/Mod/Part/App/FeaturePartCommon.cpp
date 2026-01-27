// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
#include <BRepCheck_Analyzer.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopTools_IndexedMapOfShape.hxx>


#include "FeaturePartCommon.h"
#include "TopoShapeOpCode.h"
#include "modelRefine.h"

#include <Base/ProgramVersion.h>


using namespace Part;

namespace Part
{
extern void throwIfInvalidIfCheckModel(const TopoDS_Shape& shape);
extern bool getRefineModelParameter();
}  // namespace Part

PROPERTY_SOURCE(Part::Common, Part::Boolean)

Common::Common() = default;

const char* Common::opCode() const
{
    return Part::OpCodes::Common;
}

BRepAlgoAPI_BooleanOperation* Common::makeOperation(const TopoDS_Shape& base, const TopoDS_Shape& tool) const
{
    // Let's call algorithm computing a section operation:
    return new FCBRepAlgoAPI_Common(base, tool);
}

// ----------------------------------------------------

PROPERTY_SOURCE(Part::MultiCommon, Part::Feature)

const char* MultiCommon::BehaviorEnums[] = {"CommonOfAllShapes", "CommonOfFirstAndRest", nullptr};

MultiCommon::MultiCommon()
{
    ADD_PROPERTY(Shapes, (nullptr));
    Shapes.setSize(0);
    ADD_PROPERTY_TYPE(
        History,
        (ShapeHistory()),
        "Boolean",
        (App::PropertyType)(App::Prop_Output | App::Prop_Transient | App::Prop_Hidden),
        "Shape history"
    );
    History.setSize(0);

    ADD_PROPERTY_TYPE(
        Refine,
        (0),
        "Boolean",
        (App::PropertyType)(App::Prop_None),
        "Refine shape (clean up redundant edges) after this boolean operation"
    );

    ADD_PROPERTY_TYPE(
        Behavior,
        (CommonOfAllShapes),
        "Compatibility",
        App::Prop_Hidden,
        "Determines how the common operation is computed: either as the intersection of all "
        "shapes, or the intersection of the first shape with all remaining shapes (for "
        "compatibility with FreeCAD 1.0)."
    );
    Behavior.setEnums(BehaviorEnums);

    this->Refine.setValue(getRefineModelParameter());
}

short MultiCommon::mustExecute() const
{
    if (Shapes.isTouched() || Behavior.isTouched()) {
        return 1;
    }
    return 0;
}

void MultiCommon::Restore(Base::XMLReader& reader)
{
    Feature::Restore(reader);

    // For 1.0 and 1.0 only the order was common of first and the rest due to a bug
    if (Base::getVersion(reader.ProgramVersion) == Base::Version::v1_0) {
        Behavior.setValue(CommonOfFirstAndRest);
    }
}

App::DocumentObjectExecReturn* MultiCommon::execute()
{
    std::vector<TopoShape> shapes;
    for (auto obj : Shapes.getValues()) {
        TopoShape sh = Feature::getTopoShape(obj, ShapeOption::ResolveLink | ShapeOption::Transform);
        if (sh.isNull()) {
            return new App::DocumentObjectExecReturn("Input shape is null");
        }
        shapes.push_back(sh);
    }

    TopoShape res;

    if (Behavior.getValue() == CommonOfAllShapes) {
        // special case - if there is only one argument, and it is compound - expand it
        if (shapes.size() == 1) {
            TopoShape shape = shapes.front();

            if (shape.shapeType() == TopAbs_COMPOUND) {
                shapes.clear();
                std::ranges::copy(shape.getSubTopoShapes(), std::back_inserter(shapes));
            }
        }

        res = shapes.front();

        // to achieve common of all shapes, we need to do it one shape at a time
        for (const auto& tool : shapes) {
            res = res.makeElementBoolean(OpCodes::Common, {res, tool});
        }
    }
    else {
        res = TopoShape(0);
        res.makeElementBoolean(OpCodes::Common, shapes);
    }

    if (res.isNull()) {
        throw Base::RuntimeError("Resulting shape is null");
    }

    throwIfInvalidIfCheckModel(res.getShape());

    if (this->Refine.getValue()) {
        res = res.makeElementRefine();
    }
    this->Shape.setValue(res);
    if (Shapes.getSize() > 0) {
        App::DocumentObject* link = Shapes.getValues()[0];
        copyMaterial(link);
    }

    return Part::Feature::execute();
}
