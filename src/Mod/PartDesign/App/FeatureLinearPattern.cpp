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

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::LinearPattern, PartDesign::Transformed)

LinearPattern::LinearPattern()
{
    Part::LinearPatternExtension::initExtension(this);
    Direction.setScope(App::LinkScope::Global);
    Direction2.setScope(App::LinkScope::Global);
}

gp_Dir LinearPattern::getDirectionFromProperty(const App::PropertyLinkSub& dirProp) const
{
    App::DocumentObject* refObject = dirProp.getValue();
    if (!refObject) {
        throw Base::ValueError("No direction reference specified");
    }

    if (refObject->isDerivedFrom<PartDesign::Plane>()
        || refObject->isDerivedFrom<PartDesign::Line>()) {
        Base::Vector3d direction;
        getObjectPlacementInLocalCoordinates(refObject).getRotation().multVec(
            Base::Vector3d(0, 0, 1),
            direction
        );
        gp_Dir dir(direction.x, direction.y, direction.z);
        dir.Transform(getLocation().Inverted().Transformation());
        return Base::convertTo<gp_Vec>(dir);
    }

    const auto& subStrings = dirProp.getSubValues();
    if (subStrings.empty()) {
        throw Base::ValueError("No direction reference specified");
    }

    gp_Dir dir;
    if (auto* refSketch = freecad_cast<Part::Part2DObject*>(refObject)) {
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
            int axisId = std::atoi(subStrings[0].substr(4, 4000).c_str());
            if (axisId < 0 || axisId >= refSketch->getAxisCount()) {
                throw Base::ValueError("No valid direction axis specified");
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
            if (curve.GetType() != GeomAbs_Line) {
                throw Base::TypeError("Direction edge must be a straight line");
            }
            dir = curve.Line().Direction();
            TopLoc_Location invObjLoc = getLocation().Inverted();
            dir.Transform(invObjLoc.Transformation());
            return Base::convertTo<gp_Vec>(dir);
        }
        axis *= getObjectPlacementInLocalCoordinates(refObject);
        dir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    }
    else if (refObject->isDerivedFrom<App::Plane>() || refObject->isDerivedFrom<App::Line>()) {
        Base::Vector3d direction;
        getObjectPlacementInLocalCoordinates(refObject).getRotation().multVec(
            Base::Vector3d(0, 0, 1),
            direction
        );
        dir = gp_Dir(direction.x, direction.y, direction.z);
    }
    else if (refObject->isDerivedFrom<Part::Feature>()) {
        auto shape = getTopoShapeInLocalCoordinates(
            refObject,
            Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink,
            subStrings[0].c_str()
        );
        if (shape.shapeType(true) == TopAbs_FACE) {
            BRepAdaptor_Surface surface(TopoDS::Face(shape.getShape()));
            if (surface.GetType() != GeomAbs_Plane) {
                throw Base::TypeError("Direction face must be planar");
            }
            dir = surface.Plane().Axis().Direction();
        }
        else if (shape.shapeType(true) == TopAbs_EDGE) {
            BRepAdaptor_Curve curve(TopoDS::Edge(shape.getShape()));
            if (curve.GetType() != GeomAbs_Line) {
                throw Base::TypeError("Direction edge must be a straight line");
            }
            dir = curve.Line().Direction();
        }
        else {
            throw Base::TypeError("Direction reference must be an edge or face");
        }
    }
    else {
        throw Base::TypeError(
            "Direction reference must be an edge or face of a feature, datum line or datum plane"
        );
    }

    TopLoc_Location invObjLoc = getLocation().Inverted();
    dir.Transform(invObjLoc.Transformation());
    return Base::convertTo<gp_Vec>(dir);
}

const std::list<gp_Trsf> LinearPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations();
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

}  // namespace PartDesign
