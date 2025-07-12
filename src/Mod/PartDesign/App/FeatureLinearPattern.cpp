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
# include <limits>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <App/Datums.h>
#include <Base/Axis.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeatureLinearPattern.h"
#include "DatumLine.h"
#include "DatumPlane.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::LinearPattern, PartDesign::Transformed)

const App::PropertyIntegerConstraint::Constraints LinearPattern::intOccurrences = {
    1, std::numeric_limits<int>::max(), 1 };

const char* LinearPattern::ModeEnums[] = { "Extent", "Spacing", nullptr };

LinearPattern::LinearPattern()
{
    auto initialMode = LinearPatternMode::Extent;

    ADD_PROPERTY_TYPE(Direction,(nullptr),"Direction 1",(App::PropertyType)(App::Prop_None),"Direction");
    ADD_PROPERTY_TYPE(Reversed,(0), "Direction 1", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Mode, (long(initialMode)), "Direction 1", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Length,(100.0), "Direction 1", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Offset,(10.0), "Direction 1", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Spacings, ({ -1.0 }), "Direction 1", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(SpacingPattern, ({}), "Direction 1", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Occurrences,(2), "Direction 1", (App::PropertyType)(App::Prop_None), "Direction");
    Occurrences.setConstraints(&intOccurrences);
    Mode.setEnums(ModeEnums);
    setReadWriteStatusForMode(initialMode);

    ADD_PROPERTY_TYPE(Direction2,(nullptr),"Direction 2",(App::PropertyType)(App::Prop_None),"Direction");
    ADD_PROPERTY_TYPE(Reversed2,(0), "Direction 2", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Mode2, (long(initialMode)), "Direction 2", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Length2,(100.0), "Direction 2", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Offset2,(10.0), "Direction 2", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Spacings2, ({}), "Direction 2", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(SpacingPattern2, ({}), "Direction 2", (App::PropertyType)(App::Prop_None), "Direction");
    ADD_PROPERTY_TYPE(Occurrences2,(1), "Direction 2", (App::PropertyType)(App::Prop_None), "Direction");
    Occurrences2.setConstraints(&intOccurrences);
    Mode2.setEnums(ModeEnums);
    setReadWriteStatusForMode2(initialMode);
}

short LinearPattern::mustExecute() const
{
    if (Direction.isTouched() ||
        Reversed.isTouched() ||
        Mode.isTouched() ||
        // Length and Offset are mutually exclusive, only one could be updated at once
        Length.isTouched() || 
        Offset.isTouched() || 
        Spacings.isTouched() ||
        SpacingPattern.isTouched() ||
        Occurrences.isTouched() ||
        Direction2.isTouched() ||
        Reversed2.isTouched() ||
        Mode2.isTouched() ||
        Length2.isTouched() ||
        Offset2.isTouched() ||
        Spacings2.isTouched() ||
        SpacingPattern2.isTouched() ||
        Occurrences2.isTouched())
        return 1;
    return Transformed::mustExecute();
}

void LinearPattern::setReadWriteStatusForMode(LinearPatternMode mode)
{
    Length.setReadOnly(mode != LinearPatternMode::Extent);
    Offset.setReadOnly(mode != LinearPatternMode::Spacing);
}

void LinearPattern::setReadWriteStatusForMode2(LinearPatternMode mode)
{
    Length2.setReadOnly(mode != LinearPatternMode::Extent);
    Offset2.setReadOnly(mode != LinearPatternMode::Spacing);
}

const std::list<gp_Trsf> LinearPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    int occurrences = Occurrences.getValue();
    int occurrences2 = Occurrences2.getValue();
    if (occurrences < 1 || occurrences2 < 1) {
        throw Base::ValueError("At least one occurrence required");
    }

    if (occurrences == 1 && occurrences2 == 1) {
        return { gp_Trsf() };
    }

    double distance = Length.getValue();
    double distance2 = Length2.getValue();
    if (distance < Precision::Confusion() && occurrences != 1) {
        throw Base::ValueError("Pattern length too small");
    }
    else if (distance2 < Precision::Confusion() && occurrences2 != 1) {
        throw Base::ValueError("Pattern length2 too small");
    }

    gp_Vec offset, offset2;
    if (occurrences != 1) {
        offset = getDirectionFromProperty(Direction);
        if (Reversed.getValue()) {
            offset.Reverse();
        }

        if (Mode.getValue() == (int)LinearPatternMode::Extent && occurrences != 1) {
            offset *= distance / (occurrences - 1);
        }
    }

    if (occurrences2 != 1) {
        offset2 = getDirectionFromProperty(Direction2);
        if (Reversed2.getValue()) {
            offset2.Reverse();
        }

        if (Mode2.getValue() == (int)LinearPatternMode::Extent && occurrences2 != 1) {
            offset2 *= distance2 / (occurrences2 - 1);
        }
    }

    // make sure spacings are correct size :
    updateSpacings();

    const std::vector<double> spacings = Spacings.getValues();
    const std::vector<double> pattern = SpacingPattern.getValues();
    bool usePattern = pattern.size() > 1;

    const std::vector<double> spacings2 = Spacings2.getValues();
    const std::vector<double> pattern2 = SpacingPattern2.getValues();
    bool usePattern2 = pattern2.size() > 1;

    double cumulativeSpacings = 0.0;
    double cumulativeSpacings2 = 0.0;

    std::list<gp_Trsf> transformations;

    std::vector<gp_Vec> steps1 { gp_Vec() };
    for (int i = 1; i < occurrences; ++i) { // First is empty
        gp_Vec step;
        if (Mode.getValue() == (int)LinearPatternMode::Spacing) {
            double spacing;
            if (spacings[i - 1] != -1.0) {
                spacing = spacings[i - 1];
            }
            else if (usePattern) {
                spacing = pattern[(size_t)fmod(i - 1, pattern.size())];
            }
            else {
                spacing = Offset.getValue();
            }
            cumulativeSpacings += spacing;
            step = offset * cumulativeSpacings;
        }
        else {
            step = offset * i;
        }
        steps1.push_back(step);
    }

    std::vector<gp_Vec> steps2 { gp_Vec() };
    for (int j = 1; j < occurrences2; ++j) { // First is empty
        gp_Vec step;

        if (Mode2.getValue() == (int)LinearPatternMode::Spacing) {
            double spacing2;
            if (spacings2[j - 1] != -1.0) {
                spacing2 = spacings2[j - 1];
            }
            else if (usePattern2) {
                spacing2 = pattern2[(size_t)fmod(j - 1, pattern2.size())];
            }
            else {
                spacing2 = Offset2.getValue();
            }
            cumulativeSpacings2 += spacing2;
            step = offset2 * cumulativeSpacings2;
        }
        else {
            step = offset2 * j;
        }
        steps2.push_back(step);
    }

    for (int i = 0; i < occurrences; ++i) {
        for (int j = 0; j < occurrences2; ++j) {
            gp_Trsf trans;
            trans.SetTranslation(steps1[i] + steps2[j]);
            transformations.push_back(trans);
        }
    }

    return transformations;
}

gp_Dir LinearPattern::getDirectionFromProperty(const App::PropertyLinkSub& dirProp) const
{
    App::DocumentObject* refObject = dirProp.getValue();
    if (!refObject) {
        throw Base::ValueError("No direction reference specified");
    }

    std::vector<std::string> subStrings = dirProp.getSubValues();
    if (subStrings.empty()) {
        throw Base::ValueError("No direction reference specified");
    }

    gp_Dir dir;
    if (refObject->isDerivedFrom<Part::Part2DObject>()) {
        auto* refSketch = static_cast<Part::Part2DObject*>(refObject);
        Base::Axis axis;
        if (subStrings[0] == "H_Axis") {
            axis = refSketch->getAxis(Part::Part2DObject::H_Axis);
            axis *= refSketch->Placement.getValue();
        }
        else if (subStrings[0] == "V_Axis") {
            axis = refSketch->getAxis(Part::Part2DObject::V_Axis);
            axis *= refSketch->Placement.getValue();
        }
        else if (subStrings[0] == "N_Axis") {
            axis = refSketch->getAxis(Part::Part2DObject::N_Axis);
            axis *= refSketch->Placement.getValue();
        }
        else if (subStrings[0].compare(0, 4, "Axis") == 0) {
            int AxId = std::atoi(subStrings[0].substr(4, 4000).c_str());
            if (AxId >= 0 && AxId < refSketch->getAxisCount()) {
                axis = refSketch->getAxis(AxId);
                axis *= refSketch->Placement.getValue();
            }
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

            // the axis is not given in local coordinates and mustn't be multiplied with the
            // placement
            axis.setBase(Base::Vector3d(p.X(), p.Y(), p.Z()));
            axis.setDirection(Base::Vector3d(d.X(), d.Y(), d.Z()));
        }
        dir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    }
    else if (refObject->isDerivedFrom<PartDesign::Plane>()) {
        auto* plane = static_cast<PartDesign::Plane*>(refObject);
        Base::Vector3d d = plane->getNormal();
        dir = gp_Dir(d.x, d.y, d.z);
    }
    else if (refObject->isDerivedFrom<PartDesign::Line>()) {
        auto* line = static_cast<PartDesign::Line*>(refObject);
        Base::Vector3d d = line->getDirection();
        dir = gp_Dir(d.x, d.y, d.z);
    }
    else if (refObject->isDerivedFrom<App::Line>()) {
        auto* line = static_cast<App::Line*>(refObject);
        Base::Vector3d d = line->getDirection();
        dir = gp_Dir(d.x, d.y, d.z);
    }
    else if (refObject->isDerivedFrom<Part::Feature>()) {
        if (subStrings[0].empty()) {
            throw Base::ValueError("No direction reference specified");
        }
        auto* refFeature = static_cast<Part::Feature*>(refObject);
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
        }
        else if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull())
                throw Base::ValueError("Failed to extract direction edge");
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line)
                throw Base::ValueError("Direction edge must be a straight line");

            dir = adapt.Line().Direction();
        }
        else {
            throw Base::ValueError("Direction reference must be edge or face");
        }
    }
    else {
        throw Base::ValueError("Direction reference must be edge/face of a feature or a datum line/plane");
    }

    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    dir.Transform(invObjLoc.Transformation());

    return gp_Vec(dir.X(), dir.Y(), dir.Z());
}

void LinearPattern::updateSpacings()
{
    updateSpacings(false);
    updateSpacings(true);
}

void LinearPattern::updateSpacings(bool isSecondDir)
{

    App::PropertyFloatList& spacingsProp = isSecondDir ? Spacings2 : Spacings;
    App::PropertyLength& offsetProp = isSecondDir ? Offset2 : Offset;
    const App::PropertyIntegerConstraint& occurrencesProp = isSecondDir ? Occurrences2 : Occurrences;

    std::vector<double> spacings = spacingsProp.getValues();
    size_t targetCount = occurrencesProp.getValue() - 1; // 1 less spacing than there are occurrences.

    for (auto& spacing : spacings) {
        if (spacing == offsetProp.getValue()) {
            spacing = -1.0;
        }
    }

    if (spacings.size() == targetCount) {
        return;
    }
    else if (spacings.size() < targetCount) {
        spacings.reserve(targetCount);
        while (spacings.size() < targetCount) {
            spacings.push_back(-1.0);
        }
    }
    else if ((int)spacings.size() > targetCount) {
        spacings.resize(targetCount);
    }

    spacingsProp.setValues(spacings);
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
    auto mode2 = static_cast<LinearPatternMode>(Mode2.getValue());

    if (prop == &Mode) {
        setReadWriteStatusForMode(mode);
    }
    else if (prop == &Occurrences) {
        updateSpacings(false);
    }
    else if (prop == &Mode2) {
        setReadWriteStatusForMode2(mode2);
    }
    else if (prop == &Occurrences2) {
        updateSpacings(true);
    }

    int occurrences = Occurrences.getValue();
    // Keep Length in sync with Offset, catch Occurrences changes
    if (mode == LinearPatternMode::Spacing && (prop == &Offset || prop == &Occurrences)
        && !Length.testStatus(App::Property::Status::Immutable)) {
        if (occurrences == 1) {
            Length.setValue(Offset.getValue());
        }
        else {
            Length.setValue(Offset.getValue() * (occurrences - 1));
        }
    }

    if (mode == LinearPatternMode::Extent && (prop == &Length || prop == &Occurrences)
        && !Offset.testStatus(App::Property::Status::Immutable)) {
        if (occurrences == 1) {
            Offset.setValue(Length.getValue());
        }
        else {
            Offset.setValue(Length.getValue() / (occurrences - 1));
        }
    }

    int occurrences2 = Occurrences2.getValue();
    if (mode2 == LinearPatternMode::Spacing && (prop == &Offset2 || prop == &Occurrences2)
        && !Length2.testStatus(App::Property::Status::Immutable)) {
        if (occurrences2 == 1) {
            Length2.setValue(Offset2.getValue());
        }
        else {
            Length2.setValue(Offset2.getValue() * (occurrences2 - 1));
        }
    }

    if (mode2 == LinearPatternMode::Extent && (prop == &Length2 || prop == &Occurrences2)
        && !Offset2.testStatus(App::Property::Status::Immutable)) {
        if (occurrences2 == 1) {
            Offset2.setValue(Length2.getValue());
        }
        else {
            Offset2.setValue(Length2.getValue() / (occurrences2 - 1));
        }
    }

    Transformed::onChanged(prop);
}

}
