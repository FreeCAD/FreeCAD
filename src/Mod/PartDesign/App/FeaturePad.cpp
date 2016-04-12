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
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <Handle_Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopoDS_Compound.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <Precision.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <gp_Pln.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <App/Document.h>

//#include "Body.h"
#include "FeaturePad.h"


using namespace PartDesign;

const char* Pad::TypeEnums[]= {"Length","UpToLast","UpToFirst","UpToFace","TwoLengths",NULL};

PROPERTY_SOURCE(PartDesign::Pad, PartDesign::SketchBased)

Pad::Pad()
{
    addSubType = FeatureAddSub::Additive;
    
    ADD_PROPERTY_TYPE(Type,((long)0),"Pad",App::Prop_None,"Pad type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length,(100.0),"Pad",App::Prop_None,"Pad length");
    ADD_PROPERTY_TYPE(Length2,(100.0),"Pad",App::Prop_None,"P");
    ADD_PROPERTY_TYPE(UpToFace,(0),"Pad",App::Prop_None,"Face where pad will end");
    ADD_PROPERTY_TYPE(Offset,(0.0),"Pad",App::Prop_None,"Offset from face in which pad will end");
}

short Pad::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        Offset.isTouched() ||
        UpToFace.isTouched())
        return 1;
    return SketchBased::mustExecute();
}

App::DocumentObjectExecReturn *Pad::execute(void)
{
    // Validate parameters
    double L = Length.getValue();
    if ((std::string(Type.getValueAsString()) == "Length") && (L < Precision::Confusion()))
        return new App::DocumentObjectExecReturn("Length of pad too small");
    double L2 = Length2.getValue();
    if ((std::string(Type.getValueAsString()) == "TwoLengths") && (L < Precision::Confusion()))
        return new App::DocumentObjectExecReturn("Second length of pad too small");

    Part::Part2DObject* sketch = 0;
    std::vector<TopoDS_Wire> wires;
    try {
        sketch = getVerifiedSketch();
        wires = getSketchWires();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        base = TopoDS_Shape();
    }


    // get the Sketch plane
    Base::Placement SketchPos = sketch->Placement.getValue();
    Base::Rotation SketchOrientation = SketchPos.getRotation();
    Base::Vector3d SketchVector(0,0,1);
    SketchOrientation.multVec(SketchVector,SketchVector);

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);

        gp_Dir dir(SketchVector.x,SketchVector.y,SketchVector.z);
        dir.Transform(invObjLoc.Transformation());

        TopoDS_Shape sketchshape = makeFace(wires);
        if (sketchshape.IsNull())
            return new App::DocumentObjectExecReturn("Pad: Creating a face from sketch failed");
        sketchshape.Move(invObjLoc);

        TopoDS_Shape prism;
        std::string method(Type.getValueAsString());                
        if (method == "UpToFirst" || method == "UpToLast" || method == "UpToFace") {
              // Note: This will return an unlimited planar face if support is a datum plane
            TopoDS_Face supportface = getSupportFace();
            supportface.Move(invObjLoc);

            if (Reversed.getValue())
                dir.Reverse();

            // Find a valid face or datum plane to extrude up to
            TopoDS_Face upToFace;
            if (method == "UpToFace") {
                getUpToFaceFromLinkSub(upToFace, UpToFace);
                upToFace.Move(invObjLoc);
            }
            getUpToFace(upToFace, base, supportface, sketchshape, method, dir, Offset.getValue());

            // TODO: Write our own PrismMaker which does not depend on a solid base shape
            if (base.IsNull()) {
                // Workaround because BRepFeat_MakePrism requires the base face located on a solid to be able to extrude up to a face
                // Handle special case of extruding up to a face or plane parallel to the base face
                BRepAdaptor_Surface adapt(upToFace);
                if (adapt.GetType() != GeomAbs_Plane)
                    return new App::DocumentObjectExecReturn("Pad: Extruding up to a face or plane is only possible if the sketch is located on a face");

                double angle = dir.Angle(adapt.Plane().Axis().Direction());
                if (angle > Precision::Confusion())
                    return new App::DocumentObjectExecReturn("Pad: Extruding up to a face is only possible if the sketch plane is parallel to it");

                // Project basepoint of sketch onto the UpToFace to determine distance and direction
                gp_Pnt basePoint(SketchPos.getPosition().x, SketchPos.getPosition().y, SketchPos.getPosition().z);
                GeomAPI_ProjectPointOnSurf prj(basePoint, adapt.Surface().Surface());
                if (prj.NbPoints() != 1)
                    return new App::DocumentObjectExecReturn("Pad: Extruding up to a face failed to find extrusion direction");
                // Distance
                double length = prj.Distance(1) + Offset.getValue();
                if (length < Precision::Confusion())
                    return new App::DocumentObjectExecReturn("Pad: Extruding up to a face failed because of zero height");

                // Direction (the distance is always positive)
                gp_Pnt prjP = prj.NearestPoint();
                dir = gp_Dir(gp_Vec(basePoint, prjP));
                dir.Transform(invObjLoc.Transformation());

                generatePrism(prism, sketchshape, "Length", dir, length, 0.0, false, false);
            } else {
                // A support object is always required and we need to use BRepFeat_MakePrism
                // Problem: For Pocket/UpToFirst (or an equivalent Pocket/UpToFace) the resulting shape is invalid
                // because the feature does not add any material. This only happens with the "2" option, though
                // Note: It might be possible to pass a shell or a compound containing multiple faces
                // as the Until parameter of Perform()
                // Note: Multiple independent wires are not supported, we should check for that and
                // warn the user
                // FIXME: If the support shape is not the previous solid in the tree, then there will be unexpected results
                // Check supportface for limits, otherwise Perform() throws an exception
                TopExp_Explorer Ex(supportface,TopAbs_WIRE);
                if (!Ex.More())
                    supportface = TopoDS_Face();
                BRepFeat_MakePrism PrismMaker;
                PrismMaker.Init(base, sketchshape, supportface, dir, 2, 1);
                PrismMaker.Perform(upToFace);

                if (!PrismMaker.IsDone())
                    return new App::DocumentObjectExecReturn("Pad: Up to face: Could not extrude the sketch!");
                prism = PrismMaker.Shape();
            }
        } else {
            generatePrism(prism, sketchshape, method, dir, L, L2,
                          Midplane.getValue(), Reversed.getValue());
        }

        if (prism.IsNull())
            return new App::DocumentObjectExecReturn("Pad: Resulting shape is empty");

        // set the additive shape property for later usage in e.g. pattern
        prism = refineShapeIfActive(prism);
        const char* name = AddSubShape.getName();
        const char* name1 = this->getPropertyName(&AddSubShape);
        this->AddSubShape.setValue(prism);

        if (!base.IsNull()) {
            // Let's call algorithm computing a fuse operation:
            BRepAlgoAPI_Fuse mkFuse(base, prism);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Pad: Fusion with base feature failed");
            TopoDS_Shape result = mkFuse.Shape();
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn("Pad: Resulting shape is not a solid");
            solRes = refineShapeIfActive(solRes);
            this->Shape.setValue(solRes);
        } else {
            this->Shape.setValue(prism);
        }

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

