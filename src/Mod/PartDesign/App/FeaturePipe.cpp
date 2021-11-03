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
#include <Mod/Part/App/TopoShapeOpCode.h>

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
    Transition.setValue(1);
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

App::DocumentObjectExecReturn *Pipe::execute()
{
    TopoShape path, auxpath;
    try {
        positionByPrevious();
        auto invTrsf = getLocation().Inverted().Transformation();

        //build the paths
        path = buildPipePath(Spine,invTrsf);
        if(path.isNull())
            return new App::DocumentObjectExecReturn("Invalid spine");

        // auxiliary
        if(Mode.getValue()==3) {
            auxpath = buildPipePath(AuxillerySpine,invTrsf);
            if(auxpath.isNull())
                return new App::DocumentObjectExecReturn("invalid auxiliary spine");
        }
    } catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    } catch (const Base::Exception & e) {
        e.ReportException();
        return new App::DocumentObjectExecReturn(e.what());
    } catch (...) {
        return new App::DocumentObjectExecReturn("Unknown error");
    }

    return _execute(this,
                    path,
                    Transition.getValue(),
                    auxpath,
                    AuxilleryCurvelinear.getValue(),
                    Mode.getValue(),
                    Binormal.getValue(),
                    Transformation.getValue(),
                    Sections.getValues());
}

App::DocumentObjectExecReturn *Pipe::_execute(ProfileBased *feat,
                                              const TopoShape &path,
                                              int transition,
                                              const TopoShape &auxpath,
                                              bool auxCurveLinear,
                                              int mode,
                                              const Base::Vector3d &binormalVector,
                                              int transformation,
                                              const std::vector<App::DocumentObject*> &multisections)
{
    // if the Base property has a valid shape, fuse the pipe into it
    TopoShape base;
    try {
        base = feat->getBaseShape();
    } catch (const Base::Exception&) {
    }

    // If base is null, it means we are creating a new shape, and we shall
    // allow non closed wire to create face from sweeping wire.
    TopoShape sketchshape = feat->getVerifiedFace(false, true, base.isNull());
    if (sketchshape.isNull())
        return new App::DocumentObjectExecReturn("No valid sketch or face as section");
    auto wires = sketchshape.getSubTopoShapes(TopAbs_WIRE);
    bool closed = sketchshape.shapeType(true) == TopAbs_FACE;

    try {
        //setup the location
        TopLoc_Location invObjLoc = feat->getLocation().Inverted();
        if(!base.isNull())
            base.move(invObjLoc);

        //build up multisections
        std::vector<std::vector<TopoShape>> wiresections;
        wiresections.reserve(wires.size());
        for(TopoShape& wire : wires)
            wiresections.emplace_back(1, wire);
        //maybe we need a sacling law
        Handle(Law_Function) scalinglaw;

        TopoShape frontface = sketchshape;
        TopoShape backface = frontface;

        //see if we shall use multiple sections
        if(transformation == 1) {

            //TODO: we need to order the sections to prevent occ from crahsing, as makepieshell connects
            //the sections in the order of adding

            for(App::DocumentObject* obj : multisections) {
                backface = feat->getVerifiedFace(false, true, base.isNull(), obj);
                if(backface.countSubShapes(TopAbs_WIRE) != wiresections.size())
                    return new App::DocumentObjectExecReturn(
                            "Multisections need to have the same amount of inner wires as the base section");

                int i=0;
                for(auto &wire : backface.getSubTopoShapes(TopAbs_WIRE))
                    wiresections[i++].push_back(wire);
            }
        }
        /*//build the law functions instead
        else if(transformation == 2) {
            if(ScalingData.getValues().size()<1)
                return new App::DocumentObjectExecReturn("No valid data given for linear scaling mode");

            Handle(Law_Linear) lin = new Law_Linear();
            lin->Set(0,1,1,ScalingData[0].x);

            scalinglaw = lin;
        }
        else if(transformation == 3) {
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
            setupAlgorithm(mkPS, mode, binormalVector, transition, auxpath, auxCurveLinear);

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
                return new App::DocumentObjectExecReturn("Shape could not be built");

            TopoShape shell(0, feat->getDocument()->getStringHasher());
            shell.makEShape(mkPS,wires);
            shells.push_back(shell);

            if (closed && !mkPS.Shape().Closed()) {
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

        TopoShape result(0,feat->getDocument()->getStringHasher());

        if (!frontwires.empty()) {
            if (frontface.shapeType(true) == TopAbs_FACE && !frontface.isPlanarFace())
                frontface.makEBSplineFace(TopoShape().makECompound(frontwires, nullptr, false));
            else
                frontface = TopoShape().makEFace(frontwires);

            if (backface.shapeType(true) == TopAbs_FACE && !backface.isPlanarFace())
                backface.makEBSplineFace(TopoShape().makECompound(backwires, nullptr, false));
            else
                backface = TopoShape().makEFace(backwires);


            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            sewer.Add(frontface.getShape());
            sewer.Add(backface.getShape());

            for(auto& s : shells)
                sewer.Add(s.getShape());

            shells.push_back(frontface);
            shells.push_back(backface);

            sewer.Perform();
            result = result.makEShape(sewer,shells).makESolid();
        } else {
            // shells are already closed - add them directly
            result.makESolid(shells);
        }

        if (closed) {
            BRepClass3d_SolidClassifier SC(result.getShape());
            SC.PerformInfinitePoint(Precision::Confusion());
            if (SC.State() == TopAbs_IN) {
                result.setShape(result.getShape().Reversed(),false);
            }
            if (feat->Linearize.getValue())
                result.linearize(true, false);
        }

        //result.Move(invObjLoc);
        feat->AddSubShape.setValue(result);
        if (feat->isRecomputePaused())
            return App::DocumentObject::StdReturn;

        if(base.isNull()) {
            result = feat->refineShapeIfActive(result);
            feat->Shape.setValue(closed ? feat->getSolid(result) : result);
            return App::DocumentObject::StdReturn;
        }

        result.Tag = -feat->getID();

        TopoShape boolOp(0,feat->getDocument()->getStringHasher());

        const char *maker;
        switch(feat->getAddSubType()) {
        case Additive:
            maker = TOPOP_FUSE;
            break;
        case Subtractive:
            maker = TOPOP_CUT;
            break;
        case Intersecting:
            maker = TOPOP_COMMON;
            break;
        default:
            return new App::DocumentObjectExecReturn("Unknown operation type");
        }
        try {
            boolOp.makEShape(maker, {base,result});
        }catch(Standard_Failure &e) {
            FC_ERR(feat->getFullName() << ": " << e.GetMessageString());
            return new App::DocumentObjectExecReturn("Failed to perform boolean operation");
        }
        boolOp = feat->getSolid(boolOp);
        // lets check if the result is a solid
        if (boolOp.isNull())
            return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

        boolOp = feat->refineShapeIfActive(boolOp);
        feat->Shape.setValue(feat->getSolid(boolOp));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    } catch (const Base::Exception & e) {
        e.ReportException();
        return new App::DocumentObjectExecReturn(e.what());
    } catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making the pipe");
    }
}

void Pipe::setupAlgorithm(BRepOffsetAPI_MakePipeShell& mkPipeShell,
                          int mode,
                          const Base::Vector3d &bVec,
                          int transition,
                          const TopoShape& auxshape,
                          bool auxCurveLinear)
{
    mkPipeShell.SetTolerance(Precision::Confusion());

    switch(transition) {
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
    switch(mode) {
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
        mkPipeShell.SetMode(TopoDS::Wire(auxshape.getShape()), auxCurveLinear);
        //mkPipeShell.SetMode(TopoDS::Wire(auxshape), AuxilleryCurvelinear.getValue(), BRepFill_ContactOnBorder);
    }
}


void Pipe::getContinuousEdges(Part::TopoShape /*TopShape*/, std::vector< std::string >& /*SubNames*/) {

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
        for(auto &sub : subs) {
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
}

PROPERTY_SOURCE(PartDesign::SubtractivePipe, PartDesign::Pipe)
SubtractivePipe::SubtractivePipe() {
    initAddSubType(FeatureAddSub::Subtractive);
}
