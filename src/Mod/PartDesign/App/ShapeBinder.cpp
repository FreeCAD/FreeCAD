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
# include <boost/bind.hpp>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
#endif

#include <unordered_map>
#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include "ShapeBinder.h"
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/OriginFeature.h>
#include <Mod/Part/App/TopoShape.h>

FC_LOG_LEVEL_INIT("PartDesign",true,true)

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
        App::GeoFeature* obj = nullptr;
        std::vector<std::string> subs;

        ShapeBinder::getFilteredReferences(&Support, obj, subs);

        //if we have a link we rebuild the shape, but we change nothing if we are a simple copy
        if (obj) {
            Part::TopoShape shape(ShapeBinder::buildShapeFromReferences(obj, subs));
            //now, shape is in object's CS, and includes local Placement of obj but nothing else.

            if (TraceSupport.getValue()) {
                //compute the transform, and apply it to the shape.
                Base::Placement sourceCS = //full placement of container of obj
                        obj->globalPlacement() * obj->Placement.getValue().inverse();
                Base::Placement targetCS = //full placement of container of this shapebinder
                        this->globalPlacement() * this->Placement.getValue().inverse();
                Base::Placement transform = targetCS.inverse() * sourceCS;
                shape.setPlacement(transform * shape.getPlacement());
            }

            this->Placement.setValue(shape.getTransform());
            this->Shape.setValue(shape);
        }
    }

    return Part::Feature::execute();
}

void ShapeBinder::getFilteredReferences(App::PropertyLinkSubList* prop, App::GeoFeature*& obj,
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
    for (auto* it : objs) {
        if (it && it->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            obj = static_cast<Part::Feature*>(it);
            break;
        }
        index++;
    }

    //do we have any part feature?
    if (obj) {
        //if we have no subshpape we use the whole shape
        if (subs[index].empty()) {
            return;
        }

        //collect all subshapes for the object
        for (index = 0; index < objs.size(); index++) {
            //we only allow subshapes from a single Part::Feature
            if (objs[index] != obj)
                continue;

            //in this mode the full shape is not allowed, as we already started the subshape
            //processing
            if (subs[index].empty())
                continue;

            subobjects.push_back(subs[index]);
        }
    }
    else {
        // search for Origin features
        for (auto* it : objs) {
            if (it && it->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
                obj = static_cast<App::GeoFeature*>(it);
                break;
            }
            else if (it && it->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                obj = static_cast<App::GeoFeature*>(it);
                break;
            }
        }
    }
}

Part::TopoShape ShapeBinder::buildShapeFromReferences(App::GeoFeature* obj, std::vector< std::string > subs) {

    if (!obj)
        return TopoDS_Shape();

    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        Part::Feature* part = static_cast<Part::Feature*>(obj);
        if (subs.empty())
            return part->Shape.getValue();

        std::vector<TopoDS_Shape> shapes;
        for (std::string sub : subs) {
            shapes.push_back(part->Shape.getShape().getSubShape(sub.c_str()));
        }

        if (shapes.size() == 1) {
            //single subshape. Return directly.
            return shapes[0];
        }
        else {
            //multiple subshapes. Make a compound.
            BRep_Builder builder;
            TopoDS_Compound cmp;
            builder.MakeCompound(cmp);
            for(const TopoDS_Shape& sh : shapes){
                builder.Add(cmp, sh);
            }
            return cmp;
        }
    }
    else if (obj->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
        gp_Lin line;
        BRepBuilderAPI_MakeEdge mkEdge(line);
        Part::TopoShape shape(mkEdge.Shape());
        shape.setPlacement(obj->Placement.getValue());
        return shape;
    }
    else if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        gp_Pln plane;
        BRepBuilderAPI_MakeFace mkFace(plane);
        Part::TopoShape shape(mkFace.Shape());
        shape.setPlacement(obj->Placement.getValue());
        return shape;
    }

    return TopoDS_Shape();
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

    App::GeoFeature* obj = nullptr;
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
    ADD_PROPERTY_TYPE(Fuse, (false), "Base",App::Prop_None,"Fused linked solid shapes");
    ADD_PROPERTY_TYPE(MakeFace, (true), "Base",App::Prop_None,"Create face for linked wires");
    ADD_PROPERTY_TYPE(ClaimChildren, (false), "Base",App::Prop_Output,"Claim linked object as children");
    ADD_PROPERTY_TYPE(Relative, (true), "Base",App::Prop_None,"Enable relative sub-object linking");
    ADD_PROPERTY_TYPE(BindMode, ((long)0), "Base", App::Prop_None, "Binding mode");
    ADD_PROPERTY_TYPE(PartialLoad, (false), "Base", App::Prop_None, "Enable partial loading");
    PartialLoad.setStatus(App::Property::PartialTrigger,true);
    static const char *BindModeEnum[] = {"Synchronized", "Frozen", "Detached", 0};
    BindMode.setEnums(BindModeEnum);

    ADD_PROPERTY_TYPE(Context, (0), "Base", App::Prop_Hidden, "Enable partial loading");
    Context.setScope(App::LinkScope::Hidden);

    ADD_PROPERTY_TYPE(_Version,(0),"Base",(App::PropertyType)(
                App::Prop_Hidden|App::Prop_ReadOnly), "");
}

void SubShapeBinder::setupObject() {
    _Version.setValue(2);
    checkPropertyStatus();
}

void SubShapeBinder::update(SubShapeBinder::UpdateOption options) {
    Part::TopoShape result;
    std::vector<Part ::TopoShape> shapes;
    std::vector<const Base::Matrix4D*> shapeMats;

    bool forced = (Shape.getValue().IsNull() || (options & UpdateForced)) ? true : false;
    bool init = (!forced && (options & UpdateForced)) ? true : false;

    std::string errMsg;
    auto parent = Context.getValue();
    std::string parentSub  = Context.getSubName(false);
    if(!Relative.getValue()) 
        parent = 0;
    else {
        if(parent && parent->getSubObject(parentSub.c_str())==this) {
            auto parents = parent->getParents();
            if(parents.size()) {
                parent = parents.begin()->first;
                parentSub = parents.begin()->second + parentSub;
            }
        } else
            parent = 0;
        if(!parent && parentSub.empty()) {
            auto parents = getParents();
            if(parents.size()) {
                parent = parents.begin()->first;
                parentSub = parents.begin()->second;
            }
        }
        if(parent && (parent!=Context.getValue() || parentSub!=Context.getSubName(false)))
            Context.setValue(parent,parentSub.c_str());
    }
    bool first = false;
    std::unordered_map<const App::DocumentObject*, Base::Matrix4D> mats;
    for(auto &l : Support.getSubListValues()) {
        auto obj = l.getValue();
        if(!obj || !obj->getNameInDocument())
            continue;
        auto res = mats.emplace(obj,Base::Matrix4D());
        if(parent && res.second) {
            std::string resolvedSub = parentSub;
            std::string linkSub;
            auto link = obj;
            auto resolved = parent->resolveRelativeLink(resolvedSub,link,linkSub);
            if(!resolved) {
                if(!link) {
                    FC_WARN(getFullName() << " cannot resolve relative link of "
                            << parent->getFullName() << '.' << parentSub
                            << " -> " << obj->getFullName());
                }
            }else{
                Base::Matrix4D mat;
                auto sobj = resolved->getSubObject(resolvedSub.c_str(),0,&mat);
                if(sobj!=this) {
                    FC_LOG(getFullName() << " skip invalid parent " << resolved->getFullName() 
                            << '.' << resolvedSub);
                }else if(_Version.getValue()==0) {
                    // For existing legacy SubShapeBinder, we use its Placement
                    // to store the position adjustment of the first Support
                    if(first) {
                        auto pla = Placement.getValue()*Base::Placement(mat).inverse();
                        Placement.setValue(pla);
                        first = false;
                    } else {
                        // The remaining support will cancel the Placement
                        mat.inverseGauss();
                        res.first->second = mat;
                    }
                }else{
                    // For newer SubShapeBinder, the Placement property is free
                    // to use by the user to add additional offset to the
                    // binding object
                    mat.inverseGauss();
                    res.first->second = Placement.getValue().toMatrix()*mat;
                }
            }
        }
        if(init)
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
                auto shape = Part::Feature::getTopoShape(obj,sub.c_str(),true);
                if(!shape.isNull()) {
                    shapes.push_back(shape);
                    shapeMats.push_back(&res.first->second);
                }
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

    std::string objName;
    // lambda function for generating matrix cache property
    auto cacheName = [this,&objName](const App::DocumentObject *obj) {
        objName = "Cache_";
        objName += obj->getNameInDocument();
        if(obj->getDocument() != getDocument()) {
            objName += "_";
            objName += obj->getDocument()->getName();
        }
        return objName.c_str();
    };

    if(!init) {

        if(errMsg.size()) {
            if(!(options & UpdateInit))
                FC_THROWM(Base::RuntimeError, errMsg);
            if(!Shape.getValue().IsNull())
                return;
        }

        // If not forced, only rebuild when there is any change in
        // transformation matrix
        if(!forced) {
            bool hit = true;
            for(auto &v : mats) {
                auto prop = Base::freecad_dynamic_cast<App::PropertyMatrix>(
                        getDynamicPropertyByName(cacheName(v.first)));
                if(!prop || prop->getValue()!=v.second) {
                    hit = false;
                    break;
                }
            }
            if(hit)
                return;
        }
        
        if(shapes.size()==1 && !Relative.getValue())
            shapes.back().setPlacement(Base::Placement());
        else {
            for(size_t i=0;i<shapes.size();++i) {
                auto &shape = shapes[i];
                shape = shape.makETransform(*shapeMats[i]);
                // if(shape.Hasher
                //         && shape.getElementMapSize()
                //         && shape.Hasher != getDocument()->getStringHasher())
                // {
                //     shape.reTagElementMap(getID(),
                //             getDocument()->getStringHasher(),TOPOP_SHAPEBINDER);
                // }
            }
        }

        if(shapes.empty()) {
            // Shape.resetElementMapVersion();
            return;
        }

        result.makECompound(shapes);

        bool fused = false;
        if(Fuse.getValue()) {
            // If the compound has solid, fuse them together, and ignore other type of
            // shapes
            std::vector<TopoDS_Shape> solids;
            Part::TopoShape solid;
            for(auto &s : result.getSubTopoShapes(TopAbs_SOLID)) {
                if(!solid.isNull())
                    solid = s;
                else 
                    solids.push_back(s.getShape());
            }
            if(solids.size()) {
                solid.fuse(solids);
                result = solid.makERefine();
                fused = true;
            } else if (!solid.isNull()) {
                // wrap the single solid in compound to keep its placement
                result.makECompound({solid});
                fused = true;
            }
        } 
        
        if(!fused && MakeFace.getValue()
                && !result.hasSubShape(TopAbs_FACE)
                && result.hasSubShape(TopAbs_EDGE))
        {
            result = result.makEWires();
            try {
                result = result.makEFace(0);
            }catch(...){}
        }

        result.setPlacement(Placement.getValue());
        Shape.setValue(result);
    }

    // collect transformation matrix cache entries
    std::unordered_set<std::string> caches;
    for(const auto &name : getDynamicPropertyNames()) {
        if(boost::starts_with(name,"Cache_"))
            caches.emplace(name);
    }

    // update transformation matrix cache
    for(auto &v : mats) {
        const char *name = cacheName(v.first);
        auto prop = getDynamicPropertyByName(name);
        if(!prop || !prop->isDerivedFrom(App::PropertyMatrix::getClassTypeId())) {
            if(prop)
                removeDynamicProperty(name);
            prop = addDynamicProperty("App::PropertyMatrix",name,"Cache",0,0,false,true);
        }
        caches.erase(name);
        static_cast<App::PropertyMatrix*>(prop)->setValue(v.second);
    }

    // remove any non-used cache entries.
    for(const auto &name : caches) {
        try {
            removeDynamicProperty(name.c_str());
        } catch(...) {}
    }
}

void SubShapeBinder::slotRecomputedObject(const App::DocumentObject& Obj) {
    if(Context.getValue() == &Obj
        && !this->testStatus(App::ObjectStatus::Recompute2))
    {
        update();
    }
}

App::DocumentObjectExecReturn* SubShapeBinder::execute(void) {
    if(BindMode.getValue()==0)
        update(UpdateForced);
    return inherited::execute();
}

void SubShapeBinder::onDocumentRestored() {
    if(_Version.getValue()<2)
        update(UpdateInit);
    inherited::onDocumentRestored();
}

void SubShapeBinder::onChanged(const App::Property *prop) {
    if(prop == &Context || prop == &Relative) {
        if(!Context.getValue() || !Relative.getValue()) {
            connRecomputedObj.disconnect();
        } else if(contextDoc != Context.getValue()->getDocument() 
                || !connRecomputedObj.connected()) 
        {
            contextDoc = Context.getValue()->getDocument();
            connRecomputedObj = contextDoc->signalRecomputedObject.connect(
                    boost::bind(&SubShapeBinder::slotRecomputedObject, this, _1));
        }
    }else if(!isRestoring()) {
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

    // Make Shape transient can reduce some file size, and maybe reduce file
    // loading time as well. But there maybe complication arise when doing
    // TopoShape version upgrade. So we DO NOT set trasient at the moment.
    //
    // Shape.setStatus(App::Property::Transient, !PartialLoad.getValue() && BindMode.getValue()==0);
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
            FC_THROWM(Base::ValueError, "Cyclic reference to " << v.first->getFullName());

        if(v.second.empty()) {
            v.second.push_back("");
            continue;
        }

        std::vector<std::string> wholeSubs;
        for(auto &sub : v.second) {
            if(sub.empty()) {
                wholeSubs.clear();
                v.second.resize(1);
                v.second[0].clear();
                break;
            }else if(sub[sub.size()-1] == '.')
                wholeSubs.push_back(sub);
        }
        for(auto &whole : wholeSubs) {
            for(auto it=v.second.begin();it!=v.second.end();) {
                auto &sub = *it;
                if(!boost::starts_with(sub,whole) || sub.size()==whole.size())
                    ++it;
                else {
                    FC_LOG("Remove subname " << sub <<" because of whole selection " << whole);
                    it = v.second.erase(it);
                }
            }
        }
    }

    if(!reset) {
        for(auto &link : Support.getSubListValues()) {
            auto subs = link.getSubValues();
            auto &s = values[link.getValue()];
            if(s.empty()) {
                s = std::move(subs);
                continue;
            }else if(subs.empty() || s[0].empty())
                continue;

            for(auto &sub : s) {
                for(auto it=subs.begin();it!=subs.end();) {
                    if(sub[sub.size()-1] == '.') {
                        if(boost::starts_with(*it,sub)) {
                            FC_LOG("Remove subname " << *it <<" because of whole selection " << sub);
                            it = subs.erase(it);
                        } else
                            ++it;
                    }else if(it->empty() 
                            || (it->back()=='.' && boost::starts_with(sub,*it)))
                    {
                        FC_LOG("Remove whole subname " << *it <<" because of " << sub);
                        it = subs.erase(it);
                    } else
                        ++it;
                }
            }
            subs.insert(subs.end(),s.begin(),s.end());
            s = std::move(subs);
        }
    }
    Support.setValues(std::move(values));
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

