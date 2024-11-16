
/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
# include <BRep_Builder.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <gp_Dir.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Shape.hxx>
#endif

#include <App/Document.h>
#include <Base/Tools.h>
#include <Mod/Part/App/ExtrusionHelper.h>
#include "Mod/Part/App/TopoShapeOpCode.h"
#include <Mod/Part/App/PartFeature.h>

#include "FeatureExtrude.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::FeatureExtrude, PartDesign::ProfileBased)

App::PropertyQuantityConstraint::Constraints FeatureExtrude::signedLengthConstraint = { -DBL_MAX, DBL_MAX, 1.0 };
double FeatureExtrude::maxAngle = 90 - Base::toDegrees<double>(Precision::Angular());
App::PropertyAngle::Constraints FeatureExtrude::floatAngle = { -maxAngle, maxAngle, 1.0 };

FeatureExtrude::FeatureExtrude() = default;

short FeatureExtrude::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        TaperAngle.isTouched() ||
        TaperAngle2.isTouched() ||
        UseCustomVector.isTouched() ||
        Direction.isTouched() ||
        ReferenceAxis.isTouched() ||
        AlongSketchNormal.isTouched() ||
        Offset.isTouched() ||
        UpToFace.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

Base::Vector3d FeatureExtrude::computeDirection(const Base::Vector3d& sketchVector, bool inverse)
{
    (void) inverse;
    Base::Vector3d extrudeDirection;

    if (!UseCustomVector.getValue()) {
        if (!ReferenceAxis.getValue()) {
            // use sketch's normal vector for direction
            extrudeDirection = sketchVector;
            AlongSketchNormal.setReadOnly(true);
        }
        else {
            // update Direction from ReferenceAxis
            App::DocumentObject* pcReferenceAxis = ReferenceAxis.getValue();
            const std::vector<std::string>& subReferenceAxis = ReferenceAxis.getSubValues();
            Base::Vector3d base;
            Base::Vector3d dir;
            getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NotPerpendicularWithNormal);
            switch (addSubType) {
                case Type::Additive:
                    extrudeDirection = dir;
                    break;
                case Type::Subtractive:
                    extrudeDirection = -dir;
                    break;
            }
        }
    }
    else {
        // use the given vector
        // if null vector, use sketchVector
        if ((fabs(Direction.getValue().x) < Precision::Confusion())
            && (fabs(Direction.getValue().y) < Precision::Confusion())
            && (fabs(Direction.getValue().z) < Precision::Confusion())) {
            Direction.setValue(sketchVector);
        }
        extrudeDirection = Direction.getValue();
    }

    // disable options of UseCustomVector
    Direction.setReadOnly(!UseCustomVector.getValue());
    ReferenceAxis.setReadOnly(UseCustomVector.getValue());
    // UseCustomVector allows AlongSketchNormal but !UseCustomVector does not forbid it
    if (UseCustomVector.getValue())
        AlongSketchNormal.setReadOnly(false);

    // explicitly set the Direction so that the dialog shows also the used direction
    // if the sketch's normal vector was used
    Direction.setValue(extrudeDirection);
    return extrudeDirection;
}

bool FeatureExtrude::hasTaperedAngle() const
{
    return fabs(TaperAngle.getValue()) > Base::toRadians(Precision::Angular()) ||
           fabs(TaperAngle2.getValue()) > Base::toRadians(Precision::Angular());
}

TopoShape FeatureExtrude::makeShellFromUpToShape(TopoShape shape, TopoShape sketchshape, gp_Dir dir){

    // Find nearest/furthest face
    std::vector<Part::cutTopoShapeFaces> cfaces =
        Part::findAllFacesCutBy(shape, sketchshape, dir);
    if (cfaces.empty()) {
        dir = -dir;
        cfaces = Part::findAllFacesCutBy(shape, sketchshape, dir);
    }
    struct Part::cutTopoShapeFaces *nearFace;
    struct Part::cutTopoShapeFaces *farFace;
    nearFace = farFace = &cfaces.front();
    for (auto &face : cfaces) {
        if (face.distsq > farFace->distsq) {
            farFace = &face;
        }
        else if (face.distsq < nearFace->distsq) {
            nearFace = &face;
        }
    }

    if (nearFace != farFace) {
        std::vector<TopoShape> faceList;
        for (auto &face : shape.getSubTopoShapes(TopAbs_FACE)) {
            if (! (face == farFace->face)){
                // don't use the last face so the shell is open
                // and OCC works better
                faceList.push_back(face);
            }
        }
        return shape.makeElementCompound(faceList);
    }
    return shape;
}

// TODO: Toponaming April 2024 Deprecated in favor of TopoShape method.  Remove when possible.
void FeatureExtrude::generatePrism(TopoDS_Shape& prism,
                                   const TopoDS_Shape& sketchshape,
                                   const std::string& method,
                                   const gp_Dir& direction,
                                   const double L,
                                   const double L2,
                                   const bool midplane,
                                   const bool reversed)
{
    if (method == "Length" || method == "TwoLengths" || method == "ThroughAll") {
        double Ltotal = L;
        double Loffset = 0.;
        if (method == "ThroughAll")
            Ltotal = getThroughAllLength();


        if (method == "TwoLengths") {
            Ltotal += L2;
            if (reversed)
                Loffset = -L;
            else
                Loffset = -L2;
        }
        else if (midplane) {
            Loffset = -Ltotal / 2;
        }

        TopoDS_Shape from = sketchshape;
        if (method == "TwoLengths" || midplane) {
            gp_Trsf mov;
            mov.SetTranslation(Loffset * gp_Vec(direction));
            TopLoc_Location loc(mov);
            from = sketchshape.Moved(loc);
        }
        else if (reversed) {
            Ltotal *= -1.0;
        }

        if (fabs(Ltotal) < Precision::Confusion()) {
            if (addSubType == Type::Additive)
                throw Base::ValueError("Cannot create a pad with a height of zero.");
            else
                throw Base::ValueError("Cannot create a pocket with a depth of zero.");
        }

        // Without taper angle we create a prism because its shells are in every case no B-splines and can therefore
        // be use as support for further features like Pads, Lofts etc. B-spline shells can break certain features,
        // see e.g. https://forum.freecad.org/viewtopic.php?p=560785#p560785
        // It is better not to use BRepFeat_MakePrism here even if we have a support because the
        // resulting shape creates problems with Pocket
        BRepPrimAPI_MakePrism PrismMaker(from, Ltotal * gp_Vec(direction), Standard_False, Standard_True); // finite prism
        if (!PrismMaker.IsDone())
            throw Base::RuntimeError("ProfileBased: Length: Could not extrude the sketch!");
        prism = PrismMaker.Shape();
    }
    else {
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method '"
            << method << "' for generatePrism()";
        throw Base::RuntimeError(str.str());
    }
}

void FeatureExtrude::generatePrism(TopoDS_Shape& prism,
                                   const std::string& method,
                                   const TopoDS_Shape& baseshape,
                                   const TopoDS_Shape& profileshape,
                                   const TopoDS_Face& supportface,
                                   const TopoDS_Shape& uptoface,
                                   const gp_Dir& direction,
                                   PrismMode Mode,
                                   Standard_Boolean Modify)
{
    if (method == "UpToFirst" || method == "UpToFace") {
        BRepFeat_MakePrism PrismMaker;
        TopoDS_Shape base = baseshape;
        for (TopExp_Explorer xp(profileshape, TopAbs_FACE); xp.More(); xp.Next()) {
            PrismMaker.Init(base, xp.Current(), supportface, direction, Mode, Modify);
            PrismMaker.Perform(uptoface);
            if (!PrismMaker.IsDone())
                throw Base::RuntimeError("ProfileBased: Up to face: Could not extrude the sketch!");

            base = PrismMaker.Shape();
            if (Mode == PrismMode::None)
                Mode = PrismMode::FuseWithBase;
        }

        prism = base;
    }
    else if (method == "UpToLast") {
        BRepFeat_MakePrism PrismMaker;
        prism = baseshape;
        for (TopExp_Explorer xp(profileshape, TopAbs_FACE); xp.More(); xp.Next()) {
            PrismMaker.Init(baseshape, xp.Current(), supportface, direction, PrismMode::None, Modify);

            //Each face needs 2 prisms because if uptoFace is intersected twice the first one ends too soon
            for (int i=0; i<2; i++){
                if (i==0){
                    PrismMaker.Perform(uptoface);
                }else{
                    PrismMaker.Perform(uptoface, uptoface);
                }

                if (!PrismMaker.IsDone())
                    throw Base::RuntimeError("ProfileBased: Up to face: Could not extrude the sketch!");
                auto onePrism = PrismMaker.Shape();

                FCBRepAlgoAPI_Fuse fuse(prism, onePrism);
                prism = fuse.Shape();
            }
        }
    }
    else {
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method '"
            << method << "' for generatePrism()";
        throw Base::RuntimeError(str.str());
    }
}

void FeatureExtrude::generatePrism(TopoShape& prism,
                                   TopoShape sketchTopoShape,
                                   const std::string& method,
                                   const gp_Dir& dir,
                                   const double L,
                                   const double L2,
                                   const bool midplane,
                                   const bool reversed)
{
    auto sketchShape = sketchTopoShape.getShape();
    if (method == "Length" || method == "TwoLengths" || method == "ThroughAll") {
        double Ltotal = L;
        double Loffset = 0.;
        if (method == "ThroughAll") {
            Ltotal = getThroughAllLength();
        }

        if (method == "TwoLengths") {
            Ltotal += L2;
            if (reversed) {
                Loffset = -L;
            }
            else {
                Loffset = -L2;
            }
        }
        else if (midplane) {
            Loffset = -Ltotal / 2;
        }

        if (method == "TwoLengths" || midplane) {
            gp_Trsf mov;
            mov.SetTranslation(Loffset * gp_Vec(dir));
            TopLoc_Location loc(mov);
            sketchTopoShape.move(loc);
        }
        else if (reversed) {
            Ltotal *= -1.0;
        }

        // Without taper angle we create a prism because its shells are in every case no B-splines
        // and can therefore be use as support for further features like Pads, Lofts etc. B-spline
        // shells can break certain features, see e.g.
        // https://forum.freecad.org/viewtopic.php?p=560785#p560785 It is better not to use
        // BRepFeat_MakePrism here even if we have a support because the resulting shape creates
        // problems with Pocket
        try {
            prism.makeElementPrism(sketchTopoShape, Ltotal * gp_Vec(dir));  // finite prism
        }
        catch (Standard_Failure&) {
            throw Base::RuntimeError("FeatureExtrusion: Length: Could not extrude the sketch!");
        }
    }
    else {
        std::stringstream str;
        str << "FeatureExtrusion: Internal error: Unknown method '" << method
            << "' for generatePrism()";
        throw Base::RuntimeError(str.str());
    }
}

void FeatureExtrude::generateTaperedPrism(TopoDS_Shape& prism,
                                          const TopoDS_Shape& sketchshape,
                                          const std::string& method,
                                          const gp_Dir& direction,
                                          const double L,
                                          const double L2,
                                          const double angle,
                                          const double angle2,
                                          const bool midplane)
{
    std::list<TopoDS_Shape> drafts;
    bool isSolid = true; // in PD we only generate solids, while Part Extrude can also create only shells
    bool isPartDesign = true; // there is an OCC bug with single-edge wires (circles) we need to treat differently for PD and Part
    if (method == "ThroughAll") {
        Part::ExtrusionHelper::makeDraft(sketchshape, direction, getThroughAllLength(),
            0.0, Base::toRadians(angle), 0.0, isSolid, drafts, isPartDesign);
    }
    else if (method == "TwoLengths") {
        Part::ExtrusionHelper::makeDraft(sketchshape, direction, L, L2,
            Base::toRadians(angle), Base::toRadians(angle2), isSolid, drafts, isPartDesign);
    }
    else if (method == "Length") {
        if (midplane) {
            Part::ExtrusionHelper::makeDraft(sketchshape, direction, L / 2, L / 2,
                Base::toRadians(angle), Base::toRadians(angle), isSolid, drafts, isPartDesign);
        }
        else
            Part::ExtrusionHelper::makeDraft(sketchshape, direction, L, 0.0,
                Base::toRadians(angle), 0.0, isSolid, drafts, isPartDesign);
    }

    if (drafts.empty()) {
        throw Base::RuntimeError("Creation of tapered object failed");
    }
    else if (drafts.size() == 1) {
        prism = drafts.front();
    }
    else {
        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);
        for (const auto & draft : drafts)
            builder.Add(comp, draft);
        prism = comp;
    }
}

void FeatureExtrude::updateProperties(const std::string &method)
{
    // disable settings that are not valid on the current method
    // disable everything unless we are sure we need it
    bool isLengthEnabled = false;
    bool isLength2Enabled = false;
    bool isOffsetEnabled = false;
    bool isMidplaneEnabled = false;
    bool isReversedEnabled = false;
    bool isUpToFaceEnabled = false;
    bool isUpToShapeEnabled = false;
    bool isTaperVisible = false;
    bool isTaper2Visible = false;
    if (method == "Length") {
        isLengthEnabled = true;
        isTaperVisible = true;
        isMidplaneEnabled = true;
        isReversedEnabled = !Midplane.getValue();
    }
    else if (method == "UpToLast") {
        isOffsetEnabled = true;
        isReversedEnabled = true;
    }
    else if (method == "ThroughAll") {
        isMidplaneEnabled = true;
        isReversedEnabled = !Midplane.getValue();
    }
    else if (method == "UpToFirst") {
        isOffsetEnabled = true;
        isReversedEnabled = true;
    }
    else if (method == "UpToFace") {
        isOffsetEnabled = true;
        isReversedEnabled = true;
        isUpToFaceEnabled = true;
    }
    else if (method == "TwoLengths") {
        isLengthEnabled = true;
        isLength2Enabled = true;
        isTaperVisible = true;
        isTaper2Visible = true;
        isReversedEnabled = true;
    }
    else if (method == "UpToShape") {
        isReversedEnabled = true;
        isUpToShapeEnabled = true;
    }

    Length.setReadOnly(!isLengthEnabled);
    AlongSketchNormal.setReadOnly(!isLengthEnabled);
    Length2.setReadOnly(!isLength2Enabled);
    Offset.setReadOnly(!isOffsetEnabled);
    TaperAngle.setReadOnly(!isTaperVisible);
    TaperAngle2.setReadOnly(!isTaper2Visible);
    Midplane.setReadOnly(!isMidplaneEnabled);
    Reversed.setReadOnly(!isReversedEnabled);
    UpToFace.setReadOnly(!isUpToFaceEnabled);
    UpToShape.setReadOnly(!isUpToShapeEnabled);
}

void FeatureExtrude::setupObject()
{
    ProfileBased::setupObject();
}

App::DocumentObjectExecReturn* FeatureExtrude::buildExtrusion(ExtrudeOptions options)
{
    bool makeface = options.testFlag(ExtrudeOption::MakeFace);
    bool fuse = options.testFlag(ExtrudeOption::MakeFuse);
    bool legacyPocket = options.testFlag(ExtrudeOption::LegacyPocket);
    bool inverseDirection = options.testFlag(ExtrudeOption::InverseDirection);

    std::string method(Type.getValueAsString());

    // Validate parameters
    double L = Length.getValue();
    if ((method == "Length") && (L < Precision::Confusion())) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Length too small"));
    }
    double L2 = 0;
    if ((method == "TwoLengths")) {
        L2 = Length2.getValue();
        if (std::abs(L2) < Precision::Confusion()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Second length too small"));
        }
    }

    Part::Feature* obj = nullptr;
    TopoShape sketchshape;
    try {
        obj = getVerifiedObject();
        if (makeface) {
            sketchshape = getTopoShapeVerifiedFace();
        }
        else {
            std::vector<TopoShape> shapes;
            bool hasEdges = false;
            auto subs = Profile.getSubValues(false);
            if (subs.empty()) {
                subs.emplace_back("");
            }
            bool failed = false;
            for (auto& sub : subs) {
                if (sub.empty() && subs.size() > 1) {
                    continue;
                }
                TopoShape shape = Part::Feature::getTopoShape(obj, sub.c_str(), true);
                if (shape.isNull()) {
                    FC_ERR(getFullName()
                           << ": failed to get profile shape " << obj->getFullName() << "." << sub);
                    failed = true;
                }
                hasEdges = hasEdges || shape.hasSubShape(TopAbs_EDGE);
                shapes.push_back(shape);
            }
            if (failed) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Failed to obtain profile shape"));
            }
            if (hasEdges) {
                sketchshape.makeElementWires(shapes);
            }
            else {
                sketchshape.makeElementCompound(
                    shapes,
                    nullptr,
                    TopoShape::SingleShapeCompoundCreationPolicy::returnShape);
            }
        }
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoShape base = getBaseTopoShape(true);

    // get the normal vector of the sketch
    Base::Vector3d SketchVector = getProfileNormal();

    try {
        this->positionByPrevious();
        auto invObjLoc = getLocation().Inverted();

        auto invTrsf = invObjLoc.Transformation();

        base.move(invObjLoc);

        Base::Vector3d paddingDirection = computeDirection(SketchVector, inverseDirection);

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
        if (factor < Precision::Confusion()) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Creation failed because direction is orthogonal to sketch's normal vector"));
        }

        // perform the length correction if not along custom vector
        if (AlongSketchNormal.getValue()) {
            L = L / factor;
            L2 = L2 / factor;
        }

        // explicitly set the Direction so that the dialog shows also the used direction
        // if the sketch's normal vector was used
        Direction.setValue(paddingDirection);

        dir.Transform(invTrsf);

        if (sketchshape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Creating a face from sketch failed"));
        }
        sketchshape.move(invObjLoc);

        TopoShape prism(0, getDocument()->getStringHasher());

        if (method == "UpToFirst" || method == "UpToLast" || method == "UpToFace" || method == "UpToShape") {
            // Note: This will return an unlimited planar face if support is a datum plane
            TopoShape supportface = getTopoShapeSupportFace();
            supportface.move(invObjLoc);

            if (Reversed.getValue()) {
                dir.Reverse();
            }

            TopoShape upToShape;
            int faceCount = 1;
            // Find a valid shape, face or datum plane to extrude up to
            if (method == "UpToFace") {
                getUpToFaceFromLinkSub(upToShape, UpToFace);
                upToShape.move(invObjLoc);
                faceCount = 1;
            }
            else if (method == "UpToShape") {
                faceCount = getUpToShapeFromLinkSubList(upToShape, UpToShape);
                upToShape.move(invObjLoc);
                if (faceCount == 0){
                    // No shape selected, use the base
                    upToShape = base;
                    faceCount = 0;
                }
            }

            if (faceCount == 1) {
                getUpToFace(upToShape, base, sketchshape, method, dir);
                addOffsetToFace(upToShape, dir, Offset.getValue());
            }
            else{
                if (fabs(Offset.getValue()) > Precision::Confusion()){
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Extrude: Can only offset one face"));
                }
                // open the shell by removing the furthest face
                upToShape = makeShellFromUpToShape(upToShape, sketchshape, dir);
            }

            if (!supportface.hasSubShape(TopAbs_WIRE)) {
                supportface = TopoShape();
            }
            if (legacyPocket) {
                auto mode =
                    base.isNull() ? TopoShape::PrismMode::None : TopoShape::PrismMode::CutFromBase;
                prism = base.makeElementPrismUntil(sketchshape,
                                                   supportface,
                                                   upToShape,
                                                   dir,
                                                   mode,
                                                   false /*CheckUpToFaceLimits.getValue()*/);
                // DO NOT assign id to the generated prism, because this prism is
                // actually the final result. We obtain the subtracted shape by cut
                // this prism with the original base. Assigning a minus self id here
                // will mess up with preselection highlight. It is enough to re-tag
                // the profile shape above.
                //
                // prism.Tag = -this->getID();

                // And the really expensive way to get the SubShape...
                try {
                    TopoShape result(0, getDocument()->getStringHasher());
                    if (base.isNull()) {
                        result = prism;
                    }
                    else {
                        result.makeElementCut({base, prism});
                    }
                    result = refineShapeIfActive(result);
                    this->AddSubShape.setValue(result);
                }
                catch (Standard_Failure&) {
                    return new App::DocumentObjectExecReturn(
                        QT_TRANSLATE_NOOP("Exception", "Up to face: Could not get SubShape!"));
                }

                if (getAddSubType() == Additive) {
                    prism = base.makeElementFuse(this->AddSubShape.getShape());
                }
                else {
                    prism = refineShapeIfActive(prism);
                }

                this->Shape.setValue(getSolid(prism));
                return App::DocumentObject::StdReturn;
            }
            try {
                TopoShape _base;
                if (addSubType!=FeatureAddSub::Subtractive) {
                    _base=base; // avoid issue #16690
                }
                prism.makeElementPrismUntil(_base,
                                            sketchshape,
                                            supportface,
                                            upToShape,
                                            dir,
                                            TopoShape::PrismMode::None,
                                            true /*CheckUpToFaceLimits.getValue()*/);
            }
            catch (Base::Exception& e) {
                if (method == "UpToShape" && faceCount > 1){
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                        "Exception",
                        "Unable to reach the selected shape, please select faces"));
                }
            }
        }
        else {
            Part::ExtrusionParameters params;
            params.dir = dir;
            params.solid = makeface;
            params.taperAngleFwd = this->TaperAngle.getValue() * M_PI / 180.0;
            params.taperAngleRev = this->TaperAngle2.getValue() * M_PI / 180.0;
            if (L2 == 0.0 && Midplane.getValue()) {
                params.lengthFwd = L / 2;
                params.lengthRev = L / 2;
                if (params.taperAngleRev == 0.0) {
                    params.taperAngleRev = params.taperAngleFwd;
                }
            }
            else {
                params.lengthFwd = L;
                params.lengthRev = L2;
            }
            if (std::fabs(params.taperAngleFwd) >= Precision::Angular()
                || std::fabs(params.taperAngleRev) >= Precision::Angular()) {
                if (fabs(params.taperAngleFwd) > M_PI * 0.5 - Precision::Angular()
                    || fabs(params.taperAngleRev) > M_PI * 0.5 - Precision::Angular()) {
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                        "Exception",
                        "Magnitude of taper angle matches or exceeds 90 degrees"));
                }
                if (Reversed.getValue()) {
                    params.dir.Reverse();
                }
                std::vector<TopoShape> drafts;
                Part::ExtrusionHelper::makeElementDraft(params, sketchshape, drafts, getDocument()->getStringHasher());
                if (drafts.empty()) {
                    return new App::DocumentObjectExecReturn(
                        QT_TRANSLATE_NOOP("Exception", "Padding with draft angle failed"));
                }
                prism.makeElementCompound(
                    drafts,
                    nullptr,
                    TopoShape::SingleShapeCompoundCreationPolicy::returnShape);
            }
            else {
                generatePrism(prism,
                              sketchshape,
                              method,
                              dir,
                              L,
                              L2,
                              Midplane.getValue(),
                              Reversed.getValue());
            }
        }

        // set the additive shape property for later usage in e.g. pattern
        prism = refineShapeIfActive(prism);
        this->AddSubShape.setValue(prism);

        if (base.shapeType(true) <= TopAbs_SOLID && fuse) {
            prism.Tag = -this->getID();

            // Let's call algorithm computing a fuse operation:
            TopoShape result(0, getDocument()->getStringHasher());
            try {
                const char* maker;
                switch (getAddSubType()) {
                    case Subtractive:
                        maker = Part::OpCodes::Cut;
                        break;
                    default:
                        maker = Part::OpCodes::Fuse;
                }
                result.makeElementBoolean(maker, {base, prism});
            }
            catch (Standard_Failure&) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Fusion with base feature failed"));
            }
            // we have to get the solids (fuse sometimes creates compounds)
            auto solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.isNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));
            }
            solRes = refineShapeIfActive(solRes);
            if (!isSingleSolidRuleSatisfied(solRes.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }
            this->Shape.setValue(getSolid(solRes));
        }
        else if (prism.hasSubShape(TopAbs_SOLID)) {
            if (prism.countSubShapes(TopAbs_SOLID) > 1) {
                prism.makeElementFuse(prism.getSubTopoShapes(TopAbs_SOLID));
            }
            prism = refineShapeIfActive(prism);
            prism = getSolid(prism);
            if (!isSingleSolidRuleSatisfied(prism.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }
            this->Shape.setValue(prism);
        }
        else {
            prism = refineShapeIfActive(prism);
            if (!isSingleSolidRuleSatisfied(prism.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }
            this->Shape.setValue(prism);
        }

        // eventually disable some settings that are not valid for the current method
        updateProperties(method);

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        if (std::string(e.GetMessageString()) == "TopoDS::Face") {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed."));
        }
        else {
            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}
