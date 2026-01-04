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

#include <limits>

#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax2.hxx>
#include <BRepAdaptor_Curve.hxx>

#include "DatumLine.h"
#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>
#include <App/Datums.h>

#include "FeaturePolarPattern.h"

using namespace PartDesign;

namespace PartDesign
{


PROPERTY_SOURCE(PartDesign::PolarPattern, PartDesign::Transformed)

const App::PropertyIntegerConstraint::Constraints PolarPattern::intOccurrences
    = {1, std::numeric_limits<int>::max(), 1};
const App::PropertyAngle::Constraints PolarPattern::floatAngle
    = {Base::toDegrees<double>(Precision::Angular()), 360.0, 1.0};

const char* PolarPattern::ModeEnums[] = {"Extent", "Spacing", nullptr};

PolarPattern::PolarPattern()
{
    auto initialMode = PolarPatternMode::Extent;

    ADD_PROPERTY_TYPE(
        Axis,
        (nullptr),
        "PolarPattern",
        App::Prop_None,
        "The axis of rotation for the pattern. This can be a datum axis, a sketch "
        "axis, a circular edge, or the normal of a planar face."
    );
    ADD_PROPERTY_TYPE(
        Reversed,
        (0),
        "PolarPattern",
        App::Prop_None,
        "Reverses the pattern direction from counter-clockwise (default) to clockwise."
    );
    ADD_PROPERTY_TYPE(
        Mode,
        (long(initialMode)),
        "PolarPattern",
        App::Prop_None,
        "Selects how the pattern is dimensioned.\n'Extent': Uses the total angle to contain all "
        "instances.\n'Spacing': Uses the angle between consecutive instances."
    );
    Mode.setEnums(PolarPattern::ModeEnums);
    ADD_PROPERTY_TYPE(
        Angle,
        (360.0),
        "PolarPattern",
        App::Prop_None,
        "The total angle of the pattern, measured from the first to the last "
        "instance. This is only used when the Mode is set to 'Extent'."
    );
    ADD_PROPERTY_TYPE(
        Offset,
        (120.0),
        "PolarPattern",
        App::Prop_None,
        "The angular distance between each instance of the pattern. This is only "
        "used when the Mode is set to 'Spacing'."
    );
    ADD_PROPERTY_TYPE(
        Spacings,
        ({-1.0, -1.0, -1.0}),
        "PolarPattern",
        App::Prop_None,
        "A list of custom angular spacings between instances. If a value is -1, the "
        "global 'Offset' is used for that spacing. The list should have one less "
        "item than the number of occurrences."
    );
    ADD_PROPERTY_TYPE(
        SpacingPattern,
        ({}),
        "PolarPattern",
        App::Prop_None,
        "(Experimental and subject to change. To enable "
        "this in the UI you can add a boolean parameter 'ExperiementalFeature' in "
        "Preferences/Mod/Part)\nDefines a repeating pattern of spacings for the second direction. "
        "For example, a list of [10, 20] will create alternating spacings of 10mm and 20mm."
    );
    ADD_PROPERTY_TYPE(
        Occurrences,
        (3),
        "PolarPattern",
        App::Prop_None,
        "The total number of instances in the pattern, including the original feature."
    );
    Occurrences.setConstraints(&intOccurrences);

    setReadWriteStatusForMode(initialMode);
}

short PolarPattern::mustExecute() const
{
    if (Axis.isTouched() || Reversed.isTouched() || Mode.isTouched() ||
        // Angle and Offset are mutually exclusive, only one could be updated at once
        Angle.isTouched() || Offset.isTouched() || Spacings.isTouched()
        || SpacingPattern.isTouched() || Occurrences.isTouched()) {
        return 1;
    }
    return Transformed::mustExecute();
}

const std::list<gp_Trsf> PolarPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    int occurrences = Occurrences.getValue();
    if (occurrences < 1) {
        throw Base::ValueError("At least one occurrence required");
    }

    if (occurrences == 1) {
        return {gp_Trsf()};
    }

    bool reversed = Reversed.getValue();

    App::DocumentObject* refObject = Axis.getValue();
    if (!refObject) {
        throw Base::ValueError("No axis reference specified");
    }
    std::vector<std::string> subStrings = Axis.getSubValues();
    if (subStrings.empty()) {
        throw Base::ValueError("No axis reference specified");
    }

    gp_Pnt axbase;
    gp_Dir axdir;
    if (refObject->isDerivedFrom<Part::Part2DObject>()) {
        Part::Part2DObject* refSketch = static_cast<Part::Part2DObject*>(refObject);
        Base::Axis axis;
        if (subStrings[0] == "H_Axis") {
            axis = refSketch->getAxis(Part::Part2DObject::H_Axis);
        }
        else if (subStrings[0] == "V_Axis") {
            axis = refSketch->getAxis(Part::Part2DObject::V_Axis);
        }
        else if (subStrings[0] == "N_Axis") {
            axis = refSketch->getAxis(Part::Part2DObject::N_Axis);
        }
        else if (subStrings[0].compare(0, 4, "Axis") == 0) {
            int AxId = std::atoi(subStrings[0].substr(4, 4000).c_str());
            if (AxId >= 0 && AxId < refSketch->getAxisCount()) {
                axis = refSketch->getAxis(AxId);
            }
        }
        axis *= refSketch->Placement.getValue();
        axbase = gp_Pnt(axis.getBase().x, axis.getBase().y, axis.getBase().z);
        axdir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    }
    else if (refObject->isDerivedFrom<PartDesign::Line>()) {
        PartDesign::Line* line = static_cast<PartDesign::Line*>(refObject);
        Base::Vector3d base = line->getBasePoint();
        axbase = gp_Pnt(base.x, base.y, base.z);
        Base::Vector3d dir = line->getDirection();
        axdir = gp_Dir(dir.x, dir.y, dir.z);
    }
    else if (refObject->isDerivedFrom<App::Line>()) {
        App::Line* line = static_cast<App::Line*>(refObject);
        Base::Vector3d base = line->getBasePoint();
        axbase = gp_Pnt(base.x, base.y, base.z);
        Base::Vector3d d = line->getDirection();
        axdir = gp_Dir(d.x, d.y, d.z);
    }
    else if (refObject->isDerivedFrom<Part::Feature>()) {
        if (subStrings[0].empty()) {
            throw Base::ValueError("No axis reference specified");
        }
        Part::Feature* refFeature = static_cast<Part::Feature*>(refObject);
        Part::TopoShape refShape = refFeature->Shape.getShape();
        TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());

        if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull()) {
                throw Base::ValueError("Failed to extract axis edge");
            }
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() == GeomAbs_Line) {
                axbase = adapt.Line().Location();
                axdir = adapt.Line().Direction();
            }
            else if (adapt.GetType() == GeomAbs_Circle) {
                axbase = adapt.Circle().Location();
                axdir = adapt.Circle().Axis().Direction();
            }
            else {
                throw Base::TypeError("Rotation edge must be a straight line, circle or arc of circle");
            }
        }
        else {
            throw Base::TypeError("Axis reference must be an edge");
        }
    }
    else {
        throw Base::TypeError("Axis reference must be edge of a feature or datum line");
    }
    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    axbase.Transform(invObjLoc.Transformation());
    axdir.Transform(invObjLoc.Transformation());

    gp_Ax2 axis(axbase, axdir);

    if (reversed) {
        axis.SetDirection(axis.Direction().Reversed());
    }

    double angle;

    if (Mode.getValue() == (int)PolarPatternMode::Extent) {
        angle = Angle.getValue();

        if (std::fabs(angle - 360.0) < Precision::Confusion()) {
            angle /= occurrences;  // Because e.g. two occurrences in 360 degrees need to be 180
                                   // degrees apart
        }
        else {
            angle /= occurrences - 1;
        }

        angle = Base::toRadians(angle);
        if (fabs(angle) < Precision::Angular()) {
            throw Base::ValueError("Pattern angle can't be null");
        }
    }

    // make sure spacings are correct size :
    updateSpacings();

    const std::vector<double> spacings = Spacings.getValues();
    const std::vector<double> pattern = SpacingPattern.getValues();
    bool usePattern = pattern.size() > 1;

    double cumulativeSpacings = 0.0;

    std::list<gp_Trsf> transformations;
    gp_Trsf trans;
    transformations.push_back(trans);


    // Note: The original feature is already included in the list of transformations!
    // Therefore we start with occurrence number 1
    for (int i = 1; i < occurrences; i++) {
        if (Mode.getValue() == (int)PolarPatternMode::Spacing) {
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
            cumulativeSpacings += Base::toRadians(spacing);
            trans.SetRotation(axis.Axis(), cumulativeSpacings);
        }
        else {
            trans.SetRotation(axis.Axis(), angle * i);
        }
        transformations.push_back(trans);
    }

    return transformations;
}

void PolarPattern::handleChangedPropertyType(
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


void PolarPattern::onChanged(const App::Property* prop)
{
    if (prop == &Mode) {
        auto mode = static_cast<PolarPatternMode>(Mode.getValue());
        setReadWriteStatusForMode(mode);
    }

    Transformed::onChanged(prop);
}

void PolarPattern::setReadWriteStatusForMode(PolarPatternMode mode)
{
    Offset.setReadOnly(mode != PolarPatternMode::Spacing);
    Angle.setReadOnly(mode != PolarPatternMode::Extent);
}


void PolarPattern::updateSpacings()
{
    std::vector<double> spacings = Spacings.getValues();
    size_t targetCount = Occurrences.getValue() - 1;

    for (auto& spacing : spacings) {
        if (spacing == Offset.getValue()) {
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

    Spacings.setValues(spacings);
}

}  // namespace PartDesign
