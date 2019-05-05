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
# include <TopoDS_Solid.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <App/Document.h>
#include <Mod/Part/App/FaceMakerCheese.h>

//#include "Body.h"
#include "FeatureLoft.h"


using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Loft, PartDesign::ProfileBased)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections,(0),"Loft",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Ruled,(false),"Loft",App::Prop_None,"Create ruled surface");
    ADD_PROPERTY_TYPE(Closed,(false),"Loft",App::Prop_None,"Close Last to First Profile");
}

short Loft::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Ruled.isTouched())
        return 1;
    if (Closed.isTouched())
        return 1;

    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Loft::execute(void)
{
    std::vector<TopoShape> wires;
    try {
        wires = getProfileWires();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    // if the Base property has a valid shape, fuse the pipe into it
    TopoShape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
    }

    auto hasher = getDocument()->getStringHasher();
 
    try {
        //setup the location
        this->positionByPrevious();
        auto invObjLoc = this->getLocation().Inverted(); 
        if(!base.isNull())
            base.move(invObjLoc);
             
        //build up multisections
        auto multisections = Sections.getValues();
        if(multisections.empty())
            return new App::DocumentObjectExecReturn("Loft: At least one section is needed");
        
        std::vector<std::vector<TopoShape>> wiresections;
        wiresections.reserve(wires.size());
        for(auto& wire : wires)
            wiresections.emplace_back(1,wire);
                
        for(App::DocumentObject* obj : multisections) {
            auto shape = getTopoShape(obj);
            if(shape.isNull()) 
                return  new App::DocumentObjectExecReturn("Loft: invalid linked feature");
            if(shape.countSubShapes(TopAbs_WIRE)!=wiresections.size())
                return new App::DocumentObjectExecReturn("Loft: Sections need to have the same amount of inner wires as the base section");
            int i=0;
            for(auto &wire : shape.getSubTopoShapes(TopAbs_WIRE))
                wiresections[i++].push_back(wire);
        }

        //build all shells
        std::vector<TopoShape> shells;
        for(auto& wires : wiresections) {
            
            BRepOffsetAPI_ThruSections mkTS(false, Ruled.getValue(), Precision::Confusion());

            for(auto& wire : wires)   {
                wire.move(invObjLoc);
                mkTS.AddWire(TopoDS::Wire(wire.getShape()));
            }

            mkTS.Build();
            if (!mkTS.IsDone())
                return new App::DocumentObjectExecReturn("Loft could not be built");

            //build the shell use simulate to get the top and bottom wires in an easy way
            shells.push_back(TopoShape(0,hasher).makEShape(mkTS,wires));
        }

        //build the top and bottom face, sew the shell and build the final solid
        auto front = getVerifiedFace();
        if (front.isNull())
            return new App::DocumentObjectExecReturn("Loft: Creating a face from sketch failed");
        front.move(invObjLoc);
        std::vector<TopoShape> backwires;
        for(auto& wires : wiresections)
            backwires.push_back(wires.back());
        
        auto back = TopoShape(0,hasher).makEFace(backwires,0,"Part::FaceMakerCheese");
        
        BRepBuilderAPI_Sewing sewer;
        sewer.SetTolerance(Precision::Confusion());
        sewer.Add(front.getShape());
        sewer.Add(back.getShape());
        for(auto& s : shells)
            sewer.Add(s.getShape());      
        
        sewer.Perform();

        TopoShape result(0,hasher);
        shells.push_back(front);
        shells.push_back(back);
        result = result.makEShape(sewer,shells);
        if(result.isNull() || result.getShape().ShapeType()!=TopAbs_SHELL)
            return new App::DocumentObjectExecReturn("Loft: Failed to create shell");

        //build the solid
        BRepBuilderAPI_MakeSolid mkSolid;
        mkSolid.Add(TopoDS::Shell(result.getShape()));
        if(!mkSolid.IsDone())
            return new App::DocumentObjectExecReturn("Loft: Result is not a solid");
        
        result = result.makESolid();
        BRepClass3d_SolidClassifier SC(result.getShape());
        SC.PerformInfinitePoint(Precision::Confusion());
        if ( SC.State() == TopAbs_IN) {
            result.setShape(result.getShape().Reversed(),false);
        }

        AddSubShape.setValue(result);
        
        if(base.isNull()) {
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        if(getAddSubType() == FeatureAddSub::Additive) {
            try {
                result = TopoShape(0,hasher).makEFuse({base,result});
            }catch(Standard_Failure &) {
                return new App::DocumentObjectExecReturn("Loft: Adding the loft failed");
            }

            // we have to get the solids (fuse sometimes creates compounds)
            auto boolOp = this->getSolid(result);
            // lets check if the result is a solid
            if (boolOp.isNull())
                return new App::DocumentObjectExecReturn("Loft: Resulting shape is not a solid");

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {
            try {
                result = TopoShape(0,hasher).makECut({base,result});
            }catch(Standard_Failure &) {
                return new App::DocumentObjectExecReturn("Loft: Subtracting the loft failed");
            }
            // we have to get the solids (fuse sometimes creates compounds)
            auto boolOp = this->getSolid(result);
            // lets check if the result is a solid
            if (boolOp.isNull())
                return new App::DocumentObjectExecReturn("Loft: Resulting shape is not a solid");
            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("Loft: A fatal error occurred when making the loft");
    }
}


PROPERTY_SOURCE(PartDesign::AdditiveLoft, PartDesign::Loft)
AdditiveLoft::AdditiveLoft() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractiveLoft, PartDesign::Loft)
SubtractiveLoft::SubtractiveLoft() {
    addSubType = Subtractive;
}
