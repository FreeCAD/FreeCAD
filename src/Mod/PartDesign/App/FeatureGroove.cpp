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
# include <BRep_Builder.hxx>
# include <BRepBndLib.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <Precision.hxx>
# include <gp_Lin.hxx>
#endif

#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Tools.h>
#include <App/Document.h>

#include "FeatureGroove.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Groove, PartDesign::ProfileBased)

Groove::Groove()
{
    addSubType = FeatureAddSub::Subtractive;
    
    ADD_PROPERTY_TYPE(Base,(Base::Vector3d(0.0f,0.0f,0.0f)),"Groove", App::Prop_ReadOnly, "Base");
    ADD_PROPERTY_TYPE(Axis,(Base::Vector3d(0.0f,1.0f,0.0f)),"Groove", App::Prop_ReadOnly, "Axis");
    ADD_PROPERTY_TYPE(Angle,(360.0),"Groove", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(ReferenceAxis,(0),"Groove",(App::PropertyType)(App::Prop_None),"Reference axis of Groove");
}

short Groove::mustExecute() const
{
    if (Placement.isTouched() ||
        ReferenceAxis.isTouched() ||
        Axis.isTouched() ||
        Base.isTouched() ||
        Angle.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Groove::execute(void)
{
    // Validate parameters
    double angle = Angle.getValue();
    if (angle < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Angle of groove too small");
    if (angle > 360.0)
        return new App::DocumentObjectExecReturn("Angle of groove too large");

    angle = Base::toRadians<double>(angle);
    // Reverse angle if selected
    if (Reversed.getValue() && !Midplane.getValue())
        angle *= (-1.0);

    TopoShape sketchshape;
    try {
        sketchshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoShape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        return new App::DocumentObjectExecReturn("No sketch support and no base shape: Please tell me where to remove the material of the groove!");
    }

    updateAxis();

    // get revolve axis
    Base::Vector3d b = Base.getValue();
    gp_Pnt pnt(b.x,b.y,b.z);
    Base::Vector3d v = Axis.getValue();
    gp_Dir dir(v.x,v.y,v.z);

    try {
        if (sketchshape.isNull())
            return new App::DocumentObjectExecReturn("Creating a face from sketch failed");

        // Rotate the face by half the angle to get Groove symmetric to sketch plane
        if (Midplane.getValue()) {
            gp_Trsf mov;
            mov.SetRotation(gp_Ax1(pnt, dir), Base::toRadians<double>(Angle.getValue()) * (-1.0) / 2.0);
            TopLoc_Location loc(mov);
            sketchshape.move(loc);
        }

        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        pnt.Transform(invObjLoc.Transformation());
        dir.Transform(invObjLoc.Transformation());
        base.move(invObjLoc);
        sketchshape.move(invObjLoc);

        // Check distance between sketchshape and axis - to avoid failures and crashes
        TopExp_Explorer xp;
        xp.Init(sketchshape.getShape(), TopAbs_FACE);
        for (;xp.More(); xp.Next()) {
            if (checkLineCrossesFace(gp_Lin(pnt, dir), TopoDS::Face(xp.Current())))
                return new App::DocumentObjectExecReturn("Revolve axis intersects the sketch");
        }
        
        // revolve the face to a solid
        if(!sketchshape.Hasher)
            sketchshape.Hasher = getDocument()->getStringHasher();
        TopoShape result;
        try {
            result = sketchshape.makERevolve(gp_Ax1(pnt, dir), angle);
        }catch(Standard_Failure &) {
            return new App::DocumentObjectExecReturn("Could not revolve the sketch!");
        }
        this->AddSubShape.setValue(result);

        try {
            result = TopoShape(0,getDocument()->getStringHasher()).makECut({base,result});
        }catch(Standard_Failure &) {
            return new App::DocumentObjectExecReturn("Failed to cut base feature");
        }
        auto solRes = this->getSolid(result);
        if (solRes.isNull())
            return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

        solRes = refineShapeIfActive(solRes);
        this->Shape.setValue(getSolid(solRes));

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        if (std::string(e.GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn("Could not create face from sketch.\n"
                "Intersecting sketch entities in a sketch are not allowed.");
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

bool Groove::suggestReversed(void)
{
    updateAxis();
    return ProfileBased::getReversedAngle(Base.getValue(), Axis.getValue()) > 0.0;
}

void Groove::updateAxis(void)
{
    App::DocumentObject *pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir);

    if (dir.Length() > Precision::Confusion()) {
        Base.setValue(base.x,base.y,base.z);
        Axis.setValue(dir.x,dir.y,dir.z);
    }
}

}
