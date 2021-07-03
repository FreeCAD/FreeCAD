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
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepLProp_SLProps.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <Geom_Surface.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomLib_IsPlanarSurface.hxx>
# include <gp_Pln.hxx>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Reader.h>

#include "FeaturePad.h"

using namespace PartDesign;

const char* Pad::TypeEnums[]= {"Length", "UpToLast", "UpToFirst", "UpToFace", "TwoLengths", NULL};

PROPERTY_SOURCE(PartDesign::Pad, PartDesign::ProfileBased)

Pad::Pad()
{
    addSubType = FeatureAddSub::Additive;

    ADD_PROPERTY_TYPE(Type, (0L), "Pad", App::Prop_None, "Pad type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length, (100.0), "Pad", App::Prop_None,"Pad length");
    ADD_PROPERTY_TYPE(Length2, (100.0), "Pad", App::Prop_None,"Second Pad length");
    ADD_PROPERTY_TYPE(UseCustomVector, (false), "Pad", App::Prop_None, "Use custom vector for pad direction");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1.0, 1.0, 1.0)), "Pad", App::Prop_None, "Pad direction vector");
    ADD_PROPERTY_TYPE(AlongSketchNormal, (true), "Pad", App::Prop_None, "Measure pad length along the sketch normal direction");
    ADD_PROPERTY_TYPE(UpToFace, (0), "Pad", App::Prop_None, "Face where pad will end");
    ADD_PROPERTY_TYPE(Offset, (0.0), "Pad", App::Prop_None, "Offset from face in which pad will end");
    static const App::PropertyQuantityConstraint::Constraints signedLengthConstraint = {-DBL_MAX, DBL_MAX, 1.0};
    Offset.setConstraints(&signedLengthConstraint);

    // Remove the constraints and keep the type to allow to accept negative values
    // https://forum.freecadweb.org/viewtopic.php?f=3&t=52075&p=448410#p447636
    Length2.setConstraints(nullptr);

    // for new pads UseCustomVector is false, thus disable Direction and AlongSketchNormal
    AlongSketchNormal.setReadOnly(true);
    Direction.setReadOnly(true);
}

short Pad::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        UseCustomVector.isTouched() ||
        Direction.isTouched() ||
        AlongSketchNormal.isTouched() ||
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

    // if midplane is true, disable reversed and vice versa
    bool hasMidplane = Midplane.getValue();
    bool hasReversed = Reversed.getValue();
    Midplane.setReadOnly(hasReversed);
    Reversed.setReadOnly(hasMidplane);

    Part::Feature* obj = 0;
    TopoDS_Shape sketchshape;
    try {
        obj = getVerifiedObject();
        sketchshape = getVerifiedFace();
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
    Base::Placement SketchPos = obj->Placement.getValue();
    // get the normal vector of the sketch
    Base::Vector3d SketchVector = getProfileNormal();

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);

        Base::Vector3d paddingDirection;
        
        if (!UseCustomVector.getValue()) {
            // use sketch's normal vector for direction
            paddingDirection = SketchVector;
        }
        else {
            // use the given vector
            // if null vector, use SketchVector
            if ( (fabs(Direction.getValue().x) < Precision::Confusion())
                && (fabs(Direction.getValue().y) < Precision::Confusion())
                && (fabs(Direction.getValue().z) < Precision::Confusion()) )
            {
                Direction.setValue(SketchVector);
            }

            paddingDirection = Direction.getValue();
        }

        // disable options of UseCustomVector  
        AlongSketchNormal.setReadOnly(!UseCustomVector.getValue());
        Direction.setReadOnly(!UseCustomVector.getValue());

        // create vector in padding direction with length 1
        gp_Dir dir(paddingDirection.x, paddingDirection.y, paddingDirection.z);

        // The length of a gp_Dir is 1 so the resulting pad would have
        // the length L in the direction of dir. But we want to have its height in the
        // direction of the normal vector.
        // Therefore we must multiply L by the factor that is necessary
        // to make dir as long that its projection to the SketchVector
        // equals the SketchVector.
        // This is the scalar product of both vectors.
        // Since the pad length cannot be negative, the factor must not be negative.

        double factor = fabs(dir * gp_Dir(SketchVector.x, SketchVector.y, SketchVector.z));

        // factor would be zero if vectors are orthogonal
        if (factor < Precision::Confusion())
            return new App::DocumentObjectExecReturn("Pad: Creation failed because direction is orthogonal to sketch's normal vector");

        // perform the length correction if not along custom vector
        if (AlongSketchNormal.getValue()) {
            L = L / factor;
            L2 = L2 / factor;
        }

        // explicitly set the Direction so that the dialog shows also the used direction
        // if the sketch's normal vector was used
        Direction.setValue(paddingDirection);

        dir.Transform(invObjLoc.Transformation());

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
            getUpToFace(upToFace, base, supportface, sketchshape, method, dir);
            addOffsetToFace(upToFace, dir, Offset.getValue());

            // TODO: Write our own PrismMaker which does not depend on a solid base shape
            if (base.IsNull()) {
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
                /*TopLoc_Location upToFaceLoc;
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
                dir = gp_Dir(gp_Vec(basePoint, projPoint));*/
#endif

                //generatePrism(prism, sketchshape, "Length", dir, length, 0.0, false, false);
                base = sketchshape;
                supportface = TopoDS::Face(sketchshape);
                TopExp_Explorer Ex(supportface,TopAbs_WIRE);
                if (!Ex.More())
                    supportface = TopoDS_Face();

#if 0
                BRepFeat_MakePrism PrismMaker;
                PrismMaker.Init(base, sketchshape, supportface, dir, 2, 1);
                PrismMaker.Perform(upToFace);

                if (!PrismMaker.IsDone())
                    return new App::DocumentObjectExecReturn("Pad: Up to face: Could not extrude the sketch!");
                prism = PrismMaker.Shape();
#else
                PrismMode mode = PrismMode::None;
                generatePrism(prism, method, base, sketchshape, supportface, upToFace, dir, mode, Standard_True);
#endif
                base.Nullify();
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
#if 0
                BRepFeat_MakePrism PrismMaker;
                PrismMaker.Init(base, sketchshape, supportface, dir, 2, 1);
                PrismMaker.Perform(upToFace);

                if (!PrismMaker.IsDone())
                    return new App::DocumentObjectExecReturn("Pad: Up to face: Could not extrude the sketch!");
                prism = PrismMaker.Shape();
#else
                PrismMode mode = PrismMode::None;
                generatePrism(prism, method, base, sketchshape, supportface, upToFace, dir, mode, Standard_True);
#endif
            }
        } else {
            generatePrism(prism, sketchshape, method, dir, L, L2,
                hasMidplane, hasReversed);
        }

        if (prism.IsNull())
            return new App::DocumentObjectExecReturn("Pad: Resulting shape is empty");

        // set the additive shape property for later usage in e.g. pattern
        prism = refineShapeIfActive(prism);
        this->AddSubShape.setValue(prism);

        if (!base.IsNull()) {
//             auto obj = getDocument()->addObject("Part::Feature", "prism");
//             static_cast<Part::Feature*>(obj)->Shape.setValue(getSolid(prism));
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

            int solidCount = countSolids(result);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Pad: Result has multiple solids. This is not supported at this time.");
            }

            solRes = refineShapeIfActive(solRes);
            this->Shape.setValue(getSolid(solRes));
        } else {
            int solidCount = countSolids(prism);
            if (solidCount > 1) {
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

