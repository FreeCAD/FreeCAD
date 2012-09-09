/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRep_Builder.hxx>
# include <BRepBndLib.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Cut.hxx>
#endif

#include <Base/Axis.h>
#include <Base/Placement.h>
#include <Base/Tools.h>
#include <Mod/Part/App/Part2DObject.h>

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
    ADD_PROPERTY_TYPE(Midplane,(0),"Groove", App::Prop_None, "Mid plane");
    ADD_PROPERTY_TYPE(Reversed, (0),"Groove", App::Prop_None, "Reversed");
}

short Groove::mustExecute() const
{
    if (Placement.isTouched() ||
        ReferenceAxis.isTouched() ||
        Axis.isTouched() ||
        Base.isTouched() ||
        Angle.isTouched() ||
        Midplane.isTouched() ||
        Reversed.isTouched())
        return 1;
    return Subtractive::mustExecute();
}

App::DocumentObjectExecReturn *Groove::execute(void)
{
    App::DocumentObject* link = Sketch.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No sketch linked");
    if (!link->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Sketch or Part2DObject");

    Part::Part2DObject* pcSketch=static_cast<Part::Part2DObject*>(link);

    TopoDS_Shape shape = pcSketch->Shape.getShape()._Shape;
    if (shape.IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");

    // this is a workaround for an obscure OCC bug which leads to empty tessellations
    // for some faces. Making an explicit copy of the linked shape seems to fix it.
    // The error only happens when re-computing the shape.
    if (!this->Shape.getValue().IsNull()) {
        BRepBuilderAPI_Copy copy(shape);
        shape = copy.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Linked shape object is empty");
    }

    TopExp_Explorer ex;
    std::vector<TopoDS_Wire> wires;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        wires.push_back(TopoDS::Wire(ex.Current()));
    }
    if (wires.empty()) // there can be several wires
        return new App::DocumentObjectExecReturn("Linked shape object is not a wire");

    // get the Sketch plane
    Base::Placement SketchPlm = pcSketch->Placement.getValue();

    // get reference axis
    App::DocumentObject *pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = ReferenceAxis.getSubValues();
    if (pcReferenceAxis && pcReferenceAxis == pcSketch) {
        bool hasValidAxis=false;
        Base::Axis axis;
        if (subReferenceAxis[0] == "V_Axis") {
            hasValidAxis = true;
            axis = pcSketch->getAxis(Part::Part2DObject::V_Axis);
        }
        else if (subReferenceAxis[0] == "H_Axis") {
            hasValidAxis = true;
            axis = pcSketch->getAxis(Part::Part2DObject::H_Axis);
        }
        else if (subReferenceAxis[0].size() > 4 && subReferenceAxis[0].substr(0,4) == "Axis") {
            int AxId = std::atoi(subReferenceAxis[0].substr(4,4000).c_str());
            if (AxId >= 0 && AxId < pcSketch->getAxisCount()) {
                hasValidAxis = true;
                axis = pcSketch->getAxis(AxId);
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

    // get the support of the Sketch if any
    App::DocumentObject* pcSupport = pcSketch->Support.getValue();
    Part::Feature *SupportObject = 0;
    if (pcSupport && pcSupport->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        SupportObject = static_cast<Part::Feature*>(pcSupport);

    TopoDS_Shape aFace = makeFace(wires);
    if (aFace.IsNull())
        return new App::DocumentObjectExecReturn("Creating a face from sketch failed");

    // Rotate the face by half the angle to get Groove symmetric to sketch plane
    if (Midplane.getValue()) {
        gp_Trsf mov;
        mov.SetRotation(gp_Ax1(pnt, dir), Base::toRadians<double>(Angle.getValue()) * (-1.0) / 2.0);
        TopLoc_Location loc(mov);
        aFace.Move(loc);
    }

    this->positionBySketch();
    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    pnt.Transform(invObjLoc.Transformation());
    dir.Transform(invObjLoc.Transformation());

    // Reverse angle if selected
    double angle = Base::toRadians<double>(Angle.getValue());
    if (Reversed.getValue() && !Midplane.getValue())
        angle *= (-1.0);

    try {
        // revolve the face to a solid
        BRepPrimAPI_MakeRevol RevolMaker(aFace.Moved(invObjLoc), gp_Ax1(pnt, dir), angle);

        if (RevolMaker.IsDone()) {
            TopoDS_Shape result = RevolMaker.Shape();
            // if the sketch has a support fuse them to get one result object (PAD!)
            if (SupportObject) {
                const TopoDS_Shape& support = SupportObject->Shape.getValue();
                if (!support.IsNull() && support.ShapeType() == TopAbs_SOLID) {
                    // Let's call algorithm computing a fuse operation:
                    BRepAlgoAPI_Cut mkCut(support.Moved(invObjLoc), result);
                    // Let's check if the fusion has been successful
                    if (!mkCut.IsDone())
                        throw Base::Exception("Cut out of support failed");
                    result = mkCut.Shape();
                }
            }

            this->Shape.setValue(result);
        }
        else
            return new App::DocumentObjectExecReturn("Could not revolve the sketch!");

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

}
