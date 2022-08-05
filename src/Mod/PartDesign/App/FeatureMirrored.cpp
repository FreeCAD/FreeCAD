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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRepAdaptor_Surface.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <App/OriginFeature.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeatureMirrored.h"
#include "DatumPlane.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Mirrored, PartDesign::Transformed)

Mirrored::Mirrored()
{
    ADD_PROPERTY_TYPE(MirrorPlane,(nullptr),"Mirrored",(App::PropertyType)(App::Prop_None),"Mirror plane");
}

short Mirrored::mustExecute() const
{
    if (MirrorPlane.isTouched())
        return 1;
    return Transformed::mustExecute();
}

const std::list<gp_Trsf> Mirrored::getTransformations(const std::vector<App::DocumentObject*>)
{
    App::DocumentObject* refObject = MirrorPlane.getValue();
    if (!refObject)
        throw Base::ValueError("No mirror plane reference specified");
    std::vector<std::string> subStrings = MirrorPlane.getSubValues();
    if (subStrings.empty())
        throw Base::ValueError("No mirror plane reference specified");

    gp_Pnt axbase;
    gp_Dir axdir;
    if (refObject->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        Part::Part2DObject* refSketch = static_cast<Part::Part2DObject*>(refObject);
        Base::Axis axis;
        if (subStrings[0] == "H_Axis")
            axis = refSketch->getAxis(Part::Part2DObject::V_Axis);
        else if (subStrings[0] == "V_Axis")
            axis = refSketch->getAxis(Part::Part2DObject::H_Axis);
        else if (subStrings[0].empty())
            axis = refSketch->getAxis(Part::Part2DObject::N_Axis);
        else if (subStrings[0].compare(0, 4, "Axis") == 0) {
            int AxId = std::atoi(subStrings[0].substr(4,4000).c_str());
            if (AxId >= 0 && AxId < refSketch->getAxisCount()) {
                axis = refSketch->getAxis(AxId);
                axis.setBase(axis.getBase() + 0.5 * axis.getDirection());
                axis.setDirection(Base::Vector3d(-axis.getDirection().y, axis.getDirection().x, axis.getDirection().z));
            }
        }
        axis *= refSketch->Placement.getValue();
        axbase = gp_Pnt(axis.getBase().x, axis.getBase().y, axis.getBase().z);
        axdir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    } else if (refObject->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
        PartDesign::Plane* plane = static_cast<PartDesign::Plane*>(refObject);
        Base::Vector3d base = plane->getBasePoint();
        axbase = gp_Pnt(base.x, base.y, base.z);
        Base::Vector3d dir = plane->getNormal();
        axdir = gp_Dir(dir.x, dir.y, dir.z);
    } else if (refObject->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        App::Plane* plane = static_cast<App::Plane*>(refObject);
        Base::Vector3d base = plane->Placement.getValue().getPosition();
        axbase = gp_Pnt(base.x, base.y, base.z);
        Base::Rotation rot = plane->Placement.getValue().getRotation();
        Base::Vector3d dir(0,0,1);
        rot.multVec(dir, dir);
        axdir = gp_Dir(dir.x, dir.y, dir.z);
    } else if (refObject->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        if (subStrings[0].empty())
            throw Base::ValueError("No direction reference specified");
        Part::TopoShape baseShape = static_cast<Part::Feature*>(refObject)->Shape.getShape();
        // TODO: Check for multiple mirror planes?
        TopoDS_Shape shape = baseShape.getSubShape(subStrings[0].c_str());
        TopoDS_Face face = TopoDS::Face(shape);
        if (face.IsNull())
            throw Base::ValueError("Failed to extract mirror plane");
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() != GeomAbs_Plane)
            throw Base::TypeError("Mirror face must be planar");

        axbase = getPointFromFace(face);
        axdir = adapt.Plane().Axis().Direction();
    } else {
        throw Base::ValueError("Mirror plane reference must be a sketch axis, a face of a feature or a datum plane");
    }

    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    axbase.Transform(invObjLoc.Transformation());
    axdir.Transform(invObjLoc.Transformation());

    gp_Ax2 mirrorAxis(axbase, axdir);

    std::list<gp_Trsf> transformations;
    gp_Trsf trans;
    transformations.push_back(trans); // identity transformation
    trans.SetMirror(mirrorAxis);
    transformations.push_back(trans); // mirrored transformation
    return transformations;
}

}
