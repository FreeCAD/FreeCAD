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
#include <Base/Placement.h>
#include <Base/Tools.h>

#include "FeatureGroove.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Groove, PartDesign::Subtractive)

Groove::Groove()
{
    ADD_PROPERTY_TYPE(Base,(Base::Vector3f(0.0f,0.0f,0.0f)),"Groove", App::Prop_ReadOnly, "Base");
    ADD_PROPERTY_TYPE(Axis,(Base::Vector3f(0.0f,1.0f,0.0f)),"Groove", App::Prop_ReadOnly, "Axis");
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
    return Subtractive::mustExecute();
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

    Part::Part2DObject* sketch = 0;
    std::vector<TopoDS_Wire> wires;
    TopoDS_Shape support;
    try {
        sketch = getVerifiedSketch();
        wires = getSketchWires();
        support = getSupportShape();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // get the Sketch plane
    Base::Placement SketchPlm = sketch->Placement.getValue();

    // get reference axis
    App::DocumentObject *pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = ReferenceAxis.getSubValues();
    if (pcReferenceAxis && pcReferenceAxis == sketch) {
        bool hasValidAxis=false;
        Base::Axis axis;
        if (subReferenceAxis[0] == "V_Axis") {
            hasValidAxis = true;
            axis = sketch->getAxis(Part::Part2DObject::V_Axis);
        }
        else if (subReferenceAxis[0] == "H_Axis") {
            hasValidAxis = true;
            axis = sketch->getAxis(Part::Part2DObject::H_Axis);
        }
        else if (subReferenceAxis[0].size() > 4 && subReferenceAxis[0].substr(0,4) == "Axis") {
            int AxId = std::atoi(subReferenceAxis[0].substr(4,4000).c_str());
            if (AxId >= 0 && AxId < sketch->getAxisCount()) {
                hasValidAxis = true;
                axis = sketch->getAxis(AxId);
            }
        }
        if (hasValidAxis) {
            axis *= SketchPlm;
            Base::Vector3d base=axis.getBase();
            Base::Vector3d dir=axis.getDirection();
            Base.setValue(base.x,base.y,base.z);
            Axis.setValue(dir.x,dir.y,dir.z);
        }
    }

    // get revolve axis
    Base::Vector3f b = Base.getValue();
    gp_Pnt pnt(b.x,b.y,b.z);
    Base::Vector3f v = Axis.getValue();
    gp_Dir dir(v.x,v.y,v.z);

    try {
        TopoDS_Shape sketchshape = makeFace(wires);
        if (sketchshape.IsNull())
            return new App::DocumentObjectExecReturn("Creating a face from sketch failed");

        // Rotate the face by half the angle to get Groove symmetric to sketch plane
        if (Midplane.getValue()) {
            gp_Trsf mov;
            mov.SetRotation(gp_Ax1(pnt, dir), Base::toRadians<double>(Angle.getValue()) * (-1.0) / 2.0);
            TopLoc_Location loc(mov);
            sketchshape.Move(loc);
        }

        this->positionBySketch();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        pnt.Transform(invObjLoc.Transformation());
        dir.Transform(invObjLoc.Transformation());
        support.Move(invObjLoc);
        sketchshape.Move(invObjLoc);

        // Check distance between sketchshape and axis - to avoid failures and crashes
        if (checkLineCrossesFace(gp_Lin(pnt, dir), TopoDS::Face(sketchshape)))
            return new App::DocumentObjectExecReturn("Revolve axis intersects the sketch");

        // revolve the face to a solid
        BRepPrimAPI_MakeRevol RevolMaker(sketchshape, gp_Ax1(pnt, dir), angle);

        if (RevolMaker.IsDone()) {
            TopoDS_Shape result = RevolMaker.Shape();
            // set the subtractive shape property for later usage in e.g. pattern
            this->SubShape.setValue(result);

            // cut out groove to get one result object
            BRepAlgoAPI_Cut mkCut(support, result);
            // Let's check if the fusion has been successful
            if (!mkCut.IsDone())
                throw Base::Exception("Cut out of support failed");

            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape solRes = this->getSolid(mkCut.Shape());
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

            this->Shape.setValue(solRes);
        }
        else
            return new App::DocumentObjectExecReturn("Could not revolve the sketch!");

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        if (std::string(e->GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn("Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed.");
        else
            return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

}
