// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <gp_Ax2.hxx>
#include <gp_Lin.hxx>
#include <utility>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureRevolved.h"

using namespace PartDesign;

namespace PartDesign
{

/* TRANSLATOR PartDesign::Revolved */

PROPERTY_SOURCE_ABSTRACT(PartDesign::Revolved, PartDesign::ProfileBased)  // NOLINT

const App::PropertyAngle::Constraints Revolved::floatAngle = {-360.0, 360.0, 1.0};
const char* Revolved::SideTypesEnums[] = {"One side", "Two sides", "Symmetric", nullptr};

namespace
{

bool usesBRepFeatRevolution(Revolved::RevolMethod method, Part::RevolMode revolMode)
{
    return method == Revolved::RevolMethod::ToFirst || method == Revolved::RevolMethod::ToFace
        || (method == Revolved::RevolMethod::ToLast && revolMode == Part::RevolMode::FuseWithBase);
}

bool isLegacyTwoAngles(const std::string& method)
{
    return method == "?TwoAngles" || method == "TwoAngles";
}

}  // namespace

Revolved::Revolved()
{
    Angle.setConstraints(&floatAngle);
    Angle2.setConstraints(&floatAngle);
}

short Revolved::mustExecute() const
{
    if (Placement.isTouched() || SideType.isTouched() || Type.isTouched() || Type2.isTouched()
        || ReferenceAxis.isTouched() || Axis.isTouched() || Base.isTouched() || UpToFace.isTouched()
        || UpToFace2.isTouched() || Angle.isTouched() || Angle2.isTouched()) {
        return 1;
    }
    return ProfileBased::mustExecute();
}

void Revolved::onChanged(const App::Property* prop)
{
    if (!isRestoring() && prop == &Midplane) {
        App::DocumentObject* obj = Profile.getValue();
        auto baseName = obj ? obj->getNameInDocument() : "";
        Base::Console().warning(
            "The 'Midplane' property being set for the revolution of %s is deprecated and has "
            "been replaced by the 'SideType' property in Revolved. Please update your script,"
            " this property will be removed in a future version.\n",
            baseName
        );
        if (Midplane.getValue()) {
            SideType.setValue("Symmetric");
        }
        else {
            Base::Console()
                .warning("Deprecated Midplane property was explicitly set to False: assuming SideType='One side'\n");
            SideType.setValue("One side");
        }
    }

    ProfileBased::onChanged(prop);
}

App::DocumentObjectExecReturn* Revolved::executeRevolved(Part::RevolMode revolMode)
{
    try {
        return tryExecuteRevolved(revolMode);
    }
    catch (const Standard_Failure& e) {
        if (std::string(e.GetMessageString()) == "TopoDS::Face") {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Could not create face from sketch.\n"
                "Intersecting sketch entities in a sketch are not allowed."
            ));
        }

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

App::DocumentObjectExecReturn* Revolved::tryExecuteRevolved(Part::RevolMode revolMode)
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    constexpr double maxDegree = 360.0;
    std::string sideType = SideType.getValueAsString();
    auto method = static_cast<RevolMethod>(Type.getValue());
    auto method2 = static_cast<RevolMethod>(Type2.getValue());

    if (method == RevolMethod::TwoAngles) {
        sideType = "Two sides";
        method = RevolMethod::Angle;
        method2 = RevolMethod::Angle;
    }
    if (method2 == RevolMethod::TwoAngles) {
        method2 = RevolMethod::Angle;
    }

    // Validate parameters
    double angleDeg = Angle.getValue();
    if (std::fabs(angleDeg) > maxDegree) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Angle of revolution too large")
        );
    }

    auto angle = Base::toRadians<double>(angleDeg);
    if (sideType != "Two sides" && std::fabs(angle) < Precision::Angular()
        && method == RevolMethod::Angle) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Angle of revolution too small")
        );
    }

    double angle2Deg = Angle2.getValue();
    if (std::fabs(angle2Deg) > maxDegree) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Angle of revolution too large")
        );
    }

    double angle2 = Base::toRadians(angle2Deg);

    TopoShape sketchshape = getTopoShapeVerifiedFace();

    // if the Base property has a valid shape, fuse the AddShape into it
    TopoShape base = tryGetBaseShape();

    // update Axis from ReferenceAxis
    updateAxis();

    // get revolve axis
    const Base::Vector3d v = Axis.getValue();
    if (v.IsNull()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Reference axis is invalid")
        );
    }

    if (sketchshape.isNull()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Creating a face from sketch failed")
        );
    }

    const Base::Vector3d b = Base.getValue();
    gp_Dir dir(v.x, v.y, v.z);
    gp_Pnt pnt(b.x, b.y, b.z);
    positionByPrevious();
    auto invObjLoc = getLocation().Inverted();
    pnt.Transform(invObjLoc.Transformation());
    dir.Transform(invObjLoc.Transformation());
    base.move(invObjLoc);
    sketchshape.move(invObjLoc);
    if (Reversed.getValue()) {
        dir.Reverse();
    }

    // Check distance between sketchshape and axis - to avoid failures and crashes
    TopExp_Explorer xp;
    xp.Init(sketchshape.getShape(), TopAbs_FACE);
    for (; xp.More(); xp.Next()) {
        if (checkLineCrossesFace(gp_Lin(pnt, dir), TopoDS::Face(xp.Current()))) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Revolve axis intersects the sketch")
            );
        }
    }

    // Generate the revolution tool first; PartDesign applies the final base operation below.
    TopoShape result(0, getDocument()->getStringHasher());
    TopoShape supportface = tryGetSupportShape();

    supportface.move(invObjLoc);

    std::vector<TopoShape> revolutions;
    auto addRevolution = [&revolutions](TopoShape&& revolution) {
        if (!revolution.isNull() && !revolution.getShape().IsNull()) {
            revolutions.emplace_back(std::move(revolution));
        }
    };

    bool fuseSideResults = false;

    if (sideType == "Two sides") {
        fuseSideResults = usesBRepFeatRevolution(method, revolMode)
            || usesBRepFeatRevolution(method2, revolMode);

        addRevolution(generateSingleRevolutionSide(
            method,
            angle,
            UpToFace,
            sketchshape.makeElementCopy(),
            base,
            supportface,
            pnt,
            dir,
            invObjLoc,
            revolMode
        ));

        gp_Dir dir2 = dir;
        dir2.Reverse();
        addRevolution(generateSingleRevolutionSide(
            method2,
            angle2,
            UpToFace2,
            sketchshape.makeElementCopy(),
            base,
            supportface,
            pnt,
            dir2,
            invObjLoc,
            revolMode
        ));
    }
    else if (sideType == "Symmetric") {
        const bool isThroughAll = method == RevolMethod::ThroughAll
            && revolMode == Part::RevolMode::CutFromBase;
        if (method == RevolMethod::Angle || isThroughAll) {
            generateRevolution(result, sketchshape, gp_Ax1(pnt, dir), angle, 0.0, true, false, method);
            addRevolution(std::move(result));
        }
        else {
            fuseSideResults = usesBRepFeatRevolution(method, revolMode);
            gp_Ax1 axis(pnt, dir);
            TopoShape upToFace
                = getRevolutionUpToFace(method, UpToFace, base, sketchshape, invObjLoc, axis);
            TopoShape side = tryToRevolveToFace(
                upToFace,
                axis,
                base,
                supportface,
                sketchshape.makeElementCopy(),
                revolMode
            );
            if (!side.isNull() && !side.getShape().IsNull()) {
                addRevolution(side.makeElementCopy());

                Base::Vector3d sketchNormal = getProfileNormal();
                gp_Dir sketchNormalDir(sketchNormal.x, sketchNormal.y, sketchNormal.z);
                sketchNormalDir.Transform(invObjLoc.Transformation());
                Base::Vector3d sketchCenter = sketchshape.getBoundBox().GetCenter();
                gp_Ax2 mirrorPlane(
                    gp_Pnt(sketchCenter.x, sketchCenter.y, sketchCenter.z),
                    sketchNormalDir
                );
                TopoShape mirroredUpToFace = upToFace.makeElementMirror(mirrorPlane);
                gp_Dir dir2 = dir;
                dir2.Reverse();
                addRevolution(tryToRevolveToFace(
                    mirroredUpToFace,
                    gp_Ax1(pnt, dir2),
                    base,
                    supportface,
                    sketchshape.makeElementCopy(),
                    revolMode
                ));
            }
        }
    }
    else {
        addRevolution(generateSingleRevolutionSide(
            method,
            angle,
            UpToFace,
            sketchshape,
            base,
            supportface,
            pnt,
            dir,
            invObjLoc,
            revolMode
        ));
    }

    if (revolutions.empty()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "No revolution geometry was generated")
        );
    }
    if (revolutions.size() == 1) {
        result = revolutions.front();
    }
    else if (fuseSideResults) {
        // Up-to-face revolutions go through BRepFeat. Fuse the generated tools so that the
        // preview stays continuous before PartDesign applies the final base operation.
        result.makeElementFuse(revolutions, Part::OpCodes::Revolve);
    }
    else {
        result.makeElementXor(revolutions, Part::OpCodes::Revolve);
    }

    setResult(base, result);

    // eventually disable some settings that are not valid for the current method
    updateProperties();

    return App::DocumentObject::StdReturn;
}

void Revolved::setResult(const TopoShape& base, const TopoShape& revolved)
{
    if (revolved.isNull()) {
        throw Base::RuntimeError(QT_TRANSLATE_NOOP("Exception", "Could not revolve the sketch!"));
    }
    // store shape before refinement
    this->rawShape = revolved;
    TopoShape result = refineShapeIfActive(revolved);
    // set the additive shape property for later usage in e.g. pattern
    this->AddSubShape.setValue(result);

    if (!base.isNull()) {
        result = makeShape(base, result);
        // store shape before refinement
        this->rawShape = result;
        result = refineShapeIfActive(result);
    }
    if (!isSingleSolidRuleSatisfied(result.getShape())) {
        throw Base::RuntimeError(QT_TRANSLATE_NOOP(
            "Exception",
            "Result has multiple solids: enable 'Allow Compound' in the active body."
        ));
    }
    result = getSolid(result);
    this->Shape.setValue(result);
}

TopoShape Revolved::tryToRevolveToFace(
    const TopoShape& upToFace,
    const gp_Ax1& axis,
    const TopoShape& base,
    TopoShape supportface,
    const TopoShape& sketchshape,
    Part::RevolMode revolMode
) const
{
    TopExp_Explorer Ex(supportface.getShape(), TopAbs_WIRE);
    if (!Ex.More()) {
        supportface = TopoDS_Face();
    }

    auto makeRevolution = [&](Part::RevolMode mode, Standard_Boolean modify) {
        TopoShape revolution(0, getDocument()->getStringHasher());
        revolution.makeElementRevolution(
            base,
            TopoDS::Face(sketchshape.getShape()),
            axis,
            TopoDS::Face(supportface.getShape()),
            TopoDS::Face(upToFace.getShape()),
            nullptr,
            mode,
            modify
        );
        if (revolution.isNull() || revolution.getShape().IsNull()) {
            throw Base::RuntimeError("Could not revolve the sketch!");
        }
        return revolution;
    };

    auto makeGeneratedTool = [&](const TopoShape& baseResult) {
        if (base.isNull()) {
            return baseResult;
        }

        TopoShape tool(0, getDocument()->getStringHasher());
        tool.makeElementCut({baseResult, base}, Part::OpCodes::Revolve);
        if (tool.isNull() || tool.getShape().IsNull()) {
            throw Base::RuntimeError("Could not extract generated revolution tool!");
        }
        return tool;
    };

    auto makeRemovedVolume = [&](const TopoShape& baseResult) {
        TopoShape tool(0, getDocument()->getStringHasher());
        tool.makeElementCut({base, baseResult}, Part::OpCodes::Revolve);
        if (tool.isNull() || tool.getShape().IsNull()) {
            throw Base::RuntimeError("Could not extract generated groove tool!");
        }
        return tool;
    };

    if (revolMode == Part::RevolMode::CutFromBase) {
        try {
            return makeRemovedVolume(makeRevolution(revolMode, Standard_True));
        }
        catch (const Base::Exception&) {
            // If the groove side removes no material, keep trying to return the cutter tool
            // itself so a no-op side behaves like an angle-based groove.
        }
        catch (const Standard_Failure&) {
            // Fall through to cutter generation below.
        }
    }

    try {
        return makeGeneratedTool(makeRevolution(Part::RevolMode::None, Standard_False));
    }
    catch (const Base::Exception&) {
        // Some OCCT cases require a feature mode. Falling back keeps up-to-face usable.
    }
    catch (const Standard_Failure&) {
        // Fall through to the feature mode below.
    }

    try {
        return makeGeneratedTool(makeRevolution(Part::RevolMode::FuseWithBase, Standard_True));
    }
    catch (const Base::Exception&) {
        throw Base::RuntimeError("Could not revolve the sketch!");
    }
    catch (const Standard_Failure&) {
        throw Base::RuntimeError("Could not revolve the sketch!");
    }
}

TopoShape Revolved::generateSingleRevolutionSide(
    RevolMethod method,
    double angle,
    App::PropertyLinkSub& upToFaceProp,
    const TopoShape& sketchshape,
    const TopoShape& base,
    TopoShape supportface,
    const gp_Pnt& pnt,
    const gp_Dir& dir,
    const TopLoc_Location& invObjLoc,
    Part::RevolMode revolMode
)
{
    TopoShape revolution(0, getDocument()->getStringHasher());
    const bool isThroughAll = method == RevolMethod::ThroughAll
        && revolMode == Part::RevolMode::CutFromBase;

    if (method == RevolMethod::Angle || isThroughAll) {
        if (method == RevolMethod::Angle && std::fabs(angle) < Precision::Angular()) {
            return revolution;
        }
        generateRevolution(revolution, sketchshape, gp_Ax1(pnt, dir), angle, 0.0, false, false, method);
    }
    else if (
        method == RevolMethod::ToFirst
        || (method == RevolMethod::ToLast && revolMode == Part::RevolMode::FuseWithBase)
        || method == RevolMethod::ToFace
    ) {
        gp_Ax1 axis(pnt, dir);
        TopoShape upToFace
            = getRevolutionUpToFace(method, upToFaceProp, base, sketchshape, invObjLoc, axis);

        revolution = tryToRevolveToFace(upToFace, axis, base, supportface, sketchshape, revolMode);
    }
    else {
        throw Base::RuntimeError(
            "ProfileBased: Internal error: Unknown method for generateSingleRevolutionSide()"
        );
    }

    return revolution;
}

TopoShape Revolved::getRevolutionUpToFace(
    RevolMethod method,
    const App::PropertyLinkSub& upToFaceProp,
    const TopoShape& base,
    const TopoShape& sketchshape,
    const TopLoc_Location& invObjLoc,
    const gp_Ax1& axis
) const
{
    TopoShape upToFace;
    if (method == RevolMethod::ToFace) {
        getUpToFaceFromLinkSub(upToFace, upToFaceProp);
        upToFace.move(invObjLoc);
    }
    else {
        getUpToFace(
            upToFace,
            base,
            sketchshape,
            method == RevolMethod::ToFirst ? "UpToFirst" : "UpToLast",
            axis
        );
    }
    return upToFace;
}

TopoShape Revolved::tryGetBaseShape() const
{
    TopoShape base;
    try {
        base = getBaseTopoShape();
    }
    catch (const Base::Exception&) {
        // fall back to support (for legacy features)
    }

    return base;
}

TopoShape Revolved::tryGetSupportShape() const
{
    TopoShape supportface(0);
    try {
        supportface = getSupportFace();
    }
    catch (...) {
        // do nothing, null shape is handled later
    }

    return supportface;
}

bool Revolved::suggestReversed()
{
    try {
        updateAxis();
    }
    catch (const Base::Exception&) {
        return false;
    }

    double angle = ProfileBased::getReversedAngle(Base.getValue(), Axis.getValue());
    return suggestReversedAngle(angle);
}

void Revolved::updateAxis()
{
    App::DocumentObject* pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string>& subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NotParallelWithNormal);

    Base.setValue(base);
    Axis.setValue(dir);
}

void Revolved::generateRevolution(
    TopoShape& revol,
    const TopoShape& sketchshape,
    const gp_Ax1& axis,
    double angle,
    double angle2,
    bool midplane,
    bool reversed,
    RevolMethod method
)
{
    if (method == RevolMethod::Angle || method == RevolMethod::TwoAngles
        || method == RevolMethod::ThroughAll) {
        double angleTotal = angle;
        double angleOffset = 0.;

        if (method == RevolMethod::TwoAngles) {
            // Rotate the face by `angle2`/`angle` to get "second" angle
            angleTotal += angle2;
            angleOffset = angle2 * -1.0;
        }
        else if (method == RevolMethod::ThroughAll) {
            angleTotal = 2 * std::numbers::pi;
        }
        else if (midplane) {
            // Rotate the face by half the angle to get Revolution symmetric to sketch plane
            angleOffset = -angle / 2;
        }

        if (std::fabs(angleTotal) < Precision::Angular()) {
            throw Base::ValueError("Cannot create a revolution with zero angle.");
        }

        gp_Ax1 revolAx(axis);
        if (reversed) {
            revolAx.Reverse();
        }

        TopoShape from = sketchshape;
        if (method == RevolMethod::TwoAngles || midplane) {
            gp_Trsf mov;
            mov.SetRotation(revolAx, angleOffset);
            TopLoc_Location loc(mov);
            from.move(loc);
        }

        // revolve the face to a solid
        // BRepPrimAPI is the only option that allows use of this shape for patterns.
        // See https://forum.freecad.org/viewtopic.php?f=8&t=70185&p=611673#p611673.
        revol = from;
        revol = revol.makeElementRevolve(revolAx, angleTotal);
        revol.Tag = -getID();
    }
    else {
        throw Base::RuntimeError(
            "ProfileBased: Internal error: Unknown method for generateRevolution()"
        );
    }
}

void Revolved::generateRevolution(
    TopoShape& revol,
    const TopoShape& baseshape,
    const TopoDS_Shape& profileshape,
    const TopoDS_Face& supportface,
    const TopoDS_Face& uptoface,
    const gp_Ax1& axis,
    RevolMethod method,
    Part::RevolMode Mode,
    Standard_Boolean Modify
)
{
    if (method == RevolMethod::ToFirst || method == RevolMethod::ToFace
        || method == RevolMethod::ToLast) {
        revol = revol.makeElementRevolution(
            baseshape,
            profileshape,
            axis,
            supportface,
            uptoface,
            nullptr,
            Mode,
            Modify,
            nullptr
        );
    }
    else {
        throw Base::RuntimeError(
            "ProfileBased: Internal error: Unknown method for generateRevolution()"
        );
    }
}

void Revolved::updateProperties()
{
    const std::string sideType = SideType.getValueAsString();
    const std::string method = Type.getValueAsString();
    const std::string method2 = Type2.getValueAsString();

    bool isAngleEnabled = method == "Angle";
    bool isUpToFaceEnabled = method == "UpToFace";
    bool isType2Enabled = sideType == "Two sides";
    bool isAngle2Enabled = isType2Enabled && (method2 == "Angle" || isLegacyTwoAngles(method2));
    bool isUpToFace2Enabled = isType2Enabled && method2 == "UpToFace";

    if (isLegacyTwoAngles(method)) {
        isAngleEnabled = true;
        isType2Enabled = true;
        isAngle2Enabled = true;
    }

    const bool isSymmetricAngleLike = sideType == "Symmetric"
        && (method == "Angle" || method == "ThroughAll");

    Angle.setReadOnly(!isAngleEnabled);
    Type2.setReadOnly(!isType2Enabled);
    Angle2.setReadOnly(!isAngle2Enabled);
    Midplane.setReadOnly(true);
    Reversed.setReadOnly(isSymmetricAngleLike);
    UpToFace.setReadOnly(!isUpToFaceEnabled);
    UpToFace2.setReadOnly(!isUpToFace2Enabled);
}

void Revolved::onDocumentRestored()
{
    if (isLegacyTwoAngles(Type.getValueAsString())) {
        Type.setValue("Angle");
        Type2.setValue("Angle");
        SideType.setValue("Two sides");
    }
    else {
        if (isLegacyTwoAngles(Type2.getValueAsString())) {
            Type2.setValue("Angle");
        }
        if (Midplane.getValue()) {
            Midplane.setValue(false);
            SideType.setValue("Symmetric");
        }
    }

    ProfileBased::onDocumentRestored();
}

}  // namespace PartDesign
