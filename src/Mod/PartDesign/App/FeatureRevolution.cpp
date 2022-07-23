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
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepFeat_MakeRevol.hxx>
# include <gp_Lin.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
#endif

#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Tools.h>

#include "FeatureRevolution.h"

using namespace PartDesign;

namespace PartDesign {

const char* Revolution::TypeEnums[]= {"Angle", "UpToLast", "UpToFirst", "UpToFace", "TwoAngles", nullptr};

PROPERTY_SOURCE(PartDesign::Revolution, PartDesign::ProfileBased)

const App::PropertyAngle::Constraints Revolution::floatAngle = { Base::toDegrees<double>(Precision::Angular()), 360.0, 1.0 };

Revolution::Revolution()
{
    addSubType = FeatureAddSub::Additive;

    ADD_PROPERTY_TYPE(Type, (0L), "Revolution", App::Prop_None, "Revolution type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Base,(Base::Vector3d(0.0,0.0,0.0)),"Revolution", App::Prop_ReadOnly, "Base");
    ADD_PROPERTY_TYPE(Axis,(Base::Vector3d(0.0,1.0,0.0)),"Revolution", App::Prop_ReadOnly, "Axis");
    ADD_PROPERTY_TYPE(Angle,(360.0),"Revolution", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Revolution", App::Prop_None, "Face where revolution will end");
    ADD_PROPERTY_TYPE(Angle2, (60.0), "Revolution", App::Prop_None, "Revolution length in 2nd direction");

    Angle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(ReferenceAxis,(nullptr),"Revolution",(App::Prop_None),"Reference axis of revolution");
}

short Revolution::mustExecute() const
{
    if (Placement.isTouched() ||
        ReferenceAxis.isTouched() ||
        Axis.isTouched() ||
        Base.isTouched() ||
        UpToFace.isTouched() ||
        Angle.isTouched() ||
        Angle2.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Revolution::execute()
{
    // Validate parameters
    // All angles are in radians unless explicitly stated
    double angleDeg = Angle.getValue();
    if (angleDeg > 360.0)
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Angle of revolution too large"));

    double angle = Base::toRadians<double>(angleDeg);
    if (angle < Precision::Angular())
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Angle of revolution too small"));

    double angle2 = Base::toRadians(Angle2.getValue());

    TopoDS_Shape sketchshape;
    try {
        sketchshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the AddShape into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        // fall back to support (for legacy features)
        base = TopoDS_Shape();
    }

    // update Axis from ReferenceAxis
    try {
        updateAxis();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // get revolve axis
    Base::Vector3d b = Base.getValue();
    gp_Pnt pnt(b.x,b.y,b.z);
    Base::Vector3d v = Axis.getValue();
    gp_Dir dir(v.x,v.y,v.z);

    try {
        if (sketchshape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Creating a face from sketch failed"));

        std::string method(Type.getValueAsString());

        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        pnt.Transform(invObjLoc.Transformation());
        dir.Transform(invObjLoc.Transformation());
        base.Move(invObjLoc);
        sketchshape.Move(invObjLoc);

        // Check distance between sketchshape and axis - to avoid failures and crashes
        TopExp_Explorer xp;
        xp.Init(sketchshape, TopAbs_FACE);
        for (;xp.More(); xp.Next()) {
            if (checkLineCrossesFace(gp_Lin(pnt, dir), TopoDS::Face(xp.Current())))
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Revolve axis intersects the sketch"));
        }

        // Create a fresh support even when base exists so that it can be used for patterns
        TopoDS_Shape result;
        TopoDS_Face supportface = getSupportFace();
        supportface.Move(invObjLoc);

        if (method == "UpToFace" || method == "UpToFirst" || method == "UpToLast") {
            TopoDS_Face upToFace;
            if (method == "UpToFace") {
                getFaceFromLinkSub(upToFace, UpToFace);
                upToFace.Move(invObjLoc);
            }
            else
                throw Base::RuntimeError("ProfileBased: Revolution up to first/last is not yet supported");

            // TODO: This method is designed for extrusions. needs to be adapted for revolutions.
            getUpToFace(upToFace, base, sketchshape, method, dir);

            TopoDS_Face supportface = getSupportFace();
            supportface.Move(invObjLoc);

            if (Reversed.getValue())
                dir.Reverse();

            TopExp_Explorer Ex(supportface,TopAbs_WIRE);
            if (!Ex.More())
                supportface = TopoDS_Face();
            RevolMode mode = RevolMode::None;
            generateRevolution(result, method, base, sketchshape, supportface, upToFace, gp_Ax1(pnt, dir), mode, Standard_True);
        }
        else {
            bool midplane = Midplane.getValue();
            bool reversed = Reversed.getValue();
            generateRevolution(result, sketchshape, method, gp_Ax1(pnt, dir), angle, angle2, midplane, reversed);
        }

        if (!result.IsNull()) {
            result = refineShapeIfActive(result);
            // set the additive shape property for later usage in e.g. pattern
            this->AddSubShape.setValue(result);

            if (!base.IsNull()) {
                // Let's call algorithm computing a fuse operation:
                BRepAlgoAPI_Fuse mkFuse(base, result);
                // Let's check if the fusion has been successful
                if (!mkFuse.IsDone())
                    throw Part::BooleanException(QT_TRANSLATE_NOOP("Exception", "Fusion with base feature failed"));
                result = mkFuse.Shape();
                result = refineShapeIfActive(result);
            }

            this->Shape.setValue(getSolid(result));
        }
        else
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Could not revolve the sketch!"));

        // eventually disable some settings that are not valid for the current method
        updateProperties(method);

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        if (std::string(e.GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Could not create face from sketch.\n"
                "Intersecting sketch entities in a sketch are not allowed."));
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

bool Revolution::suggestReversed()
{
    try {
        updateAxis();
    } catch (const Base::Exception&) {
        return false;
    }

    return ProfileBased::getReversedAngle(Base.getValue(), Axis.getValue()) < 0.0;
}

void Revolution::updateAxis()
{
    App::DocumentObject *pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NotParallelWithNormal);

    Base.setValue(base.x,base.y,base.z);
    Axis.setValue(dir.x,dir.y,dir.z);
}

void Revolution::generateRevolution(TopoDS_Shape& revol,
                                    const TopoDS_Shape& sketchshape,
                                    const std::string& method,
                                    const gp_Ax1& axis,
                                    const double angle,
                                    const double angle2,
                                    const bool midplane,
                                    const bool reversed)
{
    if (method == "Angle" || method == "TwoAngles" || method == "ThroughAll") {
    double angleTotal = angle;
    double angleOffset = 0.;

    if (method == "TwoAngles") {
        // Rotate the face by `angle2`/`angle` to get "second" angle
        angleTotal += angle2;
        angleOffset = angle2 * -1.0;
    }
    else if (midplane) {
        // Rotate the face by half the angle to get Revolution symmetric to sketch plane
        angleOffset = -angle / 2;
    }

    if (fabs(angleTotal) < Precision::Angular())
        throw Base::ValueError("Cannot create a revolution with zero angle.");

    gp_Ax1 revolAx(axis);
    if (reversed) {
        revolAx.Reverse();
    }

    TopoDS_Shape from = sketchshape;
    if (method == "TwoAngles" || midplane) {
        gp_Trsf mov;
        mov.SetRotation(revolAx, angleOffset);
        TopLoc_Location loc(mov);
        from.Move(loc);
    }

    // revolve the face to a solid
    // BRepPrimAPI is the only option that allows use of this shape for patterns.
    // See https://forum.freecadweb.org/viewtopic.php?f=8&t=70185&p=611673#p611673.
    BRepPrimAPI_MakeRevol RevolMaker(from, revolAx, angleTotal);

    if (!RevolMaker.IsDone())
        throw Base::RuntimeError("ProfileBased: RevolMaker failed! Could not revolve the sketch!");
    else
        revol = RevolMaker.Shape();
    }
    else {
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method '"
            << method << "' for generateRevolution()";
        throw Base::RuntimeError(str.str());
    }
}

void Revolution::generateRevolution(TopoDS_Shape& revol,
                                    const std::string& method,
                                    const TopoDS_Shape& baseshape,
                                    const TopoDS_Shape& profileshape,
                                    const TopoDS_Face& supportface,
                                    const TopoDS_Face& uptoface,
                                    const gp_Ax1& axis,
                                    RevolMode Mode,
                                    Standard_Boolean Modify)
{
    if (method == "UpToFirst" || method == "UpToFace" || method == "UpToLast") {
        BRepFeat_MakeRevol RevolMaker;
        TopoDS_Shape base = baseshape;
        for (TopExp_Explorer xp(profileshape, TopAbs_FACE); xp.More(); xp.Next()) {
            RevolMaker.Init(base, xp.Current(), supportface, axis, Mode, Modify);
            RevolMaker.Perform(uptoface);
            if (!RevolMaker.IsDone())
                throw Base::RuntimeError("ProfileBased: Up to face: Could not revolve the sketch!");

            base = RevolMaker.Shape();
            if (Mode == RevolMode::None)
                Mode = RevolMode::FuseWithBase;
        }

        revol = base;
    }
    else {
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method '"
            << method << "' for generateRevolution()";
        throw Base::RuntimeError(str.str());
    }
}

void Revolution::updateProperties(const std::string &method)
{
    // disable settings that are not valid on the current method
    // disable everything unless we are sure we need it
    bool isAngleEnabled = false;
    bool isAngle2Enabled = false;
    bool isMidplaneEnabled = false;
    bool isReversedEnabled = false;
    bool isUpToFaceEnabled = false;
    if (method == "Angle") {
        isAngleEnabled = true;
        isMidplaneEnabled = true;
        isReversedEnabled = !Midplane.getValue();
    }
    else if (method == "UpToLast") {
        isReversedEnabled = true;
    }
    else if (method == "ThroughAll") {
        isMidplaneEnabled = true;
        isReversedEnabled = !Midplane.getValue();
    }
    else if (method == "UpToFirst") {
        isReversedEnabled = true;
    }
    else if (method == "UpToFace") {
        isReversedEnabled = true;
        isUpToFaceEnabled = true;
    }
    else if (method == "TwoAngles") {
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

}
