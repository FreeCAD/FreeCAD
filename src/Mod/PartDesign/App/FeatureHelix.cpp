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
# include <BRep_Builder.hxx>
# include <BRepBndLib.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <Precision.hxx>
# include <gp_Lin.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Law_Function.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <ShapeAnalysis.hxx>
# include <gp_Ax1.hxx>
# include <gp_Ax3.hxx>
#endif

# include <Standard_Version.hxx>
# include <Base/Axis.h>
# include <Base/Console.h>
# include <Base/Exception.h>
# include <Base/Placement.h>
# include <Base/Tools.h>

# include <Mod/Part/App/TopoShape.h>
# include <Mod/Part/App/FaceMakerCheese.h>

# include "FeaturePipe.h"
# include "FeatureHelix.h"

const double PI = 3.14159265359;

using namespace PartDesign;

const char* Helix::ModeEnums[] = {"pitch-height", "pitch-turns", "height-turns", NULL};

PROPERTY_SOURCE(PartDesign::Helix, PartDesign::ProfileBased)

Helix::Helix()
{
    addSubType = FeatureAddSub::Additive;

    ADD_PROPERTY_TYPE(Base,(Base::Vector3d(0.0,0.0,0.0)),"Helix", App::Prop_ReadOnly, "Base");
    ADD_PROPERTY_TYPE(Axis,(Base::Vector3d(0.0,1.0,0.0)),"Helix", App::Prop_ReadOnly, "Axis");
    ADD_PROPERTY_TYPE(Pitch,(10.),"Helix", App::Prop_None, "Pitch");
    ADD_PROPERTY_TYPE(Height,(30.0),"Helix", App::Prop_None, "Height");
    ADD_PROPERTY_TYPE(Turns,(3.0),"Helix", App::Prop_None, "Turns");
    ADD_PROPERTY_TYPE(LeftHanded,(long(0)),"Helix", App::Prop_None, "LeftHanded");
    ADD_PROPERTY_TYPE(Reversed,(long(0)),"Helix", App::Prop_None, "Reversed");
    ADD_PROPERTY_TYPE(Angle,(0.0),"Helix", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(ReferenceAxis,(0),"Helix", App::Prop_None, "Reference axis of revolution");
    ADD_PROPERTY_TYPE(Mode, (long(0)), "Helix", App::Prop_None, "Helix input mode");
    ADD_PROPERTY_TYPE(Outside,(long(0)),"Helix", App::Prop_None, "Outside");
    ADD_PROPERTY_TYPE(HasBeenEdited,(long(0)),"Helix", App::Prop_None, "HasBeenEdited");
    Mode.setEnums(ModeEnums);

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

App::DocumentObjectExecReturn *Helix::execute(void)
{
     // Validate and normalize parameters
    switch (Mode.getValue()) {
        case 0:  // pitch - height
            if (Pitch.getValue() < Precision::Confusion())
                return new App::DocumentObjectExecReturn("Error: Pitch too small");
            if (Height.getValue() < Precision::Confusion())
                return new App::DocumentObjectExecReturn("Error: height too small!");
            break;
        case 1: // pitch - turns
            if (Pitch.getValue() < Precision::Confusion())
                return new App::DocumentObjectExecReturn("Error: pitch too small!");
            if (Turns.getValue() < Precision::Confusion())
                return new App::DocumentObjectExecReturn("Error: turns too small!");
            Height.setValue(Turns.getValue()*Pitch.getValue());
            break;
        case 2: // height - turns
            if (Height.getValue() < Precision::Confusion())
                return new App::DocumentObjectExecReturn("Error: height too small!");
            if (Turns.getValue() < Precision::Confusion())
                return new App::DocumentObjectExecReturn("Error turns too small!");
            Pitch.setValue(Height.getValue()/Turns.getValue());
            break;
        default:
            return new App::DocumentObjectExecReturn("Error: unsupported mode");
    }

    // update Axis from ReferenceAxis
    try {
        positionByPrevious();
        updateAxis();
        return Pipe::_execute(this, TopoShape(generateHelixPath()));
    }
    catch (Standard_Failure& e) {
        if (std::string(e.GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn("Error: Could not create face from sketch");
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

void Helix::updateAxis(void)
{
    App::DocumentObject *pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir, false);

    Base.setValue(base.x,base.y,base.z);
    Axis.setValue(dir.x,dir.y,dir.z);
}


TopoDS_Shape Helix::generateHelixPath(void)
{
    double pitch = Pitch.getValue();
    double height = Height.getValue();
    bool leftHanded = LeftHanded.getValue();
    bool reversed = Reversed.getValue();
    double angle = Angle.getValue();
    if (angle < Precision::Confusion() && angle > -Precision::Confusion())
        angle = 0.0;

    // get revolve axis
    Base::Vector3d b = Base.getValue();
    gp_Pnt pnt(b.x,b.y,b.z);
    Base::Vector3d v = Axis.getValue();
    gp_Dir dir(v.x,v.y,v.z);

    Base::Vector3d normal = getProfileNormal();
    Base::Vector3d start = v.Cross(normal);  // pointing towards the desired helix start point.
    gp_Dir dir_start(start.x, start.y, start.z);

    // Find out in what quadrant relative to the axis the profile is located, and the exact position.
    Base::Vector3d profileCenter = getProfileCenterPoint();
    double axisOffset = profileCenter*start - b*start;
    double startOffset = profileCenter*v - b*v;
    double radius = std::fabs(axisOffset);
    bool turned = axisOffset < 0;

    if (radius <  Precision::Confusion()) {
        // in this case ensure that axis is not in the sketch plane
        if (std::fabs(v*normal) < Precision::Confusion())
            throw Base::ValueError("Error: Result is self intersecting");
        radius = 1.0; //fallback to radius 1
    }

    //build the helix path
    TopoShape helix = TopoShape().makeLongHelix(pitch, height, radius, angle, leftHanded);
    TopoDS_Shape path = helix.getShape();


    /*
     * The helix wire is created with the axis coinciding with z-axis and the start point at (radius, 0, 0)
     * We want to move it so that the axis becomes aligned with "dir" and "pnt", we also want (radius,0,0) to
     * map to the sketch plane.
     */

    gp_Pnt origo(0.0, 0.0, 0.0);
    gp_Dir dir_axis1(0.0, 0.0, 1.0);  // pointing along the helix axis, as created.
    gp_Dir dir_axis2(1.0, 0.0, 0.0);  // pointing towards the helix start point, as created.

    gp_Trsf mov;


    if (reversed) {
        mov.SetRotation(gp_Ax1(origo, dir_axis2), PI);
        TopLoc_Location loc(mov);
        path.Move(loc);
    }

    if (abs(startOffset) > 0) {  // translate the helix so that the starting point aligns with the profile
        mov.SetTranslation(startOffset*gp_Vec(dir_axis1));
        TopLoc_Location loc(mov);
        path.Move(loc);
    }

    if (turned) {  // turn the helix so that the starting point aligns with the profile
        mov.SetRotation(gp_Ax1(origo, dir_axis1), PI);
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
    // Below is an approximation. It is possible to do the general way by solving for the pitch
    // where the helix is self intersecting.

    double angle = Angle.getValue()/180.0*PI;

    auto sketchshape = getVerifiedFace();
    Bnd_Box bb;
    BRepBndLib::Add(sketchshape.getShape(), bb);

    double Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;
    bb.Get(Xmin, Ymin, Zmin, Xmax, Ymax, Zmax);

    double X = Xmax - Xmin, Y = Ymax - Ymin, Z = Zmax - Zmin;

    Base::Vector3d v = Axis.getValue();
    gp_Dir dir(v.x,v.y,v.z);
    gp_Vec bbvec(X, Y, Z);

    double p0 = bbvec*dir; // safe pitch if angle=0

    Base::Vector3d n = getProfileNormal();
    Base::Vector3d s = v.Cross(n);  // pointing towards the desired helix start point.
    gp_Dir dir_s(s.x, s.y, s.z);

    if (tan(abs(angle))*p0 > abs(bbvec*dir_s))
        return abs(bbvec*dir_s)/tan(abs(angle));
    else
        return p0;
}

// this function proposes pitch and height
void Helix::proposeParameters(bool force)
{
    if (force || !HasBeenEdited.getValue()) {
        auto sketchshape = getVerifiedFace();
        Bnd_Box bb;
        BRepBndLib::Add(sketchshape.getShape(), bb);
        bb.SetGap(0.0);
        double pitch = 1.1 * sqrt(bb.SquareExtent());

        Pitch.setValue(pitch);
        Height.setValue(pitch*3.0);
        HasBeenEdited.setValue(1);
    }
}

Base::Vector3d Helix::getProfileCenterPoint()
{
    auto profileshape = getVerifiedFace();
    Bnd_Box box;
    BRepBndLib::Add(profileshape.getShape(), box);
    box.SetGap(0.0);
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    return Base::Vector3d(0.5*(xmin+xmax), 0.5*(ymin+ymax), 0.5*(zmin+zmax));
}


PROPERTY_SOURCE(PartDesign::AdditiveHelix, PartDesign::Helix)
AdditiveHelix::AdditiveHelix() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractiveHelix, PartDesign::Helix)
SubtractiveHelix::SubtractiveHelix() {
    addSubType = Subtractive;
}
