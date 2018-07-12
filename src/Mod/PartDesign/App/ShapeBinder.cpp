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
# include <cfloat>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <App/Document.h>
#include "ShapeBinder.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/App/FaceMakerBullseye.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;

// ============================================================================

PROPERTY_SOURCE(PartDesign::ShapeBinder, Part::Feature)

ShapeBinder::ShapeBinder()
{
    ADD_PROPERTY_TYPE(Support, (0,0), "",(App::PropertyType)(App::Prop_None),"Support of the geometry");
    Placement.setStatus(App::Property::Hidden, true);
}

ShapeBinder::~ShapeBinder()
{
}

short int ShapeBinder::mustExecute(void) const {

    if(Support.isTouched())
        return 1;

    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn* ShapeBinder::execute(void) {

    if(! this->isRestoring()){
        Part::Feature* obj = nullptr;
        std::vector<std::string> subs;

        ShapeBinder::getFilteredReferences(&Support, obj, subs);
        //if we have a link we rebuild the shape, but we change nothing if we are a simple copy
        if(obj) {
            Part::TopoShape shape = ShapeBinder::buildShapeFromReferences(obj, subs);
            Base::Placement placement(shape.getTransform());
            Shape.setValue(shape);
            Placement.setValue(placement);
        }
    }

    return Part::Feature::execute();
}

void ShapeBinder::getFilteredReferences(App::PropertyLinkSubList* prop, Part::Feature*& obj, std::vector< std::string >& subobjects) {

    obj = nullptr;
    subobjects.clear();

    auto objs = prop->getValues();
    auto subs = prop->getSubValues();

    if(objs.empty()) {
        return;
    }

    //we only allow one part feature, so get the first one we find
    size_t index = 0;
    while(index < objs.size() && !objs[index]->isDerivedFrom(Part::Feature::getClassTypeId()))
        index++;

    //do we have any part feature?
    if(index >= objs.size())
        return;

    obj = static_cast<Part::Feature*>(objs[index]);

    //if we have no subshpape we use the whole shape
    if(subs[index].empty()) {
            return;
    }

    //collect all subshapes for the object
    index = 0;
    for(std::string sub : subs) {

        //we only allow subshapes from a single Part::Feature
        if(objs[index] != obj)
            continue;

        //in this mode the full shape is not allowed, as we already started the subshape
        //processing
        if(sub.empty())
            continue;

        subobjects.push_back(sub);
    }
}


Part::TopoShape ShapeBinder::buildShapeFromReferences( Part::Feature* obj, std::vector< std::string > subs) {

    if(!obj)
        return TopoDS_Shape();

    if(subs.empty())
        return obj->Shape.getShape();

    //if we use multiple subshapes we build a shape from them by fusing them together
    Part::TopoShape base;
    std::vector<TopoDS_Shape> operators;
    for(std::string sub : subs) {

        if(base.isNull())
            base = obj->Shape.getShape().getSubShape(sub.c_str());
        else
            operators.push_back(obj->Shape.getShape().getSubShape(sub.c_str()));
    }

    try {
        if(!operators.empty() && !base.isNull())
            return base.fuse(operators);
    }
    catch(...) {
        return base;
    }
    return base;
}

void ShapeBinder::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
{
    // The type of Support was App::PropertyLinkSubList in the past
    if (prop == &Support && strcmp(TypeName, "App::PropertyLinkSubList") == 0) {
        Support.Restore(reader);
    }
}

// ============================================================================

PROPERTY_SOURCE(PartDesign::SubShapeBinder, Part::Feature)

SubShapeBinder::SubShapeBinder()
{
    ADD_PROPERTY_TYPE(Support, (0), "",(App::PropertyType)(App::Prop_Hidden|App::Prop_None),
            "Support of the geometry");
    Support.setStatus(App::Property::Immutable,true);
    ADD_PROPERTY_TYPE(Fuse, (true), "Base",App::Prop_None,"Fused linked solid shapes");
    ADD_PROPERTY_TYPE(MakeFace, (true), "Base",App::Prop_None,"Create face for linked wires");
    ADD_PROPERTY_TYPE(ClaimChildren, (false), "Base",App::Prop_Output,"Claim linked object as children");
    ADD_PROPERTY_TYPE(Relative, (true), "Base",App::Prop_None,"Enable relative sub-object linking");
    ADD_PROPERTY_TYPE(BindMode, ((long)0), "Base", App::Prop_None, "Binding mode");
    ADD_PROPERTY_TYPE(PartialLoad, (true), "Base", App::Prop_None, "Enable partial loading");
    static const char *BindModeEnum[] = {"Syncrhonized", "Frozen", "Detached", 0};
    BindMode.setEnums(BindModeEnum);
    Placement.setStatus(App::Property::Immutable, true);
    Placement.setStatus(App::Property::ReadOnly, true);
}

void SubShapeBinder::updatePlacement(const Base::Matrix4D &mat) {
    auto placement = Placement.getValue()*Base::Placement(mat).inverse();
    Placement.setValue(placement);
}

void SubShapeBinder::update() {
    Part::TopoShape result;
    auto obj = Support.getValue();
    if(!obj || !obj->getNameInDocument() || obj->testStatus(App::PartialObject))
        return;
    std::vector<Part::TopoShape> shapes;
    const auto &subvals = Support.getSubValues();
    std::set<std::string> subs(subvals.begin(),subvals.end());
    static std::string none("");
    if(subs.empty())
        subs.insert(none);
    else if(subs.size()>1)
        subs.erase(none);
    for(const auto &sub : subs) {
        const auto &shape = Part::Feature::getTopoShape(obj,sub.c_str(),true);
        if(!shape.isNull())
            shapes.push_back(shape);
    }
    if(shapes.empty())
        return;

    result = Part::TopoShape(0,getDocument()->getStringHasher()).makECompound(shapes);

    bool fused = false;
    if(Fuse.getValue()) {
        // If the compound has solid, fuse them together, and ignore other type of
        // shapes
        auto solids = result.getSubTopoShapes(TopAbs_SOLID);
        if(solids.size()) {
            result.makEFuse(solids);
            result = result.makERefine();
            fused = true;
        }
    } 
    
    if(!fused && MakeFace.getValue() && 
       !result.hasSubShape(TopAbs_FACE) &&
       result.hasSubShape(TopAbs_EDGE))
    {
        result = result.makEWires();
        try {
            result = result.makEFace(TOPOP_SHAPEBINDER "_F");
        }catch(...){}
    }

    // Remove single element naming, so that other user of SubShapeBinder can
    // identify the element using SubShapeBinder's object id.

    auto count = result.countSubShapes(TopAbs_FACE);
    if(count == 1)
        result.setElementName("Face1",0);
    else if(count==0 && (count=result.countSubShapes(TopAbs_EDGE))==1)
        result.setElementName("Edge1",0);
    else if(count==0 && (count=result.countSubShapes(TopAbs_VERTEX))==1)
        result.setElementName("Vertex1",0);
    result.setTransform(Placement.getValue().toMatrix());
    if(count==1)
        result.initCache(true);
    Shape.setValue(result);
}

App::DocumentObjectExecReturn* SubShapeBinder::execute(void) {
    if(BindMode.getValue()==0)
        update();
    return Part::Feature::execute();
}

void SubShapeBinder::onChanged(const App::Property *prop) {
    if(!isRestoring()) {
        if(prop == &Support) {
            if(Support.getValue()) {
                update(); 
                if(BindMode.getValue() == 2)
                    Support.setValue(0);
            }
        }else if(prop == &BindMode) {
           if(BindMode.getValue() == 2)
               Support.setValue(0);
           else if(BindMode.getValue() == 0)
               update();
        }
    }
    Part::Feature::onChanged(prop);
}

void SubShapeBinder::setLinks(App::DocumentObject *obj, 
        const std::vector<std::string> &_subs, bool reset)
{
    if(!obj) {
        Support.setValue(0);
        Shape.setValue(Part::TopoShape());
        return;
    }
    if(!obj->getNameInDocument())
        throw Base::RuntimeError("Invalid object link");
    if(Support.getValue() && Support.getValue()!=obj)
        reset = true;

    std::set<std::string> subs(_subs.begin(),_subs.end());
    if(!reset) {
        if(subs.empty())
            return;
        const auto &oldSubs = Support.getSubValues();
        subs.insert(oldSubs.begin(),oldSubs.end());
    }
    if(subs.empty())
        subs.insert("");
    else if(subs.size()>1)
        subs.erase("");

    if(Relative.getValue() && obj->getDocument()!=getDocument()) 
        throw Base::RuntimeError("Direct external linking is not allowed");

    std::vector<std::string> subvals;
    if(Relative.getValue())
        subvals.insert(subvals.end(),subs.begin(),subs.end());
    else {
        App::DocumentObject *sobj = obj;
        for(auto &sub : subs) {
            const char *element = 0;
            auto subobj = obj->resolve(sub.c_str(),0,0,&element);
            if(!subobj) 
                throw Base::RuntimeError("Cannot find sub object");
            if(!sobj)
                sobj = subobj;
            else if(sobj!=subobj)
                throw Base::RuntimeError("Cannot link to different sub-object");
            subvals.push_back(element);
        }
        obj = sobj;
    }
    auto inSet = getInListEx(true);
    inSet.insert(this);
    if(inSet.find(obj)!=inSet.end())
        throw Base::RuntimeError("Cyclic dependency");
    Support.setValue(obj,subvals);
}
