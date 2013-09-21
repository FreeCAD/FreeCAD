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
# include <Bnd_Box.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <Geom_Plane.hxx>
# include <Handle_Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopoDS_Solid.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <BRepAlgoAPI_Common.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Placement.h>
#include <App/Document.h>

#include "FeaturePocket.h"


using namespace PartDesign;

const char* Pocket::TypeEnums[]= {"Length","ThroughAll","UpToFirst","UpToFace",NULL};

PROPERTY_SOURCE(PartDesign::Pocket, PartDesign::Subtractive)

Pocket::Pocket()
{
    ADD_PROPERTY(Type,((long)0));
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(Length,(100.0));
    ADD_PROPERTY_TYPE(UpToFace,(0),"Pocket",(App::PropertyType)(App::Prop_None),"Face where feature will end");
}

short Pocket::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        UpToFace.isTouched())
        return 1;
    return Subtractive::mustExecute();
}

App::DocumentObjectExecReturn *Pocket::execute(void)
{
    // Handle legacy features, these typically have Type set to 3 (previously NULL, now UpToFace),
    // empty FaceName (because it didn't exist) and a value for Length
    if (std::string(Type.getValueAsString()) == "UpToFace" &&
        (UpToFace.getValue() == NULL && Length.getValue() > Precision::Confusion()))
        Type.setValue("Length");

    // Validate parameters
    double L = Length.getValue();
    if ((std::string(Type.getValueAsString()) == "Length") && (L < Precision::Confusion()))
        return new App::DocumentObjectExecReturn("Pocket: Length of pocket too small");

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
    Base::Placement SketchPos = sketch->Placement.getValue();
    Base::Rotation SketchOrientation = SketchPos.getRotation();
    Base::Vector3d SketchVector(0,0,1);
    SketchOrientation.multVec(SketchVector,SketchVector);

    // turn around for pockets
    SketchVector *= -1;

    this->positionBySketch();
    TopLoc_Location invObjLoc = this->getLocation().Inverted();

    try {
        support.Move(invObjLoc);

        gp_Dir dir(SketchVector.x,SketchVector.y,SketchVector.z);
        dir.Transform(invObjLoc.Transformation());

        TopoDS_Shape sketchshape = makeFace(wires);
        if (sketchshape.IsNull())
            return new App::DocumentObjectExecReturn("Pocket: Creating a face from sketch failed");
        sketchshape.Move(invObjLoc);

        std::string method(Type.getValueAsString());
        if (method == "UpToFirst" || method == "UpToFace") {
            TopoDS_Face supportface = getSupportFace();
            supportface.Move(invObjLoc);

            // Find a valid face to extrude up to
            TopoDS_Face upToFace;
            if (method == "UpToFace") {
                getUpToFaceFromLinkSub(upToFace, UpToFace);
                upToFace.Move(invObjLoc);
            }
            getUpToFace(upToFace, support, supportface, sketchshape, method, dir);

            // Special treatment because often the created stand-alone prism is invalid (empty) because
            // BRepFeat_MakePrism(..., 2, 1) is buggy
            BRepFeat_MakePrism PrismMaker;
            PrismMaker.Init(support, sketchshape, supportface, dir, 0, 1);
            PrismMaker.Perform(upToFace);

            if (!PrismMaker.IsDone())
                return new App::DocumentObjectExecReturn("Pocket: Up to face: Could not extrude the sketch!");
            TopoDS_Shape prism = PrismMaker.Shape();
            prism = refineShapeIfActive(prism);

            // And the really expensive way to get the SubShape...
            BRepAlgoAPI_Cut mkCut(support, prism);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Pocket: Up to face: Could not get SubShape!");
            // FIXME: In some cases this affects the Shape property: It is set to the same shape as the SubShape!!!!
            TopoDS_Shape result = refineShapeIfActive(mkCut.Shape());
            this->SubShape.setValue(result);
            this->Shape.setValue(prism);
        } else {
            TopoDS_Shape prism;
            generatePrism(prism, sketchshape, method, dir, L, 0.0,
                          Midplane.getValue(), Reversed.getValue());
            if (prism.IsNull())
                return new App::DocumentObjectExecReturn("Pocket: Resulting shape is empty");

            // set the subtractive shape property for later usage in e.g. pattern
            prism = refineShapeIfActive(prism);
            this->SubShape.setValue(prism);

            // Cut the SubShape out of the support
            BRepAlgoAPI_Cut mkCut(support, prism);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Pocket: Cut out of support failed");
            TopoDS_Shape result = mkCut.Shape();
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape solRes = this->getSolid(result);
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn("Pocket: Resulting shape is not a solid");
            solRes = refineShapeIfActive(solRes);
            remapSupportShape(solRes);
            this->Shape.setValue(solRes);
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        if (std::string(e->GetMessageString()) == "TopoDS::Face" &&
            (std::string(Type.getValueAsString()) == "UpToFirst" || std::string(Type.getValueAsString()) == "UpToFace"))
            return new App::DocumentObjectExecReturn("Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed "
                "for making a pocket up to a face.");
        else
            return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

