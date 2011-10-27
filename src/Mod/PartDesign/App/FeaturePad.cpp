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
//# include <Bnd_Box.hxx>
//# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepBndLib.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
//# include <Geom_Plane.hxx>
# include <Handle_Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
#endif

#include <Base/Placement.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeaturePad.h"


using namespace PartDesign;

const char* Pad::SideEnums[]= {"Positive","Negative",NULL};

PROPERTY_SOURCE(PartDesign::Pad, PartDesign::Additive)

Pad::Pad()
{
    //ADD_PROPERTY(Side,((long)0));
    //Side.setEnums(SideEnums);
    ADD_PROPERTY(Length,(100.0));
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY(MirroredExtent,(0));
    
}

short Pad::mustExecute() const
{
    if (Sketch.isTouched() ||
        Length.isTouched() ||
        MirroredExtent.isTouched() ||
        Reversed.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Pad::execute(void)
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
    // The error almost happens when re-computing the shape but sometimes also for the
    // first time
    BRepBuilderAPI_Copy copy(shape);
    shape = copy.Shape();
    if (shape.IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");

    TopExp_Explorer ex;
    std::vector<TopoDS_Wire> wires;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        wires.push_back(TopoDS::Wire(ex.Current()));
    }
    if (/*shape.ShapeType() != TopAbs_WIRE*/wires.empty()) // there can be several wires
        return new App::DocumentObjectExecReturn("Linked shape object is not a wire");

    // get the Sketch plane
    Base::Placement SketchPos = static_cast<Part::Part2DObject*>(link)->Placement.getValue();
    Base::Rotation SketchOrientation = SketchPos.getRotation();
    Base::Vector3d SketchOrientationVector(0,0,1);
    if (Reversed.getValue()) // negative direction
        SketchOrientationVector *= -1;
    SketchOrientation.multVec(SketchOrientationVector,SketchOrientationVector);

    // get the support of the Sketch if any
    App::DocumentObject* SupportLink = static_cast<Part::Part2DObject*>(link)->Support.getValue();
    Part::Feature *SupportObject = 0;
    if (SupportLink && SupportLink->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        SupportObject = static_cast<Part::Feature*>(SupportLink);


    TopoDS_Shape aFace = makeFace(wires);
    if (aFace.IsNull())
        return new App::DocumentObjectExecReturn("Creating a face from sketch failed");

    // lengthen the vector
    SketchOrientationVector *= Length.getValue();

    try {
        // extrude the face to a solid
        gp_Vec vec(SketchOrientationVector.x,SketchOrientationVector.y,SketchOrientationVector.z);
        BRepPrimAPI_MakePrism PrismMaker(aFace,vec,0,1);
        if (PrismMaker.IsDone()) {
            // if the sketch has a support fuse them to get one result object (PAD!)
            if (SupportObject) {
                // At this point the prism can be a compound
                TopoDS_Shape result = PrismMaker.Shape();
                // set the additive shape property for later usage in e.g. pattern
                this->AddShape.setValue(result);

                const TopoDS_Shape& support = SupportObject->Shape.getValue();
                bool isSolid = false;
                if (!support.IsNull()) {
                    TopExp_Explorer xp;
                    xp.Init(support,TopAbs_SOLID);
                    for (;xp.More(); xp.Next()) {
                        isSolid = true;
                        break;
                    }
                }
                if (isSolid) {
                    // Let's call algorithm computing a fuse operation:
                    BRepAlgoAPI_Fuse mkFuse(support, result);
                    // Let's check if the fusion has been successful
                    if (!mkFuse.IsDone()) 
                        return new App::DocumentObjectExecReturn("Fusion with support failed");
                    result = mkFuse.Shape();
                    // we have to get the solids (fuse create seldomly compounds)
                    TopoDS_Shape solRes = this->getSolid(result);
                    // lets check if the result is a solid
                    if (solRes.IsNull())
                        return new App::DocumentObjectExecReturn("Resulting shape is not a solid");
                    this->Shape.setValue(solRes);
                }
                else
                    return new App::DocumentObjectExecReturn("Support is not a solid");
            }
            else {
                TopoDS_Shape result = this->getSolid(PrismMaker.Shape());
                // set the additive shape property for later usage in e.g. pattern
                this->AddShape.setValue(result);
                this->Shape.setValue(result);
            }
        }
        else
            return new App::DocumentObjectExecReturn("Could not extrude the sketch!");

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

