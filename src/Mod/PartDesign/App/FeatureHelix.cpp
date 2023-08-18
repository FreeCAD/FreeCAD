/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                 2020 David Österberg                                    *
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
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <gp_Ax1.hxx>
# include <gp_Ax3.hxx>
#endif

# include <Standard_Version.hxx>
# include <Base/Axis.h>
# include <Base/Exception.h>
# include <Base/Placement.h>
# include <Base/Tools.h>

# include <Mod/Part/App/TopoShape.h>
# include <Mod/Part/App/FaceMakerCheese.h>

# include "FeatureHelix.h"

using namespace PartDesign;

const char* Helix::ModeEnums[] = { "pitch-height-angle", "pitch-turns-angle", "height-turns-angle", "height-turns-growth", nullptr };

PROPERTY_SOURCE(PartDesign::Helix, PartDesign::ProfileBased)

// we purposely use not FLT_MAX because this would not be computable
const App::PropertyFloatConstraint::Constraints Helix::floatTurns = { Precision::Confusion(), INT_MAX, 1.0 };
const App::PropertyAngle::Constraints Helix::floatAngle = { -89.0, 89.0, 1.0 };

Helix::Helix()
{
    addSubType = FeatureAddSub::Additive;
    auto initialMode = HelixMode::pitch_height_angle;

    const char* group = "Helix";
    ADD_PROPERTY_TYPE(Base, (Base::Vector3d(0.0, 0.0, 0.0)), group, App::Prop_ReadOnly,
        QT_TRANSLATE_NOOP("App::Property", "The center point of the helix' start; derived from the reference axis."));
    ADD_PROPERTY_TYPE(Axis, (Base::Vector3d(0.0, 1.0, 0.0)), group, App::Prop_ReadOnly,
        QT_TRANSLATE_NOOP("App::Property", "The helix' direction; derived from the reference axis."));
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "The reference axis of the helix."));
    ADD_PROPERTY_TYPE(Mode, (long(initialMode)), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "The helix input mode specifies which properties are set by the user.\n"
            "Dependent properties are then calculated."));
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Pitch, (10.0), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "The axial distance between two turns."));
    ADD_PROPERTY_TYPE(Height, (30.0), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "The height of the helix' path, not accounting for the extent of the profile."));
    ADD_PROPERTY_TYPE(Turns, (3.0), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "The number of turns in the helix."));
    Turns.setConstraints(&floatTurns);
    ADD_PROPERTY_TYPE(Angle, (0.0), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "The angle of the cone that forms a hull around the helix.\n"
            "Non-zero values turn the helix into a conical spiral.\n"
            "Positive values make the radius grow, nevatige shrink."));
    Angle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(Growth, (0.0), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "The growth of the helix' radius per turn.\n"
            "Non-zero values turn the helix into a conical spiral."));
    ADD_PROPERTY_TYPE(LeftHanded, (false), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Sets the turning direction to left handed,\n"
            "i.e. counter-clockwise when moving along its axis."));
    ADD_PROPERTY_TYPE(Reversed, (false), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Determines whether the helix points in the opposite direction of the axis."));
    ADD_PROPERTY_TYPE(Outside, (false), group, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "If set, the result will be the intersection of the profile and the preexisting body."));
    ADD_PROPERTY_TYPE(HasBeenEdited, (false), group, App::Prop_Hidden,
        QT_TRANSLATE_NOOP("App::Property", "If false, the tool will propose an initial value for the pitch based on the profile bounding box,\n"
            "so that self intersection is avoided."));

    setReadWriteStatusForMode(initialMode);
}

short Helix::mustExecute() const
{
    if (Placement.isTouched() ||
        ReferenceAxis.isTouched() ||
        Axis.isTouched() ||
        Base.isTouched() ||
        Angle.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn* Helix::execute()
{
    // Validate and normalize parameters
    HelixMode mode = static_cast<HelixMode>(Mode.getValue());
    if (mode == HelixMode::pitch_height_angle) {
        if (Pitch.getValue() < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Pitch too small"));
        if (Height.getValue() < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: height too small!"));
        Turns.setValue(Height.getValue() / Pitch.getValue());
        Growth.setValue(Pitch.getValue() * tan(Base::toRadians(Angle.getValue())));
    }
    else if (mode == HelixMode::pitch_turns_angle) {
        if (Pitch.getValue() < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: pitch too small!"));
        if (Turns.getValue() < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: turns too small!"));
        Height.setValue(Turns.getValue() * Pitch.getValue());
        Growth.setValue(Pitch.getValue() * tan(Base::toRadians(Angle.getValue())));
    }
    else if (mode == HelixMode::height_turns_angle) {
        if (Height.getValue() < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: height too small!"));
        if (Turns.getValue() < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: turns too small!"));
        Pitch.setValue(Height.getValue() / Turns.getValue());
        Growth.setValue(Pitch.getValue() * tan(Base::toRadians(Angle.getValue())));
    }
    else if (mode == HelixMode::height_turns_growth) {
        if (Turns.getValue() < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: turns too small!"));
        if ((Height.getValue() < Precision::Confusion())
            && (abs(Growth.getValue()) < Precision::Confusion())
            && Turns.getValue() > 1.0)
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: either height or growth must not be zero!"));
        Pitch.setValue(Height.getValue() / Turns.getValue());
        if (Height.getValue() > 0) {
            Angle.setValue(Base::toDegrees(atan(
                Turns.getValue() * Growth.getValue() / Height.getValue())));
        }
        else {
            // On purpose, we're doing nothing here; the else-branch is just for this comment.
            // - we don't print a warning, as for a flat spiral a zero-height is perfectly fine
            // - we don't void the angle (somehow) so that it keeps its value. This allows in
            //   interactive usage to just go back to another mode and everything keeps working
        }
    }
    else {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: unsupported mode"));
    }

    TopoDS_Shape sketchshape;
    try {
        sketchshape = getVerifiedFace();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    if (sketchshape.IsNull())
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: No valid sketch or face"));
    else {
        //TODO: currently we only allow planar faces. the reason for this is that with other faces in front, we could
        //not use the current simulate approach and build the start and end face from the wires. As the shell
        //begins always at the spine and not the profile, the sketchshape cannot be used directly as front face.
        //We would need a method to translate the front shape to match the shell starting position somehow...
        TopoDS_Face face = TopoDS::Face(sketchshape);
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() != GeomAbs_Plane)
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Face must be planar"));
    }

    // if the Base property has a valid shape, fuse the AddShape into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    }
    catch (const Base::Exception&) {
        // fall back to support (for legacy features)
        base = TopoDS_Shape();
    }

    // update Axis from ReferenceAxis
    try {
        updateAxis();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);

        // generate the helix path
        TopoDS_Shape path = generateHelixPath();
        TopoDS_Shape auxpath = generateHelixPath(1.0);


        std::vector<TopoDS_Wire> wires;
        try {
            wires = getProfileWires();
        }
        catch (const Base::Exception& e) {
            return new App::DocumentObjectExecReturn(e.what());
        }

        std::vector<std::vector<TopoDS_Wire>> wiresections;
        for (TopoDS_Wire& wire : wires)
            wiresections.emplace_back(1, wire);

        //build all shells
        std::vector<TopoDS_Shape> shells;
        std::vector<TopoDS_Wire> frontwires, backwires;
        for (std::vector<TopoDS_Wire>& wires : wiresections) {

            BRepOffsetAPI_MakePipeShell mkPS(TopoDS::Wire(path));

            mkPS.SetTolerance(Precision::Confusion());
            mkPS.SetTransitionMode(BRepBuilderAPI_Transformed);

            if (Angle.getValue() == 0) {
                mkPS.SetMode(true);  //This is for frenet
            } else {
                mkPS.SetMode(TopoDS::Wire(auxpath), true);  // this is for auxiliary
            }

            for (TopoDS_Wire& wire : wires) {
                wire.Move(invObjLoc);
                mkPS.Add(wire);
            }

            if (!mkPS.IsReady())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Could not build"));

            shells.push_back(mkPS.Shape());

            if (!mkPS.Shape().Closed()) {
                // shell is not closed - use simulate to get the end wires
                TopTools_ListOfShape sim;
                mkPS.Simulate(2, sim);

                frontwires.push_back(TopoDS::Wire(sim.First()));
                backwires.push_back(TopoDS::Wire(sim.Last()));
            }
        }

        BRepBuilderAPI_MakeSolid mkSolid;

        if (!frontwires.empty()) {
            // build the end faces, sew the shell and build the final solid
            TopoDS_Shape front = Part::FaceMakerCheese::makeFace(frontwires);
            TopoDS_Shape back = Part::FaceMakerCheese::makeFace(backwires);

            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            sewer.Add(front);
            sewer.Add(back);

            for (TopoDS_Shape& s : shells)
                sewer.Add(s);

            sewer.Perform();
            mkSolid.Add(TopoDS::Shell(sewer.SewedShape()));
        }
        else {
            // shells are already closed - add them directly
            for (TopoDS_Shape& s : shells) {
                mkSolid.Add(TopoDS::Shell(s));
            }
        }

        if (!mkSolid.IsDone())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result is not a solid"));

        TopoDS_Shape result = mkSolid.Shape();

        BRepClass3d_SolidClassifier SC(result);
        SC.PerformInfinitePoint(Precision::Confusion());
        if (SC.State() == TopAbs_IN)
            result.Reverse();

        AddSubShape.setValue(result);

        if (base.IsNull()) {

            if (getAddSubType() == FeatureAddSub::Subtractive)
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: There is nothing to subtract"));

            int solidCount = countSolids(result);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result has multiple solids"));
            }
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        if (getAddSubType() == FeatureAddSub::Additive) {

            BRepAlgoAPI_Fuse mkFuse(base, result);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Adding the helix failed"));
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());

            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result is not a solid"));

            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result has multiple solids"));
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if (getAddSubType() == FeatureAddSub::Subtractive) {

            TopoDS_Shape boolOp;

            if (Outside.getValue()) {  // are we subtracting the inside or the outside of the profile.
                BRepAlgoAPI_Common mkCom(result, base);
                if (!mkCom.IsDone())
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Intersecting the helix failed"));
                boolOp = this->getSolid(mkCom.Shape());

            }
            else {
                BRepAlgoAPI_Cut mkCut(base, result);
                if (!mkCut.IsDone())
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Subtracting the helix failed"));
                boolOp = this->getSolid(mkCut.Shape());
            }

            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result is not a solid"));

            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result has multiple solids"));
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        if (std::string(e.GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Could not create face from sketch"));
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

void Helix::updateAxis()
{
    App::DocumentObject* pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string>& subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NoCheck);

    Base.setValue(base.x, base.y, base.z);
    Axis.setValue(dir.x, dir.y, dir.z);
}

TopoDS_Shape Helix::generateHelixPath(double startOffset0)
{
    double turns = Turns.getValue();
    double height = Height.getValue();
    bool leftHanded = LeftHanded.getValue();
    bool reversed = Reversed.getValue();
    double angle = Angle.getValue();
    double growth = Growth.getValue();

    if (fabs(angle) < Precision::Confusion())
        angle = 0.0;

    // get revolve axis
    Base::Vector3d baseVector = Base.getValue();
    gp_Pnt pnt(baseVector.x, baseVector.y, baseVector.z);
    Base::Vector3d axisVector = Axis.getValue();
    gp_Dir dir(axisVector.x, axisVector.y, axisVector.z);

    Base::Vector3d normal = getProfileNormal();
    Base::Vector3d start = axisVector.Cross(normal);  // pointing towards the desired helix start point.

    // if our axis is (nearly) aligned with the profile's normal, we're only interested in the "twist"
    // of the helix. The actual starting point, and thus the radius, isn't important as long as it's
    // somewhere in the profile's plane: an arbitrary vector perpendicular to the normal.
    if (start.IsNull()) {
        auto hopefullyNotParallel = Base::Vector3d(1.0, 2.0, 3.0);
        start = normal.Cross(hopefullyNotParallel);
        if (start.IsNull()) {
            // bad luck
            hopefullyNotParallel = Base::Vector3d(3.0, 2.0, 1.0);
            start = normal.Cross(hopefullyNotParallel);
        }
    }

    gp_Dir dir_start(start.x, start.y, start.z);

    // Find out in what quadrant relative to the axis the profile is located, and the exact position.
    Base::Vector3d profileCenter = getProfileCenterPoint();

    // The factor of 100 below ensures that profile size is small compared to the curvature of the helix.
    // This improves the issue reported in https://forum.freecad.org/viewtopic.php?f=10&t=65048
    double axisOffset = 100.0 * (profileCenter * start - baseVector * start);
    double radius = std::fabs(axisOffset);
    bool turned = axisOffset < 0;
    // since the factor does not only change the radius but also the path position, we must shift its offset back
    // using the square of the factor
    double startOffset = 10000.0 * std::fabs(startOffset0 + profileCenter * axisVector - baseVector * axisVector);

    if (radius < Precision::Confusion()) {
        // in this case ensure that axis is not in the sketch plane
        if (fabs(axisVector * normal) < Precision::Confusion())
            throw Base::ValueError("Error: Result is self intersecting");
        radius = 1000.0; //fallback to radius 1000
    }

    bool growthMode = std::string(Mode.getValueAsString()).find("growth") != std::string::npos;
    double radiusTop;
    if (growthMode)
        radiusTop = radius + turns * growth;
    else
        radiusTop = radius + height * tan(Base::toRadians(angle));

    //build the helix path
    //TopoShape helix = TopoShape().makeLongHelix(pitch, height, radius, angle, leftHanded);
    TopoDS_Shape path = TopoShape().makeSpiralHelix(radius, radiusTop, height, turns, 1, leftHanded);

    /*
     * The helix wire is created with the axis coinciding with z-axis and the start point at (radius, 0, 0)
     * We want to move it so that the axis becomes aligned with "dir" and "pnt", we also want (radius,0,0) to
     * map to the sketch plane.
     */

    gp_Pnt origo(0.0, 0.0, 0.0);
    gp_Dir dir_axis1(0.0, 0.0, 1.0);  // pointing along the helix axis, as created.
    gp_Dir dir_axis2(1.0, 0.0, 0.0);  // pointing towards the helix start point, as created.
    gp_Trsf mov;

    if (abs(startOffset) > 0) {  // translate the helix so that the starting point aligns with the profile
        mov.SetTranslation(startOffset * gp_Vec(dir_axis1));
        TopLoc_Location loc(mov);
        path.Move(loc);
    }

    // because of the radius factor we used above, we must reverse after the
    // startOffset movement (that brings the path back to the desired position)
    if (reversed) {
        mov.SetRotation(gp_Ax1(origo, dir_axis2), M_PI);
        TopLoc_Location loc(mov);
        path.Move(loc);
    }

    if (turned) {  // turn the helix so that the starting point aligns with the profile
        mov.SetRotation(gp_Ax1(origo, dir_axis1), M_PI);
        TopLoc_Location loc(mov);
        path.Move(loc);
    }

    gp_Ax3 sourceCS(origo, dir_axis1, dir_axis2);
    gp_Ax3 targetCS(pnt, dir, dir_start);

    mov.SetTransformation(sourceCS, targetCS);
    TopLoc_Location loc(mov);
    path.Move(loc.Inverted());

    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    path.Move(invObjLoc);

    return path;
}

// this function calculates self intersection safe pitch based on the profile bounding box.
double Helix::safePitch()
{
    Base::Vector3d axisVec = Axis.getValue();
    Base::Vector3d startVec = axisVec.Cross(getProfileNormal()); // pointing towards the helix start point
    HelixMode mode = static_cast<HelixMode>(Mode.getValue());
    double growthValue = Growth.getValue();
    double turnsValue = Turns.getValue();

    // handle case if axis is orthogonal to profile
    // since startVec.IsNull() fails sometimes, for e.g. (0.0, 0.0, -0.0)
    // we take the precision into account
    if (startVec.Length() < Precision::Confusion()) {
        // when not in growth mode any pitch > 0 is safe
        if (mode != HelixMode::height_turns_growth) {
            return Precision::Confusion();
        }
        // if growth is not zero, there will in many cases be intersections
        // when the turn is >= 1, thus return an 'infinite' pitch
        // Note: The resulting helix body is in this case often garbage since
        // the OCC algorithm to create the helix fails.
        // Nevertheless, the result is a valid body so it should be valuable for users
        // to get this correct warning anyway.
        else {
            if (abs(turnsValue) >= 1.0 && abs(growthValue) > 0.0)
                return Precision::Infinite();
        }
    }

    double angle = Angle.getValue() / 180.0 * M_PI;
    gp_Dir direction(axisVec.x, axisVec.y, axisVec.z);
    gp_Dir directionStart(startVec.x, startVec.y, startVec.z);
    TopoDS_Shape sketchshape = getVerifiedFace();
    Bnd_Box boundingBox;
    BRepBndLib::Add(sketchshape, boundingBox);

    // get boundary and dimensions of boundingBox
    double Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;
    boundingBox.Get(Xmin, Ymin, Zmin, Xmax, Ymax, Zmax);
    double X = Xmax - Xmin, Y = Ymax - Ymin, Z = Zmax - Zmin;
    gp_Vec boundingBoxVec(X, Y, Z);

    // Below is an approximation because since we take the bounding box it is
    // impossible to calculate it precisely. For example a circle has as bounding
    // box a square and thus results in a larger pitch than really necessary

    // minimal safe pitch if the angle or growth is 0
    double pitch0 = boundingBoxVec * direction;

    if (mode == HelixMode::height_turns_growth) {
        // if the distance perpendicular to axisVec
        // between two turns is larger than the bounding box size in this direction
        // the minimal necessary pitch is zero
        if (abs(growthValue) > abs(boundingBoxVec * directionStart)) {
            return 0.0;
        }
        else {
            // if less than one turn, every pitch is safe
            // Note: at the moment helices with a growth end with a plane
            // whose normal is the final direction of the helix path.
            // In case this might be changed in future, also 1.0 turn would be safe.
            if (turnsValue < 1.0)
                return 0.0;
            else
                return pitch0;
        }
    }
    else {
        // if the angle is so large that the distance perpendicular to axisVec
        // between two turns is larger than the bounding box size in this direction
        // the pitch can be smaller than pitch0
        if (tan(abs(angle)) * pitch0 > abs(boundingBoxVec * directionStart))
            return abs(boundingBoxVec * directionStart) / tan(abs(angle));
        else
            return pitch0;
    }
}

// this function proposes pitch and height
void Helix::proposeParameters(bool force)
{
    if (force || !HasBeenEdited.getValue()) {
        TopoDS_Shape sketchshape = getVerifiedFace();
        Bnd_Box bb;
        BRepBndLib::Add(sketchshape, bb);
        bb.SetGap(0.0);
        double pitch = 1.1 * sqrt(bb.SquareExtent());

        Pitch.setValue(pitch);
        Height.setValue(pitch * 3.0);
        HasBeenEdited.setValue(true);
    }
}

Base::Vector3d Helix::getProfileCenterPoint()
{
    TopoDS_Shape profileshape;
    profileshape = getVerifiedFace();
    Bnd_Box box;
    BRepBndLib::Add(profileshape, box);
    box.SetGap(0.0);
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    return Base::Vector3d(0.5 * (xmin + xmax), 0.5 * (ymin + ymax), 0.5 * (zmin + zmax));
}

void Helix::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
{
    // property Turns had the App::PropertyFloat and was changed to App::PropertyFloatConstraint
    if (prop == &Turns && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat TurnsProperty;
        // restore the PropertyFloat to be able to set its value
        TurnsProperty.Restore(reader);
        Turns.setValue(TurnsProperty.getValue());
    }
    // property Growth had the App::PropertyLength and was changed to App::PropertyDistance
    else if (prop == &Growth && strcmp(TypeName, "App::PropertyLength") == 0) {
        App::PropertyLength GrowthProperty;
        // restore the PropertyLength to be able to set its value
        GrowthProperty.Restore(reader);
        Growth.setValue(GrowthProperty.getValue());
    }
    else {
        ProfileBased::handleChangedPropertyType(reader, TypeName, prop);
    }
}

void Helix::onChanged(const App::Property* prop)
{
    if (prop == &Mode) {
        // Depending on the mode, the derived properties are set read-only
        auto inputMode = static_cast<HelixMode>(Mode.getValue());
        setReadWriteStatusForMode(inputMode);
    }

    ProfileBased::onChanged(prop);
}

void Helix::setReadWriteStatusForMode(HelixMode inputMode)
{
    switch (inputMode)
    {
    case HelixMode::pitch_height_angle:
        // primary input:
        Pitch.setStatus(App::Property::ReadOnly, false);
        Height.setStatus(App::Property::ReadOnly, false);
        Angle.setStatus(App::Property::ReadOnly, false);
        // derived props:
        Turns.setStatus(App::Property::ReadOnly, true);
        Growth.setStatus(App::Property::ReadOnly, true);
        break;

    case HelixMode::pitch_turns_angle:
        // primary input:
        Pitch.setStatus(App::Property::ReadOnly, false);
        Turns.setStatus(App::Property::ReadOnly, false);
        Angle.setStatus(App::Property::ReadOnly, false);
        // derived props:
        Height.setStatus(App::Property::ReadOnly, true);
        Growth.setStatus(App::Property::ReadOnly, true);
        break;

    case HelixMode::height_turns_angle:
        // primary input:
        Height.setStatus(App::Property::ReadOnly, false);
        Turns.setStatus(App::Property::ReadOnly, false);
        Angle.setStatus(App::Property::ReadOnly, false);
        // derived props:
        Pitch.setStatus(App::Property::ReadOnly, true);
        Growth.setStatus(App::Property::ReadOnly, true);
        break;

    case HelixMode::height_turns_growth:
        // primary input:
        Height.setStatus(App::Property::ReadOnly, false);
        Turns.setStatus(App::Property::ReadOnly, false);
        Growth.setStatus(App::Property::ReadOnly, false);
        // derived props:
        Pitch.setStatus(App::Property::ReadOnly, true);
        Angle.setStatus(App::Property::ReadOnly, true);
        break;

    default:
        Pitch.setStatus(App::Property::ReadOnly, false);
        Height.setStatus(App::Property::ReadOnly, false);
        Turns.setStatus(App::Property::ReadOnly, false);
        Angle.setStatus(App::Property::ReadOnly, false);
        Growth.setStatus(App::Property::ReadOnly, false);
        break;
    }
}

PROPERTY_SOURCE(PartDesign::AdditiveHelix, PartDesign::Helix)
AdditiveHelix::AdditiveHelix() {
    addSubType = Additive;
    Outside.setStatus(App::Property::Hidden, true);
}

PROPERTY_SOURCE(PartDesign::SubtractiveHelix, PartDesign::Helix)
SubtractiveHelix::SubtractiveHelix() {
    addSubType = Subtractive;
    Outside.setStatus(App::Property::Hidden, false);
}
