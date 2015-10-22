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
# include <gp_Pln.hxx>
# include <gp_Dir.hxx>
# include <gp_Ax1.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
#endif


#include "FeatureLinearPattern.h"
#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::LinearPattern, PartDesign::Transformed)

LinearPattern::LinearPattern()
{
    ADD_PROPERTY_TYPE(Direction,(0),"LinearPattern",(App::PropertyType)(App::Prop_None),"Direction");
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY(Length,(100.0));
    ADD_PROPERTY(Occurrences,(3));
}

short LinearPattern::mustExecute() const
{
    if (Direction.isTouched() ||
        Reversed.isTouched() ||
        Length.isTouched() ||
        Occurrences.isTouched())
        return 1;
    return Transformed::mustExecute();
}

const std::list<gp_Trsf> LinearPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    double distance = Length.getValue();
    if (distance < Precision::Confusion())
        throw Base::Exception("Pattern length too small");
    int occurrences = Occurrences.getValue();
    if (occurrences < 2)
        throw Base::Exception("At least two occurrences required");
    bool reversed = Reversed.getValue();

    double offset = distance / (occurrences - 1);

    App::DocumentObject* refObject = Direction.getValue();
    if (refObject == NULL)
        throw Base::Exception("No direction reference specified");
    if (!refObject->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        throw Base::Exception("Direction reference must be edge or face of a feature");
    std::vector<std::string> subStrings = Direction.getSubValues();
    if (subStrings.empty() || subStrings[0].empty())
        throw Base::Exception("No direction reference specified");

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
        else if (subStrings[0].size() > 4 && subStrings[0].substr(0,4) == "Axis") {
            int AxId = std::atoi(subStrings[0].substr(4,4000).c_str());
            if (AxId >= 0 && AxId < refSketch->getAxisCount())
                axis = refSketch->getAxis(AxId);
        }
        axis *= refSketch->Placement.getValue();
        dir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    } else {
        Part::Feature* refFeature = static_cast<Part::Feature*>(refObject);
        Part::TopoShape refShape = refFeature->Shape.getShape();
        TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());

        if (ref.ShapeType() == TopAbs_FACE) {
            TopoDS_Face refFace = TopoDS::Face(ref);
            if (refFace.IsNull())
                throw Base::Exception("Failed to extract direction plane");
            BRepAdaptor_Surface adapt(refFace);
            if (adapt.GetType() != GeomAbs_Plane)
                throw Base::Exception("Direction face must be planar");

            dir = adapt.Plane().Axis().Direction();
        } else if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull())
                throw Base::Exception("Failed to extract direction edge");
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line)
                throw Base::Exception("Direction edge must be a straight line");

            dir = adapt.Line().Direction();
        } else {
            throw Base::Exception("Direction reference must be edge or face");
        }
    }
    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    dir.Transform(invObjLoc.Transformation());

    gp_Vec direction(dir.X(), dir.Y(), dir.Z());

    if (reversed)
        direction.Reverse();

    // Note: The original feature is NOT included in the list of transformations! Therefore
    // we start with occurrence number 1, not number 0
    std::list<gp_Trsf> transformations;
    gp_Trsf trans;
    transformations.push_back(trans); // identity transformation

    for (int i = 1; i < occurrences; i++) {
        trans.SetTranslation(direction * i * offset);
        transformations.push_back(trans);
    }

    return transformations;
}

}
