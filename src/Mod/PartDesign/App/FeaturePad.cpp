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
# include <Geom_Surface.hxx>
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
# include <BRepLProp_SLProps.hxx>
# include <GeomLib_IsPlanarSurface.hxx>
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

PROPERTY_SOURCE(PartDesign::Pad, PartDesign::ProfileBased)

Pad::Pad()
{
    addSubType = FeatureAddSub::Additive;
    
    ADD_PROPERTY_TYPE(Type,((long)0),"Pad",App::Prop_None,"Pad type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length,(100.0),"Pad",App::Prop_None,"Pad length");
    ADD_PROPERTY_TYPE(Length2,(100.0),"Pad",App::Prop_None,"P");
    ADD_PROPERTY_TYPE(UpToFace,(0),"Pad",App::Prop_None,"Face where pad will end");
    ADD_PROPERTY_TYPE(Offset,(0.0),"Pad",App::Prop_None,"Offset from face in which pad will end");
    static const App::PropertyQuantityConstraint::Constraints signedLengthConstraint = {-DBL_MAX, DBL_MAX, 1.0};
    Offset.setConstraints ( &signedLengthConstraint );
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
    return ProfileBased::mustExecute();
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

    Part::Feature* obj = 0;
    TopoShape sketchshape;
    try {
        obj = getVerifiedObject();
        sketchshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoShape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
    }


    // get the Sketch plane
    Base::Placement SketchPos    = obj->Placement.getValue(); 
    Base::Vector3d  SketchVector = getProfileNormal();

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        auto invTrsf = invObjLoc.Transformation();

        base.move(invObjLoc);

        gp_Dir dir(SketchVector.x,SketchVector.y,SketchVector.z);
        dir.Transform(invTrsf);

        if (sketchshape.isNull())
            return new App::DocumentObjectExecReturn("Pad: Creating a face from sketch failed");
        sketchshape.move(invObjLoc);

        TopoShape prism(0,getDocument()->getStringHasher());
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
            getUpToFace(upToFace, base.getShape(), supportface, 
                    sketchshape.getShape(), method, dir, Offset.getValue());

            // TODO: Write our own PrismMaker which does not depend on a solid base shape
            if (base.isNull()) {
                // This implementation suffers from some problems:
                // * it explicitly checks for planes only but e.g. a B-spline may work too
                // * The extracted surface passed to GeomAPI_ProjectPointOnSurf may lack of
                //   its placement and thus computes a wrong result
                // * the direction computed by base and projection point must not be transformed
#if 0
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
#else
                TopLoc_Location upToFaceLoc;
                Handle(Geom_Surface) surf = BRep_Tool::Surface(upToFace, upToFaceLoc);
                GeomLib_IsPlanarSurface checkSurface(surf);
                if (surf.IsNull() || !checkSurface.IsPlanar())
                    return new App::DocumentObjectExecReturn("Pad: Extruding up to a face or plane is only possible if the sketch is located on a face");

                gp_Pln upToPlane = checkSurface.Plan().Transformed(upToFaceLoc);
                gp_Dir planeNorm = upToPlane.Axis().Direction();
                gp_Pnt planeBase = upToPlane.Location();
                double angle = dir.Angle(planeNorm);
                if (angle > Precision::Confusion())
                    return new App::DocumentObjectExecReturn("Pad: Extruding up to a face is only possible if the sketch plane is parallel to it");

                // Project basepoint of sketch onto the UpToFace to determine distance and direction
                gp_Pnt basePoint(SketchPos.getPosition().x, SketchPos.getPosition().y, SketchPos.getPosition().z);
                Standard_Real pn = planeBase.XYZ().Dot(planeNorm.XYZ());
                Standard_Real qn = basePoint.XYZ().Dot(planeNorm.XYZ());
                gp_Pnt projPoint = basePoint.Translated(planeNorm.XYZ().Multiplied(pn-qn));

                // Distance
                double length = projPoint.Distance(basePoint) + Offset.getValue();
                if (length < Precision::Confusion())
                    return new App::DocumentObjectExecReturn("Pad: Extruding up to a face failed because of zero height");

                // Direction (the distance is always positive)
                dir = gp_Dir(gp_Vec(basePoint, projPoint));
#endif

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
                PrismMaker.Init(base.getShape(), sketchshape.getShape(), supportface, dir, 2, 1);
                PrismMaker.Perform(upToFace);

                if (!PrismMaker.IsDone())
                    return new App::DocumentObjectExecReturn("Pad: Up to face: Could not extrude the sketch!");
                if (PrismMaker.Shape().IsNull())
                    return new App::DocumentObjectExecReturn("Pad: Resulting shape is empty");
                prism.makEShape(PrismMaker,{base,sketchshape});
            }
        } else {
            generatePrism(prism, sketchshape, method, dir, L, L2,
                          Midplane.getValue(), Reversed.getValue());
        }

        // set the additive shape property for later usage in e.g. pattern
        prism = refineShapeIfActive(prism);
        this->AddSubShape.setValue(prism);

        if (!base.isNull()) {
//             auto obj = getDocument()->addObject("Part::Feature", "prism");
//             static_cast<Part::Feature*>(obj)->Shape.setValue(getSolid(prism));
            // Let's call algorithm computing a fuse operation:
            TopoShape result(0,getDocument()->getStringHasher());
            try {
                result.makEFuse({base,prism});
            }catch(Standard_Failure &){
                return new App::DocumentObjectExecReturn("Pad: Fusion with base feature failed");
            }
            // we have to get the solids (fuse sometimes creates compounds)
            auto solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.isNull())
                return new App::DocumentObjectExecReturn("Pad: Resulting shape is not a solid");

            solRes = refineShapeIfActive(solRes);
            this->Shape.setValue(getSolid(solRes));
        } else {
            if(prism.countSubShapes(TopAbs_SOLID)>1){
                return new App::DocumentObjectExecReturn("Pad: Result has multiple solids. This is not supported at this time.");
            }

           this->Shape.setValue(getSolid(prism));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        if (std::string(e.GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn("Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed.");
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

