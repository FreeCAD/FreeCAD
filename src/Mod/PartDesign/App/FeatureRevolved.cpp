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

#include <algorithm>
#include <vector>

#include <gp_Lin.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureRevolved.h"

using namespace PartDesign;

namespace PartDesign
{

/* TRANSLATOR PartDesign::Revolved */

PROPERTY_SOURCE_ABSTRACT(PartDesign::Revolved, PartDesign::ProfileBased)  // NOLINT

const App::PropertyAngle::Constraints Revolved::floatAngle = {0.0, 360.0, 1.0};

Revolved::Revolved()
{
    Angle.setConstraints(&floatAngle);
    Angle2.setConstraints(&floatAngle);
    StartAngle.setConstraints(&floatAngle);
    StartAngle2.setConstraints(&floatAngle);
}

short Revolved::mustExecute() const
{
    if (Placement.isTouched() || ReferenceAxis.isTouched() || Axis.isTouched() || Base.isTouched()
        || UpToFace.isTouched() || Angle.isTouched() || Angle2.isTouched() || StartAngle.isTouched()
        || StartAngle2.isTouched()) {
        return 1;
    }
    return ProfileBased::mustExecute();
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
    const auto method = static_cast<RevolMethod>(Type.getValue());

    // Validate parameters
    double angleDeg = Angle.getValue();
    double angle2Deg = Angle2.getValue();
    double startAngleDeg = StartAngle.getValue();
    double startAngle2Deg = StartAngle2.getValue();
    if (angleDeg > maxDegree || angle2Deg > maxDegree || startAngleDeg > maxDegree
        || startAngle2Deg > maxDegree) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Angle of revolution too large")
        );
    }

    auto angle = Base::toRadians<double>(angleDeg);
    auto startAngle = Base::toRadians<double>(startAngleDeg);
    double angle2 = Base::toRadians(angle2Deg);
    double startAngle2 = Base::toRadians(startAngle2Deg);

    if (method == RevolMethod::Angle) {
        if (angle - startAngle < Precision::Angular()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "End angle must be greater than start angle")
            );
        }
    }
    else if (method == RevolMethod::TwoAngles) {
        if (angle - startAngle < -Precision::Angular()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "End angle must not be smaller than start angle on side 1")
            );
        }
        if (angle2 - startAngle2 < -Precision::Angular()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "End angle must not be smaller than start angle on side 2")
            );
        }
        if (std::fabs(angle - startAngle) + std::fabs(angle2 - startAngle2) < Precision::Angular()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Cannot create a revolution with zero angle")
            );
        }
    }

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

    // Create a fresh support even when base exists so that it can be used for patterns
    TopoShape result(0);
    TopoShape supportface = tryGetSupportShape();

    supportface.move(invObjLoc);

    if (method == RevolMethod::ToFirst
        || (method == RevolMethod::ToLast && revolMode == Part::RevolMode::FuseWithBase)) {
        TopoShape upToFace;
        gp_Ax1 axis(pnt, dir);
        if (Reversed.getValue()) {
            axis.Reverse();
        }
        getUpToFace(
            upToFace,
            base,
            sketchshape,
            method == RevolMethod::ToFirst ? "UpToFirst" : "UpToLast",
            axis
        );
        result = tryToRevolveToFace(upToFace, pnt, dir, base, supportface, sketchshape, revolMode);
    }
    else if (method == RevolMethod::ToFace) {
        TopoShape upToFace;
        getUpToFaceFromLinkSub(upToFace, UpToFace);
        upToFace.move(invObjLoc);
        result = tryToRevolveToFace(upToFace, pnt, dir, base, supportface, sketchshape, revolMode);
    }
    else {
        bool midplane = Midplane.getValue();
        bool reversed = Reversed.getValue();
        generateRevolution(
            result,
            sketchshape,
            gp_Ax1(pnt, dir),
            angle,
            angle2,
            startAngle,
            startAngle2,
            midplane,
            reversed,
            method
        );
    }

    setResult(base, result);

    // eventually disable some settings that are not valid for the current method
    updateProperties(method);

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
    gp_Pnt pnt,
    gp_Dir dir,
    TopoShape& base,
    TopoShape& supportface,
    const TopoShape& sketchshape,
    Part::RevolMode revolMode
) const
{
    if (Reversed.getValue()) {
        dir.Reverse();
    }

    TopExp_Explorer Ex(supportface.getShape(), TopAbs_WIRE);
    if (!Ex.More()) {
        supportface = TopoDS_Face();
    }

    try {
        return base.makeElementRevolution(
            base,
            TopoDS::Face(sketchshape.getShape()),
            gp_Ax1(pnt, dir),
            TopoDS::Face(supportface.getShape()),
            TopoDS::Face(upToFace.getShape()),
            nullptr,
            revolMode,
            Standard_True
        );
    }
    catch (const Standard_Failure&) {
        throw Base::RuntimeError("Could not revolve the sketch!");
    }
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
    double startAngle,
    double startAngle2,
    bool midplane,
    bool reversed,
    RevolMethod method
)
{
    if (method == RevolMethod::Angle || method == RevolMethod::TwoAngles
        || method == RevolMethod::ThroughAll) {
        gp_Ax1 revolAx(axis);
        if (reversed) {
            revolAx.Reverse();
        }

        auto makeRevolutionSegment = [&](const gp_Ax1& segmentAxis, double start, double end) {
            double angleTotal = end - start;
            if (std::fabs(angleTotal) < Precision::Angular()) {
                return TopoShape(0);
            }

            TopoShape from = sketchshape;
            if (std::fabs(start) >= Precision::Angular()) {
                gp_Trsf mov;
                mov.SetRotation(segmentAxis, start);
                TopLoc_Location loc(mov);
                from.move(loc);
            }

            // revolve the face to a solid
            // BRepPrimAPI is the only option that allows use of this shape for patterns.
            // See https://forum.freecad.org/viewtopic.php?f=8&t=70185&p=611673#p611673.
            TopoShape segment = from;
            segment = segment.makeElementRevolve(segmentAxis, angleTotal);
            segment.Tag = -getID();
            return segment;
        };

        std::vector<TopoShape> revolutions;
        if (method == RevolMethod::ThroughAll) {
            revolutions.push_back(makeRevolutionSegment(revolAx, 0.0, 2 * std::numbers::pi));
        }
        else if (method == RevolMethod::TwoAngles) {
            revolutions.push_back(makeRevolutionSegment(revolAx, startAngle, angle));

            // Reversed is already applied to revolAx; the second side intentionally uses
            // the opposite axis.
            gp_Ax1 revolAx2(revolAx);
            revolAx2.Reverse();
            revolutions.push_back(makeRevolutionSegment(revolAx2, startAngle2, angle2));
        }
        else if (midplane) {
            // Start and end remain the total angular span in midplane mode and are split
            // equally to both sides of the sketch plane, matching symmetric extrusion.
            const double symmetricStart = startAngle / 2.0;
            const double symmetricAngle = angle / 2.0;

            revolutions.push_back(makeRevolutionSegment(revolAx, symmetricStart, symmetricAngle));

            gp_Ax1 revolAx2(revolAx);
            revolAx2.Reverse();
            revolutions.push_back(makeRevolutionSegment(revolAx2, symmetricStart, symmetricAngle));
        }
        else {
            revolutions.push_back(makeRevolutionSegment(revolAx, startAngle, angle));
        }

        revolutions.erase(
            std::remove_if(
                revolutions.begin(),
                revolutions.end(),
                [](const TopoShape& shape) { return shape.isNull() || shape.getShape().IsNull(); }
            ),
            revolutions.end()
        );

        if (revolutions.empty()) {
            throw Base::ValueError("Cannot create a revolution with zero angle.");
        }
        if (revolutions.size() == 1) {
            revol = revolutions.front();
        }
        else {
            revol.makeElementFuse(revolutions);
            revol.Tag = -getID();
        }
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

void Revolved::updateProperties(RevolMethod method)
{
    // disable settings that are not valid on the current method
    // disable everything unless we are sure we need it
    bool isStartAngleEnabled = false;
    bool isAngleEnabled = false;
    bool isStartAngle2Enabled = false;
    bool isAngle2Enabled = false;
    bool isMidplaneEnabled = false;
    bool isReversedEnabled = false;
    bool isUpToFaceEnabled = false;
    switch (method) {
        case RevolMethod::Angle:
            isStartAngleEnabled = true;
            isAngleEnabled = true;
            isMidplaneEnabled = true;
            isReversedEnabled = !Midplane.getValue();
            break;
        case RevolMethod::ThroughAll:
            isMidplaneEnabled = true;
            isReversedEnabled = !Midplane.getValue();
            break;
        case RevolMethod::ToFirst:
            isReversedEnabled = true;
            break;
        case RevolMethod::ToFace:
            isReversedEnabled = true;
            isUpToFaceEnabled = true;
            break;
        case RevolMethod::TwoAngles:
            isStartAngleEnabled = true;
            isAngleEnabled = true;
            isStartAngle2Enabled = true;
            isAngle2Enabled = true;
            isReversedEnabled = true;
            break;
    }

    StartAngle.setReadOnly(!isStartAngleEnabled);
    Angle.setReadOnly(!isAngleEnabled);
    StartAngle2.setReadOnly(!isStartAngle2Enabled);
    Angle2.setReadOnly(!isAngle2Enabled);
    Midplane.setReadOnly(!isMidplaneEnabled);
    Reversed.setReadOnly(!isReversedEnabled);
    UpToFace.setReadOnly(!isUpToFaceEnabled);
}

}  // namespace PartDesign
