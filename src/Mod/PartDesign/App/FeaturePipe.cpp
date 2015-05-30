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
# include <Handle_Geom_Surface.hxx>
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
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopExp.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <App/Document.h>

//#include "Body.h"
#include "FeaturePipe.h"


using namespace PartDesign;

const char* Pipe::TypeEnums[] = {"FullPath","UpToFace",NULL};
const char* Pipe::TransitionEnums[] = {"Transformed","Right corner", "Round corner",NULL};
const char* Pipe::ModeEnums[] = {"Standart", "Fixed", "Frenet", "Auxillery", "Binormal", NULL};
const char* Pipe::TransformEnums[] = {"Constant", "Multisection", "Auxillery", NULL};


PROPERTY_SOURCE(PartDesign::Pipe, PartDesign::SketchBased)

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
    ADD_PROPERTY_TYPE(Binormal,(Base::Vector3d()),"Sweep",App::Prop_None,"Binormal vector for coresponding orientation mode");
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
    return SketchBased::mustExecute();
}

App::DocumentObjectExecReturn *Pipe::execute(void)
{
    
    Part::Part2DObject* sketch = 0;
    std::vector<TopoDS_Wire> wires;
    try {
        sketch = getVerifiedSketch();
        wires = getSketchWires();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    
    TopoDS_Shape sketchshape = makeFace(wires);
    if (sketchshape.IsNull())
        return new App::DocumentObjectExecReturn("Pipe: Creating a face from sketch failed");

    // if the Base property has a valid shape, fuse the pipe into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        try {
            // fall back to support (for legacy features)
            base = getSupportShape();
        } catch (const Base::Exception&) {
            // ignore, because support isn't mandatory
            base = TopoDS_Shape();
        }
    }
 
    try {
        //build the paths
        App::DocumentObject* spine = Spine.getValue();
        if (!(spine && spine->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
            return new App::DocumentObjectExecReturn("No spine linked.");
        std::vector<std::string> subedge = Spine.getSubValues();
        TopoDS_Shape path;
        const Part::TopoShape& shape = static_cast<Part::Feature*>(spine)->Shape.getValue();
        buildPipePath(shape, subedge, path);
        
        
        TopoDS_Shape auxpath;
        if(Mode.getValue()==3) {
            App::DocumentObject* auxspine = AuxillerySpine.getValue();
            if (!(auxspine && auxspine->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
                return new App::DocumentObjectExecReturn("No auxillery spine linked.");
            std::vector<std::string> auxsubedge = AuxillerySpine.getSubValues();
            TopoDS_Shape path;
            const Part::TopoShape& auxshape = static_cast<Part::Feature*>(auxspine)->Shape.getValue();
            buildPipePath(auxshape, auxsubedge, auxpath);
        }
        
        //build the outer wire first
        BRepOffsetAPI_MakePipeShell mkPipeShell(TopoDS::Wire(path));
        setupAlgorithm(mkPipeShell, auxpath);
        mkPipeShell.Add(wires.front());

        if (!mkPipeShell.IsReady())
            Standard_Failure::Raise("shape is not ready to build");
         mkPipeShell.Build();
         mkPipeShell.MakeSolid();
        
        //now remove all inner wires
        TopoDS_Compound comp;
        TopoDS_Builder mkCmp;
        mkCmp.MakeCompound(comp);
        for(int i=1; i<wires.size();++i) {
            
            BRepOffsetAPI_MakePipeShell mkPS(TopoDS::Wire(path));
            setupAlgorithm(mkPS, auxpath);
            mkPS.Add(wires[i]);

            if (!mkPS.IsReady())
                Standard_Failure::Raise("shape is not ready to build");
            mkPS.Build();
            mkPS.MakeSolid();
            
            mkCmp.Add(comp, mkPS.Shape());
        }
        
        //TODO: This method is very slow. It wuld be better to build the individual pipes as shells 
        //and create the solid by hand from the shells. As we can assume no intersection between inner and outer 
        //shell this should work.
        BRepAlgoAPI_Cut cut(mkPipeShell.Shape(), comp);
        if (!cut.IsDone())
            return new App::DocumentObjectExecReturn("Building inner wire failed");
            
/*
        TopTools_ListOfShape sim;
        mkPipeShell.Simulate(5, sim);
        BRep_Builder builder;
        TopoDS_Compound Comp;
        builder.MakeCompound(Comp);
        
        TopTools_ListIteratorOfListOfShape simIt;
        for (simIt.Initialize(sim); simIt.More(); simIt.Next())
            builder.Add(Comp, simIt.Value()) ;
        */
        this->Shape.setValue(cut.Shape());
        return App::DocumentObject::StdReturn;
        
        return SketchBased::execute();   
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making the sweep");
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
        
        bool auxillery = false;
        const Base::Vector3d& bVec = Binormal.getValue();
        switch(Mode.getValue()) {
            case 1:
                mkPipeShell.SetMode(gp_Ax2(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0)));
                break;
            case 2:
                mkPipeShell.SetMode(true);
                break;
            case 3:
                auxillery = true;
            case 4:
                mkPipeShell.SetMode(gp_Dir(bVec.x,bVec.y,bVec.z));
                break;
        }
        
        if(auxillery) {
            
            if(Transformation.getValue()!=2)
                mkPipeShell.SetMode(TopoDS::Wire(auxshape), AuxilleryCurvelinear.getValue());
            else
                mkPipeShell.SetMode(TopoDS::Wire(auxshape), AuxilleryCurvelinear.getValue(), BRepFill_ContactOnBorder);
        }
}


void Pipe::getContiniusEdges(Part::TopoShape TopShape, std::vector< std::string >& SubNames) {

    
    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeEdge;
    TopExp::MapShapesAndAncestors(TopShape._Shape, TopAbs_EDGE, TopAbs_EDGE, mapEdgeEdge);
    TopExp::MapShapes(TopShape._Shape, TopAbs_EDGE, mapOfEdges);

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
}

void Pipe::buildPipePath(const Part::TopoShape& shape, const std::vector< std::string >& subedge, TopoDS_Shape& path) {

    if (!shape._Shape.IsNull()) {
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
            else if (shape._Shape.ShapeType() == TopAbs_EDGE) {
                path = shape._Shape;
            }
            else if (shape._Shape.ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape._Shape));
                path = mkWire.Wire();
            }
            else if (shape._Shape.ShapeType() == TopAbs_COMPOUND) {
                TopoDS_Iterator it(shape._Shape);
                for (; it.More(); it.Next()) {
                    if (it.Value().IsNull())
                        throw Base::Exception("In valid element in spine.");
                    if ((it.Value().ShapeType() != TopAbs_EDGE) &&
                        (it.Value().ShapeType() != TopAbs_WIRE)) {
                        throw Base::Exception("Element in spine is neither an edge nor a wire.");
                    }
                }

                Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
                Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
                for (TopExp_Explorer xp(shape._Shape, TopAbs_EDGE); xp.More(); xp.Next())
                    hEdges->Append(xp.Current());

                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, Precision::Confusion(), Standard_True, hWires);
                int len = hWires->Length();
                if (len != 1)
                    throw Base::Exception("Spine is not connected.");
                path = hWires->Value(1);
            }
            else {
                throw Base::Exception("Spine is neither an edge nor a wire.");
            }
        }
        catch (Standard_Failure) {
            throw Base::Exception("Invalid spine.");
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


