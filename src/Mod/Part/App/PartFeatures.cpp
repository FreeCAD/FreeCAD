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
#include <memory>
#include <BRepAdaptor_CompCurve.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepFill.hxx>
#include <BRepLib_MakeWire.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Precision.hxx>
#include <ShapeAnalysis.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shell.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include <App/Link.h>

#include <App/Document.h>
#include "PartFeatures.h"
#include "TopoShapeOpCode.h"

using namespace Part;

PROPERTY_SOURCE(Part::RuledSurface, Part::Feature)

const char* RuledSurface::OrientationEnums[] = {"Automatic", "Forward", "Reversed", nullptr};

RuledSurface::RuledSurface()
{
    ADD_PROPERTY_TYPE(Curve1, (nullptr), "Ruled Surface", App::Prop_None, "Curve of ruled surface");
    ADD_PROPERTY_TYPE(Curve2, (nullptr), "Ruled Surface", App::Prop_None, "Curve of ruled surface");
    ADD_PROPERTY_TYPE(Orientation,
                      ((long)0),
                      "Ruled Surface",
                      App::Prop_None,
                      "Orientation of ruled surface");
    Orientation.setEnums(OrientationEnums);
}

short RuledSurface::mustExecute() const
{
    if (Curve1.isTouched()) {
        return 1;
    }
    if (Curve2.isTouched()) {
        return 1;
    }
    if (Orientation.isTouched()) {
        return 1;
    }
    return 0;
}

void RuledSurface::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn* RuledSurface::getShape(const App::PropertyLinkSub& link,
                                                      TopoDS_Shape& shape) const
{
    App::DocumentObject* obj = link.getValue();
    const Part::TopoShape part = Part::Feature::getTopoShape(obj, ShapeOption::ResolveLink | ShapeOption::Transform);
    if (part.isNull()) {
        return new App::DocumentObjectExecReturn("No shape linked.");
    }

    // if no explicit sub-shape is selected use the whole part
    const std::vector<std::string>& element = link.getSubValues();
    if (element.empty()) {
        shape = part.getShape();
        return nullptr;
    }
    else if (element.size() != 1) {
        return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");
    }

    if (!part.getShape().IsNull()) {
        if (!element[0].empty()) {
            // shape = Part::Feature::getTopoShape(obj, element[0].c_str(), true /*need
            // element*/).getShape();
            shape = part.getSubShape(element[0].c_str());
        }
        else {
            // the sub-element is an empty string, so use the whole part
            shape = part.getShape();
        }
    }

    return nullptr;
}

App::DocumentObjectExecReturn* RuledSurface::execute()
{
    try {
        std::vector<TopoShape> shapes;
        std::array<App::PropertyLinkSub*, 2> links = {&Curve1, &Curve2};
        for (auto link : links) {
            App::DocumentObject* obj = link->getValue();
            const Part::TopoShape part = Part::Feature::getTopoShape(obj, ShapeOption::ResolveLink | ShapeOption::Transform);
            if (part.isNull()) {
                return new App::DocumentObjectExecReturn("No shape linked.");
            }
            // if no explicit sub-shape is selected use the whole part
            const auto& subs = link->getSubValues();
            if (subs.empty()) {
                shapes.push_back(part);
            }
            else if (subs.size() != 1) {
                return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");
            }
            else {
                shapes.push_back(getTopoShape(link->getValue(),
                                                 ShapeOption::NeedSubElement
                                               | ShapeOption::ResolveLink
                                               | ShapeOption::Transform,
                                              subs.front().c_str()));
            }
            if (shapes.back().isNull()) {
                return new App::DocumentObjectExecReturn("Invalid link.");
            }
        }
        TopoShape res(0);
        res.makeElementRuledSurface(shapes, Orientation.getValue());
        this->Shape.setValue(res);
        return Part::Feature::execute();

    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("General error in RuledSurface::execute()");
    }
}

// ----------------------------------------------------------------------------

App::PropertyIntegerConstraint::Constraints Loft::Degrees = {2,
                                                             Geom_BSplineSurface::MaxDegree(),
                                                             1};

PROPERTY_SOURCE(Part::Loft, Part::Feature)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections, (nullptr), "Loft", App::Prop_None, "List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Solid, (true), "Loft", App::Prop_None, "Create solid");
    ADD_PROPERTY_TYPE(Ruled, (false), "Loft", App::Prop_None, "Ruled surface");
    ADD_PROPERTY_TYPE(Closed, (false), "Loft", App::Prop_None, "Close Last to First Profile");
    ADD_PROPERTY_TYPE(MaxDegree, (5), "Loft", App::Prop_None, "Maximum Degree");
    ADD_PROPERTY_TYPE(Linearize,(false), "Loft", App::Prop_None,
                      "Linearize the result shape by simplifying linear edge and planar face into line and plane");
    MaxDegree.setConstraints(&Degrees);
}

short Loft::mustExecute() const
{
    if (Sections.isTouched()) {
        return 1;
    }
    if (Solid.isTouched()) {
        return 1;
    }
    if (Ruled.isTouched()) {
        return 1;
    }
    if (Closed.isTouched()) {
        return 1;
    }
    if (MaxDegree.isTouched()) {
        return 1;
    }
    return 0;
}

void Loft::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn* Loft::execute()
{
    if (Sections.getSize() == 0) {
        return new App::DocumentObjectExecReturn("No sections linked.");
    }

    try {
        std::vector<TopoShape> shapes;
        for (auto& obj : Sections.getValues()) {
            shapes.emplace_back(getTopoShape(obj, ShapeOption::ResolveLink | ShapeOption::Transform));
            if (shapes.back().isNull()) {
                return new App::DocumentObjectExecReturn("Invalid section link");
            }
        }
        IsSolid isSolid = Solid.getValue() ? IsSolid::solid : IsSolid::notSolid;
        IsRuled isRuled = Ruled.getValue() ? IsRuled::ruled : IsRuled::notRuled;
        IsClosed isClosed = Closed.getValue() ? IsClosed::closed : IsClosed::notClosed;
        int degMax = MaxDegree.getValue();
        TopoShape result(0);
        result.makeElementLoft(shapes, isSolid, isRuled, isClosed, degMax);
        if (Linearize.getValue()) {
            result.linearize( LinearizeFace::linearizeFaces, LinearizeEdge::noEdges);
        }
        this->Shape.setValue(result);
        return Part::Feature::execute();
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

// ----------------------------------------------------------------------------

const char* Part::Sweep::TransitionEnums[] = {"Transformed",
                                              "Right corner",
                                              "Round corner",
                                              nullptr};

PROPERTY_SOURCE(Part::Sweep, Part::Feature)

Sweep::Sweep()
{
    ADD_PROPERTY_TYPE(Sections, (nullptr), "Sweep", App::Prop_None, "List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Spine, (nullptr), "Sweep", App::Prop_None, "Path to sweep along");
    ADD_PROPERTY_TYPE(Solid, (true), "Sweep", App::Prop_None, "Create solid");
    ADD_PROPERTY_TYPE(Frenet, (true), "Sweep", App::Prop_None, "Frenet");
    ADD_PROPERTY_TYPE(Transition, (long(1)), "Sweep", App::Prop_None, "Transition mode");
    ADD_PROPERTY_TYPE(Linearize,(false), "Sweep", App::Prop_None,
                      "Linearize the result shape by simplifying linear edge and planar face into line and plane");
    Transition.setEnums(TransitionEnums);
}

short Sweep::mustExecute() const
{
    if (Sections.isTouched()) {
        return 1;
    }
    if (Spine.isTouched()) {
        return 1;
    }
    if (Solid.isTouched()) {
        return 1;
    }
    if (Frenet.isTouched()) {
        return 1;
    }
    if (Transition.isTouched()) {
        return 1;
    }
    return 0;
}

void Sweep::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn* Sweep::execute()
{
    if (Sections.getSize() == 0) {
        return new App::DocumentObjectExecReturn("No sections linked.");
    }
    if (!Spine.getValue()) {
        return new App::DocumentObjectExecReturn("No spine");
    }
    TopoShape spine = getTopoShape(Spine.getValue(), ShapeOption::ResolveLink | ShapeOption::Transform);
    const auto& subs = Spine.getSubValues();
    if (spine.isNull()) {
        return new App::DocumentObjectExecReturn("Invalid spine");
    }
    if (subs.size()) {
        std::vector<TopoShape> spineShapes;
        for (auto sub : subs) {
            auto shape = spine.getSubTopoShape(sub.c_str());
            if (shape.isNull()) {
                return new App::DocumentObjectExecReturn("Invalid spine");
            }
            spineShapes.push_back(shape);
        }
        spine = TopoShape().makeElementCompound(spineShapes, 0, TopoShape::SingleShapeCompoundCreationPolicy::returnShape);
    }
    std::vector<TopoShape> shapes;
    shapes.push_back(spine);
    for (auto& obj : Sections.getValues()) {
        shapes.emplace_back(getTopoShape(obj, ShapeOption::ResolveLink | ShapeOption::Transform));
        if (shapes.back().isNull()) {
            return new App::DocumentObjectExecReturn("Invalid section link");
        }
    }
    MakeSolid isSolid = Solid.getValue() ? MakeSolid::makeSolid : MakeSolid::noSolid;
    Standard_Boolean isFrenet = Frenet.getValue() ? Standard_True : Standard_False;
    auto transMode = static_cast<TransitionMode>(Transition.getValue());
    try {
        TopoShape result(0);
        result.makeElementPipeShell(shapes, isSolid, isFrenet, transMode, Part::OpCodes::Sweep);
        if (Linearize.getValue()) {
            result.linearize(LinearizeFace::linearizeFaces, LinearizeEdge::noEdges);
        }
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making the sweep");
    }
}

// ----------------------------------------------------------------------------

const char* Part::Thickness::ModeEnums[] = {"Skin", "Pipe", "RectoVerso", nullptr};
const char* Part::Thickness::JoinEnums[] = {"Arc", "Tangent", "Intersection", nullptr};

PROPERTY_SOURCE(Part::Thickness, Part::Feature)

Thickness::Thickness()
{
    ADD_PROPERTY_TYPE(Faces, (nullptr), "Thickness", App::Prop_None, "Faces to be removed");
    ADD_PROPERTY_TYPE(Value, (1.0), "Thickness", App::Prop_None, "Thickness value");
    ADD_PROPERTY_TYPE(Mode, (long(0)), "Thickness", App::Prop_None, "Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join, (long(0)), "Thickness", App::Prop_None, "Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(Intersection, (false), "Thickness", App::Prop_None, "Intersection");
    ADD_PROPERTY_TYPE(SelfIntersection, (false), "Thickness", App::Prop_None, "Self Intersection");

    // Value should have length as unit
    Value.setUnit(Base::Unit::Length);
}

short Thickness::mustExecute() const
{
    if (Faces.isTouched()) {
        return 1;
    }
    if (Value.isTouched()) {
        return 1;
    }
    if (Mode.isTouched()) {
        return 1;
    }
    if (Join.isTouched()) {
        return 1;
    }
    if (Intersection.isTouched()) {
        return 1;
    }
    if (SelfIntersection.isTouched()) {
        return 1;
    }
    return 0;
}

void Thickness::handleChangedPropertyType(Base::XMLReader& reader,
                                          const char* TypeName,
                                          App::Property* prop)
{
    if (prop == &Value && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat v;

        v.Restore(reader);

        Value.setValue(v.getValue());
    }
    else {
        Part::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

App::DocumentObjectExecReturn* Thickness::execute()
{
    std::vector<TopoShape> shapes;
    auto base = getTopoShape(Faces.getValue(), ShapeOption::ResolveLink | ShapeOption::Transform);
    if (base.isNull()) {
        return new App::DocumentObjectExecReturn("Invalid source shape");
    }
    if (base.countSubShapes(TopAbs_SOLID) != 1) {
        return new App::DocumentObjectExecReturn("Source shape is not single solid.");
    }
    for (auto& sub : Faces.getSubValues(true)) {
        shapes.push_back(base.getSubTopoShape(sub.c_str()));
        if (shapes.back().getShape().ShapeType() != TopAbs_FACE) {
            return new App::DocumentObjectExecReturn("Invalid face selection");
        }
    }
    double thickness = Value.getValue();
    double tol = Precision::Confusion();
    bool inter = Intersection.getValue();
    bool self = SelfIntersection.getValue();
    short mode = (short)Mode.getValue();
    short join = (short)Join.getValue();

    this->Shape.setValue(TopoShape(0,getDocument()->getStringHasher())
                             .makeElementThickSolid(base,
                                                    shapes,
                                                    thickness,
                                                    tol,
                                                    inter,
                                                    self,
                                                    mode,
                                                    static_cast<JoinType>(join)));
    return Part::Feature::execute();
}

// ----------------------------------------------------------------------------

PROPERTY_SOURCE(Part::Refine, Part::Feature)

Refine::Refine()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Refine", App::Prop_None, "Source shape");
}

App::DocumentObjectExecReturn* Refine::execute()
{
    Part::Feature* source = Source.getValue<Part::Feature*>();
    if (!source) {
        return new App::DocumentObjectExecReturn("No part object linked.");
    }

    try {
        TopoShape myShape = source->Shape.getShape();
        this->Shape.setValue(myShape.removeSplitter());
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

// ----------------------------------------------------------------------------

PROPERTY_SOURCE(Part::Reverse, Part::Feature)

Reverse::Reverse()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Reverse", App::Prop_None, "Source shape");
}

App::DocumentObjectExecReturn* Reverse::execute()
{
    App::DocumentObject* source = Source.getValue<App::DocumentObject*>();
    Part::TopoShape topoShape = Part::Feature::getTopoShape(source, ShapeOption::ResolveLink | ShapeOption::Transform);
    if (topoShape.isNull()) {
        return new App::DocumentObjectExecReturn("No part object linked.");
    }

    try {
        TopoDS_Shape myShape = topoShape.getShape();
        if (!myShape.IsNull()) {
            this->Shape.setValue(myShape.Reversed());
            Base::Placement p;
            p.fromMatrix(topoShape.getTransform());
            this->Placement.setValue(p);
            return App::DocumentObject::StdReturn;
        }
        return new App::DocumentObjectExecReturn("Shape is null.");
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
