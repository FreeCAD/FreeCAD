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
# include <ShapeAnalysis.hxx>
#endif

#include "TopoShapeOpCode.h"
#include "FaceMaker.h"

#include "TopoShape.h"
#include <memory>

#include <QtGlobal>

TYPESYSTEM_SOURCE_ABSTRACT(Part::FaceMaker, Base::BaseClass)
TYPESYSTEM_SOURCE_ABSTRACT(Part::FaceMakerPublic, Part::FaceMaker)

void Part::FaceMaker::addWire(const TopoDS_Wire& w)
{
    this->addShape(w);
}

void Part::FaceMaker::addShape(const TopoDS_Shape& sh)
{
    addTopoShape(sh);
}

void Part::FaceMaker::addTopoShape(const TopoShape& shape) {
    const TopoDS_Shape &sh = shape.getShape();
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
    this->mySourceShapes.push_back(shape);
}

void Part::FaceMaker::useCompound(const TopoDS_Compound& comp)
{
    TopoDS_Iterator it(comp);
    for(; it.More(); it.Next()){
        this->addShape(it.Value());
    }
}

void Part::FaceMaker::useTopoCompound(const TopoShape& comp)
{
    for(auto &s : comp.getSubTopoShapes())
        this->addTopoShape(s);
}

const TopoDS_Face& Part::FaceMaker::Face()
{
    return TopoDS::Face(TopoFace().getShape());
}

const Part::TopoShape &Part::FaceMaker::TopoFace() const{
    if(this->myTopoShape.isNull())
        throw NullShapeException("Part::FaceMaker: result shape is null.");
    if (this->myTopoShape.getShape().ShapeType() != TopAbs_FACE)
        throw Base::TypeError("Part::FaceMaker: return shape is not a single face.");
    return this->myTopoShape;
}

const Part::TopoShape &Part::FaceMaker::getTopoShape() const{
    if(this->myTopoShape.isNull())
        throw NullShapeException("Part::FaceMaker: result shape is null.");
    return this->myTopoShape;
}

void Part::FaceMaker::Build()
{
    this->NotDone();
    this->myShapesToReturn.clear();
    this->myGenerated.Clear();

    this->Build_Essence();//adds stuff to myShapesToReturn

    for(const TopoDS_Compound& cmp : this->myCompounds){
        std::unique_ptr<FaceMaker> facemaker = Part::FaceMaker::ConstructFromType(this->getTypeId());

        facemaker->useCompound(cmp);

        facemaker->Build();
        const TopoDS_Shape &subfaces = facemaker->Shape();
        if (subfaces.IsNull())
            continue;
        if (subfaces.ShapeType() == TopAbs_COMPOUND){
            this->myShapesToReturn.push_back(subfaces);
        } else {
            //result is not a compound (probably, a face)... but we want to follow compounding structure of input, so wrap it into compound.
            TopoDS_Builder builder;
            TopoDS_Compound cmp_res;
            builder.MakeCompound(cmp_res);
            builder.Add(cmp_res,subfaces);
            this->myShapesToReturn.push_back(cmp_res);
        }
    }

    if(this->myShapesToReturn.empty()){
        //nothing to do, null shape will be returned.
        this->myShape = TopoDS_Shape();
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

    postBuild();
}

void Part::FaceMaker::postBuild() {
    this->myTopoShape.setShape(this->myShape);
    this->myTopoShape.Hasher = this->MyHasher;
    this->myTopoShape.mapSubElement(this->mySourceShapes);
    int i = 0;
    std::ostringstream ss;
    const char *op = this->MyOp;
    if(!op) op = TOPOP_FACE;
    const auto &faces = this->myTopoShape.getSubTopoShapes(TopAbs_FACE);
    // name the face using the edges of its outer wire
    long wireTag = 0;
    bool sameTag = true;
    for(auto &face : faces) {
        ++i;
        TopoShape wire(ShapeAnalysis::OuterWire(TopoDS::Face(face.getShape())));
        wire.mapSubElement(face);
        std::set<std::string> edgeNames;
        int count = wire.countSubShapes(TopAbs_EDGE);
        for(int i=1;i<=count;++i) {
            std::string element("Edge");
            element += std::to_string(i);
            const char *name = face.getElementName(element.c_str(),true);
            if(name == element) {
                // only name the face if all edges are named
                edgeNames.clear();
                break;
            }
            if(faces.size()==1 && sameTag) {
                long tag = wire.getElementHistory(name);
                if(!wireTag)
                    wireTag = tag;
                else
                    sameTag = (wireTag==tag);
            }
            edgeNames.insert(name);
        }
        if(edgeNames.empty())
            continue;
        ss.str("");
        ss << "Face" << i;
        std::vector<std::string> names;
        if(faces.size()==1 && sameTag && wireTag) {
            // If there is only one face, and all edges coming from the same
            // object (tag), we abstract away individual edge names, and simply
            // use a pseduo element name "Wire". The advantage is that the
            // resulting face name will not be affected by any edge changes
            // (e.g. edge changess from editing a sketch).
            std::string name("Wire");
            std::ostringstream ss;
            std::vector<App::StringIDRef> sids;
            TopoShape().encodeElementName(name[0],name,ss,sids,0,wireTag);
            names.push_back(name + ss.str());
        }else
            names.insert(names.end(),edgeNames.begin(),edgeNames.end());
        this->myTopoShape.setElementComboName(ss.str().c_str(),names,op);
    }
    this->myTopoShape.initCache(true);
    this->Done();
}

std::unique_ptr<Part::FaceMaker> Part::FaceMaker::ConstructFromType(const char* className)
{
    Base::Type fmType = Base::Type::fromName(className);
    if (fmType.isBad()){
        std::stringstream ss;
        ss << "Class '"<< className <<"' not found.";
        throw Base::TypeError(ss.str().c_str());
    }
    return Part::FaceMaker::ConstructFromType(fmType);
}

std::unique_ptr<Part::FaceMaker> Part::FaceMaker::ConstructFromType(Base::Type type)
{
    if (!type.isDerivedFrom(Part::FaceMaker::getClassTypeId())){
        std::stringstream ss;
        ss << "Class '" << type.getName() << "' is not derived from Part::FaceMaker.";
        throw Base::TypeError(ss.str().c_str());
    }
    std::unique_ptr<FaceMaker> instance(static_cast<Part::FaceMaker*>(type.createInstance()));
    if (!instance){
        std::stringstream ss;
        ss << "Cannot create FaceMaker from abstract type '" << type.getName() << "'";
        throw Base::TypeError(ss.str().c_str());
    }
    return instance;
}

void Part::FaceMaker::throwNotImplemented()
{
    throw Base::NotImplementedError("Not implemented yet...");
}


//----------------------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::FaceMakerSimple, Part::FaceMakerPublic)


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
