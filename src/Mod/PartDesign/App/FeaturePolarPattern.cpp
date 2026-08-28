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

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::PolarPattern, PartDesign::Transformed)

PolarPattern::PolarPattern()
{
    Part::PolarPatternExtension::initExtension(this);
    Axis.setScope(App::LinkScope::Global);
}

gp_Ax2 PolarPattern::getRotation() const
{
    App::DocumentObject* refObject = Axis.getValue();
    if (!refObject) {
        return gp_Ax2();
    }

    if (refObject->isDerivedFrom<PartDesign::Line>()) {
        Base::Placement placement = getObjectPlacementInLocalCoordinates(refObject);
        const Base::Vector3d& base = placement.getPosition();
        Base::Vector3d direction;
        placement.getRotation().multVec(Base::Vector3d(0, 0, 1), direction);
        gp_Pnt axbase(base.x, base.y, base.z);
        gp_Dir axdir(direction.x, direction.y, direction.z);
        gp_Trsf transform = getLocation().Inverted().Transformation();
        axbase.Transform(transform);
        axdir.Transform(transform);

        gp_Ax2 axis(axbase, axdir);
        if (Reversed.getValue()) {
            axis.SetDirection(axis.Direction().Reversed());
        }
        return axis;
    }

    const auto& subStrings = Axis.getSubValues();
    if (subStrings.empty()) {
        return gp_Ax2();
    }

    gp_Pnt axbase;
    gp_Dir axdir;
    if (auto* refSketch = freecad_cast<Part::Part2DObject*>(refObject)) {
        Base::Axis axis;
        bool shapeAxis = false;
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
            int axisId = std::atoi(subStrings[0].substr(4, 4000).c_str());
            if (axisId < 0 || axisId >= refSketch->getAxisCount()) {
                throw Base::ValueError("No valid rotation axis specified");
            }
            axis = refSketch->getAxis(axisId);
        }
        else {
            auto shape = getTopoShapeInLocalCoordinates(
                refObject,
                Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink,
                subStrings[0].c_str()
            );
            BRepAdaptor_Curve curve(TopoDS::Edge(shape.getShape()));
            if (curve.GetType() != GeomAbs_Line && curve.GetType() != GeomAbs_Circle) {
                throw Base::TypeError("Rotation edge must be a straight line, circle or arc of circle");
            }
            axbase = curve.GetType() == GeomAbs_Line ? curve.Line().Location()
                                                     : curve.Circle().Location();
            axdir = curve.GetType() == GeomAbs_Line ? curve.Line().Direction()
                                                    : curve.Circle().Axis().Direction();
            shapeAxis = true;
        }
        if (!shapeAxis) {
            axis *= getObjectPlacementInLocalCoordinates(refObject);
            const Base::Vector3d& base = axis.getBase();
            const Base::Vector3d& direction = axis.getDirection();
            axbase = gp_Pnt(base.x, base.y, base.z);
            axdir = gp_Dir(direction.x, direction.y, direction.z);
        }
    }
    else if (refObject->isDerivedFrom<App::Line>()) {
        Base::Placement placement = getObjectPlacementInLocalCoordinates(refObject);
        const Base::Vector3d& base = placement.getPosition();
        axbase = gp_Pnt(base.x, base.y, base.z);
        Base::Vector3d direction;
        placement.getRotation().multVec(Base::Vector3d(0, 0, 1), direction);
        axdir = gp_Dir(direction.x, direction.y, direction.z);
    }
    else if (refObject->isDerivedFrom<Part::Feature>()) {
        auto shape = getTopoShapeInLocalCoordinates(
            refObject,
            Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink,
            subStrings[0].c_str()
        );
        BRepAdaptor_Curve curve(TopoDS::Edge(shape.getShape()));
        if (curve.GetType() == GeomAbs_Line) {
            axbase = curve.Line().Location();
            axdir = curve.Line().Direction();
        }
        else if (curve.GetType() == GeomAbs_Circle) {
            axbase = curve.Circle().Location();
            axdir = curve.Circle().Axis().Direction();
        }
        else {
            throw Base::TypeError("Rotation edge must be a straight line, circle or arc of circle");
        }
    }
    else {
        throw Base::TypeError("Axis reference must be an edge of a feature or datum line");
    }

    TopLoc_Location invObjLoc = getLocation().Inverted();
    axbase.Transform(invObjLoc.Transformation());
    axdir.Transform(invObjLoc.Transformation());

    gp_Ax2 axis(axbase, axdir);
    if (Reversed.getValue()) {
        axis.SetDirection(axis.Direction().Reversed());
    }
    return axis;
}

const std::list<gp_Trsf> PolarPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations();
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

}  // namespace PartDesign
