/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <Bnd_Box.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <Geom_Plane.hxx>
# include <Handle_Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Cut.hxx>
#endif

#include <Base/Placement.h>

#include "FeatureHole.h"


using namespace PartDesign;

const char* Hole::TypeEnums[]    = {"Dimension","UpToLast","UpToFirst",NULL};
const char* Hole::HoleTypeEnums[]= {"Simple","Counterbore","Countersunk",NULL};
const char* Hole::ThreadEnums[]  = {"None","Metric","MetricFine",NULL};

PROPERTY_SOURCE(PartDesign::Hole, PartDesign::Subtractive)

Hole::Hole()
{
    ADD_PROPERTY(Type,((long)0));
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(HoleType,((long)0));
    Type.setEnums(HoleTypeEnums);
    ADD_PROPERTY(ThreadType,((long)0));
    Type.setEnums(ThreadEnums);
    ADD_PROPERTY(Length,(100.0));
    ADD_PROPERTY(ThreadSize,(6.0));
}

//short Hole::mustExecute() const
//{
//    if (Sketch.isTouched() ||
//        Length.isTouched())
//        return 1;
//    return Subtractive::mustExecute();
//}

App::DocumentObjectExecReturn *Hole::execute(void)
{
    //App::DocumentObject* link = Sketch.getValue();
    //if (!link)
    //    return new App::DocumentObjectExecReturn("No sketch linked");
    //if (!link->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId()))
    //    return new App::DocumentObjectExecReturn("Linked object is not a Sketch or Part2DObject");
    //TopoDS_Shape shape = static_cast<Part::Part2DObject*>(link)->Shape.getShape()._Shape;
    //if (shape.IsNull())
    //    return new App::DocumentObjectExecReturn("Linked shape object is empty");

    //// this is a workaround for an obscure OCC bug which leads to empty tessellations
    //// for some faces. Making an explicit copy of the linked shape seems to fix it.
    //// The error almost happens when re-computing the shape but sometimes also for the
    //// first time
    //BRepBuilderAPI_Copy copy(shape);
    //shape = copy.Shape();
    //if (shape.IsNull())
    //    return new App::DocumentObjectExecReturn("Linked shape object is empty");

    //TopExp_Explorer ex;
    //std::vector<TopoDS_Wire> wires;
    //for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
    //    wires.push_back(TopoDS::Wire(ex.Current()));
    //}
    //if (/*shape.ShapeType() != TopAbs_WIRE*/wires.empty()) // there can be several wires
    //    return new App::DocumentObjectExecReturn("Linked shape object is not a wire");

    //// get the Sketch plane
    //Base::Placement SketchPos = static_cast<Part::Part2DObject*>(link)->Placement.getValue();
    //Base::Rotation SketchOrientation = SketchPos.getRotation();
    //Base::Vector3d SketchVector(0,0,1);
    //SketchOrientation.multVec(SketchVector,SketchVector);

    //// get the support of the Sketch if any
    //App::DocumentObject* SupportLink = static_cast<Part::Part2DObject*>(link)->Support.getValue();
    //Part::Feature *SupportObject = 0;
    //if (SupportLink && SupportLink->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
    //    SupportObject = static_cast<Part::Feature*>(SupportLink);

    //if (!SupportObject)
    //    return new App::DocumentObjectExecReturn("No support in Sketch!");

    //TopoDS_Shape aFace = makeFace(wires);
    //if (aFace.IsNull())
    //    return new App::DocumentObjectExecReturn("Creating a face from sketch failed");

    //// lengthen the vector
    //SketchVector *= Length.getValue();

    //// turn around for pockets
    //SketchVector *= -1;

    //// extrude the face to a solid
    //gp_Vec vec(SketchVector.x,SketchVector.y,SketchVector.z);
    //BRepPrimAPI_MakePrism PrismMaker(aFace,vec,0,1);
    //if (PrismMaker.IsDone()) {
    //    // if the sketch has a support fuse them to get one result object (PAD!)
    //    if (SupportObject) {
    //        // Set the subtractive shape property for later usage in e.g. pattern
    //        this->SubShape.setValue(PrismMaker.Shape());
    //        const TopoDS_Shape& support = SupportObject->Shape.getValue();
    //        if (support.IsNull())
    //            return new App::DocumentObjectExecReturn("Support shape is invalid");
    //        TopExp_Explorer xp (support, TopAbs_SOLID);
    //        if (!xp.More())
    //            return new App::DocumentObjectExecReturn("Support shape is not a solid");
    //        // Let's call algorithm computing a fuse operation:
    //        BRepAlgoAPI_Cut mkCut(support, PrismMaker.Shape());
    //        // Let's check if the fusion has been successful
    //        if (!mkCut.IsDone()) 
    //            return new App::DocumentObjectExecReturn("Cut with support failed");
    //        this->Shape.setValue(mkCut.Shape());
    //    }
    //    else{
    //        return new App::DocumentObjectExecReturn("Cannot create a tool out of sketch with no support");
    //    }
    //}
    //else
    //    return new App::DocumentObjectExecReturn("Could not extrude the sketch!");

    return App::DocumentObject::StdReturn;
}

