
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

#include <limits>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <BRep_Builder.hxx>
#include <BRepFeat_MakePrism.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax2.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <App/Document.h>
#include <Base/Tools.h>
#include <Mod/Part/App/ExtrusionHelper.h>
#include "Mod/Part/App/TopoShapeOpCode.h"
#include <Mod/Part/App/PartFeature.h>

#include "FeatureExtrude.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

const char* FeatureExtrude::SideTypesEnums[] = {"One side", "Two sides", "Symmetric", nullptr};

PROPERTY_SOURCE(PartDesign::FeatureExtrude, PartDesign::ProfileBased)

App::PropertyQuantityConstraint::Constraints FeatureExtrude::signedLengthConstraint
    = {-std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 1.0};
double FeatureExtrude::maxAngle = 90 - Base::toDegrees<double>(Precision::Angular());
App::PropertyAngle::Constraints FeatureExtrude::floatAngle = {-maxAngle, maxAngle, 1.0};

FeatureExtrude::FeatureExtrude() = default;

short FeatureExtrude::mustExecute() const
{
    if (Placement.isTouched() || SideType.isTouched() || Type.isTouched() || Type2.isTouched()
        || Length.isTouched() || Length2.isTouched() || TaperAngle.isTouched()
        || TaperAngle2.isTouched() || UseCustomVector.isTouched() || Direction.isTouched()
        || ReferenceAxis.isTouched() || AlongSketchNormal.isTouched() || Offset.isTouched()
        || Offset2.isTouched() || UpToFace.isTouched() || UpToFace2.isTouched()
        || UpToShape.isTouched() || UpToShape2.isTouched()) {
        return 1;
    }
    return ProfileBased::mustExecute();
}

Base::Vector3d FeatureExtrude::computeDirection(const Base::Vector3d& sketchVector, bool inverse)
{
    (void)inverse;
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
    if (UseCustomVector.getValue()) {
        AlongSketchNormal.setReadOnly(false);
    }

    // explicitly set the Direction so that the dialog shows also the used direction
    // if the sketch's normal vector was used
    Direction.setValue(extrudeDirection);
    return extrudeDirection;
}

bool FeatureExtrude::hasTaperedAngle() const
{
    return fabs(TaperAngle.getValue()) > Base::toRadians(Precision::Angular())
        || fabs(TaperAngle2.getValue()) > Base::toRadians(Precision::Angular());
}

void FeatureExtrude::onChanged(const App::Property* prop)
{
    if (!isRestoring() && prop == &Midplane) {
        // Deprecation notice: Midplane property is deprecated and has been replaced by SideType in
        // FreeCAD 1.1 when FeatureExtrude was refactored.
        App::DocumentObject* obj = Profile.getValue();
        auto baseName = obj ? obj->getNameInDocument() : "";
        Base::Console().warning(
            "The 'Midplane' property being set for the extrusion of %s is deprecated and has "
            "been replaced by the 'SideType' property in FeatureExtrude. Please update your script,"
            " this property will be removed in a future version.\n",
            baseName
        );
        if (Midplane.getValue()) {
            SideType.setValue("Symmetric");
        }
    }
    ProfileBased::onChanged(prop);
}

TopoShape FeatureExtrude::makeShellFromUpToShape(TopoShape shape, TopoShape sketchshape, gp_Dir dir)
{

    // Find nearest/furthest face
    std::vector<Part::cutTopoShapeFaces> cfaces = Part::findAllFacesCutBy(shape, sketchshape, dir);
    if (cfaces.empty()) {
        dir = -dir;
        cfaces = Part::findAllFacesCutBy(shape, sketchshape, dir);
    }

    if (cfaces.empty()) {
        return shape;
    }

    struct Part::cutTopoShapeFaces* nearFace {};
    struct Part::cutTopoShapeFaces* farFace {};
    nearFace = farFace = &cfaces.front();
    for (auto& face : cfaces) {
        if (face.distsq > farFace->distsq) {
            farFace = &face;
        }
        else if (face.distsq < nearFace->distsq) {
            nearFace = &face;
        }
    }

    if (nearFace != farFace) {
        std::vector<TopoShape> faceList;
        for (auto& face : shape.getSubTopoShapes(TopAbs_FACE)) {
            if (!(face == farFace->face)) {
                // don't use the last face so the shell is open
                // and OCC works better
                faceList.push_back(face);
            }
        }
        return shape.makeElementCompound(faceList);
    }
    return shape;
}

void FeatureExtrude::updateProperties()
{
    std::string sideTypeVal = SideType.getValueAsString();
    std::string methodSide1 = Type.getValueAsString();
    std::string methodSide2 = Type2.getValueAsString();

    bool isLength1Enabled = false;
    bool isTaper1Visible = false;
    bool isUpToFace1Enabled = false;
    bool isUpToShape1Enabled = false;
    bool isOffset1Enabled = false;

    bool isType2Enabled = false;
    bool isLength2Enabled = false;
    bool isTaper2Visible = false;
    bool isUpToFace2Enabled = false;
    bool isUpToShape2Enabled = false;
    bool isOffset2Enabled = false;

    bool currentAlongSketchNormalEnabled = false;

    auto configureSideProperties = [&](const std::string& method,
                                       bool& lengthEnabled,
                                       bool& taperVisible,
                                       bool& upToFaceEnabled,
                                       bool& upToShapeEnabled,
                                       bool& localAlongSketchNormal,
                                       bool& localOffset) {
        if (method == "Length") {
            lengthEnabled = true;
            taperVisible = true;
            localAlongSketchNormal = true;
        }
        else if (method == "UpToFace") {
            upToFaceEnabled = true;
            localOffset = true;
        }
        else if (method == "UpToShape") {
            upToShapeEnabled = true;
            localOffset = true;
        }
        else if (method == "UpToLast" || method == "UpToFirst") {
            localOffset = true;
        }
        else if (method == "ThroughAll") {
            taperVisible = true;
        }
    };

    if (sideTypeVal == "One side") {
        bool side1ASN = false;
        configureSideProperties(
            methodSide1,
            isLength1Enabled,
            isTaper1Visible,
            isUpToFace1Enabled,
            isUpToShape1Enabled,
            side1ASN,
            isOffset1Enabled
        );
        currentAlongSketchNormalEnabled = side1ASN;
    }
    else if (sideTypeVal == "Two sides") {
        isType2Enabled = true;

        bool side1ASN = false;
        configureSideProperties(
            methodSide1,
            isLength1Enabled,
            isTaper1Visible,
            isUpToFace1Enabled,
            isUpToShape1Enabled,
            side1ASN,
            isOffset1Enabled
        );

        bool side2ASN = false;
        configureSideProperties(
            methodSide2,
            isLength2Enabled,
            isTaper2Visible,
            isUpToFace2Enabled,
            isUpToShape2Enabled,
            side2ASN,
            isOffset2Enabled
        );

        currentAlongSketchNormalEnabled = side1ASN || side2ASN;  // Enable if either side needs it
    }
    else if (sideTypeVal == "Symmetric") {
        bool symASN = false;
        configureSideProperties(
            methodSide1,
            isLength1Enabled,
            isTaper1Visible,
            isUpToFace1Enabled,
            isUpToShape1Enabled,
            symASN,
            isOffset1Enabled
        );
        currentAlongSketchNormalEnabled = symASN;
    }

    Length.setReadOnly(!isLength1Enabled);
    TaperAngle.setReadOnly(!isTaper1Visible);
    UpToFace.setReadOnly(!isUpToFace1Enabled);
    UpToShape.setReadOnly(!isUpToShape1Enabled);
    Offset.setReadOnly(!isOffset1Enabled);

    Type2.setReadOnly(!isType2Enabled);
    Length2.setReadOnly(!isLength2Enabled);
    TaperAngle2.setReadOnly(!isTaper2Visible);
    UpToFace2.setReadOnly(!isUpToFace2Enabled);
    UpToShape2.setReadOnly(!isUpToShape2Enabled);
    Offset2.setReadOnly(!isOffset2Enabled);

    AlongSketchNormal.setReadOnly(!currentAlongSketchNormalEnabled);
}

void FeatureExtrude::setupObject()
{
    ProfileBased::setupObject();
}

App::DocumentObjectExecReturn* FeatureExtrude::buildExtrusion(ExtrudeOptions options)
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }


    bool makeface = options.testFlag(ExtrudeOption::MakeFace);
    bool fuse = options.testFlag(ExtrudeOption::MakeFuse);
    bool inverseDirection = options.testFlag(ExtrudeOption::InverseDirection);

    std::string Sidemethod(SideType.getValueAsString());
    std::string method(Type.getValueAsString());
    std::string method2(Type2.getValueAsString());

    // Validate parameters
    double L = method == "ThroughAll" ? getThroughAllLength()
        : method == "Length"          ? Length.getValue()
                                      : 0.0;
    double L2 = Sidemethod == "Two sides" ? method2 == "ThroughAll" ? getThroughAllLength()
            : method2 == "Length"                                   ? Length2.getValue()
                                                                    : 0.0
                                          : 0.0;

    if ((Sidemethod == "One side" && method == "Length")
        || (Sidemethod == "Two sides" && method == "Length" && method2 == "Length")) {

        if (std::abs(L + L2) < Precision::Confusion()) {
            if (addSubType == Type::Additive) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Cannot create a pad with a total length of zero.")
                );
            }
            else {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Cannot create a pocket with a total length of zero.")
                );
            }
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
                TopoShape shape = Part::Feature::getTopoShape(
                    obj,
                    Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                        | Part::ShapeOption::Transform,
                    sub.c_str()
                );

                if (shape.isNull()) {
                    FC_ERR(
                        getFullName()
                        << ": failed to get profile shape " << obj->getFullName() << "." << sub
                    );
                    failed = true;
                }
                hasEdges = hasEdges || shape.hasSubShape(TopAbs_EDGE);
                shapes.push_back(shape);
            }
            if (failed) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Failed to obtain profile shape")
                );
            }
            if (hasEdges) {
                sketchshape.makeElementWires(shapes);
            }
            else {
                sketchshape.makeElementCompound(
                    shapes,
                    nullptr,
                    TopoShape::SingleShapeCompoundCreationPolicy::returnShape
                );
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
                "Creation failed because direction is orthogonal to sketch's normal vector"
            ));
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
        if (Reversed.getValue()) {
            dir.Reverse();
        }

        if (sketchshape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Creating a face from sketch failed")
            );
        }
        sketchshape.move(invObjLoc);

        std::vector<TopoShape> prisms;  // Stores prisms, all in global CS
        double taper1 = TaperAngle.getValue();
        double offset1 = Offset.getValue();

        if (Sidemethod == "One side") {
            TopoShape prism1 = generateSingleExtrusionSide(
                sketchshape,
                method,
                L,
                taper1,
                UpToFace,
                UpToShape,
                dir,
                offset1,
                makeface,
                base
            );
            prisms.push_back(prism1);
        }
        else if (Sidemethod == "Symmetric") {
            // For Length mode, we are not doing a mirror, but we extrude along the same axis
            // in both directions as it is what users expect.
            if (method == "Length") {
                if (std::fabs(taper1) > Precision::Angular()) {
                    // TAPERED case: We must create two separate prisms and fuse them
                    // to ensure the taper originates correctly from the sketch plane in both
                    // directions.
                    L /= 2.0;
                    TopoShape prism1 = generateSingleExtrusionSide(
                        sketchshape.makeElementCopy(),
                        method,
                        L,
                        taper1,
                        UpToFace,
                        UpToShape,
                        dir,
                        offset1,
                        makeface,
                        base
                    );
                    if (!prism1.isNull() && !prism1.getShape().IsNull()) {
                        prisms.push_back(prism1);
                    }

                    gp_Dir dir2 = dir;
                    dir2.Reverse();
                    TopoShape prism2 = generateSingleExtrusionSide(
                        sketchshape.makeElementCopy(),
                        method,
                        L,
                        taper1,
                        UpToFace,
                        UpToShape,
                        dir2,
                        offset1,
                        makeface,
                        base
                    );
                    if (!prism2.isNull() && !prism2.getShape().IsNull()) {
                        prisms.push_back(prism2);
                    }
                }
                else {
                    // NON-TAPERED case: We can optimize by creating a single prism.
                    // Translate the sketch to the start position (-L/2) and extrude by the full
                    // length L.
                    gp_Trsf start_transform;
                    start_transform.SetTranslation(gp_Vec(dir).Reversed() * (L / 2.0));

                    TopoShape moved_sketch = sketchshape.makeElementCopy();
                    moved_sketch.move(start_transform);

                    TopoShape prism1 = generateSingleExtrusionSide(
                        moved_sketch,
                        method,
                        L,
                        taper1,
                        UpToFace,
                        UpToShape,
                        dir,
                        offset1,
                        makeface,
                        base
                    );
                    if (!prism1.isNull() && !prism1.getShape().IsNull()) {
                        prisms.push_back(prism1);
                    }
                }
            }
            else {
                // For "UpToFace", "UpToShape", etc., mirror the result.
                TopoShape prism1 = generateSingleExtrusionSide(
                    sketchshape,
                    method,
                    L,
                    taper1,
                    UpToFace,
                    UpToShape,
                    dir,
                    offset1,
                    makeface,
                    base
                );
                prisms.push_back(prism1);

                // Prism 2: Mirror prism1 across the sketch plane.
                // The mirror plane's normal must be the sketch normal, not the extrusion direction.
                gp_Dir sketchNormalDir(SketchVector.x, SketchVector.y, SketchVector.z);
                sketchNormalDir.Transform(invTrsf);  // Transform to global CS, like 'dir' was.

                Base::Vector3d sketchCenter = sketchshape.getBoundBox().GetCenter();
                gp_Ax2 mirrorPlane(
                    gp_Pnt(sketchCenter.x, sketchCenter.y, sketchCenter.z),
                    sketchNormalDir
                );
                TopoShape prism2 = prism1.makeElementMirror(mirrorPlane);
                prisms.push_back(prism2);
            }
        }
        else if (Sidemethod == "Two sides") {
            double taper2 = TaperAngle2.getValue();
            double offset2 = Offset2.getValue();
            gp_Dir dir2 = dir;
            dir2.Reverse();
            bool noTaper = std::fabs(taper1) < Precision::Angular()
                && std::fabs(taper2) < Precision::Angular();
            bool method1LengthBased = method == "Length" || method == "ThroughAll";
            bool method2LengthBased = method2 == "Length" || method2 == "ThroughAll";

            if (method1LengthBased && method2 != "UpToFirst" && noTaper) {
                gp_Trsf start_transform;
                start_transform.SetTranslation(gp_Vec(dir) * L);

                TopoShape moved_sketch = sketchshape.makeElementCopy();
                moved_sketch.move(start_transform);
                TopoShape prism = generateSingleExtrusionSide(
                    moved_sketch,
                    method2,
                    L + L2,
                    0.0,
                    UpToFace2,
                    UpToShape2,
                    dir2,
                    offset2,
                    makeface,
                    base
                );
                if (!prism.isNull() && !prism.getShape().IsNull()) {
                    prisms.push_back(prism);
                }
            }
            else if (method2LengthBased && method != "UpToFirst" && noTaper) {
                gp_Trsf start_transform;
                start_transform.SetTranslation(gp_Vec(dir).Reversed() * L2);

                TopoShape moved_sketch = sketchshape.makeElementCopy();
                moved_sketch.move(start_transform);
                TopoShape prism = generateSingleExtrusionSide(
                    moved_sketch,
                    method,
                    L + L2,
                    0.0,
                    UpToFace,
                    UpToShape,
                    dir,
                    offset1,
                    makeface,
                    base
                );
                if (!prism.isNull() && !prism.getShape().IsNull()) {
                    prisms.push_back(prism);
                }
            }
            else {
                TopoShape prism1 = generateSingleExtrusionSide(
                    sketchshape.makeElementCopy(),
                    method,
                    L,
                    taper1,
                    UpToFace,
                    UpToShape,
                    dir,
                    offset1,
                    makeface,
                    base
                );
                if (!prism1.isNull() && !prism1.getShape().IsNull()) {
                    prisms.push_back(prism1);
                }

                // Side 2
                TopoShape prism2 = generateSingleExtrusionSide(
                    sketchshape.makeElementCopy(),
                    method2,
                    L2,
                    taper2,
                    UpToFace2,
                    UpToShape2,
                    dir2,
                    offset2,
                    makeface,
                    base
                );
                if (!prism2.isNull() && !prism2.getShape().IsNull()) {
                    prisms.push_back(prism2);
                }
            }
        }

        // --- Combine generated prisms (all in global CS) ---
        TopoShape prism(0, getDocument()->getStringHasher());
        if (prisms.empty()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "No extrusion geometry was generated.")
            );
        }
        else if (prisms.size() == 1) {
            prism = prisms[0];
        }
        else {
            try {
                prism.makeElementXor(prisms, Part::OpCodes::Extrude);
            }
            catch (const Standard_Failure& e) {
                return new App::DocumentObjectExecReturn(
                    std::string("Failed to xor extrusion sides (OCC): ") + e.GetMessageString()
                );
            }
            catch (const Base::Exception& e) {
                return new App::DocumentObjectExecReturn(
                    std::string("Failed to xor extrusion sides: ") + e.what()
                );
            }
        }

        if (prism.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Resulting fused extrusion is null.")
            );
        }

        // store shape before refinement
        this->rawShape = prism;
        prism = refineShapeIfActive(prism);
        // set the additive shape property for later usage in e.g. pattern
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
                    QT_TRANSLATE_NOOP("Exception", "Fusion with base feature failed")
                );
            }
            // we have to get the solids (fuse sometimes creates compounds)
            auto solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.isNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid")
                );
            }

            // store shape before refinement
            this->rawShape = result;
            solRes = refineShapeIfActive(result);

            if (!isSingleSolidRuleSatisfied(solRes.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            this->Shape.setValue(getSolid(solRes));
        }
        else if (prism.hasSubShape(TopAbs_SOLID)) {
            if (prism.countSubShapes(TopAbs_SOLID) > 1) {
                prism.makeElementFuse(prism.getSubTopoShapes(TopAbs_SOLID));
            }

            // store shape before refinement
            this->rawShape = prism;
            prism = refineShapeIfActive(prism);
            if (!isSingleSolidRuleSatisfied(prism.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            prism = getSolid(prism);
            this->Shape.setValue(prism);
        }
        else {
            // store shape before refinement
            this->rawShape = prism;
            prism = refineShapeIfActive(prism);
            if (!isSingleSolidRuleSatisfied(prism.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            this->Shape.setValue(prism);
        }

        // eventually disable some settings that are not valid for the current method
        updateProperties();

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        if (std::string(e.GetMessageString()) == "TopoDS::Face") {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed."
            ));
        }
        else {
            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

TopoShape FeatureExtrude::generateSingleExtrusionSide(
    const TopoShape& sketchshape,
    const std::string& method,
    double length,
    double taperAngleDeg,
    App::PropertyLinkSub& upToFacePropHandle,
    App::PropertyLinkSubList& upToShapePropHandle,
    gp_Dir dir,
    double offsetVal,
    bool makeFace,
    const TopoShape& base
)
{
    TopoShape prism(0, getDocument()->getStringHasher());

    if (method == "UpToFirst" || method == "UpToLast" || method == "UpToFace"
        || method == "UpToShape") {
        // Note: This will return an unlimited planar face if support is a datum plane
        TopoShape supportface = getTopoShapeSupportFace();
        auto invObjLoc = getLocation().Inverted();
        supportface.move(invObjLoc);

        if (!supportface.hasSubShape(TopAbs_WIRE)) {
            supportface = TopoShape();
        }

        TopoShape upToShape;
        int faceCount = 1;
        // Find a valid shape, face or datum plane to extrude up to
        if (method == "UpToFace") {
            getUpToFaceFromLinkSub(upToShape, upToFacePropHandle);
            upToShape.move(invObjLoc);
        }
        else if (method == "UpToShape") {
            faceCount = getUpToShapeFromLinkSubList(upToShape, upToShapePropHandle);
            upToShape.move(invObjLoc);
            if (faceCount == 0) {
                // No shape selected, use the base
                upToShape = base;
            }
        }

        if (faceCount == 1) {
            getUpToFace(upToShape, base, sketchshape, method, dir);
            addOffsetToFace(upToShape, dir, offsetVal);
        }
        else {
            if (fabs(offsetVal) > Precision::Confusion()) {
                throw Base::RuntimeError("Extrude: Can only offset one face");
            }
            // open the shell by removing the furthest face
            upToShape = makeShellFromUpToShape(upToShape, sketchshape, dir);
        }

        try {
            TopoShape _base;
            if (addSubType != FeatureAddSub::Subtractive) {
                _base = base;  // avoid issue #16690
            }
            prism.makeElementPrismUntil(
                _base,
                sketchshape,
                supportface,
                upToShape,
                dir,
                TopoShape::PrismMode::None,
                true /*CheckUpToFaceLimits.getValue()*/
            );
        }
        catch (Base::Exception&) {
            if (method == "UpToShape" && faceCount > 1) {
                throw Base::RuntimeError(
                    "Extrude: Unable to reach the selected shape, please select faces"
                );
            }
        }
    }
    else if (method == "Length" || method == "ThroughAll") {
        using std::numbers::pi;

        Part::ExtrusionParameters params;
        params.taperAngleFwd = Base::toRadians(taperAngleDeg);

        if (std::fabs(params.taperAngleFwd) >= Precision::Angular()
            || std::fabs(params.taperAngleRev) >= Precision::Angular()) {
            if (fabs(params.taperAngleFwd) > pi * 0.5 - Precision::Angular()
                || fabs(params.taperAngleRev) > pi * 0.5 - Precision::Angular()) {
                return prism;
            }
            params.dir = dir;
            params.solid = makeFace;
            params.lengthFwd = length;

            std::vector<TopoShape> drafts;
            Part::ExtrusionHelper::makeElementDraft(
                params,
                sketchshape,
                drafts,
                getDocument()->getStringHasher()
            );
            if (drafts.empty()) {
                return prism;
            }
            prism.makeElementCompound(
                drafts,
                nullptr,
                TopoShape::SingleShapeCompoundCreationPolicy::returnShape
            );
        }
        else {
            // Without taper angle we create a prism because its shells are in every case no
            // B-splines and can therefore be use as support for further features like Pads,
            // Lofts etc. B-spline shells can break certain features, see e.g.
            // https://forum.freecad.org/viewtopic.php?p=560785#p560785 It is better not to use
            // BRepFeat_MakePrism here even if we have a support because the resulting shape
            // creates problems with Pocket
            try {
                prism.makeElementPrism(sketchshape, length * gp_Vec(dir));
            }
            catch (Standard_Failure&) {
                throw Base::RuntimeError("FeatureExtrusion: Length: Could not extrude the sketch!");
            }
        }
    }

    return prism;
}

void FeatureExtrude::onDocumentRestored()
{
    // property Type no longer has TwoLengths.
    if (strcmp(Type.getValueAsString(), "?TwoLengths") == 0) {
        Type.setValue("Length");
        Type2.setValue("Length");
        SideType.setValue("Two sides");
    }
    else if (Midplane.getValue()) {
        Midplane.setValue(false);
        SideType.setValue("Symmetric");
    }

    ProfileBased::onDocumentRestored();
}
