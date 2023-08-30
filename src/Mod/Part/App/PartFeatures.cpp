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
# include <memory>
# include <BRepAdaptor_CompCurve.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepFill.hxx>
# include <BRepLib_MakeWire.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS_Shell.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include <App/Link.h>

#include "PartFeatures.h"


using namespace Part;

PROPERTY_SOURCE(Part::RuledSurface, Part::Feature)

const char* RuledSurface::OrientationEnums[]    = {"Automatic","Forward","Reversed",nullptr};

RuledSurface::RuledSurface()
{
    ADD_PROPERTY_TYPE(Curve1,(nullptr),"Ruled Surface",App::Prop_None,"Curve of ruled surface");
    ADD_PROPERTY_TYPE(Curve2,(nullptr),"Ruled Surface",App::Prop_None,"Curve of ruled surface");
    ADD_PROPERTY_TYPE(Orientation,((long)0),"Ruled Surface",App::Prop_None,"Orientation of ruled surface");
    Orientation.setEnums(OrientationEnums);
}

short RuledSurface::mustExecute() const
{
    if (Curve1.isTouched())
        return 1;
    if (Curve2.isTouched())
        return 1;
    if (Orientation.isTouched())
        return 1;
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
    const Part::TopoShape part = Part::Feature::getTopoShape(obj);
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
            //shape = Part::Feature::getTopoShape(obj, element[0].c_str(), true /*need element*/).getShape();
            shape = part.getSubShape(element[0].c_str());
        }
        else {
            // the sub-element is an empty string, so use the whole part
            shape = part.getShape();
        }
    }

    return nullptr;
}

App::DocumentObjectExecReturn *RuledSurface::execute()
{
    try {
        App::DocumentObjectExecReturn* ret;

        // get the first input shape
        TopoDS_Shape S1;
        ret = getShape(Curve1, S1);
        if (ret)
            return ret;

        // get the second input shape
        TopoDS_Shape S2;
        ret = getShape(Curve2, S2);
        if (ret)
            return ret;

        // check for expected type
        if (S1.IsNull() || S2.IsNull())
            return new App::DocumentObjectExecReturn("Linked shapes are empty.");

        if (S1.ShapeType() != TopAbs_EDGE && S1.ShapeType() != TopAbs_WIRE)
            return new App::DocumentObjectExecReturn("Linked shape is neither edge nor wire.");

        if (S2.ShapeType() != TopAbs_EDGE && S2.ShapeType() != TopAbs_WIRE)
            return new App::DocumentObjectExecReturn("Linked shape is neither edge nor wire.");

        // https://forum.freecad.org/viewtopic.php?f=8&t=24052
        //
        // if both shapes are sub-elements of one common shape then the fill algorithm
        // leads to problems if the shape has set a placement
        // The workaround is to copy the sub-shape
        S1 = BRepBuilderAPI_Copy(S1).Shape();
        S2 = BRepBuilderAPI_Copy(S2).Shape();

        // make both shapes to have the same type
        Standard_Boolean isWire = Standard_False;
        if (S1.ShapeType() == TopAbs_WIRE)
            isWire = Standard_True;

        if (isWire) {
            if (S2.ShapeType() == TopAbs_EDGE)
                S2 = BRepLib_MakeWire(TopoDS::Edge(S2));
        }
        else {
            // S1 is an edge, if S2 is a wire convert S1 to a wire, too
            if (S2.ShapeType() == TopAbs_WIRE) {
                S1 = BRepLib_MakeWire(TopoDS::Edge(S1));
                isWire = Standard_True;
            }
        }

        if (Orientation.getValue() == 0) {
            // Automatic
            std::unique_ptr<Adaptor3d_Curve> a1;
            std::unique_ptr<Adaptor3d_Curve> a2;
            if (!isWire) {
                a1 = std::make_unique<BRepAdaptor_Curve>(TopoDS::Edge(S1));
                a2 = std::make_unique<BRepAdaptor_Curve>(TopoDS::Edge(S2));
            }
            else {
                a1 = std::make_unique<BRepAdaptor_CompCurve>(TopoDS::Wire(S1));
                a2 = std::make_unique<BRepAdaptor_CompCurve>(TopoDS::Wire(S2));
            }

            if (a1 && a2) {
                // get end points of 1st curve
                Standard_Real first, last;
                first = a1->FirstParameter();
                last = a1->LastParameter();
                if (S1.Closed())
                    last = (first + last)/2;
                gp_Pnt p1 = a1->Value(first);
                gp_Pnt p2 = a1->Value(last);
                if (S1.Orientation() == TopAbs_REVERSED) {
                    std::swap(p1, p2);
                }

                // get end points of 2nd curve
                first = a2->FirstParameter();
                last = a2->LastParameter();
                if (S2.Closed())
                    last = (first + last)/2;
                gp_Pnt p3 = a2->Value(first);
                gp_Pnt p4 = a2->Value(last);
                if (S2.Orientation() == TopAbs_REVERSED) {
                    std::swap(p3, p4);
                }

                // Form two triangles (P1,P2,P3) and (P4,P3,P2) and check their normals.
                // If the dot product is negative then it's assumed that the resulting face
                // is twisted, hence the 2nd edge is reversed.
                gp_Vec v1(p1, p2);
                gp_Vec v2(p1, p3);
                gp_Vec n1 = v1.Crossed(v2);

                gp_Vec v3(p4, p3);
                gp_Vec v4(p4, p2);
                gp_Vec n2 = v3.Crossed(v4);

                if (n1.Dot(n2) < 0) {
                    S2.Reverse();
                }
            }
        }
        else if (Orientation.getValue() == 2) {
            // Reverse
            S2.Reverse();
        }

        TopoDS_Shape ruledShape;
        if (!isWire) {
            ruledShape = BRepFill::Face(TopoDS::Edge(S1), TopoDS::Edge(S2));
        }
        else {
            ruledShape = BRepFill::Shell(TopoDS::Wire(S1), TopoDS::Wire(S2));
        }

        this->Shape.setValue(ruledShape);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("General error in RuledSurface::execute()");
    }
}

// ----------------------------------------------------------------------------

App::PropertyIntegerConstraint::Constraints Loft::Degrees = {2,Geom_BSplineSurface::MaxDegree(),1};

PROPERTY_SOURCE(Part::Loft, Part::Feature)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections,(nullptr),"Loft",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Solid,(false),"Loft",App::Prop_None,"Create solid");
    ADD_PROPERTY_TYPE(Ruled,(false),"Loft",App::Prop_None,"Ruled surface");
    ADD_PROPERTY_TYPE(Closed,(false),"Loft",App::Prop_None,"Close Last to First Profile");
    ADD_PROPERTY_TYPE(MaxDegree,(5),"Loft",App::Prop_None,"Maximum Degree");
    MaxDegree.setConstraints(&Degrees);
}

short Loft::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Solid.isTouched())
        return 1;
    if (Ruled.isTouched())
        return 1;
    if (Closed.isTouched())
        return 1;
    if (MaxDegree.isTouched())
        return 1;
    return 0;
}

void Loft::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn *Loft::execute()
{
    if (Sections.getSize() == 0)
        return new App::DocumentObjectExecReturn("No sections linked.");

    try {
        TopTools_ListOfShape profiles;
        const std::vector<App::DocumentObject*>& shapes = Sections.getValues();
        std::vector<App::DocumentObject*>::const_iterator it;
        for (it = shapes.begin(); it != shapes.end(); ++it) {
            TopoDS_Shape shape = Feature::getShape(*it);
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Linked shape is invalid.");

            // Allow compounds with a single face, wire or vertex or
            // if there are only edges building one wire
            if (shape.ShapeType() == TopAbs_COMPOUND) {
                Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
                Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();

                TopoDS_Iterator it(shape);
                int numChilds=0;
                TopoDS_Shape child;
                for (; it.More(); it.Next(), numChilds++) {
                    if (!it.Value().IsNull()) {
                        child = it.Value();
                        if (child.ShapeType() == TopAbs_EDGE) {
                            hEdges->Append(child);
                        }
                    }
                }

                // a single child
                if (numChilds == 1) {
                    shape = child;
                }
                // or all children are edges
                else if (hEdges->Length() == numChilds) {
                    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges,
                        Precision::Confusion(), Standard_False, hWires);
                    if (hWires->Length() == 1)
                        shape = hWires->Value(1);
                }
            }
            if (shape.ShapeType() == TopAbs_FACE) {
                TopoDS_Wire faceouterWire = ShapeAnalysis::OuterWire(TopoDS::Face(shape));
                profiles.Append(faceouterWire);
            }
            else if (shape.ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape));
                profiles.Append(mkWire.Wire());
            }
            else if (shape.ShapeType() == TopAbs_EDGE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(shape));
                profiles.Append(mkWire.Wire());
            }
            else if (shape.ShapeType() == TopAbs_VERTEX) {
                profiles.Append(shape);
            }
            else {
                return new App::DocumentObjectExecReturn("Linked shape is not a vertex, edge, wire nor face.");
            }
        }

        Standard_Boolean isSolid = Solid.getValue() ? Standard_True : Standard_False;
        Standard_Boolean isRuled = Ruled.getValue() ? Standard_True : Standard_False;
        Standard_Boolean isClosed = Closed.getValue() ? Standard_True : Standard_False;
        int degMax = MaxDegree.getValue();

        TopoShape myShape;
        this->Shape.setValue(myShape.makeLoft(profiles, isSolid, isRuled, isClosed, degMax));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

// ----------------------------------------------------------------------------

const char* Part::Sweep::TransitionEnums[]= {"Transformed","Right corner", "Round corner",nullptr};

PROPERTY_SOURCE(Part::Sweep, Part::Feature)

Sweep::Sweep()
{
    ADD_PROPERTY_TYPE(Sections,(nullptr),"Sweep",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Spine,(nullptr),"Sweep",App::Prop_None,"Path to sweep along");
    ADD_PROPERTY_TYPE(Solid,(false),"Sweep",App::Prop_None,"Create solid");
    ADD_PROPERTY_TYPE(Frenet,(false),"Sweep",App::Prop_None,"Frenet");
    ADD_PROPERTY_TYPE(Transition,(long(1)),"Sweep",App::Prop_None,"Transition mode");
    Transition.setEnums(TransitionEnums);
}

short Sweep::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Spine.isTouched())
        return 1;
    if (Solid.isTouched())
        return 1;
    if (Frenet.isTouched())
        return 1;
    if (Transition.isTouched())
        return 1;
    return 0;
}

void Sweep::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn *Sweep::execute()
{
    if (Sections.getSize() == 0)
        return new App::DocumentObjectExecReturn("No sections linked.");
    App::DocumentObject* spine = Spine.getValue();
    if (!spine)
        return new App::DocumentObjectExecReturn("No spine linked.");
    const std::vector<std::string>& subedge = Spine.getSubValues();

    TopoDS_Shape path;
    const Part::TopoShape& shape = Feature::getTopoShape(spine);
    if (!shape.getShape().IsNull()) {
        try {
            if (!subedge.empty()) {
                BRepBuilderAPI_MakeWire mkWire;
                for (const auto & it : subedge) {
                    TopoDS_Shape subshape = Feature::getTopoShape(spine, it.c_str(), true /*need element*/).getShape();
                    mkWire.Add(TopoDS::Edge(subshape));
                }
                path = mkWire.Wire();
            }
            else if (shape.getShape().ShapeType() == TopAbs_EDGE) {
                path = shape.getShape();
            }
            else if (shape.getShape().ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape.getShape()));
                path = mkWire.Wire();
            }
            else if (shape.getShape().ShapeType() == TopAbs_COMPOUND) {
                TopoDS_Iterator it(shape.getShape());
                for (; it.More(); it.Next()) {
                    if (it.Value().IsNull())
                        return new App::DocumentObjectExecReturn("In valid element in spine.");
                    if ((it.Value().ShapeType() != TopAbs_EDGE) &&
                        (it.Value().ShapeType() != TopAbs_WIRE)) {
                        return new App::DocumentObjectExecReturn("Element in spine is neither an edge nor a wire.");
                    }
                }

                Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
                Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
                for (TopExp_Explorer xp(shape.getShape(), TopAbs_EDGE); xp.More(); xp.Next())
                    hEdges->Append(xp.Current());

                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, Precision::Confusion(), Standard_True, hWires);
                int len = hWires->Length();
                if (len != 1)
                    return new App::DocumentObjectExecReturn("Spine is not connected.");
                path = hWires->Value(1);
            }
            else {
                return new App::DocumentObjectExecReturn("Spine is neither an edge nor a wire.");
            }
        }
        catch (Standard_Failure&) {
            return new App::DocumentObjectExecReturn("Invalid spine.");
        }
    }

    try {
        TopTools_ListOfShape profiles;
        const std::vector<App::DocumentObject*>& shapes = Sections.getValues();
        std::vector<App::DocumentObject*>::const_iterator it;
        for (it = shapes.begin(); it != shapes.end(); ++it) {
            TopoDS_Shape shape = Feature::getShape(*it);
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Linked shape is invalid.");

            // Allow compounds with a single face, wire or vertex or
            // if there are only edges building one wire
            if (shape.ShapeType() == TopAbs_COMPOUND) {
                Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
                Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();

                TopoDS_Iterator it(shape);
                int numChilds=0;
                TopoDS_Shape child;
                for (; it.More(); it.Next(), numChilds++) {
                    if (!it.Value().IsNull()) {
                        child = it.Value();
                        if (child.ShapeType() == TopAbs_EDGE) {
                            hEdges->Append(child);
                        }
                    }
                }

                // a single child
                if (numChilds == 1) {
                    shape = child;
                }
                // or all children are edges
                else if (hEdges->Length() == numChilds) {
                    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges,
                        Precision::Confusion(), Standard_False, hWires);
                    if (hWires->Length() == 1)
                        shape = hWires->Value(1);
                }
            }
            // There is a weird behaviour of BRepOffsetAPI_MakePipeShell when trying to add the wire as is.
            // If we re-create the wire then everything works fine.
            // http://forum.freecad.org/viewtopic.php?f=10&t=2673&sid=fbcd2ff4589f0b2f79ed899b0b990648#p20268
            if (shape.ShapeType() == TopAbs_FACE) {
                TopoDS_Wire faceouterWire = ShapeAnalysis::OuterWire(TopoDS::Face(shape));
                profiles.Append(faceouterWire);
            }
            else if (shape.ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape));
                profiles.Append(mkWire.Wire());
            }
            else if (shape.ShapeType() == TopAbs_EDGE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(shape));
                profiles.Append(mkWire.Wire());
            }
            else if (shape.ShapeType() == TopAbs_VERTEX) {
                profiles.Append(shape);
            }
            else {
                return new App::DocumentObjectExecReturn("Linked shape is not a vertex, edge, wire nor face.");
            }
        }

        Standard_Boolean isSolid = Solid.getValue() ? Standard_True : Standard_False;
        Standard_Boolean isFrenet = Frenet.getValue() ? Standard_True : Standard_False;
        BRepBuilderAPI_TransitionMode transMode;
        switch (Transition.getValue()) {
            case 1: transMode = BRepBuilderAPI_RightCorner;
                break;
            case 2: transMode = BRepBuilderAPI_RoundCorner;
                break;
            default: transMode = BRepBuilderAPI_Transformed;
                break;
        }

        if(path.IsNull()) {
            return new App::DocumentObjectExecReturn("Spine path missing, sweep operation stopped.");
        }

        if (path.ShapeType() == TopAbs_EDGE) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(path));
            path = mkWire.Wire();
        }

        BRepOffsetAPI_MakePipeShell mkPipeShell(TopoDS::Wire(path));
        mkPipeShell.SetMode(isFrenet);
        mkPipeShell.SetTransitionMode(transMode);
        TopTools_ListIteratorOfListOfShape iter;
        for (iter.Initialize(profiles); iter.More(); iter.Next()) {
            mkPipeShell.Add(TopoDS_Shape(iter.Value()));
        }

        if (!mkPipeShell.IsReady())
            Standard_Failure::Raise("shape is not ready to build");
        mkPipeShell.Build();
        if (isSolid)
            mkPipeShell.MakeSolid();

        this->Shape.setValue(mkPipeShell.Shape());
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

const char *Part::Thickness::ModeEnums[] = {"Skin", "Pipe", "RectoVerso", nullptr};
const char *Part::Thickness::JoinEnums[] = {"Arc", "Tangent", "Intersection", nullptr};

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
    if (Faces.isTouched())
        return 1;
    if (Value.isTouched())
        return 1;
    if (Mode.isTouched())
        return 1;
    if (Join.isTouched())
        return 1;
    if (Intersection.isTouched())
        return 1;
    if (SelfIntersection.isTouched())
        return 1;
    return 0;
}

void Thickness::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
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

App::DocumentObjectExecReturn *Thickness::execute()
{
    App::DocumentObject* source = Faces.getValue();
    if (!source)
        return new App::DocumentObjectExecReturn("No source shape linked.");
    const TopoShape& shape = Feature::getTopoShape(source);
    if (shape.isNull())
        return new App::DocumentObjectExecReturn("Source shape is empty.");

    int countSolids = 0;
    TopExp_Explorer xp;
    xp.Init(shape.getShape(),TopAbs_SOLID);
    for (;xp.More(); xp.Next()) {
        countSolids++;
    }
    if (countSolids != 1)
        return new App::DocumentObjectExecReturn("Source shape is not a solid.");

    TopTools_ListOfShape closingFaces;
    const std::vector<std::string>& subStrings = Faces.getSubValues();
    for (const auto & it : subStrings) {
        TopoDS_Face face = TopoDS::Face(shape.getSubShape(it.c_str()));
        closingFaces.Append(face);
    }

    double thickness = Value.getValue();
    double tol = Precision::Confusion();
    bool inter = Intersection.getValue();
    bool self = SelfIntersection.getValue();
    short mode = (short)Mode.getValue();
    short join = (short)Join.getValue();

    if (fabs(thickness) > 2*tol)
        this->Shape.setValue(shape.makeThickSolid(closingFaces, thickness, tol, inter, self, mode, join));
    else
        this->Shape.setValue(shape);
    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------------

PROPERTY_SOURCE(Part::Refine, Part::Feature)

Refine::Refine()
{
    ADD_PROPERTY_TYPE(Source,(nullptr),"Refine",App::Prop_None,"Source shape");
}

App::DocumentObjectExecReturn *Refine::execute()
{
    Part::Feature* source = Source.getValue<Part::Feature*>();
    if (!source)
        return new App::DocumentObjectExecReturn("No part object linked.");

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
    Part::TopoShape topoShape = Part::Feature::getShape(source);
    if (topoShape.isNull())
        return new App::DocumentObjectExecReturn("No part object linked.");

    try {
        TopoDS_Shape myShape = topoShape.getShape();
        if (!myShape.IsNull()){
            this->Shape.setValue(myShape.Reversed());
            Base::Placement p;
            p.fromMatrix(topoShape.getTransform());
            this->Placement.setValue(p);
            return App::DocumentObject::StdReturn;
        }
        return new App::DocumentObjectExecReturn("Shape is null.");
    }
    catch (Standard_Failure & e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
