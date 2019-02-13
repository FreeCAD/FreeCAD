/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <gp_Lin.hxx>
# include <gp_Ax2.hxx>
# include <BRepAdaptor_Curve.hxx>
#endif


#include "FeaturePolarPattern.h"
#include "DatumLine.h"
#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>
#include <App/OriginFeature.h>

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::PolarPattern, PartDesign::Transformed)

PolarPattern::PolarPattern()
{
    ADD_PROPERTY_TYPE(Axis,(0),"PolarPattern",(App::PropertyType)(App::Prop_None),"Direction");
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY(Angle,(360.0));
    ADD_PROPERTY(Occurrences,(3));
}

short PolarPattern::mustExecute() const
{
    if (Axis.isTouched() ||
        Reversed.isTouched() ||
        Angle.isTouched() ||
        Occurrences.isTouched())
        return 1;
    return Transformed::mustExecute();
}

std::list<gp_Trsf> PolarPattern::getTransformations(const std::vector<Part::TopoShape> &)
{
    double angle = Angle.getValue();
    if (angle < Precision::Confusion())
        throw Base::ValueError("Pattern angle too small");
    int occurrences = Occurrences.getValue();
    if (occurrences < 2)
        throw Base::ValueError("At least two occurrences required");
    bool reversed = Reversed.getValue();

    double offset;
    if (std::fabs(angle - 360.0) < Precision::Confusion())
        offset = Base::toRadians<double>(angle) / occurrences; // Because e.g. two occurrences in 360 degrees need to be 180 degrees apart
    else
        offset = Base::toRadians<double>(angle) / (occurrences - 1);

    App::DocumentObject* refObject = Axis.getValue();
    if (refObject == NULL)
        throw Base::ValueError("No axis reference specified");
    std::vector<std::string> subStrings = Axis.getSubValues();
    if (subStrings.empty())
        throw Base::ValueError("No axis reference specified");

    gp_Pnt axbase;
    gp_Dir axdir;
    if (refObject->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        Part::Part2DObject* refSketch = static_cast<Part::Part2DObject*>(refObject);
        Base::Axis axis;
        if (subStrings[0] == "H_Axis")
            axis = refSketch->getAxis(Part::Part2DObject::H_Axis);
        else if (subStrings[0] == "V_Axis")
            axis = refSketch->getAxis(Part::Part2DObject::V_Axis);
        else if (subStrings[0] == "N_Axis")
            axis = refSketch->getAxis(Part::Part2DObject::N_Axis);
        else if (subStrings[0].size() > 4 && subStrings[0].substr(0,4) == "Axis") {
            int AxId = std::atoi(subStrings[0].substr(4,4000).c_str());
            if (AxId >= 0 && AxId < refSketch->getAxisCount())
                axis = refSketch->getAxis(AxId);
        }
        axis *= refSketch->Placement.getValue();
        axbase = gp_Pnt(axis.getBase().x, axis.getBase().y, axis.getBase().z);
        axdir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    } else if (refObject->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
        PartDesign::Line* line = static_cast<PartDesign::Line*>(refObject);
        Base::Vector3d base = line->getBasePoint();
        axbase = gp_Pnt(base.x, base.y, base.z);
        Base::Vector3d dir = line->getDirection();
        axdir = gp_Dir(dir.x, dir.y, dir.z);
    } else if (refObject->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
        App::Line* line = static_cast<App::Line*>(refObject);
        Base::Rotation rot = line->Placement.getValue().getRotation();
        Base::Vector3d d(1,0,0);
        rot.multVec(d, d);
        axdir = gp_Dir(d.x, d.y, d.z);
    } else if (refObject->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        if (subStrings[0].empty())
            throw Base::ValueError("No axis reference specified");
        Part::Feature* refFeature = static_cast<Part::Feature*>(refObject);
        Part::TopoShape refShape = refFeature->Shape.getShape();
        TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());

        if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull())
                throw Base::ValueError("Failed to extract axis edge");
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line)
                throw Base::TypeError("Axis edge must be a straight line");

            axbase = adapt.Value(adapt.FirstParameter());
            axdir = adapt.Line().Direction();
        } else {
            throw Base::TypeError("Axis reference must be an edge");
        }
    } else {
        throw Base::TypeError("Axis reference must be edge of a feature or datum line");
    }
    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    axbase.Transform(invObjLoc.Transformation());
    axdir.Transform(invObjLoc.Transformation());

    gp_Ax2 axis(axbase, axdir);

    if (reversed)
        axis.SetDirection(axis.Direction().Reversed());

    // Note: The original feature is NOT included in the list of transformations! Therefore
    // we start with occurrence number 1, not number 0
    std::list<gp_Trsf> transformations;
    gp_Trsf trans;
    transformations.push_back(trans); // identity transformation

    for (int i = 1; i < occurrences; i++) {
        trans.SetRotation(axis.Axis(), i * offset);
        transformations.push_back(trans);
    }

    return transformations;
}

}
