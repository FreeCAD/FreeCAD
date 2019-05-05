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

FC_LOG_LEVEL_INIT("PartDesign",true,true);

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
    std::vector<TopoShape> wires;
    try {
        wires = getProfileWires();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    
    TopoShape sketchshape = getVerifiedFace();
    if (sketchshape.isNull())
        return new App::DocumentObjectExecReturn("Pipe: No valid sketch or face as first section");
    else {
        //TODO: currently we only allow planar faces. the reason for this is that with other faces in front, we could
        //not use the current simulate approach and build the start and end face from the wires. As the shell
        //begins always at the spine and not the profile, the sketchshape cannot be used directly as front face.
        //We would need a method to translate the front shape to match the shell starting position somehow...
        TopoDS_Face face = TopoDS::Face(sketchshape.getShape());
        BRepAdaptor_Surface adapt(face);
        if(adapt.GetType() != GeomAbs_Plane)
            return new App::DocumentObjectExecReturn("Pipe: Only planar faces supported");
    }

    // if the Base property has a valid shape, fuse the pipe into it
    TopoShape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
    }

    try {
        //setup the location
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        auto invTrsf = invObjLoc.Transformation();
        if(!base.isNull())
            base.move(invObjLoc);
        
        //build the paths
        auto path = buildPipePath(Spine,invTrsf);
        if(path.isNull())
            return new App::DocumentObjectExecReturn("Invalid spine.");

        // auxiliary
        TopoShape auxpath;
        if(Mode.getValue()==3) {
            auxpath = buildPipePath(AuxillerySpine,invTrsf);
            if(auxpath.isNull())
                return new App::DocumentObjectExecReturn("invalid auxiliary spine.");
        }

        //build up multisections
        auto multisections = Sections.getValues();
        std::vector<std::vector<TopoShape>> wiresections;
        wiresections.reserve(wires.size());
        for(TopoShape& wire : wires)
            wiresections.emplace_back(1, wire);
        //maybe we need a sacling law
        Handle(Law_Function) scalinglaw;

        //see if we shall use multiple sections
        if(Transformation.getValue() == 1) {

            //TODO: we need to order the sections to prevent occ from crahsing, as makepieshell connects
            //the sections in the order of adding

            for(App::DocumentObject* obj : multisections) {
                auto shape = getTopoShape(obj);
                if(shape.countSubShapes(TopAbs_WIRE) != wiresections.size())
                    return new App::DocumentObjectExecReturn("Multisections need to have the same amount of inner wires as the base section");
                int i=0;
                for(auto &wire : shape.getSubTopoShapes(TopAbs_WIRE))
                    wiresections[i++].push_back(wire);
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
        std::vector<TopoShape> shells;
        std::vector<TopoShape> frontwires, backwires;
        for(auto& wires : wiresections) {
            
            BRepOffsetAPI_MakePipeShell mkPS(TopoDS::Wire(path.getShape()));
            setupAlgorithm(mkPS, auxpath);

            if(!scalinglaw) {
                for(TopoShape& wire : wires) {
                    wire.move(invObjLoc);
                    mkPS.Add(TopoDS::Wire(wire.getShape()));
                }
            }
            else {
                for(TopoShape& wire : wires)  {
                    wire.move(invObjLoc);
                    mkPS.SetLaw(TopoDS::Wire(wire.getShape()), scalinglaw);
                }
            }

            if (!mkPS.IsReady())
                return new App::DocumentObjectExecReturn("Pipe could not be built");

            TopoShape shell(0,getDocument()->getStringHasher());
            shell.makEShape(mkPS,wires);
            shells.push_back(shell);
            
            if (!mkPS.Shape().Closed()) {
                // shell is not closed - use simulate to get the end wires
                TopTools_ListOfShape sim;
                mkPS.Simulate(2, sim);

                TopoShape front(sim.First());
                if(front.countSubShapes(TopAbs_EDGE)==wires.front().countSubShapes(TopAbs_EDGE)) {
                    front = wires.front();
                    front.setShape(sim.First(),false);
                }else
                    front.Tag = -wires.front().Tag;
                TopoShape back(sim.Last());
                if(back.countSubShapes(TopAbs_EDGE)==wires.back().countSubShapes(TopAbs_EDGE)) {
                    back = wires.back();
                    back.setShape(sim.Last(),false);
                }else
                    back.Tag = -wires.back().Tag;
                    
                frontwires.push_back(front);
                backwires.push_back(back);
            }
        }

        TopoShape result(0,getDocument()->getStringHasher());

        if (!frontwires.empty()) {
            // build the end faces, sew the shell and build the final solid
            auto front = TopoShape().makEFace(frontwires,0,"Part::FaceMakerCheese");
            auto back = TopoShape().makEFace(backwires,0,"Part::FaceMakerCheese");

            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            sewer.Add(front.getShape());
            sewer.Add(back.getShape());

            for(auto& s : shells)
                sewer.Add(s.getShape());

            shells.push_back(front);
            shells.push_back(back);

            sewer.Perform();
            result = result.makEShape(sewer,shells).makESolid();
        } else {
            // shells are already closed - add them directly
            result.makESolid(shells);
        }

        BRepClass3d_SolidClassifier SC(result.getShape());
        SC.PerformInfinitePoint(Precision::Confusion());
        if (SC.State() == TopAbs_IN) {
            result.setShape(result.getShape().Reversed(),false);
        }

        //result.Move(invObjLoc);
        AddSubShape.setValue(result);

        if(base.isNull()) {
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        TopoShape boolOp(0,getDocument()->getStringHasher());

        if(getAddSubType() == FeatureAddSub::Additive) {
            try {
                boolOp.makEFuse({base,result});
            }catch(Standard_Failure&) {
                return new App::DocumentObjectExecReturn("Adding the pipe failed");
            }
            boolOp = this->getSolid(boolOp);
            // lets check if the result is a solid
            if (boolOp.isNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {
            try {
                boolOp.makECut({base,result});
            }catch(Standard_Failure&) {
                return new App::DocumentObjectExecReturn("Subtracting the pipe failed");
            }
            // we have to get the solids (fuse sometimes creates compounds)
            boolOp = this->getSolid(boolOp);
            // lets check if the result is a solid
            if (boolOp.isNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

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

void Pipe::setupAlgorithm(BRepOffsetAPI_MakePipeShell& mkPipeShell, TopoShape& auxshape) {

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
        mkPipeShell.SetMode(TopoDS::Wire(auxshape.getShape()), AuxilleryCurvelinear.getValue());
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

TopoShape Pipe::buildPipePath(const App::PropertyLinkSub &link, const gp_Trsf &trsf) {
    TopoShape result(0,getDocument()->getStringHasher());
    auto obj = link.getValue();
    if(!obj) return result;
    std::vector<TopoShape> shapes;
    const auto &subs = link.getSubValues(true);
    if(subs.empty()) {
        shapes.push_back(getTopoShape(obj));
        if(shapes.back().isNull())
            return result;
    }else{
        for(auto &sub : link.getSubValues(true)) {
            shapes.push_back(getTopoShape(obj,sub.c_str(),true));
            if(shapes.back().isNull()) 
                return result;
        }
    }
    result.makEWires(shapes);
    if(result.countSubShapes(TopAbs_WIRE)>1)
        FC_WARN("Sweep path contain more than one wire");
    return result.getSubTopoShape(TopAbs_WIRE,1).makETransform(trsf);
}


PROPERTY_SOURCE(PartDesign::AdditivePipe, PartDesign::Pipe)
AdditivePipe::AdditivePipe() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractivePipe, PartDesign::Pipe)
SubtractivePipe::SubtractivePipe() {
    addSubType = Subtractive;
}
