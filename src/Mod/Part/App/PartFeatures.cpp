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
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopExp_Explorer.hxx>
# include <Precision.hxx>
#endif


#include "PartFeatures.h"


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
    try {
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

        TopoDS_Shape curve1;
        const Part::TopoShape& shape1 = static_cast<Part::Feature*>(c1)->Shape.getValue();
        if (!shape1._Shape.IsNull()) {
            if (!element1[0].empty()) {
                curve1 = shape1.getSubShape(element1[0].c_str());
            }
            else {
                if (shape1._Shape.ShapeType() == TopAbs_EDGE)
                    curve1 = shape1._Shape;
                else if (shape1._Shape.ShapeType() == TopAbs_WIRE)
                    curve1 = shape1._Shape;
            }
        }

        TopoDS_Shape curve2;
        const Part::TopoShape& shape2 = static_cast<Part::Feature*>(c2)->Shape.getValue();
        if (!shape2._Shape.IsNull()) {
            if (!element2[0].empty()) {
                curve2 = shape2.getSubShape(element2[0].c_str());
            }
            else {
                if (shape2._Shape.ShapeType() == TopAbs_EDGE)
                    curve2 = shape2._Shape;
                else if (shape2._Shape.ShapeType() == TopAbs_WIRE)
                    curve2 = shape2._Shape;
            }
        }

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
    catch (...) {
        return new App::DocumentObjectExecReturn("General error in RuledSurface::execute()");
    }
}

// ----------------------------------------------------------------------------

PROPERTY_SOURCE(Part::Loft, Part::Feature)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections,(0),"Loft",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Solid,(false),"Loft",App::Prop_None,"Create solid");
    ADD_PROPERTY_TYPE(Ruled,(false),"Loft",App::Prop_None,"Ruled surface");
}

short Loft::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Solid.isTouched())
        return 1;
    if (Ruled.isTouched())
        return 1;
    return 0;
}

void Loft::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn *Loft::execute(void)
{
    if (Sections.getSize() == 0)
        return new App::DocumentObjectExecReturn("No sections linked.");

    try {
        TopTools_ListOfShape profiles;
        const std::vector<App::DocumentObject*>& shapes = Sections.getValues();
        std::vector<App::DocumentObject*>::const_iterator it;
        for (it = shapes.begin(); it != shapes.end(); ++it) {
            if (!(*it)->isDerivedFrom(Part::Feature::getClassTypeId()))
                return new App::DocumentObjectExecReturn("Linked object is not a shape.");
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Linked shape is invalid.");
            if (shape.ShapeType() == TopAbs_WIRE) {
                profiles.Append(shape);
            }
            else if (shape.ShapeType() == TopAbs_EDGE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(shape));
                profiles.Append(mkWire.Wire());
            }
            else if (shape.ShapeType() == TopAbs_VERTEX) {
                profiles.Append(shape);
            }
            else {
                return new App::DocumentObjectExecReturn("Linked shape is not a vertex, edge nor wire.");
            }
        }

        Standard_Boolean isSolid = Solid.getValue() ? Standard_True : Standard_False;
        Standard_Boolean isRuled = Ruled.getValue() ? Standard_True : Standard_False;

        TopoShape myShape;
        this->Shape.setValue(myShape.makeLoft(profiles, isSolid, isRuled));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

// ----------------------------------------------------------------------------

const char* Part::Sweep::TransitionEnums[]= {"Transformed","Right corner", "Round corner",NULL};

PROPERTY_SOURCE(Part::Sweep, Part::Feature)

Sweep::Sweep()
{
    ADD_PROPERTY_TYPE(Sections,(0),"Sweep",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Spine,(0),"Sweep",App::Prop_None,"Path to sweep along");
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

App::DocumentObjectExecReturn *Sweep::execute(void)
{
    if (Sections.getSize() == 0)
        return new App::DocumentObjectExecReturn("No sections linked.");
    App::DocumentObject* spine = Spine.getValue();
    if (!(spine && spine->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
        return new App::DocumentObjectExecReturn("No spine linked.");
    const std::vector<std::string>& subedge = Spine.getSubValues();

    TopoDS_Shape path;
    const Part::TopoShape& shape = static_cast<Part::Feature*>(spine)->Shape.getValue();
    if (!shape._Shape.IsNull()) {
        try {
            BRepBuilderAPI_MakeWire mkWire;
            for (std::vector<std::string>::const_iterator it = subedge.begin(); it != subedge.end(); ++it) {
                TopoDS_Shape subshape = shape.getSubShape(it->c_str());
                mkWire.Add(TopoDS::Edge(subshape));
            }
            path = mkWire.Wire();
        }
        catch (Standard_Failure) {
            if (shape._Shape.ShapeType() == TopAbs_EDGE)
                path = shape._Shape;
            else if (shape._Shape.ShapeType() == TopAbs_WIRE)
                path = shape._Shape;
            else
                return new App::DocumentObjectExecReturn("Spine is neither an edge nor a wire.");
        }
    }

    try {
        TopTools_ListOfShape profiles;
        const std::vector<App::DocumentObject*>& shapes = Sections.getValues();
        std::vector<App::DocumentObject*>::const_iterator it;
        for (it = shapes.begin(); it != shapes.end(); ++it) {
            if (!(*it)->isDerivedFrom(Part::Feature::getClassTypeId()))
                return new App::DocumentObjectExecReturn("Linked object is not a shape.");
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Linked shape is invalid.");
            // There is a weird behaviour of BRepOffsetAPI_MakePipeShell when trying to add the wire as is.
            // If we re-create the wire then everything works fine.
            // https://sourceforge.net/apps/phpbb/free-cad/viewtopic.php?f=10&t=2673&sid=fbcd2ff4589f0b2f79ed899b0b990648#p20268
            if (shape.ShapeType() == TopAbs_WIRE) {
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
                return new App::DocumentObjectExecReturn("Linked shape is not a vertex, edge nor wire.");
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
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

// ----------------------------------------------------------------------------

const char* Part::Offset::ModeEnums[]= {"Skin","Pipe", "RectoVerso",NULL};
const char* Part::Offset::JoinEnums[]= {"Arc","Tangent", "Intersection",NULL};

PROPERTY_SOURCE(Part::Offset, Part::Feature)

Offset::Offset()
{
    ADD_PROPERTY_TYPE(Source,(0),"Offset",App::Prop_None,"Source shape");
    ADD_PROPERTY_TYPE(Value,(1.0),"Offset",App::Prop_None,"Offset value");
    ADD_PROPERTY_TYPE(Mode,(long(0)),"Offset",App::Prop_None,"Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join,(long(0)),"Offset",App::Prop_None,"Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(Intersection,(false),"Offset",App::Prop_None,"Intersection");
    ADD_PROPERTY_TYPE(SelfIntersection,(false),"Offset",App::Prop_None,"Self Intersection");
    ADD_PROPERTY_TYPE(Fill,(false),"Offset",App::Prop_None,"Fill offset");
}

short Offset::mustExecute() const
{
    if (Source.isTouched())
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
    if (Fill.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Offset::execute(void)
{
    App::DocumentObject* source = Source.getValue();
    if (!(source && source->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
        return new App::DocumentObjectExecReturn("No source shape linked.");
    double offset = Value.getValue();
    double tol = Precision::Confusion();
    bool inter = Intersection.getValue();
    bool self = SelfIntersection.getValue();
    short mode = (short)Mode.getValue();
    short join = (short)Join.getValue();
    bool fill = Fill.getValue();
    const TopoShape& shape = static_cast<Part::Feature*>(source)->Shape.getShape();
    if (fabs(offset) > 2*tol)
        this->Shape.setValue(shape.makeOffsetShape(offset, tol, inter, self, mode, join, fill));
    else
        this->Shape.setValue(shape);
    return App::DocumentObject::StdReturn;
}

// ----------------------------------------------------------------------------

const char* Part::Thickness::ModeEnums[]= {"Skin","Pipe", "RectoVerso",NULL};
const char* Part::Thickness::JoinEnums[]= {"Arc","Tangent", "Intersection",NULL};

PROPERTY_SOURCE(Part::Thickness, Part::Feature)

Thickness::Thickness()
{
    ADD_PROPERTY_TYPE(Faces,(0),"Thickness",App::Prop_None,"Source shape");
    ADD_PROPERTY_TYPE(Value,(1.0),"Thickness",App::Prop_None,"Thickness value");
    ADD_PROPERTY_TYPE(Mode,(long(0)),"Thickness",App::Prop_None,"Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join,(long(0)),"Thickness",App::Prop_None,"Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(Intersection,(false),"Thickness",App::Prop_None,"Intersection");
    ADD_PROPERTY_TYPE(SelfIntersection,(false),"Thickness",App::Prop_None,"Self Intersection");
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

App::DocumentObjectExecReturn *Thickness::execute(void)
{
    App::DocumentObject* source = Faces.getValue();
    if (!(source && source->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
        return new App::DocumentObjectExecReturn("No source shape linked.");
    const TopoShape& shape = static_cast<Part::Feature*>(source)->Shape.getShape();
    if (shape.isNull())
        return new App::DocumentObjectExecReturn("Source shape is empty.");

    int countSolids = 0;
    TopExp_Explorer xp;
    xp.Init(shape._Shape,TopAbs_SOLID);
    for (;xp.More(); xp.Next()) {
        countSolids++;
    }
    if (countSolids != 1)
        return new App::DocumentObjectExecReturn("Source shape is not a solid.");

    TopTools_ListOfShape closingFaces;
    const std::vector<std::string>& subStrings = Faces.getSubValues();
    for (std::vector<std::string>::const_iterator it = subStrings.begin(); it != subStrings.end(); ++it) {
        TopoDS_Face face = TopoDS::Face(shape.getSubShape(it->c_str()));
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
