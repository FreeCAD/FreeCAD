// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include "PolarPatternExtension.h"
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
#include <Base/Tools.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>
#include <App/DocumentObject.h>
#include "PartFeature.h"

using namespace Part;

EXTENSION_PROPERTY_SOURCE(Part::PolarPatternExtension, App::DocumentObjectExtension)

const App::PropertyIntegerConstraint::Constraints PolarPatternExtension::intOccurrences
    = {1, std::numeric_limits<int>::max(), 1};

const char* PolarPatternExtension::ModeEnums[] = {"Extent", "Spacing", nullptr};

PolarPatternExtension::PolarPatternExtension()
{
    initExtensionType(PolarPatternExtension::getExtensionClassTypeId());

    auto initialMode = PolarPatternMode::Extent;

    EXTENSION_ADD_PROPERTY_TYPE(
        Axis,
        (nullptr),
        "PolarPattern",
        App::Prop_None,
        "The axis of rotation for the pattern. This can be a datum axis, a sketch "
        "axis, a circular edge, or the normal of a planar face."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Reversed,
        (0),
        "PolarPattern",
        App::Prop_None,
        "Reverses the pattern direction from counter-clockwise (default) to clockwise."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Mode,
        (long(initialMode)),
        "PolarPattern",
        App::Prop_None,
        "Selects how the pattern is dimensioned.\n'Extent': Uses the total angle to contain all "
        "instances.\n'Spacing': Uses the angle between consecutive instances."
    );
    Mode.setEnums(PolarPatternExtension::ModeEnums);
    EXTENSION_ADD_PROPERTY_TYPE(
        Angle,
        (360.0),
        "PolarPattern",
        App::Prop_None,
        "The total angle of the pattern, measured from the first to the last "
        "instance. This is only used when the Mode is set to 'Extent'."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Offset,
        (120.0),
        "PolarPattern",
        App::Prop_None,
        "The angular distance between each instance of the pattern. This is only "
        "used when the Mode is set to 'Spacing'."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Spacings,
        ({-1.0, -1.0, -1.0}),
        "PolarPattern",
        App::Prop_None,
        "A list of custom angular spacings between instances. If a value is -1, the "
        "global 'Offset' is used for that spacing. The list should have one less "
        "item than the number of occurrences."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        SpacingPattern,
        ({}),
        "PolarPattern",
        App::Prop_None,
        "(Experimental and subject to change. To enable "
        "this in the UI you can add a boolean parameter 'ExperiementalFeature' in "
        "Preferences/Mod/Part)\nDefines a repeating pattern of spacings for the second direction. "
        "For example, a list of [10, 20] will create alternating spacings of 10mm and 20mm."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Occurrences,
        (3),
        "PolarPattern",
        App::Prop_None,
        "The total number of instances in the pattern, including the original feature."
    );
    Occurrences.setConstraints(&intOccurrences);

    setReadWriteStatusForMode(initialMode);
}

const std::list<gp_Trsf> PolarPatternExtension::calculateTransformations() const
{
    int occurrences = Occurrences.getValue();
    if (occurrences < 1) {
        throw Base::ValueError("At least one occurrence required");
    }

    if (occurrences == 1) {
        return {gp_Trsf()};
    }

    gp_Ax2 axis = getRotation();

    double angle = 0.0;

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
    const_cast<PolarPatternExtension*>(this)->updateSpacings();

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

gp_Ax2 PolarPatternExtension::getRotation() const
{
    App::DocumentObject* refObject = Axis.getValue();
    if (!refObject) {
        return gp_Ax2();
    }
    std::vector<std::string> subStrings = Axis.getSubValues();
    if (subStrings.empty()) {
        return gp_Ax2();
    }

    gp_Pnt axbase;
    gp_Dir axdir;
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
            }
            axis *= refSketch->Placement.getValue();
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

            axis.setBase(Base::Vector3d(p.X(), p.Y(), p.Z()));
            axis.setDirection(Base::Vector3d(d.X(), d.Y(), d.Z()));
        }

        axbase = gp_Pnt(axis.getBase().x, axis.getBase().y, axis.getBase().z);
        axdir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    }
    else if (auto* line = freecad_cast<App::Line*>(refObject)) {
        Base::Vector3d base = line->getBasePoint();
        axbase = gp_Pnt(base.x, base.y, base.z);
        Base::Vector3d d = line->getDirection();
        axdir = gp_Dir(d.x, d.y, d.z);
    }
    else if (auto* refFeature = freecad_cast<Part::Feature*>(refObject)) {
        if (subStrings[0].empty()) {
            throw Base::ValueError("No axis reference specified");
        }
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

    TopLoc_Location loc;
    if (auto* feat = freecad_cast<Part::Feature*>(getExtendedObject())) {
        loc = feat->getLocation();
    }
    else if (auto* obj = getExtendedObject()) {
        if (auto* prop = freecad_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"))) {
            Base::Placement pl = prop->getValue();
            Base::Rotation rot(pl.getRotation());
            Base::Vector3d axis;
            double angle;
            rot.getValue(axis, angle);
            gp_Trsf trf;
            trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(axis.x, axis.y, axis.z)), angle);
            trf.SetTranslationPart(gp_Vec(pl.getPosition().x, pl.getPosition().y, pl.getPosition().z));
            loc = TopLoc_Location(trf);
        }
    }

    // Transform the axis into the local coordinate system of this feature.
    TopLoc_Location invObjLoc = loc.Inverted();
    axbase.Transform(invObjLoc.Transformation());
    axdir.Transform(invObjLoc.Transformation());

    gp_Ax2 axis(axbase, axdir);

    if (Reversed.getValue()) {
        axis.SetDirection(axis.Direction().Reversed());
    }

    return axis;
}

void PolarPatternExtension::setReadWriteStatusForMode(PolarPatternMode mode)
{
    Offset.setReadOnly(mode != PolarPatternMode::Spacing);
    Angle.setReadOnly(mode != PolarPatternMode::Extent);
}

void PolarPatternExtension::updateSpacings()
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

short PolarPatternExtension::extensionMustExecute()
{
    if (Axis.isTouched() || Reversed.isTouched() || Mode.isTouched() || Angle.isTouched()
        || Offset.isTouched() || Spacings.isTouched() || SpacingPattern.isTouched()
        || Occurrences.isTouched()) {
        return 1;
    }
    return 0;
}

void PolarPatternExtension::extensionOnChanged(const App::Property* prop)
{
    auto* parent = getExtendedObject();
    if (!parent || parent->isRestoring()) {
        App::DocumentObjectExtension::extensionOnChanged(prop);
        return;
    }

    if (prop == &Mode) {
        auto mode = static_cast<PolarPatternMode>(Mode.getValue());
        setReadWriteStatusForMode(mode);
    }

    App::DocumentObjectExtension::extensionOnChanged(prop);
}
