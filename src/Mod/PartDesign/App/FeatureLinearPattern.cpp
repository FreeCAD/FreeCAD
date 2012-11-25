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
#include <Mod/Part/App/TopoShape.h>
#include <Base/Axis.h>

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::LinearPattern, PartDesign::Transformed)

LinearPattern::LinearPattern()
{
    ADD_PROPERTY_TYPE(Direction,(0),"LinearPattern",(App::PropertyType)(App::Prop_None),"Direction");
    ADD_PROPERTY(StdDirection,(""));
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY(Length,(100.0));
    ADD_PROPERTY(Occurrences,(3));
}

short LinearPattern::mustExecute() const
{
    if (Direction.isTouched() ||
        StdDirection.isTouched() ||
        Reversed.isTouched() ||
        Length.isTouched() ||
        Occurrences.isTouched())
        return 1;
    return Transformed::mustExecute();
}

const std::list<gp_Trsf> LinearPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    std::string stdDirection = StdDirection.getValue();
    float distance = Length.getValue();
    if (distance < Precision::Confusion())
        throw Base::Exception("Pattern length too small");
    int occurrences = Occurrences.getValue();
    if (occurrences < 2)
        throw Base::Exception("At least two occurrences required");
    bool reversed = Reversed.getValue();

    gp_Dir dir;
    double offset = distance / (occurrences - 1);

    if (!stdDirection.empty()) {
        // Note: The placement code commented out below had the defect of working always on the
        // absolute X,Y,Z direction, not on the relative coordinate system of the Body feature.
        // It requires positionBySupport() to be called in Transformed::Execute() AFTER
        // the call to getTransformations()
        // New code thanks to logari81
        if (stdDirection == "X") {
            //dir = Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(1,0,0));
            dir = gp_Dir(1,0,0);
        } else if (stdDirection == "Y") {
            //dir = Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(0,1,0));
            dir = gp_Dir(0,1,0);
        } else if(stdDirection == "Z") {
            //dir = Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(0,0,1));
            dir = gp_Dir(0,0,1);
        } else {
            throw Base::Exception("Invalid direction (must be X, Y or Z)");
        }
    } else {
        App::DocumentObject* refObject = Direction.getValue();
        if (refObject == NULL)
            throw Base::Exception("No direction specified");
        if (!refObject->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
            throw Base::Exception("Direction reference must be edge or face of a feature");
        std::vector<std::string> subStrings = Direction.getSubValues();
        if (subStrings.empty() || subStrings[0].empty())
            throw Base::Exception("No direction reference specified");

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
            //gp_Dir d = adapt.Plane().Axis().Direction();
            //dir = Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(d.X(), d.Y(), d.Z()));
        } else if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull())
                throw Base::Exception("Failed to extract direction edge");
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line)
                throw Base::Exception("Direction edge must be a straight line");

            //gp_Dir d = adapt.Line().Direction();
            //dir = Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(d.X(), d.Y(), d.Z()));
            dir = adapt.Line().Direction();
        } else {
            throw Base::Exception("Direction reference must be edge or face");
        }
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        dir.Transform(invObjLoc.Transformation());
    }

    // get the support placement
    // TODO: Check for NULL pointer
    /*Part::Feature* supportFeature = static_cast<Part::Feature*>(originals.front());
    if (supportFeature == NULL)
        throw Base::Exception("Cannot work on invalid support shape");
    Base::Placement supportPlacement = supportFeature->Placement.getValue();
    dir *= supportPlacement;
    gp_Vec direction(dir.getDirection().x, dir.getDirection().y, dir.getDirection().z);*/
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
