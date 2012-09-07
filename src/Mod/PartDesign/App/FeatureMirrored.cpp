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
# include <BRepAdaptor_Surface.hxx>
#endif


#include "FeatureMirrored.h"
#include <Mod/Part/App/TopoShape.h>

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Mirrored, PartDesign::Transformed)

Mirrored::Mirrored()
{
    ADD_PROPERTY_TYPE(MirrorPlane,(0),"Mirrored",(App::PropertyType)(App::Prop_None),"Mirror plane");
    ADD_PROPERTY(StdMirrorPlane,(""));
}

short Mirrored::mustExecute() const
{
    if (MirrorPlane.isTouched() ||
        StdMirrorPlane.isTouched())
        return 1;
    return Transformed::mustExecute();
}

const std::list<gp_Trsf> Mirrored::getTransformations(const std::vector<App::DocumentObject*>)
{
    App::DocumentObject* ref = MirrorPlane.getValue();
    std::vector<std::string> subStrings = MirrorPlane.getSubValues();
    std::string stdPlane = StdMirrorPlane.getValue();

    gp_Pnt p;
    gp_Dir d;
    if (!stdPlane.empty()) {
        p = gp_Pnt(0,0,0);
        if (stdPlane == "XY") {
            d = gp_Dir(0,0,1);
        } else if (stdPlane == "XZ") {
            d = gp_Dir(0,1,0);
        } else if(stdPlane == "YZ") {
            d = gp_Dir(1,0,0);
        } else {
            throw Base::Exception("Invalid mirror plane (must be XY, XZ or YZ)");
        }
    } else {
        if (ref == NULL)
            throw Base::Exception("No mirror plane selected");
        if (!ref->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
            throw Base::Exception("Mirror plane must be face of a feature");
        Part::TopoShape baseShape = static_cast<Part::Feature*>(ref)->Shape.getShape();

        if (subStrings.empty() || subStrings[0].empty())
            throw Base::Exception("No mirror plane defined");
        // TODO: Check for multiple mirror planes?

        TopoDS_Face face = TopoDS::Face(baseShape.getSubShape(subStrings[0].c_str()));
        if (face.IsNull())
            throw Base::Exception("Failed to extract mirror plane");
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() != GeomAbs_Plane)
            throw Base::Exception("Mirror face must be planar");

        p = getPointFromFace(face);
        d = adapt.Plane().Axis().Direction();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        p.Transform(invObjLoc.Transformation());
        d.Transform(invObjLoc.Transformation());
    }

    // get the support placement
    // TODO: Check for NULL pointer
    /*Part::Feature* supportFeature = static_cast<Part::Feature*>(originals.front());
    if (supportFeature == NULL)
        throw Base::Exception("Cannot work on invalid support shape");
    Base::Placement supportPlacement = supportFeature->Placement.getValue();
    ax *= supportPlacement;
    gp_Ax2 mirrorAxis(gp_Pnt(ax.getBase().x, ax.getBase().y, ax.getBase().z), gp_Dir(ax.getDirection().x, ax.getDirection().y, ax.getDirection().z));*/
    gp_Ax2 mirrorAxis(p, d);

    std::list<gp_Trsf> transformations;
    gp_Trsf trans;
    transformations.push_back(trans); // identity transformation
    trans.SetMirror(mirrorAxis);
    transformations.push_back(trans); // mirrored transformation
    return transformations;
}

}
