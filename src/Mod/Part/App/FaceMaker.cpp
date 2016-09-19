/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC)      <vv.titov@gmail.com>  *
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
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRep_Tool.hxx>
#endif

#include "FaceMaker.h"

#include <Base/Exception.h>
#include <memory>

#include <QtGlobal>

TYPESYSTEM_SOURCE_ABSTRACT(Part::FaceMaker, Base::BaseClass);
TYPESYSTEM_SOURCE_ABSTRACT(Part::FaceMakerPublic, Part::FaceMaker);

Part::FaceMaker::FaceMaker(const TopoDS_Compound& comp)
{
    TopoDS_Iterator it(comp);
    for(; it.More(); it.Next()){
        this->addShape(it.Value());
    }
    this->Build();
}

void Part::FaceMaker::addWire(const TopoDS_Wire& w)
{
    this->addShape(w);
}

void Part::FaceMaker::addShape(const TopoDS_Shape& sh)
{
    if(sh.IsNull())
        throw Base::ValueError("Input shape is null.");
    switch(sh.ShapeType()){
        case TopAbs_COMPOUND:
            this->myCompounds.push_back(TopoDS::Compound(sh));
        break;
        case TopAbs_WIRE:
            this->myWires.push_back(TopoDS::Wire(sh));
        break;
        case TopAbs_EDGE:
            this->myWires.push_back(BRepBuilderAPI_MakeWire(TopoDS::Edge(sh)).Wire());
        break;
        default:
            throw Base::TypeError("Shape must be a wire, edge or compound. Something else was supplied.");
        break;
    }
    this->mySourceShapes.push_back(sh);
}

const TopoDS_Face& Part::FaceMaker::Face()
{
    const TopoDS_Shape &sh = this->Shape();
    if(sh.IsNull())
        throw Base::Exception("Part::FaceMaker: result shape is null.");
    if (sh.ShapeType() != TopAbs_FACE)
        throw Base::TypeError("Part::FaceMaker: return shape is not a single face.");
    return TopoDS::Face(sh);
}

void Part::FaceMaker::Build()
{
    this->myShapesToReturn.clear();
    this->NotDone();
    this->myGenerated.Clear();

    this->Build_Essence();//adds stuff to myShapesToReturn

    for(const TopoDS_Compound& cmp : this->myCompounds){
        std::unique_ptr<FaceMaker> facemaker_instance ( static_cast<FaceMaker*>(this->getTypeId().createInstance()) ); //using unique_ptr allows to avoid exception handling to delete the new facemaker
        FaceMaker* facemaker = &(*facemaker_instance);

        TopoDS_Iterator it(cmp);
        for(; it.More(); it.Next()){
            facemaker->addShape(it.Value());
        }
        this->Build();
        const TopoDS_Shape &result = facemaker->Shape();
        if (result.IsNull())
            continue;
        if (result.ShapeType() == TopAbs_COMPOUND){
            this->myShapesToReturn.push_back(result);
        } else {
            TopoDS_Builder builder;
            TopoDS_Compound cmp_res;
            builder.MakeCompound(cmp_res);
            builder.Add(cmp_res,result);
            this->myShapesToReturn.push_back(cmp_res);
        }
    }

    if(this->myShapesToReturn.size() == 0){

    } else if (this->myShapesToReturn.size() == 1){
        this->myShape = this->myShapesToReturn[0];
    } else {
        TopoDS_Builder builder;
        TopoDS_Compound cmp_res;
        builder.MakeCompound(cmp_res);
        for(TopoDS_Shape &sh: this->myShapesToReturn){
            builder.Add(cmp_res,sh);
        }
        this->myShape = cmp_res;
    }
    this->Done();
}


//----------------------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::FaceMakerSimple, Part::FaceMakerPublic);


std::string Part::FaceMakerSimple::getUserFriendlyName() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker","Simple"));
}

std::string Part::FaceMakerSimple::getBriefExplanation() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker","Makes separate plane face from every wire independently. No support for holes; wires can be on different planes."));
}

void Part::FaceMakerSimple::Build_Essence()
{
    for(TopoDS_Wire &w: myWires){
        this->myShapesToReturn.push_back(BRepBuilderAPI_MakeFace(w).Shape());
    }
}
