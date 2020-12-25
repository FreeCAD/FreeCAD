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

    TopoDS_Shape sketchshape;
    try {
        sketchshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    if (sketchshape.IsNull())
        return new App::DocumentObjectExecReturn("Error: No valid sketch or face");
    else {
        //TODO: currently we only allow planar faces. the reason for this is that with other faces in front, we could
        //not use the current simulate approach and build the start and end face from the wires. As the shell
        //begins always at the spine and not the profile, the sketchshape cannot be used directly as front face.
        //We would need a method to translate the front shape to match the shell starting position somehow...
        TopoDS_Face face = TopoDS::Face(sketchshape);
        BRepAdaptor_Surface adapt(face);
        if(adapt.GetType() != GeomAbs_Plane)
            return new App::DocumentObjectExecReturn("Error: Face must be planar");
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
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);

        // generate the helix path
        TopoDS_Shape path = generateHelixPath();

        // Below is basically a copy paste (with some simplification) from  FeaturePipe.cpp Pipe::execute
        // TODO: find a way to reduce code repetition. E.g can I rip out this functionality of Pipe:execute to a static helper
        // function and call from here?

        std::vector<TopoDS_Wire> wires;
        try {
            wires = getProfileWires();
        } catch (const Base::Exception& e) {
            return new App::DocumentObjectExecReturn(e.what());
        }

        std::vector<std::vector<TopoDS_Wire>> wiresections;
        for(TopoDS_Wire& wire : wires)
            wiresections.emplace_back(1, wire);

        //maybe we need a scaling law
        Handle(Law_Function) scalinglaw;

        //build all shells
        std::vector<TopoDS_Shape> shells;
        std::vector<TopoDS_Wire> frontwires, backwires;
        for(std::vector<TopoDS_Wire>& wires : wiresections) {

            BRepOffsetAPI_MakePipeShell mkPS(TopoDS::Wire(path));

            mkPS.SetTolerance(Precision::Confusion());
            mkPS.SetTransitionMode(BRepBuilderAPI_Transformed);

            mkPS.SetMode(true);  //This is for frenet
            //mkPipeShell.SetMode(TopoDS::Wire(auxpath), true);  // this is for two rails


            if(!scalinglaw) {
                for(TopoDS_Wire& wire : wires) {
                    wire.Move(invObjLoc);
                    mkPS.Add(wire);
                }
            }
            else {
                for(TopoDS_Wire& wire : wires)  {
                    wire.Move(invObjLoc);
                    mkPS.SetLaw(wire, scalinglaw);
                }
            }

            if (!mkPS.IsReady())
                return new App::DocumentObjectExecReturn("Error: Could not build");

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
            TopoDS_Shape back  = Part::FaceMakerCheese::makeFace(backwires);

            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            sewer.Add(front);
            sewer.Add(back);

            for(TopoDS_Shape& s : shells)
                sewer.Add(s);

            sewer.Perform();
            mkSolid.Add(TopoDS::Shell(sewer.SewedShape()));
        } else {
            // shells are already closed - add them directly
            for (TopoDS_Shape& s : shells) {
                mkSolid.Add(TopoDS::Shell(s));
            }
        }

        if(!mkSolid.IsDone())
            return new App::DocumentObjectExecReturn("Error: Result is not a solid");

        TopoDS_Shape result = mkSolid.Shape();
        BRepClass3d_SolidClassifier SC(result);
        SC.PerformInfinitePoint(Precision::Confusion());
        if (SC.State() == TopAbs_IN)
            result.Reverse();

        AddSubShape.setValue(result);


        if(base.IsNull()) {

            if (getAddSubType() == FeatureAddSub::Subtractive)
                return new App::DocumentObjectExecReturn("Error: There is nothing to subtract\n");

            int solidCount = countSolids(result);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Error: Result has multiple solids");
            }
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        if(getAddSubType() == FeatureAddSub::Additive) {

            BRepAlgoAPI_Fuse mkFuse(base, result);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Error: Adding the helix failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Error: Result is not a solid");

            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Error: Result has multiple solids");
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {

            TopoDS_Shape boolOp;

            if (Outside.getValue()) {  // are we subtracting the inside or the outside of the profile.
                BRepAlgoAPI_Common mkCom(result, base);
                if (!mkCom.IsDone())
                    return new App::DocumentObjectExecReturn("Error: Intersecting the helix failed");
                boolOp = this->getSolid(mkCom.Shape());

            } else {
                BRepAlgoAPI_Cut mkCut(base, result);
                if (!mkCut.IsDone())
                    return new App::DocumentObjectExecReturn("Error: Subtracting the helix failed");
                boolOp = this->getSolid(mkCut.Shape());
            }

            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Error: Result is not a solid");

            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Error: Result has multiple solids");
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }

        return App::DocumentObject::StdReturn;
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

    // Determine radius as the minimum distance between sketchshape and axis.
    // also find out in what quadrant relative to the axis the profile is located.
    double radius = 1e99;
    bool turned = false;
    double startOffset = 1e99;
    TopoDS_Shape sketchshape = getVerifiedFace();
    BRepBuilderAPI_MakeEdge axisEdge(gp_Lin(pnt, dir));
    BRepBuilderAPI_MakeEdge startEdge(gp_Lin(pnt, dir_start));
    for (TopExp_Explorer xp(sketchshape, TopAbs_FACE); xp.More(); xp.Next()) {
        const TopoDS_Face face =  TopoDS::Face(xp.Current());
        TopoDS_Wire wire = ShapeAnalysis::OuterWire(face);
        BRepExtrema_DistShapeShape distR(wire, axisEdge.Shape(), Precision::Confusion());
        if (distR.IsDone()) {
            if (distR.Value() < radius) {
                radius = distR.Value();
                const gp_Pnt p1 = distR.PointOnShape1(1);
                const gp_Pnt p2 = distR.PointOnShape2(1);
                double offsetProfile = p1.X()*dir_start.X() + p1.Y()*dir_start.Y() + p1.Z()*dir_start.Z();
                double offsetAxis = p2.X()*dir_start.X() + p2.Y()*dir_start.Y() + p2.Z()*dir_start.Z();
                turned = (offsetProfile < offsetAxis);
            }
        }
        BRepExtrema_DistShapeShape distStart(wire, startEdge.Shape(), Precision::Confusion());
        if (distStart.IsDone()) {
            if (distStart.Value() < abs(startOffset)) {
                const gp_Pnt p1 = distStart.PointOnShape1(1);
                const gp_Pnt p2 = distStart.PointOnShape2(1);
                double offsetProfile = p1.X()*dir.X() + p1.Y()*dir.Y() + p1.Z()*dir.Z();
                double offsetAxis = p2.X()*dir.X() + p2.Y()*dir.Y() + p2.Z()*dir.Z();
                startOffset = offsetProfile - offsetAxis;
            }
        }

    }

    if (radius <  Precision::Confusion()) {
        // in this case ensure that axis is not in the sketch plane
        if (v*normal < Precision::Confusion())
            throw Base::ValueError("Error: Result is self intersecting");
        radius = 1.0; //fallback to radius 1
        startOffset = 0.0;
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


# if OCC_VERSION_HEX < 0x70500
    /* I initially tried using path.Move(invObjLoc) like usual. But it does not give the right result
     * The starting point of the helix is not correct and I don't know why! With below hack it works.
     */
    Base::Vector3d placeAxis;
    double placeAngle;
    this->Placement.getValue().getRotation().getValue(placeAxis, placeAngle);
    gp_Dir placeDir(placeAxis.x, placeAxis.y, placeAxis.z);
    mov.SetRotation(gp_Ax1(origo, placeDir), placeAngle);
    TopLoc_Location loc2(mov);
    path.Move(loc2.Inverted());
# else
    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    path.Move(invObjLoc);
# endif

    return path;
}

// this function calculates self intersection safe pitch based on the profile bounding box.
double Helix::safePitch()
{
    // Below is an approximation. It is possible to do the general way by solving for the pitch
    // where the helix is self intersecting.

    double angle = Angle.getValue()/180.0*PI;

    TopoDS_Shape sketchshape = getVerifiedFace();
    Bnd_Box bb;
    BRepBndLib::Add(sketchshape, bb);

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
        double pitch = 1.1*safePitch();
        Pitch.setValue(pitch);
        Height.setValue(pitch*3.0);
        HasBeenEdited.setValue(1);
    }
}


PROPERTY_SOURCE(PartDesign::AdditiveHelix, PartDesign::Helix)
AdditiveHelix::AdditiveHelix() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractiveHelix, PartDesign::Helix)
SubtractiveHelix::SubtractiveHelix() {
    addSubType = Subtractive;
}
