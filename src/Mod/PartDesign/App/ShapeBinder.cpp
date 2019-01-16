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
#include <boost/bind.hpp>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include "ShapeBinder.h"
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/App/FaceMakerBullseye.h>

FC_LOG_LEVEL_INIT("PartDesign",true,true);

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
    ADD_PROPERTY_TYPE(TraceSupport, (false), "", App::Prop_None, "Trace support shape");
}

ShapeBinder::~ShapeBinder()
{
    this->connectDocumentChangedObject.disconnect();
}

short int ShapeBinder::mustExecute(void) const {

    if (Support.isTouched())
        return 1;
    if (TraceSupport.isTouched())
        return 1;

    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn* ShapeBinder::execute(void) {

    if (!this->isRestoring()) {
        Part::Feature* obj = nullptr;
        std::vector<std::string> subs;

        ShapeBinder::getFilteredReferences(&Support, obj, subs);
        //if we have a link we rebuild the shape, but we change nothing if we are a simple copy
        if (obj) {
            Part::TopoShape shape = ShapeBinder::buildShapeFromReferences(obj, subs);
            Base::Placement placement(shape.getTransform());
            Shape.setValue(shape);

            if (TraceSupport.getValue()) {
                // this is the inverted global placement of the parent group ...
                placement = this->globalPlacement() * Placement.getValue().inverse();
                // multiplied with the global placement of the support shape
                placement = placement.inverse() * obj->globalPlacement();
            }
            Placement.setValue(placement);
        }
    }

    return Part::Feature::execute();
}

void ShapeBinder::getFilteredReferences(App::PropertyLinkSubList* prop, Part::Feature*& obj,
                                        std::vector< std::string >& subobjects)
{
    obj = nullptr;
    subobjects.clear();

    auto objs = prop->getValues();
    auto subs = prop->getSubValues();

    if (objs.empty()) {
        return;
    }

    //we only allow one part feature, so get the first one we find
    size_t index = 0;
    while (index < objs.size() && !objs[index]->isDerivedFrom(Part::Feature::getClassTypeId()))
        index++;

    //do we have any part feature?
    if (index >= objs.size())
        return;

    obj = static_cast<Part::Feature*>(objs[index]);

    //if we have no subshpape we use the whole shape
    if (subs[index].empty()) {
        return;
    }

    //collect all subshapes for the object
    index = 0;
    for (std::string sub : subs) {

        //we only allow subshapes from a single Part::Feature
        if (objs[index] != obj)
            continue;

        //in this mode the full shape is not allowed, as we already started the subshape
        //processing
        if (sub.empty())
            continue;

        subobjects.push_back(sub);
    }
}

Part::TopoShape ShapeBinder::buildShapeFromReferences( Part::Feature* obj, std::vector< std::string > subs) {

    if (!obj)
        return TopoDS_Shape();

    if (subs.empty())
        return obj->Shape.getShape();

    //if we use multiple subshapes we build a shape from them by fusing them together
    Part::TopoShape base;
    std::vector<TopoDS_Shape> operators;
    for (std::string sub : subs) {
        if (base.isNull())
            base = obj->Shape.getShape().getSubShape(sub.c_str());
        else
            operators.push_back(obj->Shape.getShape().getSubShape(sub.c_str()));
    }

    try {
        if (!operators.empty() && !base.isNull())
            return base.fuse(operators);
    }
    catch (...) {
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

void ShapeBinder::onSettingDocument()
{
    App::Document* document = getDocument();
    if (document) {
        this->connectDocumentChangedObject = document->signalChangedObject.connect(boost::bind
            (&ShapeBinder::slotChangedObject, this, _1, _2));
    }
}

void ShapeBinder::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
    App::Document* doc = getDocument();
    if (!doc || doc->testStatus(App::Document::Restoring))
        return;
    if (this == &Obj)
        return;
    if (!TraceSupport.getValue())
        return;
    if (!Prop.getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId()))
        return;

    Part::Feature* obj = nullptr;
    std::vector<std::string> subs;
    ShapeBinder::getFilteredReferences(&Support, obj, subs);
    if (obj) {
        if (obj == &Obj) {
            // the directly referenced object has changed
            enforceRecompute();
        }
        else if (Obj.hasExtension(App::GroupExtension::getExtensionClassTypeId())) {
            // check if the changed property belongs to a group-like object
            // like Body or Part
            std::vector<App::DocumentObject*> chain;
            std::vector<App::DocumentObject*> list = getInListRecursive();
            chain.insert(chain.end(), list.begin(), list.end());
            list = obj->getInListRecursive();
            chain.insert(chain.end(), list.begin(), list.end());

            auto it = std::find(chain.begin(), chain.end(), &Obj);
            if (it != chain.end()) {
                enforceRecompute();
            }
        }
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
    ADD_PROPERTY_TYPE(PartialLoad, (false), "Base", App::Prop_None, "Enable partial loading");
    PartialLoad.setStatus(App::Property::PartialTrigger,true);
    static const char *BindModeEnum[] = {"Synchronized", "Frozen", "Detached", 0};
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
    std::vector<Part::TopoShape> shapes;

    std::string errMsg;
    for(auto &l : Support.getSubListValues()) {
        auto obj = l.getValue();
        if(!obj || !obj->getNameInDocument())
            continue;
        const auto &subvals = l.getSubValues();
        std::set<std::string> subs(subvals.begin(),subvals.end());
        static std::string none("");
        if(subs.empty())
            subs.insert(none);
        else if(subs.size()>1)
            subs.erase(none);
        for(const auto &sub : subs) {
            try {
                const auto &shape = Part::Feature::getTopoShape(obj,sub.c_str(),true);
                if(!shape.isNull())
                    shapes.push_back(shape);
            } catch(Base::Exception &e) {
                e.ReportException();
                FC_ERR(getFullName() << " failed to obtain shape from " 
                        << obj->getFullName() << '.' << sub);
                if(errMsg.empty()) {
                    std::ostringstream ss;
                    ss << "Failed to obtain shape " <<
                        obj->getFullName() << '.' 
                        << Data::ComplexGeoData::oldElementName(sub.c_str());
                    errMsg = ss.str();
                }
            }
        }
    }
    if(errMsg.size()) 
        FC_THROWM(Base::RuntimeError, errMsg);

    if(shapes.empty()) {
        Shape.resetElementMapVersion();
        return;
    }

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
    else
        Shape.resetElementMapVersion();
    return inherited::execute();
}

void SubShapeBinder::onDocumentRestored() {
    if(Shape.testStatus(App::Property::Transient))
        update();
    inherited::onDocumentRestored();
}

void SubShapeBinder::onChanged(const App::Property *prop) {
    if(!isRestoring()) {
        if(prop == &Support) {
            if(Support.getSubListValues().size()) {
                update(); 
                if(BindMode.getValue() == 2)
                    Support.setValue(0);
            }
        }else if(prop == &BindMode) {
           if(BindMode.getValue() == 2)
               Support.setValue(0);
           else if(BindMode.getValue() == 0)
               update();
           checkPropertyStatus();
        }else if(prop == &PartialLoad) {
           checkPropertyStatus();
        }
    }
    inherited::onChanged(prop);
}

void SubShapeBinder::checkPropertyStatus() {
    Support.setAllowPartial(PartialLoad.getValue());
    Shape.setStatus(App::Property::Transient, !PartialLoad.getValue() && BindMode.getValue()==0);
}

void SubShapeBinder::setLinks(std::map<App::DocumentObject *, std::vector<std::string> >&&values, bool reset)
{
    if(values.empty()) {
        if(reset) {
            Support.setValue(0);
            Shape.setValue(Part::TopoShape());
        }
        return;
    }
    auto inSet = getInListEx(true);
    inSet.insert(this);

    for(auto &v : values) {
        if(!v.first || !v.first->getNameInDocument())
            FC_THROWM(Base::ValueError,"Invalid document object");
        if(inSet.find(v.first)!=inSet.end())
            FC_THROWM(Base::ValueError, "Cyclic referece to " << v.first->getFullName());
    }

    for(auto &v : values) 
        Support.addValue(v.first,std::move(v.second),reset);
}
    
void SubShapeBinder::handleChangedPropertyType(
        Base::XMLReader &reader, const char * TypeName, App::Property * prop) 
{
   if(prop == &Support) {
       Support.upgrade(reader,TypeName);
       return;
   }
   inherited::handleChangedPropertyType(reader,TypeName,prop);
}

