/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <Precision.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <gp_Pln.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopExp.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <Law_Function.hxx>
# include <Law_Linear.hxx>
# include <Law_S.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <App/Document.h>
#include <Mod/Part/App/FaceMakerCheese.h>

//#include "Body.h"
#include "FeaturePipe.h"


using namespace PartDesign;

const char* Pipe::TypeEnums[] = {"FullPath","UpToFace",NULL};
const char* Pipe::TransitionEnums[] = {"Transformed","Right corner", "Round corner",NULL};
const char* Pipe::ModeEnums[] = {"Standard", "Fixed", "Frenet", "Auxiliary", "Binormal", NULL};
const char* Pipe::TransformEnums[] = {"Constant", "Multisection", "Linear", "S-shape", "Interpolation", NULL};


PROPERTY_SOURCE(PartDesign::Pipe, PartDesign::ProfileBased)

Pipe::Pipe()
{
    ADD_PROPERTY_TYPE(Sections,(0),"Sweep",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Spine,(0),"Sweep",App::Prop_None,"Path to sweep along");
    ADD_PROPERTY_TYPE(SpineTangent,(false),"Sweep",App::Prop_None,"Include tangent edges into path");
    ADD_PROPERTY_TYPE(AuxillerySpine,(0),"Sweep",App::Prop_None,"Secondary path to orient sweep");
    ADD_PROPERTY_TYPE(AuxillerySpineTangent,(false),"Sweep",App::Prop_None,"Include tangent edges into secondary path");
    ADD_PROPERTY_TYPE(AuxilleryCurvelinear, (true), "Sweep", App::Prop_None,"Calculate normal between equidistant points on both spines");
    ADD_PROPERTY_TYPE(Mode,(long(0)),"Sweep",App::Prop_None,"Profile mode");
    ADD_PROPERTY_TYPE(Binormal,(Base::Vector3d()),"Sweep",App::Prop_None,"Binormal vector for corresponding orientation mode");
    ADD_PROPERTY_TYPE(Transition,(long(0)),"Sweep",App::Prop_None,"Transition mode");
    ADD_PROPERTY_TYPE(Transformation,(long(0)),"Sweep",App::Prop_None,"Section transformation mode");
    Mode.setEnums(ModeEnums);
    Transition.setEnums(TransitionEnums);
    Transformation.setEnums(TransformEnums);
}

short Pipe::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Spine.isTouched())
        return 1;
    if (Mode.isTouched())
        return 1;
    if (Transition.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Pipe::execute(void)
{
    std::vector<TopoDS_Wire> wires;
    try {
        wires = getProfileWires();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    TopoDS_Shape sketchshape = getVerifiedFace();
    if (sketchshape.IsNull())
        return new App::DocumentObjectExecReturn("Pipe: No valid sketch or face as first section");
    else {
        //TODO: currently we only allow planar faces. the reason for this is that with other faces in front, we could
        //not use the current simulate approach and build the start and end face from the wires. As the shell
        //begins always at the spine and not the profile, the sketchshape cannot be used directly as front face.
        //We would need a method to translate the front shape to match the shell starting position somehow...
        TopoDS_Face face = TopoDS::Face(sketchshape);
        BRepAdaptor_Surface adapt(face);
        if(adapt.GetType() != GeomAbs_Plane)
            return new App::DocumentObjectExecReturn("Pipe: Only planar faces supported");
    }

    // if the Base property has a valid shape, fuse the pipe into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        base = TopoDS_Shape();
    }

    try {
        //setup the location
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        if(!base.IsNull())
            base.Move(invObjLoc);

        //build the paths
        App::DocumentObject* spine = Spine.getValue();
        if (!(spine && spine->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
            return new App::DocumentObjectExecReturn("No spine linked.");

        std::vector<std::string> subedge = Spine.getSubValues();
        TopoDS_Shape path;
        const Part::TopoShape& shape = static_cast<Part::Feature*>(spine)->Shape.getValue();
        buildPipePath(shape, subedge, path);
        path.Move(invObjLoc);


        // auxiliary
        TopoDS_Shape auxpath;
        if(Mode.getValue()==3) {
            App::DocumentObject* auxspine = AuxillerySpine.getValue();
            if (!(auxspine && auxspine->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
                return new App::DocumentObjectExecReturn("No auxiliary spine linked.");
            std::vector<std::string> auxsubedge = AuxillerySpine.getSubValues();

            const Part::TopoShape& auxshape = static_cast<Part::Feature*>(auxspine)->Shape.getValue();
            buildPipePath(auxshape, auxsubedge, auxpath);
            auxpath.Move(invObjLoc);
        }

        //build up multisections
        auto multisections = Sections.getValues();
        std::vector<std::vector<TopoDS_Wire>> wiresections;
        for(TopoDS_Wire& wire : wires)
            wiresections.push_back(std::vector<TopoDS_Wire>(1, wire));
        //maybe we need a sacling law
        Handle(Law_Function) scalinglaw;

        //see if we shall use multiple sections
        if(Transformation.getValue() == 1) {

            //TODO: we need to order the sections to prevent occ from crahsing, as makepieshell connects
            //the sections in the order of adding

            for(App::DocumentObject* obj : multisections) {
                if(!obj->isDerivedFrom(Part::Feature::getClassTypeId()))
                    return  new App::DocumentObjectExecReturn("All sections need to be part features");

                TopExp_Explorer ex;
                size_t i=0;
                for (ex.Init(static_cast<Part::Feature*>(obj)->Shape.getValue(), TopAbs_WIRE); ex.More(); ex.Next()) {
                    if(i>=wiresections.size())
                        return new App::DocumentObjectExecReturn("Multisections need to have the same amount of inner wires as the base section");
                    wiresections[i].push_back(TopoDS::Wire(ex.Current()));

                    ++i;
                }

                if(i<wiresections.size())
                    return new App::DocumentObjectExecReturn("Multisections need to have the same amount of inner wires as the base section");
            }
        }
        /*//build the law functions instead
        else if(Transformation.getValue() == 2) {
            if(ScalingData.getValues().size()<1)
                return new App::DocumentObjectExecReturn("No valid data given for linear scaling mode");

            Handle(Law_Linear) lin = new Law_Linear();
            lin->Set(0,1,1,ScalingData[0].x);

            scalinglaw = lin;
        }
        else if(Transformation.getValue() == 3) {
            if(ScalingData.getValues().size()<1)
                return new App::DocumentObjectExecReturn("No valid data given for S-shape scaling mode");

            Handle(Law_S) s = new Law_S();
            s->Set(0,1,ScalingData[0].y, 1, ScalingData[0].x, ScalingData[0].z);

            scalinglaw = s;
        }*/

        //build all shells
        std::vector<TopoDS_Shape> shells;
        std::vector<TopoDS_Wire> frontwires, backwires;
        for(std::vector<TopoDS_Wire>& wires : wiresections) {

            BRepOffsetAPI_MakePipeShell mkPS(TopoDS::Wire(path));
            setupAlgorithm(mkPS, auxpath);

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
                return new App::DocumentObjectExecReturn("Pipe could not be built");

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
            return new App::DocumentObjectExecReturn("Result is not a solid");

        TopoDS_Shape result = mkSolid.Shape();
        BRepClass3d_SolidClassifier SC(result);
        SC.PerformInfinitePoint(Precision::Confusion());
        if (SC.State() == TopAbs_IN) {
            result.Reverse();
        }

        //result.Move(invObjLoc);
        AddSubShape.setValue(result);

        if(base.IsNull()) {
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        if(getAddSubType() == FeatureAddSub::Additive) {

            BRepAlgoAPI_Fuse mkFuse(base, result);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Adding the pipe failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Pipe: Result has multiple solids. This is not supported at this time.");
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {

            BRepAlgoAPI_Cut mkCut(base, result);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Subtracting the pipe failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkCut.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Pipe: Result has multiple solids. This is not supported at this time.");
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making the pipe");
    }
}

void Pipe::setupAlgorithm(BRepOffsetAPI_MakePipeShell& mkPipeShell, TopoDS_Shape& auxshape) {

    mkPipeShell.SetTolerance(Precision::Confusion());

    switch(Transition.getValue()) {
        case 0:
            mkPipeShell.SetTransitionMode(BRepBuilderAPI_Transformed);
            break;
        case 1:
            mkPipeShell.SetTransitionMode(BRepBuilderAPI_RightCorner);
            break;
        case 2:
            mkPipeShell.SetTransitionMode(BRepBuilderAPI_RoundCorner);
            break;
    }

    bool auxiliary = false;
    const Base::Vector3d& bVec = Binormal.getValue();
    switch(Mode.getValue()) {
        case 1:
            mkPipeShell.SetMode(gp_Ax2(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0)));
            break;
        case 2:
            mkPipeShell.SetMode(true);
            break;
        case 3:
            auxiliary = true;
            break;
        case 4:
            mkPipeShell.SetMode(gp_Dir(bVec.x,bVec.y,bVec.z));
            break;
    }

    if(auxiliary) {
        mkPipeShell.SetMode(TopoDS::Wire(auxshape), AuxilleryCurvelinear.getValue());
        //mkPipeShell.SetMode(TopoDS::Wire(auxshape), AuxilleryCurvelinear.getValue(), BRepFill_ContactOnBorder);
    }
}


void Pipe::getContiniusEdges(Part::TopoShape /*TopShape*/, std::vector< std::string >& /*SubNames*/) {

    /*
    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeEdge;
    TopExp::MapShapesAndAncestors(TopShape.getShape(), TopAbs_EDGE, TopAbs_EDGE, mapEdgeEdge);
    TopExp::MapShapes(TopShape.getShape(), TopAbs_EDGE, mapOfEdges);

    Base::Console().Message("Initial edges:\n");
    for(int i=0; i<SubNames.size(); ++i)
        Base::Console().Message("Subname: %s\n", SubNames[i].c_str());

    unsigned int i = 0;
    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.size() > 4 && aSubName.substr(0,4) == "Edge") {
            TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(aSubName.c_str()));
            const TopTools_ListOfShape& los = mapEdgeEdge.FindFromKey(edge);

            if(los.Extent() != 2)
            {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            const TopoDS_Shape& face1 = los.First();
            const TopoDS_Shape& face2 = los.Last();
            GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                       TopoDS::Face(face1),
                                                       TopoDS::Face(face2));
            if (cont != GeomAbs_C0) {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            i++;
        }
        // empty name or any other sub-element
        else {
            SubNames.erase(SubNames.begin()+i);
        }
    }

    Base::Console().Message("Final edges:\n");
    for(int i=0; i<SubNames.size(); ++i)
        Base::Console().Message("Subname: %s\n", SubNames[i].c_str());
    */
}

void Pipe::buildPipePath(const Part::TopoShape& shape, const std::vector< std::string >& subedge, TopoDS_Shape& path) {

    if (!shape.getShape().IsNull()) {
        try {
            if (!subedge.empty()) {
                //if(SpineTangent.getValue())
                    //getContiniusEdges(shape, subedge);

                BRepBuilderAPI_MakeWire mkWire;
                for (std::vector<std::string>::const_iterator it = subedge.begin(); it != subedge.end(); ++it) {
                    TopoDS_Shape subshape = shape.getSubShape(it->c_str());
                    mkWire.Add(TopoDS::Edge(subshape));
                }
                path = mkWire.Wire();
            }
            else if (shape.getShape().ShapeType() == TopAbs_EDGE) {
                path = shape.getShape();
            }
            else if (shape.getShape().ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape.getShape()));
                path = mkWire.Wire();
            }
            else if (shape.getShape().ShapeType() == TopAbs_COMPOUND) {
                TopoDS_Iterator it(shape.getShape());
                for (; it.More(); it.Next()) {
                    if (it.Value().IsNull())
                        throw Base::ValueError("In valid element in spine.");
                    if ((it.Value().ShapeType() != TopAbs_EDGE) &&
                        (it.Value().ShapeType() != TopAbs_WIRE)) {
                        throw Base::TypeError("Element in spine is neither an edge nor a wire.");
                    }
                }

                Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
                Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
                for (TopExp_Explorer xp(shape.getShape(), TopAbs_EDGE); xp.More(); xp.Next())
                    hEdges->Append(xp.Current());

                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, Precision::Confusion(), Standard_True, hWires);
                int len = hWires->Length();
                if (len != 1)
                    throw Base::ValueError("Spine is not connected.");
                path = hWires->Value(1);
            }
            else {
                throw Base::TypeError("Spine is neither an edge nor a wire.");
            }
        }
        catch (Standard_Failure&) {
            throw Base::CADKernelError("Invalid spine.");
        }
    }
}



PROPERTY_SOURCE(PartDesign::AdditivePipe, PartDesign::Pipe)
AdditivePipe::AdditivePipe() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractivePipe, PartDesign::Pipe)
SubtractivePipe::SubtractivePipe() {
    addSubType = Subtractive;
}
