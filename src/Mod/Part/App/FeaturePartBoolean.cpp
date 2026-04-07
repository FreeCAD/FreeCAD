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

#include <FCConfig.h>

#include <memory>

#include <Mod/Part/App/FCBRepAlgoAPI_BooleanOperation.h>
#include <BOPAlgo_ArgumentAnalyzer.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Standard_Failure.hxx>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/ProgramVersion.h>

#include "FeaturePartBoolean.h"
#include "TopoShapeOpCode.h"
#include "modelRefine.h"


using namespace Part;

namespace Part
{
const char* shapeTypeName(TopAbs_ShapeEnum type)
{
    switch (type) {
        case TopAbs_COMPOUND:
            return "Compound";
        case TopAbs_COMPSOLID:
            return "CompSolid";
        case TopAbs_SOLID:
            return "Solid";
        case TopAbs_SHELL:
            return "Shell";
        case TopAbs_FACE:
            return "Face";
        case TopAbs_WIRE:
            return "Wire";
        case TopAbs_EDGE:
            return "Edge";
        case TopAbs_VERTEX:
            return "Vertex";
        default:
            return "Shape";
    }
}

void throwIfInvalidIfCheckModel(const TopoDS_Shape& shape)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part/Boolean");

    if (hGrp->GetBool("CheckModel", true)) {
        BRepCheck_Analyzer aChecker(shape);
        if (!aChecker.IsValid()) {
            throw Base::RuntimeError("Resulting shape is invalid");
        }
    }
}

bool getRefineModelParameter()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part/Boolean");
    return hGrp->GetBool("RefineModel", true);
}

bool getCheckRefineParameter()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part/Boolean");
    return hGrp->GetBool("CheckRefine", false);
}

bool refineResultIsValid(const TopoDS_Shape& shape, bool checkRefine)
{
    if (!checkRefine) {
        return true;  // validation disabled, assume valid
    }

    // Run BOPAlgo_ArgumentAnalyzer to detect self-intersections that
    // BRepCheck_Analyzer misses.  This catches corruption introduced by
    // ShapeUpgrade_UnifySameDomain (removeSplitter/Refine) on shapes with
    // coplanar faces and partial overlap.
    TopoDS_Shape copy = BRepBuilderAPI_Copy(shape).Shape();
    BOPAlgo_ArgumentAnalyzer checker;
    checker.SetShape1(copy);
    checker.SelfInterMode() = true;
    checker.SetRunParallel(true);
    checker.Perform();
    return !checker.HasFaulty();
}

}  // namespace Part

PROPERTY_SOURCE_ABSTRACT(Part::Boolean, Part::Feature)


Boolean::Boolean()
{
    ADD_PROPERTY(Base, (nullptr));
    ADD_PROPERTY(Tool, (nullptr));
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
        CheckRefine,
        (false),
        "Boolean",
        (App::PropertyType)(App::Prop_None),
        "Validate refine result and revert if it introduces self-intersections (slower)"
    );

    this->Refine.setValue(getRefineModelParameter());
    this->CheckRefine.setValue(getCheckRefineParameter());
}

short Boolean::mustExecute() const
{
    if (Base.getValue() && Tool.getValue()) {
        if (Base.isTouched()) {
            return 1;
        }
        if (Tool.isTouched()) {
            return 1;
        }
    }
    return 0;
}

const char* Boolean::opCode() const
{
    return Part::OpCodes::Boolean;
}

App::DocumentObjectExecReturn* Boolean::execute()
{
    try {
#if defined(__GNUC__) && defined(FC_OS_LINUX)
        Base::SignalException se;
#endif
        auto base = Base.getValue();
        auto tool = Tool.getValue();

        if (!base || !tool) {
            return new App::DocumentObjectExecReturn("Linked object is not a Part object");
        }
        std::vector<TopoShape> shapes;
        shapes.reserve(2);
        // Now, let's get the TopoDS_Shape
        shapes.push_back(
            Feature::getTopoShape(Base.getValue(), ShapeOption::ResolveLink | ShapeOption::Transform)
        );
        auto BaseShape = shapes[0].getShape();
        if (BaseShape.IsNull()) {
            throw NullShapeException("Base shape is null");
        }
        shapes.push_back(
            Feature::getTopoShape(Tool.getValue(), ShapeOption::ResolveLink | ShapeOption::Transform)
        );
        auto ToolShape = shapes[1].getShape();
        if (ToolShape.IsNull()) {
            throw NullShapeException("Tool shape is null");
        }

        std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool(makeOperation(BaseShape, ToolShape));
        if (!mkBool->IsDone()) {
            std::stringstream error;
            error << "Boolean operation failed";
            error << std::endl << std::endl;
            error << "This is usually caused by a limitation in the geometry "
                     "engine, not a problem with your model. Faces that are "
                     "exactly aligned, nearly touching, or in complex coplanar "
                     "arrangements can be difficult for the engine to process.";
            error << std::endl << std::endl;
            error << "Things to try:" << std::endl;
            error << "  - Reposition one shape slightly (e.g. add 0.01 mm "
                     "in Placement)"
                  << std::endl;
            error << "  - Combine shapes in a different order" << std::endl;
            error << "  - Check each input with Part > Check Geometry";
            error << std::endl << std::endl;
            error << "Input types: '" << base->Label.getValue() << "' ("
                  << shapeTypeName(BaseShape.ShapeType()) << "), '" << tool->Label.getValue()
                  << "' (" << shapeTypeName(ToolShape.ShapeType()) << ")";
            error << std::endl;
            error << "See: https://wiki.freecad.org/Boolean_Troubleshooting";
            return new App::DocumentObjectExecReturn(error.str());
        }
        TopoDS_Shape resShape = mkBool->Shape();
        if (resShape.IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }

        throwIfInvalidIfCheckModel(resShape);

        TopoShape res(0);
        res.makeElementShape(*mkBool, shapes, opCode());
        if (this->Refine.getValue()) {
            TopoShape preRefine = res;
            res = res.makeElementRefine();
            if (!refineResultIsValid(res.getShape(), this->CheckRefine.getValue())) {
                res = preRefine;
                Base::Console().warning(
                    "'%s': The boolean result is correct, but the Refine "
                    "(cleanup) step damaged it and was skipped. The result "
                    "may have extra internal edges. To prevent this, disable "
                    "Refine in this feature's properties. This is a known "
                    "limitation of the geometry engine. "
                    "See: https://wiki.freecad.org/Boolean_Troubleshooting\n",
                    this->Label.getValue()
                );
            }
        }
        this->Shape.setValue(res);
        copyMaterial(base);
        return Part::Feature::execute();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(
            "A fatal error occurred when running boolean operation"
        );
    }
}

void Boolean::Restore(Base::XMLReader& reader)
{
    ExtensionContainer::Restore(reader);

    // The Refine property was added in FreeCAD 0.17, so any file before that will not have it set.
    // For these files, the appropriate default value is false.
    if (Base::getVersion(reader.ProgramVersion) < Base::Version::v0_17) {
        Refine.setValue(false);
    }
}
