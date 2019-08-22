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

    std::vector<TopoDS_Wire> wires;
    try {
        wires = getProfileWires();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    TopoDS_Shape sketchshape = getVerifiedFace();
    if (sketchshape.IsNull())
        return new App::DocumentObjectExecReturn("Loft: Creating a face from sketch failed");

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

        //build up multisections
        auto multisections = Sections.getValues();
        if(multisections.empty())
            return new App::DocumentObjectExecReturn("Loft: At least one section is needed");

        std::vector<std::vector<TopoDS_Wire>> wiresections;
        for(TopoDS_Wire& wire : wires)
            wiresections.push_back(std::vector<TopoDS_Wire>(1, wire));

        for(App::DocumentObject* obj : multisections) {
            if(!obj->isDerivedFrom(Part::Feature::getClassTypeId()))
                return  new App::DocumentObjectExecReturn("Loft: All sections need to be part features");

            TopExp_Explorer ex;
            size_t i=0;
            for (ex.Init(static_cast<Part::Feature*>(obj)->Shape.getValue(), TopAbs_WIRE); ex.More(); ex.Next(), ++i) {
                if(i>=wiresections.size())
                    return new App::DocumentObjectExecReturn("Loft: Sections need to have the same amount of inner wires as the base section");
                wiresections[i].push_back(TopoDS::Wire(ex.Current()));
            }
            if(i<wiresections.size())
                    return new App::DocumentObjectExecReturn("Loft: Sections need to have the same amount of inner wires as the base section");

        }

        //build all shells
        std::vector<TopoDS_Shape> shells;
        for(std::vector<TopoDS_Wire>& wires : wiresections) {

            BRepOffsetAPI_ThruSections mkTS(false, Ruled.getValue(), Precision::Confusion());

            for(TopoDS_Wire& wire : wires)   {
                 wire.Move(invObjLoc);
                 mkTS.AddWire(wire);
            }

            mkTS.Build();
            if (!mkTS.IsDone())
                return new App::DocumentObjectExecReturn("Loft could not be built");

            //build the shell use simulate to get the top and bottom wires in an easy way
            shells.push_back(mkTS.Shape());
        }

        //build the top and bottom face, sew the shell and build the final solid
        TopoDS_Shape front = getVerifiedFace();
        front.Move(invObjLoc);
        std::vector<TopoDS_Wire> backwires;
        for(std::vector<TopoDS_Wire>& wires : wiresections)
            backwires.push_back(wires.back());

        TopoDS_Shape back = Part::FaceMakerCheese::makeFace(backwires);

        BRepBuilderAPI_Sewing sewer;
        sewer.SetTolerance(Precision::Confusion());
        sewer.Add(front);
        sewer.Add(back);
        for(TopoDS_Shape& s : shells)
            sewer.Add(s);

        sewer.Perform();

        //build the solid
        BRepBuilderAPI_MakeSolid mkSolid;
        mkSolid.Add(TopoDS::Shell(sewer.SewedShape()));
        if(!mkSolid.IsDone())
            return new App::DocumentObjectExecReturn("Loft: Result is not a solid");

        TopoDS_Shape result = mkSolid.Shape();
        BRepClass3d_SolidClassifier SC(result);
        SC.PerformInfinitePoint(Precision::Confusion());
        if ( SC.State() == TopAbs_IN) {
            result.Reverse();
        }

        AddSubShape.setValue(result);

        if(base.IsNull()) {
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        if(getAddSubType() == FeatureAddSub::Additive) {

            BRepAlgoAPI_Fuse mkFuse(base, result);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Loft: Adding the loft failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Loft: Resulting shape is not a solid");
            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Loft: Result has multiple solids. This is not supported at this time.");
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {

            BRepAlgoAPI_Cut mkCut(base, result);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Loft: Subtracting the loft failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkCut.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Loft: Resulting shape is not a solid");
            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Loft: Result has multiple solids. This is not supported at this time.");
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
