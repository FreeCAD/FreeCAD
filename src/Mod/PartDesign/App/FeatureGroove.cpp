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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepFeat_MakeRevol.hxx>
# include <gp_Lin.hxx>
# include <TopoDS.hxx>
# include <TopExp_Explorer.hxx>
# include <Precision.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Tools.h>

#include "FeatureGroove.h"


using namespace PartDesign;

namespace PartDesign {

/* TRANSLATOR PartDesign::Groove */

const char* Groove::TypeEnums[]= {"Angle", "ThroughAll", "UpToFirst", "UpToFace", "TwoAngles", nullptr};

PROPERTY_SOURCE(PartDesign::Groove, PartDesign::ProfileBased)

const App::PropertyAngle::Constraints Groove::floatAngle = { Base::toDegrees<double>(Precision::Angular()), 360.0, 1.0 };

Groove::Groove()
{
    addSubType = FeatureAddSub::Subtractive;

    ADD_PROPERTY_TYPE(Type, (0L), "Groove", App::Prop_None, "Groove type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Base, (Base::Vector3d(0.0f,0.0f,0.0f)), "Groove", App::Prop_ReadOnly, "Base");
    ADD_PROPERTY_TYPE(Axis, (Base::Vector3d(0.0f,1.0f,0.0f)), "Groove", App::Prop_ReadOnly, "Axis");
    ADD_PROPERTY_TYPE(Angle, (360.0),"Groove", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(Angle2, (60.0), "Groove", App::Prop_None, "Groove length in 2nd direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Groove", App::Prop_None, "Face where groove will end");
    Angle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Groove", (App::PropertyType)(App::Prop_None), "Reference axis of Groove");
}

short Groove::mustExecute() const
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

App::DocumentObjectExecReturn *Groove::execute()
{
    // Validate parameters
    // All angles are in radians unless explicitly stated
    double angleDeg = Angle.getValue();
    if (angleDeg > 360.0)
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Angle of groove too large"));

    double angle = Base::toRadians<double>(angleDeg);
    if (angle < Precision::Angular())
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Angle of groove too small"));

    double angle2 = Base::toRadians(Angle2.getValue());

    TopoDS_Shape sketchshape;
    try {
        sketchshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    }
    catch (const Base::Exception&) {
        std::string text(QT_TRANSLATE_NOOP("Exception", "The requested feature cannot be created. The reason may be that:\n"
                                    "  - the active Body does not contain a base shape, so there is no\n"
                                    "  material to be removed;\n"
                                    "  - the selected sketch does not belong to the active Body."));
        return new App::DocumentObjectExecReturn(text);
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

        RevolMethod method = methodFromString(Type.getValueAsString());

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

        if (method == RevolMethod::ToFace || method == RevolMethod::ToFirst) {
            TopoDS_Face upToFace;
            if (method == RevolMethod::ToFace) {
                getFaceFromLinkSub(upToFace, UpToFace);
                upToFace.Move(invObjLoc);
            }
            else
                throw Base::RuntimeError("ProfileBased: Groove up to first is not yet supported");

            // TODO: This method is designed for extrusions. needs to be adapted for grooves.
            // getUpToFace(upToFace, base, supportface, sketchshape, method, dir);

            TopoDS_Face supportface = getSupportFace();
            supportface.Move(invObjLoc);

            if (Reversed.getValue())
                dir.Reverse();

            TopExp_Explorer Ex(supportface,TopAbs_WIRE);
            if (!Ex.More())
                supportface = TopoDS_Face();
            RevolMode mode = RevolMode::CutFromBase;
            generateRevolution(result, base, sketchshape, supportface, upToFace, gp_Ax1(pnt, dir), method, mode, Standard_True);

            result = refineShapeIfActive(result);

            // the result we get here is the shape _after_ the operation is done
            // And the really expensive way to get the SubShape...
            BRepAlgoAPI_Cut mkCut(base, result);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Groove: Up to face: Could not get SubShape!");
            // FIXME: In some cases this affects the Shape property: It is set to the same shape as the SubShape!!!!
            TopoDS_Shape subshape = refineShapeIfActive(mkCut.Shape());
            this->AddSubShape.setValue(subshape);

            int resultCount = countSolids(result);
            if (resultCount > 1) {
                return new App::DocumentObjectExecReturn("Groove: Result has multiple solids. This is not supported at this time.");
            }

            this->Shape.setValue(getSolid(result));
        }
        else {
            bool midplane = Midplane.getValue();
            bool reversed = Reversed.getValue();
            generateRevolution(result, sketchshape, gp_Ax1(pnt, dir), angle, angle2, midplane, reversed, method);

            if (result.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Could not revolve the sketch!"));

            // set the subtractive shape property for later usage in e.g. pattern
            result = refineShapeIfActive(result);
            this->AddSubShape.setValue(result);

            // cut out groove to get one result object
            BRepAlgoAPI_Cut mkCut(base, result);
            // Let's check if the fusion has been successful
            if (!mkCut.IsDone())
                throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Cut out of base feature failed"));

            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape solRes = this->getSolid(mkCut.Shape());
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

            solRes = refineShapeIfActive(solRes);
            this->Shape.setValue(getSolid(solRes));

            int solidCount = countSolids(solRes);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }
        }

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

bool Groove::suggestReversed()
{
    updateAxis();
    return ProfileBased::getReversedAngle(Base.getValue(), Axis.getValue()) > 0.0;
}

void Groove::updateAxis()
{
    App::DocumentObject *pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NotParallelWithNormal);

    if (dir.Length() > Precision::Confusion()) {
        Base.setValue(base.x,base.y,base.z);
        Axis.setValue(dir.x,dir.y,dir.z);
    }
}

Groove::RevolMethod Groove::methodFromString(const std::string& methodStr)
{
    if (methodStr == "Angle")
        return RevolMethod::Dimension;
    if (methodStr == "UpToLast")
        return RevolMethod::ToLast;
    if (methodStr == "ThroughAll")
        return RevolMethod::ThroughAll;
    if (methodStr == "UpToFirst")
        return RevolMethod::ToFirst;
    if (methodStr == "UpToFace")
        return RevolMethod::ToFace;
    if (methodStr == "TwoAngles")
        return RevolMethod::TwoDimensions;

    throw Base::ValueError("Groove:: No such method");
    return RevolMethod::Dimension;
}

void Groove::generateRevolution(TopoDS_Shape& revol,
                                const TopoDS_Shape& sketchshape,
                                const gp_Ax1& axis,
                                const double angle,
                                const double angle2,
                                const bool midplane,
                                const bool reversed,
                                RevolMethod method)
{
    if (method == RevolMethod::Dimension || method == RevolMethod::TwoDimensions || method == RevolMethod::ThroughAll) {
        double angleTotal = angle;
        double angleOffset = 0.;

        if (method == RevolMethod::TwoDimensions) {
            // Rotate the face by `angle2`/`angle` to get "second" angle
            angleTotal += angle2;
            angleOffset = angle2 * -1.0;
        }
        else if (method == RevolMethod::ThroughAll) {
            angleTotal = M_PI * 2;
        }
        else if (midplane) {
            // Rotate the face by half the angle to get Groove symmetric to sketch plane
            angleOffset = -angle / 2;
        }

        if (fabs(angleTotal) < Precision::Angular())
            throw Base::ValueError("Cannot create a revolution with zero angle.");

        TopoDS_Shape from = sketchshape;
        if (method == RevolMethod::TwoDimensions || midplane) {
            gp_Trsf mov;
            mov.SetRotation(axis, angleOffset);
            TopLoc_Location loc(mov);
            from.Move(loc);
        }
        else if (reversed) {
            angleTotal *= -1.0;
        }

        // revolve the face to a solid
        // BRepPrimAPI is the only option that allows use of this shape for patterns.
        // See https://forum.freecadweb.org/viewtopic.php?f=8&t=70185&p=611673#p611673.
        BRepPrimAPI_MakeRevol RevolMaker(from, axis, angleTotal);

        if (!RevolMaker.IsDone())
            throw Base::RuntimeError("ProfileBased: RevolMaker failed! Could not revolve the sketch!");
        else
            revol = RevolMaker.Shape();
    }
    else {
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method for generateGroove()";
        throw Base::RuntimeError(str.str());
    }
}

void Groove::generateRevolution(TopoDS_Shape& revol,
                                const TopoDS_Shape& baseshape,
                                const TopoDS_Shape& profileshape,
                                const TopoDS_Face& supportface,
                                const TopoDS_Face& uptoface,
                                const gp_Ax1& axis,
                                RevolMethod method,
                                RevolMode Mode,
                                Standard_Boolean Modify)
{
    if (method == RevolMethod::ToFirst || method == RevolMethod::ToFace || method == RevolMethod::ToLast) {
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
    if (method == RevolMethod::Dimension) {
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
    else if (method == RevolMethod::TwoDimensions) {
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
