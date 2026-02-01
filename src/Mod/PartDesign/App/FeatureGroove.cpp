// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepFeat_MakeRevol.hxx>
#include <gp_Lin.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <Base/Exception.h>
#include <Base/Tools.h>

#include "FeatureGroove.h"
#include "Mod/Part/App/TopoShapeOpCode.h"

using namespace PartDesign;

namespace PartDesign
{

/* TRANSLATOR PartDesign::Groove */

const char* Groove::TypeEnums[]
    = {"Angle", "ThroughAll", "UpToFirst", "UpToFace", "TwoAngles", nullptr};

PROPERTY_SOURCE(PartDesign::Groove, PartDesign::ProfileBased)

const App::PropertyAngle::Constraints Groove::floatAngle = {0.0, 360.0, 1.0};

Groove::Groove()
{
    addSubType = FeatureAddSub::Subtractive;

    ADD_PROPERTY_TYPE(Type, (0L), "Groove", App::Prop_None, "Groove type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(
        Base,
        (Base::Vector3d(0.0f, 0.0f, 0.0f)),
        "Groove",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden),
        "Base"
    );
    ADD_PROPERTY_TYPE(
        Axis,
        (Base::Vector3d(0.0f, 1.0f, 0.0f)),
        "Groove",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden),
        "Axis"
    );
    ADD_PROPERTY_TYPE(Angle, (360.0), "Groove", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(Angle2, (0.0), "Groove", App::Prop_None, "Groove length in 2nd direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Groove", App::Prop_None, "Face where groove will end");
    Angle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Groove", (App::Prop_None), "Reference axis of groove");
}

short Groove::mustExecute() const
{
    if (Placement.isTouched() || ReferenceAxis.isTouched() || Axis.isTouched() || Base.isTouched()
        || UpToFace.isTouched() || Angle.isTouched() || Angle2.isTouched()) {
        return 1;
    }
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn* Groove::execute()
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    constexpr double maxDegree = 360.0;
    auto method = static_cast<RevolMethod>(Type.getValue());

    // Validate parameters
    double angleDeg = Angle.getValue();
    if (angleDeg > maxDegree) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Angle of groove too large")
        );
    }

    double angle = Base::toRadians<double>(angleDeg);
    if (angle < Precision::Angular() && method == RevolMethod::Angle) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Angle of groove too small")
        );
    }

    double angle2 = Base::toRadians(Angle2.getValue());
    if (std::fabs(angle + angle2) < Precision::Angular() && method == RevolMethod::TwoAngles) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Angles of groove nullify each other")
        );
    }

    TopoShape sketchshape = getTopoShapeVerifiedFace();

    // if the Base property has a valid shape, fuse the AddShape into it
    TopoShape base;
    try {
        base = getBaseTopoShape();
    }
    catch (const Base::Exception&) {
        // fall back to support (for legacy features)
    }

    // update Axis from ReferenceAxis
    try {
        updateAxis();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    try {
        // get revolve axis
        Base::Vector3d b = Base.getValue();
        gp_Pnt pnt(b.x, b.y, b.z);
        Base::Vector3d v = Axis.getValue();

        if (v.IsNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Reference axis is invalid")
            );
        }

        gp_Dir dir(v.x, v.y, v.z);

        if (sketchshape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Creating a face from sketch failed")
            );
        }

        this->positionByPrevious();
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
        TopoShape supportface(0);
        try {
            supportface = getSupportFace();
        }
        catch (...) {
            // do nothing, null shape is handled below
        }

        supportface.move(invObjLoc);

        if (method == RevolMethod::ToFace || method == RevolMethod::ToFirst) {
            TopoShape upToFace;
            if (method == RevolMethod::ToFace) {
                getUpToFaceFromLinkSub(upToFace, UpToFace);
                upToFace.move(invObjLoc);
            }
            else {
                // TODO: Implement finding the first face this revolution would intersect with
                return new App::DocumentObjectExecReturn("Groove up to first is not yet supported");
            }

            if (Reversed.getValue()) {
                dir.Reverse();
            }

            TopExp_Explorer Ex(supportface.getShape(), TopAbs_WIRE);
            if (!Ex.More()) {
                supportface = TopoDS_Face();
            }

            try {
                result = base.makeElementRevolution(
                    base,
                    TopoDS::Face(sketchshape.getShape()),
                    gp_Ax1(pnt, dir),
                    TopoDS::Face(supportface.getShape()),
                    TopoDS::Face(upToFace.getShape()),
                    nullptr,
                    Part::RevolMode::CutFromBase,
                    Standard_True
                );
            }
            catch (Standard_Failure&) {
                return new App::DocumentObjectExecReturn("Could not revolve the sketch!");
            }
        }
        else {
            bool midplane = Midplane.getValue();
            bool reversed = Reversed.getValue();
            generateRevolution(
                result,
                sketchshape.getShape(),
                gp_Ax1(pnt, dir),
                angle,
                angle2,
                midplane,
                reversed,
                method
            );
        }

        if (!result.isNull()) {
            // store shape before refinement
            this->rawShape = result;
            result = refineShapeIfActive(result);
            // set the additive shape property for later usage in e.g. pattern
            this->AddSubShape.setValue(result);

            if (!base.isNull()) {
                result = base.makeElementCut(result);
                // store shape before refinement
                this->rawShape = result;
                result = refineShapeIfActive(result);
            }
            if (!isSingleSolidRuleSatisfied(result.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            result = getSolid(result);
            this->Shape.setValue(result);
        }
        else {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Could not revolve the sketch!")
            );
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
                "Intersecting sketch entities in a sketch are not allowed."
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

bool Groove::suggestReversed()
{
    try {
        updateAxis();
    }
    catch (const Base::Exception&) {
        return false;
    }

    return ProfileBased::getReversedAngle(Base.getValue(), Axis.getValue()) > 0.0;
}

void Groove::updateAxis()
{
    App::DocumentObject* pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string>& subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NotParallelWithNormal);

    Base.setValue(base.x, base.y, base.z);
    Axis.setValue(dir.x, dir.y, dir.z);
}

void Groove::generateRevolution(
    TopoShape& revol,
    const TopoShape& sketchshape,
    const gp_Ax1& axis,
    const double angle,
    const double angle2,
    const bool midplane,
    const bool reversed,
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

        if (fabs(angleTotal) < Precision::Angular()) {
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
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method for generateRevolution()";
        throw Base::RuntimeError(str.str());
    }
}

void Groove::generateRevolution(
    TopoShape& revol,
    const TopoShape& baseshape,
    const TopoDS_Shape& profileshape,
    const TopoDS_Face& supportface,
    const TopoDS_Face& uptoface,
    const gp_Ax1& axis,
    RevolMethod method,
    RevolMode Mode,
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
            static_cast<Part::RevolMode>(Mode),
            Modify,
            nullptr
        );
    }
    else {
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method for generateRevolution()";
        throw Base::RuntimeError(str.str());
    }
}

void Groove::updateProperties(RevolMethod method)
{
    // disable settings that are not valid on the current method
    // disable everything unless we are sure we need it
    bool isAngleEnabled = false;
    bool isAngle2Enabled = false;
    bool isMidplaneEnabled = false;
    bool isReversedEnabled = false;
    bool isUpToFaceEnabled = false;
    if (method == RevolMethod::Angle) {
        isAngleEnabled = true;
        isMidplaneEnabled = true;
        isReversedEnabled = !Midplane.getValue();
    }
    else if (method == RevolMethod::ToLast) {
        isReversedEnabled = true;
    }
    else if (method == RevolMethod::ThroughAll) {
        isMidplaneEnabled = true;
        isReversedEnabled = !Midplane.getValue();
    }
    else if (method == RevolMethod::ToFirst) {
        isReversedEnabled = true;
    }
    else if (method == RevolMethod::ToFace) {
        isReversedEnabled = true;
        isUpToFaceEnabled = true;
    }
    else if (method == RevolMethod::TwoAngles) {
        isAngleEnabled = true;
        isAngle2Enabled = true;
        isReversedEnabled = true;
    }

    Angle.setReadOnly(!isAngleEnabled);
    Angle2.setReadOnly(!isAngle2Enabled);
    Midplane.setReadOnly(!isMidplaneEnabled);
    Reversed.setReadOnly(!isReversedEnabled);
    UpToFace.setReadOnly(!isUpToFaceEnabled);
}

}  // namespace PartDesign
