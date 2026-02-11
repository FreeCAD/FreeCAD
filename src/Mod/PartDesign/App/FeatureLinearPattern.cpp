// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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

#include <limits>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <App/Datums.h>
#include <Base/Axis.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeatureLinearPattern.h"
#include "DatumLine.h"
#include "DatumPlane.h"


using namespace PartDesign;

namespace PartDesign
{


PROPERTY_SOURCE(PartDesign::LinearPattern, PartDesign::Transformed)

const App::PropertyIntegerConstraint::Constraints LinearPattern::intOccurrences
    = {1, std::numeric_limits<int>::max(), 1};

const char* LinearPattern::ModeEnums[] = {"Extent", "Spacing", nullptr};

LinearPattern::LinearPattern()
{
    auto initialMode = LinearPatternMode::Extent;

    ADD_PROPERTY_TYPE(
        Direction,
        (nullptr),
        "Direction 1",
        App::Prop_None,
        "The first direction of the pattern. This can be a straight edge, a datum "
        "line, a sketch axis, or the normal of a planar face."
    );
    ADD_PROPERTY_TYPE(
        Reversed,
        (0),
        "Direction 1",
        App::Prop_None,
        "Reverse the first direction of the pattern"
    );
    ADD_PROPERTY_TYPE(
        Mode,
        (long(initialMode)),
        "Direction 1",
        App::Prop_None,
        "Selects how the pattern is dimensioned.\n'Extent': Uses the total length from the first "
        "to the last instance.\n'Spacing': Uses the distance between consecutive instances."
    );
    ADD_PROPERTY_TYPE(
        Length,
        (100.0),
        "Direction 1",
        App::Prop_None,
        "The total length of the pattern, measured from the first to the last "
        "instance. This is only used when the Mode is set to 'Extent'."
    );
    ADD_PROPERTY_TYPE(
        Offset,
        (10.0),
        "Direction 1",
        App::Prop_None,
        "The distance between each instance of the pattern. This is only used when "
        "the Mode is set to 'Spacing'."
    );
    ADD_PROPERTY_TYPE(
        Spacings,
        ({-1.0}),
        "Direction 1",
        App::Prop_None,
        "A list of custom spacings between instances. If a value is -1, the global "
        "'Offset' is used for that spacing. The list should have one less item than "
        "the number of occurrences."
    );
    ADD_PROPERTY_TYPE(
        SpacingPattern,
        ({}),
        "Direction 1",
        App::Prop_None,
        "(Experimental and subject to change. To enable "
        "this in the UI you can add a boolean parameter 'ExperiementalFeature' in "
        "Preferences/Mod/Part)\nDefines a repeating pattern of spacings for the second direction. "
        "For example, a list of [10, 20] will create alternating spacings of 10mm and 20mm."
    );
    ADD_PROPERTY_TYPE(
        Occurrences,
        (2),
        "Direction 1",
        App::Prop_None,
        "The total number of instances in the first direction, including the original feature."
    );
    Occurrences.setConstraints(&intOccurrences);
    Mode.setEnums(ModeEnums);
    setReadWriteStatusForMode(LinearPatternDirection::First);

    ADD_PROPERTY_TYPE(
        Direction2,
        (nullptr),
        "Direction 2",
        App::Prop_None,
        "The second direction of the pattern. This can be a straight edge, a datum "
        "line, a sketch axis, or the normal of a planar face."
    );
    ADD_PROPERTY_TYPE(
        Reversed2,
        (0),
        "Direction 2",
        App::Prop_None,
        "Reverse the second direction of the pattern"
    );
    ADD_PROPERTY_TYPE(
        Mode2,
        (long(initialMode)),
        "Direction 2",
        App::Prop_None,
        "Selects how the pattern is dimensioned in the second direction.\n'Extent': "
        "Uses the total length.\n'Spacing': Uses the distance between instances."
    );
    ADD_PROPERTY_TYPE(
        Length2,
        (100.0),
        "Direction 2",
        App::Prop_None,
        "The total length of the pattern in the second direction, measured from the first to the "
        "last instance. This is only used when the Mode is set to 'Extent'."
    );
    ADD_PROPERTY_TYPE(
        Offset2,
        (10.0),
        "Direction 2",
        App::Prop_None,
        "The distance between each instance of the pattern in the second direction. "
        "This is only used when the Mode is set to 'Spacing'."
    );
    ADD_PROPERTY_TYPE(
        Spacings2,
        ({}),
        "Direction 2",
        App::Prop_None,
        "A list of custom spacings for the second direction. If a value is -1, the global 'Offset' "
        "is used. The list should have one less item than the number of occurrences."
    );
    ADD_PROPERTY_TYPE(
        SpacingPattern2,
        ({}),
        "Direction 2",
        App::Prop_None,
        "(Experimental and subject to change. To enable "
        "this in the UI you can add a boolean parameter 'ExperiementalFeature' in "
        "Preferences/Mod/Part)\nDefines a repeating pattern of spacings for the second direction. "
        "For example, a list of [10, 20] will create alternating spacings of 10mm and 20mm."
    );
    ADD_PROPERTY_TYPE(
        Occurrences2,
        (1),
        "Direction 2",
        App::Prop_None,
        "The total number of instances in the second direction, including the original feature."
    );
    Occurrences2.setConstraints(&intOccurrences);
    Mode2.setEnums(ModeEnums);
    setReadWriteStatusForMode(LinearPatternDirection::Second);
}

short LinearPattern::mustExecute() const
{
    if (Direction.isTouched() || Reversed.isTouched() || Mode.isTouched() ||
        // Length and Offset are mutually exclusive, only one could be updated at once
        Length.isTouched() || Offset.isTouched() || Spacings.isTouched()
        || SpacingPattern.isTouched() || Occurrences.isTouched() || Direction2.isTouched()
        || Reversed2.isTouched() || Mode2.isTouched() || Length2.isTouched() || Offset2.isTouched()
        || Spacings2.isTouched() || SpacingPattern2.isTouched() || Occurrences2.isTouched()) {
        return 1;
    }
    return Transformed::mustExecute();
}

void LinearPattern::setReadWriteStatusForMode(LinearPatternDirection dir)
{
    bool isExtentMode = false;
    if (dir == LinearPatternDirection::First) {
        isExtentMode = (Mode.getValue() == static_cast<long>(LinearPatternMode::Extent));
        Length.setReadOnly(!isExtentMode);
        Offset.setReadOnly(isExtentMode);
    }
    else {
        isExtentMode = (Mode2.getValue() == static_cast<long>(LinearPatternMode::Extent));
        Length2.setReadOnly(!isExtentMode);
        Offset2.setReadOnly(isExtentMode);
    }
}

const std::list<gp_Trsf> LinearPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    int occurrences = Occurrences.getValue();
    int occurrences2 = Occurrences2.getValue();
    if (occurrences < 1 || occurrences2 < 1) {
        throw Base::ValueError("At least one occurrence required");
    }

    if (occurrences == 1 && occurrences2 == 1) {
        return {gp_Trsf()};
    }

    // make sure spacings are correct size :
    updateSpacings();

    // Calculate the base offset vector and final step positions for each direction
    gp_Vec offset1 = calculateOffsetVector(LinearPatternDirection::First);
    std::vector<gp_Vec> steps1 = calculateSteps(LinearPatternDirection::First, offset1);

    gp_Vec offset2 = calculateOffsetVector(LinearPatternDirection::Second);
    std::vector<gp_Vec> steps2 = calculateSteps(LinearPatternDirection::Second, offset2);

    // Combine the steps from both directions
    std::list<gp_Trsf> transformations;
    for (const auto& step1 : steps1) {
        for (const auto& step2 : steps2) {
            gp_Trsf trans;
            trans.SetTranslation(step1 + step2);
            transformations.push_back(trans);
        }
    }

    return transformations;
}

gp_Vec LinearPattern::calculateOffsetVector(LinearPatternDirection dir) const
{
    bool firstDir = dir == LinearPatternDirection::First;
    const auto& occurrencesProp = firstDir ? Occurrences : Occurrences2;
    int occurrences = occurrencesProp.getValue();
    if (occurrences <= 1) {
        return gp_Vec();  // Return zero vector if no transformation is needed
    }

    const auto& dirProp = firstDir ? Direction : Direction2;
    if (!dirProp.getValue()) {
        return gp_Vec();
    }
    const auto& reversedProp = firstDir ? Reversed : Reversed2;
    const auto& modeProp = firstDir ? Mode : Mode2;
    const auto& lengthProp = firstDir ? Length : Length2;

    double distance = lengthProp.getValue();
    if (distance < Precision::Confusion()) {
        throw Base::ValueError("Pattern length too small");
    }

    gp_Vec offset = getDirectionFromProperty(dirProp);
    if (reversedProp.getValue()) {
        offset.Reverse();
    }

    // For 'Extent' mode, the vector represents one full step.
    // For 'Spacing' mode, it's just a normalized direction vector.
    if (static_cast<LinearPatternMode>(modeProp.getValue()) == LinearPatternMode::Extent) {
        offset *= distance / (occurrences - 1);
    }

    return offset;
}

std::vector<gp_Vec> LinearPattern::calculateSteps(
    LinearPatternDirection dir,
    const gp_Vec& offsetVector
) const
{
    const auto& occurrencesProp = (dir == LinearPatternDirection::First) ? Occurrences : Occurrences2;
    const auto& modeProp = (dir == LinearPatternDirection::First) ? Mode : Mode2;
    const auto& offsetValueProp = (dir == LinearPatternDirection::First) ? Offset : Offset2;
    const auto& spacingsProp = (dir == LinearPatternDirection::First) ? Spacings : Spacings2;
    const auto& spacingPatternProp = (dir == LinearPatternDirection::First) ? SpacingPattern
                                                                            : SpacingPattern2;

    int occurrences = occurrencesProp.getValue();
    std::vector<gp_Vec> steps {gp_Vec()};  // First step is always zero
    steps.reserve(occurrences);

    if (occurrences <= 1) {
        return steps;
    }

    if (modeProp.getValue() == static_cast<int>(LinearPatternMode::Spacing)) {
        const std::vector<double> spacings = spacingsProp.getValues();
        const std::vector<double> pattern = spacingPatternProp.getValues();
        bool usePattern = pattern.size() > 1;
        double cumulativeDistance = 0.0;

        // Spacing priority: individual spacing > pattern > global offset
        const auto spacingAt = [&](unsigned i) {
            if (spacings.at(i - 1) != -1.0) {
                return spacings.at(i - 1);
            }

            if (usePattern) {
                return pattern.at(static_cast<size_t>(fmod(i - 1, pattern.size())));
            }

            return offsetValueProp.getValue();
        };

        for (int i = 1; i < occurrences; ++i) {
            cumulativeDistance += spacingAt(i);
            steps.push_back(offsetVector * cumulativeDistance);
        }
    }
    else {  // Extent Mode
        for (int i = 1; i < occurrences; ++i) {
            steps.push_back(offsetVector * i);
        }
    }

    return steps;
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
    if (auto* refSketch = freecad_cast<Part::Part2DObject*>(refObject)) {
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
            if (refEdge.IsNull()) {
                throw Base::ValueError("Failed to extract direction edge");
            }
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line) {
                throw Base::TypeError("Direction edge must be a straight line");
            }

            gp_Pnt p = adapt.Line().Location();
            gp_Dir d = adapt.Line().Direction();

            // the axis is not given in local coordinates and mustn't be multiplied with the
            // placement
            axis.setBase(Base::Vector3d(p.X(), p.Y(), p.Z()));
            axis.setDirection(Base::Vector3d(d.X(), d.Y(), d.Z()));
        }
        dir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    }
    else if (auto* plane = freecad_cast<PartDesign::Plane*>(refObject)) {
        Base::Vector3d d = plane->getNormal();
        dir = gp_Dir(d.x, d.y, d.z);
    }
    else if (auto* line = freecad_cast<PartDesign::Line*>(refObject)) {
        Base::Vector3d d = line->getDirection();
        dir = gp_Dir(d.x, d.y, d.z);
    }
    else if (auto* plane = freecad_cast<App::Plane*>(refObject)) {
        Base::Vector3d d = plane->getDirection();
        dir = gp_Dir(d.x, d.y, d.z);
    }
    else if (auto* line = freecad_cast<App::Line*>(refObject)) {
        Base::Vector3d d = line->getDirection();
        dir = gp_Dir(d.x, d.y, d.z);
    }
    else if (auto* refFeature = freecad_cast<Part::Feature*>(refObject)) {
        if (subStrings[0].empty()) {
            throw Base::ValueError("No direction reference specified");
        }
        Part::TopoShape refShape = refFeature->Shape.getShape();
        TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());

        if (ref.ShapeType() == TopAbs_FACE) {
            TopoDS_Face refFace = TopoDS::Face(ref);
            if (refFace.IsNull()) {
                throw Base::ValueError("Failed to extract direction plane");
            }
            BRepAdaptor_Surface adapt(refFace);
            if (adapt.GetType() != GeomAbs_Plane) {
                throw Base::TypeError("Direction face must be planar");
            }

            dir = adapt.Plane().Axis().Direction();
        }
        else if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull()) {
                throw Base::ValueError("Failed to extract direction edge");
            }
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line) {
                throw Base::ValueError("Direction edge must be a straight line");
            }

            dir = adapt.Line().Direction();
        }
        else {
            throw Base::ValueError("Direction reference must be edge or face");
        }
    }
    else {
        throw Base::ValueError(
            "Direction reference must be edge/face of a feature or a datum line/plane"
        );
    }

    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    dir.Transform(invObjLoc.Transformation());

    return Base::convertTo<gp_Vec>(dir);
}

void LinearPattern::updateSpacings()
{
    updateSpacings(LinearPatternDirection::First);
    updateSpacings(LinearPatternDirection::Second);
}

void LinearPattern::updateSpacings(LinearPatternDirection dir)
{
    bool isSecondDir = dir == LinearPatternDirection::Second;

    App::PropertyFloatList& spacingsProp = isSecondDir ? Spacings2 : Spacings;
    App::PropertyLength& offsetProp = isSecondDir ? Offset2 : Offset;
    const App::PropertyIntegerConstraint& occurrencesProp = isSecondDir ? Occurrences2 : Occurrences;

    std::vector<double> spacings = spacingsProp.getValues();
    size_t targetCount = occurrencesProp.getValue()
        - 1;  // 1 less spacing than there are occurrences.

    for (auto& spacing : spacings) {
        if (spacing == offsetProp.getValue()) {
            spacing = -1.0;
        }
    }

    if (spacings.size() == targetCount) {
        return;
    }
    if (spacings.size() < targetCount) {
        spacings.reserve(targetCount);
        while (spacings.size() < targetCount) {
            spacings.push_back(-1.0);
        }
    }
    else {
        spacings.resize(targetCount);
    }

    spacingsProp.setValues(spacings);
}

void LinearPattern::handleChangedPropertyType(
    Base::XMLReader& reader,
    const char* TypeName,
    App::Property* prop
)
// transforms properties that had been changed
{
    // property Occurrences had the App::PropertyInteger and was changed to
    // App::PropertyIntegerConstraint
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
        setReadWriteStatusForMode(LinearPatternDirection::First);
    }
    else if (prop == &Occurrences) {
        updateSpacings(LinearPatternDirection::First);
        syncLengthAndOffset(LinearPatternDirection::First);
    }
    else if (prop == &Offset && mode == LinearPatternMode::Spacing) {
        syncLengthAndOffset(LinearPatternDirection::First);
    }
    else if (prop == &Length && mode == LinearPatternMode::Extent) {
        syncLengthAndOffset(LinearPatternDirection::First);
    }

    else if (prop == &Mode2) {
        setReadWriteStatusForMode(LinearPatternDirection::Second);
    }
    else if (prop == &Occurrences2) {
        updateSpacings(LinearPatternDirection::Second);
        syncLengthAndOffset(LinearPatternDirection::Second);
    }
    else if (prop == &Offset2 && mode2 == LinearPatternMode::Spacing) {
        syncLengthAndOffset(LinearPatternDirection::Second);
    }
    else if (prop == &Length2 && mode2 == LinearPatternMode::Extent) {
        syncLengthAndOffset(LinearPatternDirection::Second);
    }

    Transformed::onChanged(prop);
}

void LinearPattern::syncLengthAndOffset(LinearPatternDirection dir)
{
    // Get references to the correct properties based on the direction
    auto& modeProp = (dir == LinearPatternDirection::First) ? Mode : Mode2;
    auto& lengthProp = (dir == LinearPatternDirection::First) ? Length : Length2;
    auto& offsetProp = (dir == LinearPatternDirection::First) ? Offset : Offset2;
    auto& occurrencesProp = (dir == LinearPatternDirection::First) ? Occurrences : Occurrences2;

    auto mode = static_cast<LinearPatternMode>(modeProp.getValue());
    int occurrences = occurrencesProp.getValue();
    occurrences = occurrences <= 1 ? 1 : occurrences - 1;

    if (mode == LinearPatternMode::Spacing
        && !lengthProp.testStatus(App::Property::Status::Immutable)) {
        lengthProp.setValue(offsetProp.getValue() * occurrences);
    }
    else if (mode == LinearPatternMode::Extent
             && !offsetProp.testStatus(App::Property::Status::Immutable)) {
        offsetProp.setValue(lengthProp.getValue() / occurrences);
    }
}

}  // namespace PartDesign
