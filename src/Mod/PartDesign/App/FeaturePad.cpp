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
# include <BRepBuilderAPI_MakeEdge.hxx>
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
#include <Mod/Part/App/FeatureExtrusion.h>

#include "FeaturePad.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

const char* Pad::TypeEnums[]= {"Length","UpToLast","UpToFirst","UpToFace","TwoLengths",NULL};

PROPERTY_SOURCE(PartDesign::Pad, PartDesign::ProfileBased)

Pad::Pad()
{
    addSubType = FeatureAddSub::Additive;

    ADD_PROPERTY_TYPE(Type, (0L), "Pad", App::Prop_None, "Pad type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length, (100.0), "Pad", App::Prop_None,"Pad length");
    ADD_PROPERTY_TYPE(Length2, (100.0), "Pad", App::Prop_None,"Second Pad length");
    ADD_PROPERTY_TYPE(UseCustomVector, (0), "Pad", App::Prop_None, "Use custom vector for pad direction");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1.0, 1.0, 1.0)), "Pad", App::Prop_None, "Pad direction vector");
    ADD_PROPERTY_TYPE(UpToFace, (0), "Pad", App::Prop_None, "Face where pad will end");

    // Remove the constraints and keep the type to allow to accept negative values
    // https://forum.freecadweb.org/viewtopic.php?f=3&t=52075&p=448410#p447636
    Length2.setConstraints(nullptr);

    ADD_PROPERTY_TYPE(Offset, (0.0), "Pad", App::Prop_None, "Offset from face in which pad will end");
    static const App::PropertyQuantityConstraint::Constraints signedLengthConstraint = {-DBL_MAX, DBL_MAX, 1.0};
    Offset.setConstraints(&signedLengthConstraint);

    ADD_PROPERTY_TYPE(TaperAngle,(0.0), "Pad", App::Prop_None, "Sets the angle of slope (draft) to apply to the sides. The angle is for outward taper; negative value yields inward tapering.");
    ADD_PROPERTY_TYPE(TaperAngleRev,(0.0), "Pad", App::Prop_None, "Taper angle of reverse part of padding.");
}

short Pad::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        UseCustomVector.isTouched() ||
        Direction.isTouched() ||
        Offset.isTouched() ||
        UpToFace.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Pad::execute(void)
{
    return _execute(true, true);
}

App::DocumentObjectExecReturn *Pad::_execute(bool makeface, bool fuse)
{
    std::string method(Type.getValueAsString());                

    // Validate parameters
    double L = Length.getValue();
    if ((method == "Length") && (L < Precision::Confusion()))
        return new App::DocumentObjectExecReturn("Length too small");
    double L2 = 0;
    if ((method == "TwoLengths")) {
        L2 = Length2.getValue();
        if (std::abs(L2) < Precision::Confusion())
            return new App::DocumentObjectExecReturn("Second length too small");
    }

    Part::Feature* obj = 0;
    TopoShape sketchshape;
    try {
        obj = getVerifiedObject();
        if (makeface) {
            sketchshape = getVerifiedFace();
        } else {
            std::vector<TopoShape> shapes;
            bool hasEdges = false;
            auto subs = Profile.getSubValues(false);
            if (subs.empty())
                subs.emplace_back("");
            bool failed = false;
            for (auto & sub : subs) {
                if (sub.empty() && subs.size()>1)
                    continue;
                TopoShape shape = Part::Feature::getTopoShape(obj, sub.c_str(), true);
                if (shape.isNull()) {
                    FC_ERR(getFullName() << ": failed to get profile shape "
                                        << obj->getFullName() << "." << sub);
                    failed = true;
                }
                hasEdges = hasEdges || shape.hasSubShape(TopAbs_EDGE);
                shapes.push_back(shape);
            }
            if (failed)
                return new App::DocumentObjectExecReturn("Failed to obtain profile shape");
            if (hasEdges)
                sketchshape.makEWires(shapes);
            else
                sketchshape.makECompound(shapes, nullptr, false);
        }
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    } catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoShape base = getBaseShape(true);

    // get the Sketch plane
    Base::Placement SketchPos = obj->Placement.getValue();
    // get the normal vector of the sketch
    Base::Vector3d SketchVector = getProfileNormal(sketchshape);

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        auto invTrsf = invObjLoc.Transformation();

        base.move(invObjLoc);

        Base::Vector3d paddingDirection;

        // use the given vector if necessary
        if (!UseCustomVector.getValue()) {
            paddingDirection = SketchVector;
        }
        else {
            // if null vector, use SketchVector
            if ( (fabs(Direction.getValue().x) < Precision::Confusion())
                && (fabs(Direction.getValue().y) < Precision::Confusion())
                && (fabs(Direction.getValue().z) < Precision::Confusion()) )
            {
                Direction.setValue(SketchVector);
            }

            paddingDirection = Direction.getValue();
        }

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
            return new App::DocumentObjectExecReturn("Creation failed because direction is orthogonal to sketch's normal vector");

        // perform the length correction
        L = L / factor;
        L2 = L2 / factor;

        dir.Transform(invTrsf);

        if (sketchshape.isNull())
            return new App::DocumentObjectExecReturn("Creating a face from sketch failed");
        sketchshape.move(invObjLoc);

        TopoShape prism(0,getDocument()->getStringHasher());

        if (method == "UpToFirst" || method == "UpToLast" || method == "UpToFace") {
            // Note: This will return an unlimited planar face if support is a datum plane
            TopoShape supportface = getSupportFace();
            supportface.move(invObjLoc);

            if (Reversed.getValue())
                dir.Reverse();

            // Find a valid face or datum plane to extrude up to
            TopoShape upToFace;
            if (method == "UpToFace") {
                getUpToFaceFromLinkSub(upToFace, UpToFace);
                upToFace.move(invObjLoc);
            }
            getUpToFace(upToFace, base, supportface, 
                    sketchshape, method, dir, Offset.getValue());

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
                supportface = sketchshape;
                if (!supportface.hasSubShape(TopAbs_WIRE))
                    supportface = TopoShape();

#if 0
                BRepFeat_MakePrism PrismMaker;
                PrismMaker.Init(base.getShape(), sketchshape.getShape(), supportface, dir, 2, 1);
                PrismMaker.Perform(upToFace);

                if (!PrismMaker.IsDone())
                    return new App::DocumentObjectExecReturn("Pad: Up to face: Could not extrude the sketch!");
                prism.makEShape(PrismMaker,{base,sketchshape});
#else
                Standard_Integer fuse = fabs(Offset.getValue()) > Precision::Confusion() ? 1 : 2;
                generatePrism(prism, method, base, sketchshape, supportface, upToFace, dir, fuse, Standard_True);
#endif
                base = TopoShape();
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
                if (!supportface.hasSubShape(TopAbs_WIRE))
                    supportface = TopoShape();
#if 0
                BRepFeat_MakePrism PrismMaker;
                PrismMaker.Init(base.getShape(), sketchshape.getShape(), supportface, dir, 2, 1);
                PrismMaker.Perform(upToFace);

                if (!PrismMaker.IsDone())
                    return new App::DocumentObjectExecReturn("Pad: Up to face: Could not extrude the sketch!");
                if (PrismMaker.Shape().IsNull())
                    return new App::DocumentObjectExecReturn("Pad: Resulting shape is empty");
                prism.makEShape(PrismMaker,{base,sketchshape});
#else

                Standard_Integer fuse = fabs(Offset.getValue()) > Precision::Confusion() ? 1 : 2;
                generatePrism(prism, method, base, sketchshape, supportface, upToFace, dir, fuse, Standard_True);
#endif
            }
        } else {
            Part::Extrusion::ExtrusionParameters params;
            params.dir = dir;
            params.taperAngleFwd = this->TaperAngle.getValue() * M_PI / 180.0;
            params.taperAngleRev = this->TaperAngleRev.getValue() * M_PI / 180.0;
            if (L2 == 0.0 && Midplane.getValue()) {
                params.lengthFwd = L/2;
                params.lengthRev = L/2;
                if (params.taperAngleRev == 0.0)
                    params.taperAngleRev = params.taperAngleFwd;
            } else {
                params.lengthFwd = L;
                params.lengthRev = L2;
            }
            params.solid = true;
            if (std::fabs(params.taperAngleFwd) >= Precision::Angular() ||
                    std::fabs(params.taperAngleRev) >= Precision::Angular() ) {
                if (fabs(params.taperAngleFwd) > M_PI * 0.5 - Precision::Angular()
                        || fabs(params.taperAngleRev) > M_PI * 0.5 - Precision::Angular())
                    return new App::DocumentObjectExecReturn("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");

                if (Reversed.getValue())
                    params.dir.Reverse();
                std::vector<TopoShape> drafts;
                Part::Extrusion::makeDraft(params, sketchshape, drafts, getDocument()->getStringHasher());
                if (drafts.empty())
                    return new App::DocumentObjectExecReturn("Padding with draft angle failed");
                prism.makECompound(drafts, nullptr, false);

            } else
                generatePrism(prism, sketchshape, method, dir, L, L2,
                            Midplane.getValue(), Reversed.getValue());
        }
        
        // set the additive shape property for later usage in e.g. pattern
        prism = refineShapeIfActive(prism);
        this->AddSubShape.setValue(prism);
        prism.Tag = -this->getID();

        if (!base.isNull() && fuse) {
//             auto obj = getDocument()->addObject("Part::Feature", "prism");
//             static_cast<Part::Feature*>(obj)->Shape.setValue(getSolid(prism));
            // Let's call algorithm computing a fuse operation:
            TopoShape result(0,getDocument()->getStringHasher());
            try {
                result.makEFuse({base,prism});
            }catch(Standard_Failure &){
                return new App::DocumentObjectExecReturn("Fusion with base feature failed");
            }
            // we have to get the solids (fuse sometimes creates compounds)
            auto solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.isNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

            solRes = refineShapeIfActive(solRes);
            this->Shape.setValue(getSolid(solRes));
        } else if (prism.hasSubShape(TopAbs_SOLID)) {
           this->Shape.setValue(getSolid(prism));
        } else
           this->Shape.setValue(prism);

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

