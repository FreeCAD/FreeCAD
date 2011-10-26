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
# include <BRepAlgoAPI_Fuse.hxx>
#endif

#include <Base/Placement.h>
#include <Base/Tools.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeatureRevolution.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Revolution, PartDesign::SketchBased)

Revolution::Revolution()
{
    ADD_PROPERTY(Base,(Base::Vector3f(0.0f,0.0f,0.0f)));
    ADD_PROPERTY(Axis,(Base::Vector3f(0.0f,1.0f,0.0f)));
    ADD_PROPERTY(Angle,(360.0));
}

short Revolution::mustExecute() const
{
    if (Sketch.isTouched() ||
        Axis.isTouched() ||
        Base.isTouched() ||
        Angle.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Revolution::execute(void)
{
    App::DocumentObject* link = Sketch.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No sketch linked");
    if (!link->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Sketch or Part2DObject");
    TopoDS_Shape shape = static_cast<Part::Part2DObject*>(link)->Shape.getShape()._Shape;
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
#if 0
    App::DocumentObject* support = sketch->Support.getValue();
    Base::Placement placement = sketch->Placement.getValue();
    Base::Vector3d axis(0,1,0);
    placement.getRotation().multVec(axis, axis);
    Base::BoundBox3d bbox = sketch->Shape.getBoundingBox();
    bbox.Enlarge(0.1);
    Base::Vector3d base(bbox.MaxX, bbox.MaxY, bbox.MaxZ);
#endif
    // get the Sketch plane
    Base::Placement SketchPos = static_cast<Part::Part2DObject*>(link)->Placement.getValue();
    Base::Rotation SketchOrientation = SketchPos.getRotation();
    // get rvolve axis
    Base::Vector3f v = Axis.getValue();
    Base::Vector3d SketchOrientationVector(v.x,v.y,v.z);
    SketchOrientation.multVec(SketchOrientationVector,SketchOrientationVector);


    Base::Vector3f b = Base.getValue();
    gp_Pnt pnt(b.x,b.y,b.z);
    gp_Dir dir(SketchOrientationVector.x,SketchOrientationVector.y,SketchOrientationVector.z);

    // get the support of the Sketch if any
    App::DocumentObject* SupportLink = static_cast<Part::Part2DObject*>(link)->Support.getValue();
    Part::Feature *SupportObject = 0;
    if (SupportLink && SupportLink->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        SupportObject = static_cast<Part::Feature*>(SupportLink);

    TopoDS_Shape aFace = makeFace(wires);
    if (aFace.IsNull())
        return new App::DocumentObjectExecReturn("Creating a face from sketch failed");

    // revolve the face to a solid
    BRepPrimAPI_MakeRevol RevolMaker(aFace,gp_Ax1(pnt, dir), Base::toRadians<double>(Angle.getValue()));

    if (RevolMaker.IsDone()) {
        TopoDS_Shape result = RevolMaker.Shape();
        // if the sketch has a support fuse them to get one result object (PAD!)
        if (SupportObject) {
            const TopoDS_Shape& support = SupportObject->Shape.getValue();
            if (!support.IsNull() && support.ShapeType() == TopAbs_SOLID) {
                // Let's call algorithm computing a fuse operation:
                BRepAlgoAPI_Fuse mkFuse(support, result);
                // Let's check if the fusion has been successful
                if (!mkFuse.IsDone()) 
                    throw Base::Exception("Fusion with support failed");
                result = mkFuse.Shape();
            }
        }

        this->Shape.setValue(result);
    }
    else
        return new App::DocumentObjectExecReturn("Could not revolve the sketch!");

    return App::DocumentObject::StdReturn;
}

}
