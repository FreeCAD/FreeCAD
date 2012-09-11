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
#include <Base/Tools.h>
#include <Base/Axis.h>
#include <Mod/Part/App/TopoShape.h>

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::PolarPattern, PartDesign::Transformed)

PolarPattern::PolarPattern()
{
    ADD_PROPERTY_TYPE(Axis,(0),"PolarPattern",(App::PropertyType)(App::Prop_None),"Direction");
    ADD_PROPERTY(StdAxis,(""));
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY(Angle,(360.0));
    ADD_PROPERTY(Occurrences,(3));
}

short PolarPattern::mustExecute() const
{
    if (Axis.isTouched() ||
        StdAxis.isTouched() ||
        Reversed.isTouched() ||
        Angle.isTouched() ||
        Occurrences.isTouched())
        return 1;
    return Transformed::mustExecute();
}

const std::list<gp_Trsf> PolarPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    std::string stdAxis = StdAxis.getValue();
    float angle = Angle.getValue();
    if (angle < Precision::Confusion())
        throw Base::Exception("Pattern angle too small");
    int occurrences = Occurrences.getValue();
    if (occurrences < 2)
        throw Base::Exception("At least two occurrences required");
    bool reversed = Reversed.getValue();

    double offset;
    if (std::abs(angle - 360.0) < Precision::Confusion())
        offset = Base::toRadians<double>(angle) / occurrences; // Because e.g. two occurrences in 360 degrees need to be 180 degrees apart
    else
        offset = Base::toRadians<double>(angle) / (occurrences - 1);

    gp_Pnt axbase;
    gp_Dir axdir;
    if (!stdAxis.empty()) {
        axbase = gp_Pnt(0,0,0);
        if (stdAxis == "X") {
            axdir = gp_Dir(1,0,0);
            //ax = Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(1,0,0));
        } else if (stdAxis == "Y") {
            axdir = gp_Dir(0,1,0);
            //ax = Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(0,1,0));
        } else if(stdAxis == "Z") {
            axdir = gp_Dir(0,0,1);
            //ax = Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(0,0,1));
        } else {
            throw Base::Exception("Invalid axis (must be X, Y or Z)");
        }
    } else {
        App::DocumentObject* refObject = Axis.getValue();
        if (refObject == NULL)
            throw Base::Exception("No axis specified");
        if (!refObject->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
            throw Base::Exception("Axis reference must be edge of a feature");
        std::vector<std::string> subStrings = Axis.getSubValues();
        if (subStrings.empty() || subStrings[0].empty())
            throw Base::Exception("No axis reference specified");

        Part::Feature* refFeature = static_cast<Part::Feature*>(refObject);
        Part::TopoShape refShape = refFeature->Shape.getShape();
        TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());

        if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull())
                throw Base::Exception("Failed to extract axis edge");
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line)
                throw Base::Exception("Axis edge must be a straight line");

            axbase = adapt.Value(adapt.FirstParameter());
            axdir = adapt.Line().Direction();
        } else {
            throw Base::Exception("Axis reference must be an edge");
        }

        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        axbase.Transform(invObjLoc.Transformation());
        axdir.Transform(invObjLoc.Transformation());
    }

    // get the support placement
    // TODO: Check for NULL pointer
    /*Part::Feature* supportFeature = static_cast<Part::Feature*>(originals.front());
    if (supportFeature == NULL)
        throw Base::Exception("Cannot work on invalid support shape");
    Base::Placement supportPlacement = supportFeature->Placement.getValue();
    ax *= supportPlacement;
    gp_Ax2 axis(gp_Pnt(ax.getBase().x, ax.getBase().y, ax.getBase().z), gp_Dir(ax.getDirection().x, ax.getDirection().y, ax.getDirection().z));*/
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
