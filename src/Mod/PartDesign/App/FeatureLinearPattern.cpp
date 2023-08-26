/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <App/OriginFeature.h>
#include <Base/Axis.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeatureLinearPattern.h"
#include "DatumLine.h"
#include "DatumPlane.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::LinearPattern, PartDesign::Transformed)

const App::PropertyIntegerConstraint::Constraints LinearPattern::intOccurrences = { 1, INT_MAX, 1 };

const char* LinearPattern::ModeEnums[] = { "length", "offset", nullptr };

LinearPattern::LinearPattern()
{
    auto initialMode = LinearPatternMode::length;

    ADD_PROPERTY_TYPE(Direction,(nullptr),"LinearPattern",(App::PropertyType)(App::Prop_None),"Direction");
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY(Mode, (long(initialMode)));
    ADD_PROPERTY(Length,(100.0));
    ADD_PROPERTY(Offset,(10.0));
    ADD_PROPERTY(Occurrences,(3));
    Occurrences.setConstraints(&intOccurrences);
    Mode.setEnums(ModeEnums);
    setReadWriteStatusForMode(initialMode);
}

short LinearPattern::mustExecute() const
{
    if (Direction.isTouched() ||
        Reversed.isTouched() ||
        Mode.isTouched() ||
        // Length and Offset are mutually exclusive, only one could be updated at once
        Length.isTouched() || 
        Offset.isTouched() || 
        Occurrences.isTouched())
        return 1;
    return Transformed::mustExecute();
}

void LinearPattern::setReadWriteStatusForMode(LinearPatternMode mode)
{
    Length.setReadOnly(mode != LinearPatternMode::length);
    Offset.setReadOnly(mode != LinearPatternMode::offset);
}

const std::list<gp_Trsf> LinearPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    int occurrences = Occurrences.getValue();
    if (occurrences < 1)
        throw Base::ValueError("At least one occurrence required");

    if (occurrences == 1)
        return {gp_Trsf()};

    double distance = Length.getValue();
    if (distance < Precision::Confusion())
        throw Base::ValueError("Pattern length too small");
    bool reversed = Reversed.getValue();

    App::DocumentObject* refObject = Direction.getValue();
    if (!refObject)
        throw Base::ValueError("No direction reference specified");

    std::vector<std::string> subStrings = Direction.getSubValues();
    if (subStrings.empty())
        throw Base::ValueError("No direction reference specified");

    gp_Dir dir;
    if (refObject->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        Part::Part2DObject* refSketch = static_cast<Part::Part2DObject*>(refObject);
        Base::Axis axis;
        if (subStrings[0] == "H_Axis")
            axis = refSketch->getAxis(Part::Part2DObject::H_Axis);
        else if (subStrings[0] == "V_Axis")
            axis = refSketch->getAxis(Part::Part2DObject::V_Axis);
        else if (subStrings[0] == "N_Axis")
            axis = refSketch->getAxis(Part::Part2DObject::N_Axis);
        else if (subStrings[0].compare(0, 4, "Axis") == 0) {
            int AxId = std::atoi(subStrings[0].substr(4,4000).c_str());
            if (AxId >= 0 && AxId < refSketch->getAxisCount())
                axis = refSketch->getAxis(AxId);
        }
        else if (subStrings[0].compare(0, 4, "Edge") == 0) {
            Part::TopoShape refShape = refSketch->Shape.getShape();
            TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull())
                throw Base::ValueError("Failed to extract direction edge");
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line)
                throw Base::TypeError("Direction edge must be a straight line");

            gp_Pnt p = adapt.Line().Location();
            gp_Dir d = adapt.Line().Direction();
            axis.setBase(Base::Vector3d(p.X(), p.Y(), p.Z()));
            axis.setDirection(Base::Vector3d(d.X(), d.Y(), d.Z()));
        }
        axis *= refSketch->Placement.getValue();
        dir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    } else if (refObject->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
        PartDesign::Plane* plane = static_cast<PartDesign::Plane*>(refObject);
        Base::Vector3d d = plane->getNormal();
        dir = gp_Dir(d.x, d.y, d.z);
    } else if (refObject->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
        PartDesign::Line* line = static_cast<PartDesign::Line*>(refObject);
        Base::Vector3d d = line->getDirection();
        dir = gp_Dir(d.x, d.y, d.z);
    } else if (refObject->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
        App::Line* line = static_cast<App::Line*>(refObject);
        Base::Rotation rot = line->Placement.getValue().getRotation();
        Base::Vector3d d(1,0,0);
        rot.multVec(d, d);
        dir = gp_Dir(d.x, d.y, d.z);
    } else if (refObject->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        if (subStrings[0].empty())
            throw Base::ValueError("No direction reference specified");
        Part::Feature* refFeature = static_cast<Part::Feature*>(refObject);
        Part::TopoShape refShape = refFeature->Shape.getShape();
        TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());

        if (ref.ShapeType() == TopAbs_FACE) {
            TopoDS_Face refFace = TopoDS::Face(ref);
            if (refFace.IsNull())
                throw Base::ValueError("Failed to extract direction plane");
            BRepAdaptor_Surface adapt(refFace);
            if (adapt.GetType() != GeomAbs_Plane)
                throw Base::TypeError("Direction face must be planar");

            dir = adapt.Plane().Axis().Direction();
        } else if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull())
                throw Base::ValueError("Failed to extract direction edge");
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line)
                throw Base::ValueError("Direction edge must be a straight line");

            dir = adapt.Line().Direction();
        } else {
            throw Base::ValueError("Direction reference must be edge or face");
        }
    } else {
        throw Base::ValueError("Direction reference must be edge/face of a feature or a datum line/plane");
    }
    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    dir.Transform(invObjLoc.Transformation());

    gp_Vec offset(dir.X(), dir.Y(), dir.Z());

    switch (static_cast<LinearPatternMode>(Mode.getValue())) {
        case LinearPatternMode::length:
            offset *= distance / (occurrences - 1);
            break;

        case LinearPatternMode::offset:
            offset *= Offset.getValue();
            break;

        default:
            throw Base::ValueError("Invalid mode");
    }

    if (reversed)
        offset.Reverse();

    std::list<gp_Trsf> transformations;
    gp_Trsf trans;
    transformations.push_back(trans);

    // Note: The original feature is already included in the list of transformations!
    // Therefore we start with occurrence number 1
    for (int i = 1; i < occurrences; i++) {
        trans.SetTranslation(offset * i);
        transformations.push_back(trans);
    }

    return transformations;
}

void LinearPattern::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
// transforms properties that had been changed
{
    // property Occurrences had the App::PropertyInteger and was changed to App::PropertyIntegerConstraint
    if (prop == &Occurrences && strcmp(TypeName, "App::PropertyInteger") == 0) {
        App::PropertyInteger OccurrencesProperty;
        // restore the PropertyInteger to be able to set its value
        OccurrencesProperty.Restore(reader);
        Occurrences.setValue(OccurrencesProperty.getValue());
    }
    else {
        Transformed::handleChangedPropertyType(reader, TypeName, prop);
    }
}

void LinearPattern::onChanged(const App::Property* prop)
{
    auto mode = static_cast<LinearPatternMode>(Mode.getValue());

    if (prop == &Mode) {
        setReadWriteStatusForMode(mode);
    }

    // Keep Length in sync with Offset
    if (mode == LinearPatternMode::offset && prop == &Offset && !Length.testStatus(App::Property::Status::Immutable)) {
        Length.setValue(Offset.getValue() * (Occurrences.getValue() - 1));
    }

    if (mode == LinearPatternMode::length && prop == &Length && !Offset.testStatus(App::Property::Status::Immutable)) {
        Offset.setValue(Length.getValue() / (Occurrences.getValue() - 1));
    }

    Transformed::onChanged(prop);
}

}
