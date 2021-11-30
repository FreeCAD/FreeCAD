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
#include <App/MappedElement.h>
#include <Mod/Part/App/FeatureExtrusion.h>
#include <Mod/Part/App/PartParams.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeaturePad.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

const char* Pad::TypeEnums[]= {"Length", "UpToLast", "UpToFirst", "UpToFace", "TwoLengths", NULL};

PROPERTY_SOURCE(PartDesign::Pad, PartDesign::ProfileBased)

Pad::Pad()
{
    ADD_PROPERTY_TYPE(Type, (0L), "Pad", App::Prop_None, "Pad type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length, (100.0), "Pad", App::Prop_None,"Pad length");
    ADD_PROPERTY_TYPE(Length2, (100.0), "Pad", App::Prop_None,"Second Pad length");
    ADD_PROPERTY_TYPE(UseCustomVector, (false), "Pad", App::Prop_None, "Use custom vector for pad direction");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1.0, 1.0, 1.0)), "Pad", App::Prop_None, "Pad direction vector");
    ADD_PROPERTY_TYPE(ReferenceAxis, (0), "Pad", App::Prop_None, "Reference axis of direction");
    ADD_PROPERTY_TYPE(AlongSketchNormal, (true), "Pad", App::Prop_None, "Measure pad length along the sketch normal direction");
    ADD_PROPERTY_TYPE(UpToFace, (0), "Pad", App::Prop_None, "Face where pad will end");

    // Remove the constraints and keep the type to allow to accept negative values
    // https://forum.freecadweb.org/viewtopic.php?f=3&t=52075&p=448410#p447636
    Length2.setConstraints(nullptr);

    ADD_PROPERTY_TYPE(Offset, (0.0), "Pad", App::Prop_None, "Offset from face in which pad will end");
    static const App::PropertyQuantityConstraint::Constraints signedLengthConstraint = {-DBL_MAX, DBL_MAX, 1.0};
    Offset.setConstraints(&signedLengthConstraint);

    ADD_PROPERTY_TYPE(TaperAngle,(0.0), "Pad", App::Prop_None, "Sets the angle of slope (draft) to apply to the sides. The angle is for outward taper; negative value yields inward tapering.");
    ADD_PROPERTY_TYPE(TaperAngleRev,(0.0), "Pad", App::Prop_None, "Taper angle of reverse part of padding.");
    ADD_PROPERTY_TYPE(InnerTaperAngle,(0.0), "Pad", App::Prop_None, "Taper angle of inner holes.");
    ADD_PROPERTY_TYPE(InnerTaperAngleRev,(0.0), "Pad", App::Prop_None, "Taper angle of the reverse part for inner holes.");

    ADD_PROPERTY_TYPE(UsePipeForDraft,(false), "Pad", App::Prop_None, "Use pipe (i.e. sweep) operation to create draft angles.");
    ADD_PROPERTY_TYPE(CheckUpToFaceLimits,(true), "Pad", App::Prop_None,
            "When using 'UpToXXXX' method, check whether the sketch shape is within\n"
            "the up-to-face. And remove the up-to-face limitation to make padding/extrusion\n"
            "work. Note that you may want to disable this if the up-to-face is concave.");
}

short Pad::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        UseCustomVector.isTouched() ||
        Direction.isTouched() ||
        ReferenceAxis.isTouched() ||
        AlongSketchNormal.isTouched() ||
        Offset.isTouched() ||
        UpToFace.isTouched() ||
        UsePipeForDraft.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Pad::execute(void)
{
    return _execute(true, true);
}

void Pad::setupObject()
{
    ProfileBased::setupObject();
    UsePipeForDraft.setValue(Part::PartParams::UsePipeForExtrusionDraft());
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

    // if midplane is true, disable reversed and vice versa
    bool hasMidplane = Midplane.getValue();
    bool hasReversed = Reversed.getValue();
    Midplane.setReadOnly(hasReversed);
    Reversed.setReadOnly(hasMidplane);

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
    Base::Vector3d SketchVector = getProfileNormal();

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        auto invTrsf = invObjLoc.Transformation();

        base.move(invObjLoc);

        Base::Vector3d paddingDirection;
        
        if (!UseCustomVector.getValue()) {
            if (ReferenceAxis.getValue() == nullptr) {
                // use sketch's normal vector for direction
                paddingDirection = SketchVector;
                AlongSketchNormal.setReadOnly(true);
            }
            else {
                // update Direction from ReferenceAxis
                try {
                    App::DocumentObject* pcReferenceAxis = ReferenceAxis.getValue();
                    const std::vector<std::string>& subReferenceAxis = ReferenceAxis.getSubValues();
                    Base::Vector3d base;
                    Base::Vector3d dir;
                    getAxis(pcReferenceAxis, subReferenceAxis, base, dir, false);
                    paddingDirection = dir;
                }
                catch (const Base::Exception& e) {
                    return new App::DocumentObjectExecReturn(e.what());
                }
            }
        }
        else {
            // use the given vector
            // if null vector, use SketchVector
            if ( (fabs(Direction.getValue().x) < Precision::Confusion())
                && (fabs(Direction.getValue().y) < Precision::Confusion())
                && (fabs(Direction.getValue().z) < Precision::Confusion()) ) {
                Direction.setValue(SketchVector);
            }
            paddingDirection = Direction.getValue();
        }

        // disable options of UseCustomVector  
        Direction.setReadOnly(!UseCustomVector.getValue());
        ReferenceAxis.setReadOnly(UseCustomVector.getValue());
        // UseCustomVector allows AlongSketchNormal but !UseCustomVector does not forbid it
        if (UseCustomVector.getValue())
            AlongSketchNormal.setReadOnly(false);

        // create vector in padding direction with length 1
        gp_Dir dir(paddingDirection.x, paddingDirection.y, paddingDirection.z);

        // store the finally used direction to display it in the dialog
        Direction.setValue(dir.X(), dir.Y(), dir.Z());

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

        // perform the length correction if not along custom vector
        if (AlongSketchNormal.getValue()) {
            L = L / factor;
            L2 = L2 / factor;
        }

        // explicitly set the Direction so that the dialog shows also the used direction
        // if the sketch's normal vector was used
        Direction.setValue(paddingDirection);

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
            getUpToFace(upToFace, base, supportface, sketchshape, method, dir);
            addOffsetToFace(upToFace, dir, Offset.getValue());

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
            auto mode = TopoShape::PrismMode::None;
            prism.makEPrism(base, sketchshape, supportface,
                    upToFace, dir, mode, CheckUpToFaceLimits.getValue());
        } else {
            Part::Extrusion::ExtrusionParameters params;
            params.dir = dir;
            params.solid = makeface;
            params.taperAngleFwd = this->TaperAngle.getValue() * M_PI / 180.0;
            params.taperAngleRev = this->TaperAngleRev.getValue() * M_PI / 180.0;
            params.innerTaperAngleFwd = this->InnerTaperAngle.getValue() * M_PI / 180.0;
            params.innerTaperAngleRev = this->InnerTaperAngleRev.getValue() * M_PI / 180.0;
            params.linearize = this->Linearize.getValue();
            if (L2 == 0.0 && Midplane.getValue()) {
                params.lengthFwd = L/2;
                params.lengthRev = L/2;
                if (params.taperAngleRev == 0.0)
                    params.taperAngleRev = params.taperAngleFwd;
            } else {
                params.lengthFwd = L;
                params.lengthRev = L2;
            }
            if (std::fabs(params.taperAngleFwd) >= Precision::Angular()
                    || std::fabs(params.taperAngleRev) >= Precision::Angular()
                    || std::fabs(params.innerTaperAngleFwd) >= Precision::Angular()
                    || std::fabs(params.innerTaperAngleRev) >= Precision::Angular()) {
                if (fabs(params.taperAngleFwd) > M_PI * 0.5 - Precision::Angular()
                        || fabs(params.taperAngleRev) > M_PI * 0.5 - Precision::Angular())
                    return new App::DocumentObjectExecReturn(
                            "Magnitude of taper angle matches or exceeds 90 degrees");
                if (fabs(params.innerTaperAngleFwd) > M_PI * 0.5 - Precision::Angular()
                        || fabs(params.innerTaperAngleRev) > M_PI * 0.5 - Precision::Angular())
                    return new App::DocumentObjectExecReturn(
                            "Magnitude of inner taper angle matches or exceeds 90 degrees");
                if (Reversed.getValue())
                    params.dir.Reverse();
                std::vector<TopoShape> drafts;
                params.usepipe = this->UsePipeForDraft.getValue();
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
        if (isRecomputePaused())
            return App::DocumentObject::StdReturn;

        if (!base.isNull() && fuse) {
            prism.Tag = -this->getID();

//             auto obj = getDocument()->addObject("Part::Feature", "prism");
//             static_cast<Part::Feature*>(obj)->Shape.setValue(getSolid(prism));
            // Let's call algorithm computing a fuse operation:
            TopoShape result(0,getDocument()->getStringHasher());
            try {
                const char *maker;
                switch (getAddSubType()) {
                case Subtractive:
                    maker = TOPOP_CUT;
                    break;
                case Intersecting:
                    maker = TOPOP_COMMON;
                    break;
                default:
                    maker = TOPOP_FUSE;
                }
                result.makEShape(maker, {base,prism});
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
