/***************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <boost_bind_bind.hpp>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/range.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

typedef boost::iterator_range<const char*> CharRange;

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Document.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/OriginFeature.h>
#include <App/Link.h>
#include <App/MappedElement.h>
#include <App/Part.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/App/FaceMakerBullseye.h>
#include "SubShapeBinder.h"
#include "BodyBase.h"

FC_LOG_LEVEL_INIT("Part",true,true)

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace Part;
namespace bp = boost::placeholders;
namespace bio = boost::iostreams;

// ============================================================================

PROPERTY_SOURCE(Part::SubShapeBinder, Part::Feature)

SubShapeBinder::SubShapeBinder()
{
    ADD_PROPERTY_TYPE(Support, (0), "",(App::PropertyType)(App::Prop_None), "Support of the geometry");
    // Support.setStatus(App::Property::ReadOnly, true);
    ADD_PROPERTY_TYPE(Fuse, (false), "Base",App::Prop_None,"Fuse solids from bound shapes");
    ADD_PROPERTY_TYPE(MakeFace, (true), "Base",App::Prop_None,"Create face using wires from bound shapes");
    ADD_PROPERTY_TYPE(FillStyle, (static_cast<long>(0)), "Base",App::Prop_None,
            "Face filling type when making curved face.\n\n"
            "Stretch: the style with the flattest patches.\n"
            "Coons: a rounded style of patch with less depth than those of Curved.\n"
            "Curved: the style with the most rounded patches.");
    static const char *FillStyleEnum[] = {"Stretch", "Coons", "Curved", 0};
    FillStyle.setEnums(FillStyleEnum);

    ADD_PROPERTY_TYPE(ClaimChildren, (false), "Base",App::Prop_Output,"Claim linked object as children");
    ADD_PROPERTY_TYPE(Relative, (true), "Base",App::Prop_None,"Enable relative sub-object binding");
    ADD_PROPERTY_TYPE(BindMode, ((long)0), "Base", App::Prop_None, 
            "Synchronized: auto update binder shape on changed of bound object.\n"
            "Frozen: disable auto update, but can be updated manually using context menu.\n"
            "Detached: copy the shape of bound object and then remove the binding immediately.");
    ADD_PROPERTY_TYPE(PartialLoad, (false), "Base", App::Prop_None,
            "Enable partial loading, which disables auto loading of external document for"
            "external bound object.");
    PartialLoad.setStatus(App::Property::PartialTrigger,true);
    static const char *BindModeEnum[] = {"Synchronized", "Frozen", "Detached", 0};
    BindMode.setEnums(BindModeEnum);

    static const char *BindCopyOnChangeEnum[] = {"Disabled", "Enabled", "Mutated", 0};
    BindCopyOnChange.setEnums(BindCopyOnChangeEnum);
    ADD_PROPERTY_TYPE(BindCopyOnChange, ((long)0), "Base", App::Prop_None,
            "Disabled: disable copy on change.\n"
            "Enabled: duplicate properties from binding object that are marked with 'CopyOnChange'.\n"
            "         Make internal copy of the object with any changed properties to obtain the\n"
            "         shape of an alternative configuration\n"
            "Mutated: indicate the binder has already mutated by changing any properties marked with\n"
            "         'CopyOnChange'. Those properties will no longer be kept in sync between the\n"
            "         binder and the binding object");

    ADD_PROPERTY_TYPE(Context, (0), "Base", App::Prop_Hidden,
            "Stores the context of this binder. It is used for monitoring and auto updating\n"
            "the relative placement of the bound shape");
    Context.setScope(App::LinkScope::Hidden);

    ADD_PROPERTY_TYPE(_Version,(0),"Base",(App::PropertyType)(
                App::Prop_Hidden|App::Prop_ReadOnly), "");

    _CopiedLink.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_CopiedLink,(0),"Base",(App::PropertyType)(
                App::Prop_Hidden|App::Prop_ReadOnly|App::Prop_NoPersist), "");

}

SubShapeBinder::~SubShapeBinder() {
    clearCopiedObjects();
}

void SubShapeBinder::setupObject() {
    _Version.setValue(8);
    checkPropertyStatus();
}

void SubShapeBinder::setupCopyOnChange() {
    copyOnChangeConns.clear();

    const auto &support = Support.getSubListValues();
    if(BindCopyOnChange.getValue()==0 || support.size()!=1) {
        if(hasCopyOnChange) {
            hasCopyOnChange = false;
            std::vector<App::Property*> props;
            getPropertyList(props);
            for(auto prop : props) {
                if(App::LinkBaseExtension::isCopyOnChangeProperty(this,*prop)) {
                    try {
                        removeDynamicProperty(prop->getName());
                    } catch (Base::Exception &e) {
                        e.ReportException();
                    } catch (...) {
                    }
                }
            }
        }
        return;
    }

    auto linked = support.front().getValue();
    hasCopyOnChange = App::LinkBaseExtension::setupCopyOnChange(this,linked,
        BindCopyOnChange.getValue()==1?&copyOnChangeConns:nullptr,hasCopyOnChange);
    if(hasCopyOnChange) {
        copyOnChangeConns.push_back(linked->signalChanged.connect(
            [this](const App::DocumentObject &, const App::Property &prop) {
                if(!prop.testStatus(App::Property::Output)
                        && !prop.testStatus(App::Property::PropOutput))
                {
                    if(this->_CopiedObjs.size()) {
                        FC_LOG("Clear binder " << getFullName() << " cache on change of "
                                << prop.getFullName());
                        this->clearCopiedObjects();
                    }
                }
            }
        ));
    }
}

void SubShapeBinder::clearCopiedObjects() {
    std::vector<App::DocumentObjectT> objs;
    objs.swap(_CopiedObjs);
    for(auto &o : objs) {
        auto obj = o.getObject();
        if(obj)
            obj->getDocument()->removeObject(obj->getNameInDocument());
    }
    _CopiedLink.setValue(0);
}

App::DocumentObject *SubShapeBinder::getSubObject(const char *subname, PyObject **pyObj,
        Base::Matrix4D *mat, bool transform, int depth) const
{
    auto sobj = Part::Feature::getSubObject(subname,pyObj,mat,transform,depth);
    if(sobj)
        return sobj;
    if(Data::ComplexGeoData::findElementName(subname)==subname)
        return nullptr;

    const char *dot = strchr(subname, '.');
    if(!dot)
        return nullptr;

    if (!App::GetApplication().checkLinkDepth(depth))
        return nullptr;

    std::string name(subname,dot-subname);
    for(auto &l : Support.getSubListValues()) {
        auto obj = l.getValue();
        if(!obj || !obj->getNameInDocument())
            continue;
        for(auto &sub : l.getSubValues()) {
            auto sobj = obj->getSubObject(sub.c_str());
            if(!sobj || !sobj->getNameInDocument())
                continue;
            if(subname[0] == '$') {
                if(sobj->Label.getStrValue() != name.c_str()+1)
                    continue;
            } else if(!boost::equals(sobj->getNameInDocument(), name))
                continue;
            name = Data::ComplexGeoData::noElementName(sub.c_str());
            name += dot+1;
            if(mat && transform)
                *mat *= Placement.getValue().toMatrix();
            return obj->getSubObject(name.c_str(),pyObj,mat,true,depth+1);
        }
    }
    return nullptr;
}

void SubShapeBinder::update(SubShapeBinder::UpdateOption options) {
    Part::TopoShape result;
    if(_Version.getValue()>2 && getDocument())
        result = Part::TopoShape(0, getDocument()->getStringHasher());

    std::vector<Part ::TopoShape> shapes;
    std::vector<std::pair<int,int> > shapeOwners;
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
    int idx = -1;
    for(auto &l : Support.getSubListValues()) {
        ++idx;
        auto obj = l.getValue();
        if(!obj || !obj->getNameInDocument()) {
            if (isRecomputing())
                FC_THROWM(Base::RuntimeError,"Missing support object");
            continue;
        }
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

        App::DocumentObject *copied = 0;

        if(BindCopyOnChange.getValue() == 2 && Support.getSubListValues().size()==1) {
            if(_CopiedObjs.size())
               copied = _CopiedObjs.front().getObject();

            bool recomputeCopy = false;
            int copyerror = 0;
            if(!copied || !copied->isValid()) {
                recomputeCopy = true;
                clearCopiedObjects();

                auto tmpDoc = App::GetApplication().newDocument(
                                "_tmp_binder", 0, false, true);
                auto objs = tmpDoc->copyObject({obj},true,true);
                if(objs.size()) {
                    for(auto it=objs.rbegin(); it!=objs.rend(); ++it)
                        _CopiedObjs.emplace_back(*it);
                    copied = objs.back();
                    // IMPORTANT! must make a recomputation first before any
                    // further change so that we can generate the correct
                    // geometry element map.
                    if (!copied->recomputeFeature(true))
                        copyerror = 1;
                }
            }

            if(copied) {
                if (!copyerror) {
                    std::vector<App::Property*> props;
                    getPropertyList(props);
                    for(auto prop : props) {
                        if(!App::LinkBaseExtension::isCopyOnChangeProperty(this,*prop))
                            continue;
                        auto p = copied->getPropertyByName(prop->getName());
                        if(p && p->getContainer()==copied
                                && p->getTypeId()==prop->getTypeId()
                                && !p->isSame(*prop)) 
                        {
                            recomputeCopy = true;
                            std::unique_ptr<App::Property> pcopy(prop->Copy());
                            p->Paste(*pcopy);
                        }
                    }
                    if(recomputeCopy && !copied->recomputeFeature(true))
                        copyerror = 2;
                }
                obj = copied;
                _CopiedLink.setValue(copied,l.getSubValues(false));
                if (copyerror) {
                    FC_THROWM(Base::RuntimeError,
                            (copyerror == 1 ? "Initial copy failed." : "Copy on change failed.")
                             << " Please check report view for more details.\n"
                             "You can show temporary document to reveal the failed objects using\n"
                             "tree view context menu.");
                }
            }
        }

        const auto &subvals = copied?_CopiedLink.getSubValues():l.getSubValues();
        int sidx = copied?-1:idx;
        int subidx = -1;
        std::set<std::string> subs(subvals.begin(),subvals.end());
        static std::string none("");
        if(subs.empty())
            subs.insert(none);
        else if(subs.size()>1)
            subs.erase(none);
        for(const auto &sub : subs) {
            ++subidx;
            try {
                auto shape = Part::Feature::getTopoShape(obj,sub.c_str(),true);
                if(shape.isNull())
                    throw Part::NullShapeException("Null shape");
                shapes.push_back(shape);
                shapeOwners.emplace_back(sidx, subidx);
                shapeMats.push_back(&res.first->second);
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
            // Notify user about restore error
            // if(!(options & UpdateInit))
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

        std::ostringstream ss;
        idx = -1;
        for(auto &shape : shapes) {
            ++idx;
            if(shape.Hasher
                    && shape.getElementMapSize()
                    && shape.Hasher != getDocument()->getStringHasher())
            {
                ss.str("");
                ss << TOPOP_SHAPEBINDER << ':' << shapeOwners[idx].first
                   << ':' << shapeOwners[idx].second;
                shape.reTagElementMap(-getID(),
                        getDocument()->getStringHasher(),ss.str().c_str());
            }
            if (!shape.hasSubShape(TopAbs_FACE) && shape.hasSubShape(TopAbs_EDGE))
                shape = shape.makECopy();
        }
        
        if(shapes.size()==1 && !Relative.getValue())
            shapes.back().setPlacement(Base::Placement());
        else {
            for(size_t i=0;i<shapes.size();++i) {
                auto &shape = shapes[i];
                shape = shape.makETransform(*shapeMats[i]);
            }
        }

        if(shapes.empty()) {
            Shape.resetElementMapVersion();
            return;
        }

        result.makECompound(shapes);

        if(MakeFace.getValue() && !result.hasSubShape(TopAbs_FACE)) {
            try {
                if (_Version.getValue()>4
                    && !result.hasSubShape(TopAbs_EDGE)
                    && result.countSubShapes(TopAbs_VERTEX) > 1) {
                    std::vector<Part::TopoShape> edges;
                    Part::TopoShape first, prev;
                    for (auto &vertex : result.getSubTopoShapes(TopAbs_VERTEX)) {
                        if (prev.isNull()) {
                            first = vertex;
                            prev = vertex;
                            continue;
                        }
                        BRepBuilderAPI_MakeEdge builder(
                                BRep_Tool::Pnt(TopoDS::Vertex(prev.getShape())),
                                BRep_Tool::Pnt(TopoDS::Vertex(vertex.getShape())));
                        Part::TopoShape edge(builder.Shape());
                        edge.mapSubElement({prev, vertex});
                        edges.push_back(edge);
                        prev = vertex;
                    }
                    auto p1 = BRep_Tool::Pnt(TopoDS::Vertex(prev.getShape()));
                    auto p2 = BRep_Tool::Pnt(TopoDS::Vertex(first.getShape()));
                    if (result.countSubShapes(TopAbs_VERTEX) > 2
                            && p1.SquareDistance(p2) > Precision::SquareConfusion()) {
                        BRepBuilderAPI_MakeEdge builder(p1, p2);
                        Part::TopoShape edge(builder.Shape());
                        edge.mapSubElement({prev, first});
                        edges.push_back(edge);
                    }
                    result.makEWires(edges);
                }
                else if (result.hasSubShape(TopAbs_EDGE))
                    result = result.makEWires();
            } catch(Base::Exception & e) {
                FC_LOG(getFullName() << " Failed to make wire: " << e.what());
            } catch(Standard_Failure & e) {
                FC_LOG(getFullName() << " Failed to make wire: " << e.GetMessageString());
            } catch(...) {
                FC_LOG(getFullName() << " Failed to make wire");
            }

            if (result.hasSubShape(TopAbs_WIRE)) {
                bool done = false;
                gp_Pln pln;
                if (_Version.getValue() > 4 && !result.findPlane(pln)) {
                    try {
                        Part::TopoShape filledFace(-getID(), getDocument()->getStringHasher());
                        filledFace.makEFilledFace({result}, Part::TopoShape());
                        if (filledFace.hasSubShape(TopAbs_FACE)) {
                            done = true;
                            result = filledFace;
                        }
                    } catch(Base::Exception & e) {
                        FC_LOG(getFullName() << " Failed to make filled face: " << e.what());
                    } catch(Standard_Failure & e) {
                        FC_LOG(getFullName() << " Failed to make filled face: " << e.GetMessageString());
                    } catch(...) {
                        FC_LOG(getFullName() << " Failed to make filled face");
                    }

                    if (!result.hasSubShape(TopAbs_FACE)) {
                        try {
                            result = result.makEBSplineFace(
                                    static_cast<Part::TopoShape::FillingStyle>(FillStyle.getValue()));
                        } catch(Base::Exception & e) {
                            FC_LOG(getFullName() << " Failed to make bspline face: " << e.what());
                        } catch(Standard_Failure & e) {
                            FC_LOG(getFullName() << " Failed to make bspline face: " << e.GetMessageString());
                        } catch(...) {
                            FC_LOG(getFullName() << " Failed to make bspline face");
                        }
                    }
                }
                if (!done) {
                    try {
                        result = result.makEFace(0);
                        done = true;
                    } catch(Base::Exception & e) {
                        FC_LOG(getFullName() << " Failed to make face: " << e.what());
                    } catch(Standard_Failure & e) {
                        FC_LOG(getFullName() << " Failed to make face: " << e.GetMessageString());
                    } catch(...) {
                        FC_LOG(getFullName() << " Failed to make face");
                    }

                    if (!done && _Version.getValue() > 7) {
                        try {
                            result = result.makERuledSurface(result.getSubTopoShapes(TopAbs_WIRE));
                        } catch(Base::Exception & e) {
                            FC_LOG(getFullName() << " Failed to make ruled face: " << e.what());
                        } catch(Standard_Failure & e) {
                            FC_LOG(getFullName() << " Failed to make ruled face: " << e.GetMessageString());
                        } catch(...) {
                            FC_LOG(getFullName() << " Failed to make ruled face");
                        }
                    }
                }
            }
        }

        if(Fuse.getValue()) {
            if (_Version.getValue() > 4
                    && !result.hasSubShape(TopAbs_SOLID)
                    && result.hasSubShape(TopAbs_FACE))
            {
                if (!result.hasSubShape(TopAbs_SHELL)) {
                    try {
                        result = result.makEShell();
                    } catch(Base::Exception & e) {
                        FC_LOG(getFullName() << " Failed to make shell: " << e.what());
                    } catch(Standard_Failure & e) {
                        FC_LOG(getFullName() << " Failed to make shell: " << e.GetMessageString());
                    } catch(...) {
                        FC_LOG(getFullName() << " Failed to make shell");
                    }
                }

                if (result.hasSubShape(TopAbs_SHELL)) {
                    try {
                        result = result.makESolid();
                    } catch(Base::Exception & e) {
                        FC_LOG(getFullName() << " Failed to make solid: " << e.what());
                    } catch(Standard_Failure & e) {
                        FC_LOG(getFullName() << " Failed to make solid: " << e.GetMessageString());
                    } catch(...) {
                        FC_LOG(getFullName() << " Failed to make solid");
                    }
                }
            }
            if (result.hasSubShape(TopAbs_SOLID)) {
                // If the compound has solid, fuse them together, and ignore other type of
                // shapes
                auto solids = result.getSubTopoShapes(TopAbs_SOLID);
                if(solids.size() > 1) {
                    try {
                        result.makEFuse(solids);
                        result = result.makERefine();
                    } catch(Base::Exception & e) {
                        FC_LOG(getFullName() << " Failed to fuse: " << e.what());
                    } catch(Standard_Failure & e) {
                        FC_LOG(getFullName() << " Failed to fuse: " << e.GetMessageString());
                    } catch(...) {
                        FC_LOG(getFullName() << " Failed to fuse");
                    }
                } else {
                    // wrap the single solid in compound to keep its placement
                    result.makECompound({solids.front().makERefine()});
                }
            } else if (result.hasSubShape(TopAbs_SHELL))
                result = result.makERefine();
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

App::DocumentObject *SubShapeBinder::getElementOwner(const Data::MappedName & name) const
{
    if(!name)
        return nullptr;

    static std::string op(TOPOP_SHAPEBINDER ":");
    int offset = name.rfind(op);
    if (offset < 0)
        return nullptr;
    
    int idx, subidx;
    char sep;
    int size;
    const char * s = name.toConstString(offset+op.size(), size);
    bio::stream<bio::array_source> iss(s, size);
    if (!(iss >> idx >> sep >> subidx) || sep!=':' || subidx<0)
        return nullptr;

    const App::PropertyXLink *link = nullptr;
    if(idx < 0)
        link = &_CopiedLink;
    else if (idx < Support.getSize()) {
        int i=0;
        for(auto &l : Support.getSubListValues()) {
            if(i++ == idx)
                link = &l;
        }
    }
    if(!link || !link->getValue())
        return nullptr;

    const auto &subs = link->getSubValues();
    if(subidx < (int)subs.size())
        return link->getValue()->getSubObject(subs[subidx].c_str());

    if(subidx == 0 && subs.empty())
        return link->getValue();

    return nullptr;
}

void SubShapeBinder::slotRecomputedObject(const App::DocumentObject& Obj) {
    if(Context.getValue() == &Obj
        && !this->testStatus(App::ObjectStatus::Recompute2))
    {
        try {
            update();
        } catch (Base::Exception &e) {
            e.ReportException();
        }
    }
}

static const char _GroupPrefix[] = "Configuration (";

App::DocumentObjectExecReturn* SubShapeBinder::execute(void) {

    setupCopyOnChange();

    if(BindMode.getValue()==0)
        update(UpdateForced);

    return inherited::execute();
}

void SubShapeBinder::onDocumentRestored() {
    if(_Version.getValue()<2)
        update(UpdateInit);
    else if (_Version.getValue()<4
            && BindMode.getValue()==0
            && !testStatus(App::ObjectStatus::PartialObject))
    {
        // Older version SubShapeBinder does not treat missing sub object as
        // error, which may cause noticeable error to user. We'll perform an
        // explicit check here, and raise exception if necessary.
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
                if(!obj->getSubObject(sub.c_str())) {
                    if (obj->getDocument() != getDocument())
                        FC_THROWM(Base::RuntimeError,
                                "Failed to get sub-object " << obj->getFullName() << "." << sub);
                    else
                        FC_THROWM(Base::RuntimeError,
                                "Failed to get sub-object " << obj->getNameInDocument() << "." << sub);
                }
            }
        }
    }
    if (_Version.getValue() < 6)
        collapseGeoChildren();
    inherited::onDocumentRestored();
}

void SubShapeBinder::collapseGeoChildren()
{
    // Geo children, i.e. children of GeoFeatureGroup may group some tool
    // features under itself but does not function as a container. In addition,
    // its parent group can directly reference the tool feature grouped without
    // referencing the child. The purpose of this function is to remove any
    // intermediate Non group features in the object path to avoid unnecessary
    // dependencies.
    if (Support.testStatus(App::Property::User3))
        return;

    Base::ObjectStatusLocker<App::Property::Status, App::Property>
        guard(App::Property::User3, &Support);
    App::PropertyXLinkSubList::atomic_change guard2(Support, false);

    std::vector<App::DocumentObject*> removes;
    std::map<App::DocumentObject*, std::vector<std::string> > newVals;
    for(auto &l : Support.getSubListValues()) {
        auto obj = l.getValue();
        if(!obj || !obj->getNameInDocument())
            continue;
        auto subvals = l.getSubValues();
        if (subvals.empty())
            continue;
        bool touched = false;
        for (auto itSub=subvals.begin(); itSub!=subvals.end();) {
            auto &sub = *itSub;
            bool changed = false;
            auto objs = obj->getSubObjectList(sub.c_str());
            for (auto it=objs.begin(); it+1!=objs.end();) {
                auto group = App::GeoFeatureGroupExtension::getGroupOfObject(*it);
                if (!group || (*it)->hasExtension(
                            App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
                    ++it;
                    continue;
                }
                if (it==objs.begin()) {
                    changed = true;
                    it = objs.erase(it);
                    continue;
                }
                auto next = *(it+1);
                auto prev = *(it-1);
                std::string subname(next->getNameInDocument());
                subname += ".";
                if (prev->getLinkedObject(true) == group
                        && prev->getSubObject(subname.c_str()) == next) {
                    it = objs.erase(it);
                    changed = true;
                } else
                    ++it;
            }
            if  (changed) {
                touched = true;
                const char *element = Data::ComplexGeoData::findElementName(sub.c_str());
                App::SubObjectT sobjT(objs, element);
                sub = sobjT.getSubName();
                if (objs.front() != obj) {
                    newVals[objs.front()].push_back(std::move(sub));
                    itSub = subvals.erase(itSub);
                    continue;
                }
            }
            ++itSub;
        }
        if (touched)
            removes.push_back(obj);
        if (!subvals.empty() && touched) {
            auto &newSubs = newVals[obj];
            if (newSubs.empty())
                newSubs = std::move(subvals);
            else
                newSubs.insert(newSubs.end(),
                                std::make_move_iterator(subvals.begin()),
                                std::make_move_iterator(subvals.end()));
        }
    }
    for (auto obj : removes)
        Support.removeValue(obj);
    if (newVals.size())
        setLinks(std::move(newVals));
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
                    boost::bind(&SubShapeBinder::slotRecomputedObject, this, bp::_1));
        }
    }else if(!isRestoring() && !getDocument()->isPerformingTransaction()) {
        if(prop == &Support) {
            if (!Support.testStatus(App::Property::User3)) {
                collapseGeoChildren();
                clearCopiedObjects();
                setupCopyOnChange();
                if(Support.getSubListValues().size())
                    update(); 
            }
        }else if(prop == &BindCopyOnChange) {
            setupCopyOnChange();
        }else if(prop == &BindMode) {
           if(BindMode.getValue() == 0)
               update();
           checkPropertyStatus();
        }else if(prop == &PartialLoad) {
           checkPropertyStatus();
        }else if(prop && !prop->testStatus(App::Property::User3)) {
            checkCopyOnChange(*prop);
        }

        if(prop == &BindMode || prop == &Support) {
            if(BindMode.getValue()==2 && Support.getSubListValues().size())
                Support.setValue(0);
        }

        if (prop == &Label)
            slotLabelChanged();
    }

    // Regardless of restoring or undo, we'll check the support for label
    // synchronization
    if (prop == &Support && _Version.getValue() >= 7) {
        connLabelChange.disconnect();
        if (Support.getValue()) {
            auto &xlink = Support.getSubListValues().front();
            const auto &subs = xlink.getSubValues();
            auto linked = xlink.getValue();
            if (linked) {
                linked = linked->getSubObject(subs.empty() ? "" : subs[0].c_str());
                if (linked) {
                    connLabelChange = linked->Label.signalChanged.connect(
                            boost::bind(&SubShapeBinder::slotLabelChanged, this));
                    slotLabelChanged();
                }
            }
        }
    }

    inherited::onChanged(prop);
}

void SubShapeBinder::slotLabelChanged()
{
    if (!getDocument()
            || !getNameInDocument()
            || isRestoring()
            || getDocument()->isPerformingTransaction()
            || _Version.getValue() < 7)
        return;

    std::string prefix = getNameInDocument();
    prefix += "(";
    if (Label.getStrValue() != getNameInDocument()
            && !boost::starts_with(Label.getStrValue(), prefix))
        return;

    if (Support.getSize()) {
        auto &xlink = Support.getSubListValues().front();
        const auto &subs = xlink.getSubValues();
        auto linked = xlink.getValue()->getSubObject(subs.empty() ? "" : subs[0].c_str());
        if (linked) {
            std::string label;
            if (linked->isDerivedFrom(SubShapeBinder::getClassTypeId())
                    || linked->isDerivedFrom(App::Link::getClassTypeId())) {
                std::string p = linked->getNameInDocument();
                p += "(";
                if (boost::starts_with(linked->Label.getValue(), p)) {
                    const char *linkedLabel = linked->Label.getValue() + p.size();
                    while (*linkedLabel == '*')
                        ++linkedLabel;
                    label = prefix + "*" + linkedLabel;
                    if (boost::ends_with(label, ")"))
                        label.resize(label.size()-1);
                    else if (boost::ends_with(label, ")...")) {
                        label.resize(label.size()-4);
                        label += "...";
                    }
                }
            }
            if (label.empty())
                label = prefix + linked->Label.getValue();
            if (Support.getSize() > 1 || subs.size() > 1)
                label += ")...";
            else
                label += ")";
            Label.setValue(label);
            return;
        }
    }
    Label.setValue(prefix + "?)");
}

void SubShapeBinder::checkCopyOnChange(const App::Property &prop) {
    if(BindCopyOnChange.getValue()!=1
            || getDocument()->isPerformingTransaction()
            || !App::LinkBaseExtension::isCopyOnChangeProperty(this,prop)
            || Support.getSubListValues().size()!=1)
        return;

    auto linked = Support.getSubListValues().front().getValue();
    if(!linked)
        return;
    auto linkedProp = linked->getPropertyByName(prop.getName());
    if(linkedProp && linkedProp->getTypeId()==prop.getTypeId() && !linkedProp->isSame(prop))
        BindCopyOnChange.setValue(2);
}

void SubShapeBinder::checkPropertyStatus() {
    Support.setAllowPartial(PartialLoad.getValue());

    // Make Shape transient can reduce some file size, and maybe reduce file
    // loading time as well. But there maybe complication arise when doing
    // TopoShape version upgrade. So we DO NOT set trasient at the moment.
    //
    // Shape.setStatus(App::Property::Transient, !PartialLoad.getValue() && BindMode.getValue()==0);
}

void SubShapeBinder::setLinks(std::map<App::DocumentObject *,
                              std::vector<std::string> >&&values,
                              bool reset)
{
    if(values.empty()) {
        if(reset) {
            Shape.setValue(Part::TopoShape());
            Support.setValue(0);
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
            auto linkedObj = link.getValue();
            if (!linkedObj || !linkedObj->getNameInDocument()) {
                FC_WARN("Discard missing support: " 
                        << link.getObjectName() << " " << link.getDocumentPath());
                continue;
            }
            auto subs = link.getSubValues();
            auto &s = values[linkedObj];
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

App::DocumentObject *SubShapeBinder::_getLinkedObject(
        bool recurse, Base::Matrix4D *mat, bool transform, int depth) const
{
    if (mat && transform) 
        *mat *= Placement.getValue().toMatrix();

    auto self = const_cast<SubShapeBinder*>(this);

    const auto &supports = Support.getSubListValues();
    if (supports.empty())
        return self;

    auto &link = supports.front();
    if (!link.getValue())
        return self;

    auto sobj = link.getValue()->getSubObject(link.getSubName(), nullptr, mat, true, depth+1);
    if (!sobj)
        return self;
    if (!recurse || sobj == link.getValue())
        return sobj;

    // set transform to false, because the above getSubObject() already include
    // the transform of sobj
    sobj = sobj->getLinkedObject(true, mat, false, depth+1);

    auto binder = Base::freecad_dynamic_cast<SubShapeBinder>(sobj);
    if (!binder)
        return sobj;

    return binder->_getLinkedObject(true, mat, false, depth+1);
}

App::SubObjectT
SubShapeBinder::import(const App::SubObjectT &feature, 
                       const App::SubObjectT &editObjT,
                       bool importWholeObject,
                       bool noSubElement,
                       bool compatible,
                       bool noSubObject)
{
    App::DocumentObject *editObj = nullptr;
    App::DocumentObject *container = nullptr;
    App::DocumentObject *topParent = nullptr;
    std::string subname = editObjT.getSubNameNoElement();

    if (noSubElement) {
        noSubObject = true;
        importWholeObject = false;
    }

    editObj = editObjT.getSubObject();
    if (!editObj)
        FC_THROWM(Base::RuntimeError, "No editing object");
    else {
        auto objs = editObjT.getSubObjectList();
        topParent = objs.front();
        for (auto rit = objs.rbegin(); rit != objs.rend(); ++rit) {
            auto obj = (*rit)->getLinkedObject();
            if (obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
                container = obj;
                break;
            }
        }
    }
    if (!container && !feature.hasSubObject()) {
        container = Part::BodyBase::findBodyOf(editObj);
        if (!container) {
            container = App::Part::getPartOfObject(editObj);
            if (!container
                    && editObjT.getDocumentName() == feature.getDocumentName())
                return feature;
        }
    }
    auto sobj = feature.getSubObject();
    if (!sobj)
        FC_THROWM(Base::RuntimeError,
                "Sub object not found: " << feature.getSubObjectFullName());
    if (sobj == editObj || editObj->getInListEx(true).count(sobj))
        FC_THROWM(Base::RuntimeError,
                "Cyclic reference to: " << feature.getSubObjectFullName());
    auto link = feature.getObject();

    const char *featName = "Import";
    App::SubObjectT resolved;
    if (!container)
        resolved = feature;
    else {
        std::string linkSub = feature.getSubName();
        topParent->resolveRelativeLink(subname, link, linkSub);
        if (!link)
            FC_THROWM(Base::RuntimeError,
                    "Failed to resolve relative link: "
                    << editObjT.getSubObjectFullName() << " -> "
                    << feature.getSubObjectFullName());

        resolved = App::SubObjectT(link, linkSub.c_str());
        if (link == container
                || Part::BodyBase::findBodyOf(link) == container
                || App::Part::getPartOfObject(link) == container) {
            if ((!noSubElement || !resolved.hasSubElement())
                    && (!noSubObject || !resolved.hasSubObject()))
                return App::SubObjectT(sobj, feature.getElementName());
            featName = "Binder";
        }
    }

    std::string resolvedSub = resolved.getSubNameNoElement();
    if (!importWholeObject)
        resolvedSub += resolved.getOldElementName();
    std::string element;
    if (importWholeObject)
        element = feature.getNewElementName();

    App::Document *doc;
    std::vector<App::DocumentObject*> objs;

    // Try to find an unused import of the same object
    App::GeoFeatureGroupExtension *group = nullptr;
    if (container) {
        doc = container->getDocument();
        group = container->getExtensionByType<App::GeoFeatureGroupExtension>(true);
        if (!group)
            FC_THROWM(Base::RuntimeError, "Invalid container: " << container->getFullName());
        objs = group->Group.getValue();
    } else {
        doc = editObj->getDocument();
        for (auto obj : doc->getObjectsOfType(SubShapeBinder::getClassTypeId())) {
            if (obj != editObj && !App::GeoFeatureGroupExtension::getGroupOfObject(obj))
                objs.push_back(obj);
        }
    }

    for (auto o : objs) {
        auto binder = Base::freecad_dynamic_cast<Part::SubShapeBinder>(o);
        if (!binder || (!boost::starts_with(o->getNameInDocument(), "Import")
                        && !boost::starts_with(o->getNameInDocument(), "BaseFeature")))
            continue;
        if (binder->Support.getSize() != 1)
            continue;
        auto &binderSupport = binder->Support.getSubListValues().front();
        const auto &subs = binderSupport.getSubValues(false);
        if (subs.size() > 1
                || binderSupport.getValue() != resolved.getObject()
                || (resolvedSub.size() && subs.empty())
                || (!subs.empty() && resolvedSub != subs[0]))
            continue;
        if (element.size()) {
            auto res = Part::Feature::getElementFromSource(binder, "", sobj, element.c_str(), true);
            if (res.size()) {
                std::string tmp;
                return App::SubObjectT(binder, res.front().index.toString(tmp));
            }
            FC_WARN("Failed to deduce bound geometry from existing import: " << binder->getFullName());
        } else
            return binder;
    }

    struct Cleaner {
        App::DocumentObjectT objT;
        Cleaner(App::DocumentObject *obj)
            :objT(obj)
        {}
        ~Cleaner()
        {
            auto doc = objT.getDocument();
            if (doc)
                doc->removeObject(objT.getObjectName().c_str());
        }
        void release()
        {
            objT = App::DocumentObjectT();
        }
    };

    auto binder = static_cast<Part::SubShapeBinder*>(
            doc->addObject(compatible ?
                "PartDesign::SubShapeBinder" : "Part::SubShapeBinder", featName));
    Cleaner guard(binder);
    binder->Visibility.setValue(false);
    if (group)
        group->addObject(binder);
    std::map<App::DocumentObject*, std::vector<std::string> > support;
    auto &supportSubs = support[link];
    if (resolvedSub.size())
        supportSubs.push_back(std::move(resolvedSub));
    binder->setLinks(std::move(support));
    if (element.size()) {
        doc->recomputeFeature(binder);
        auto res = Part::Feature::getElementFromSource(binder, "", sobj, element.c_str(), true);
        if (res.size()) {
            std::string tmp;
            guard.release();
            return App::SubObjectT(binder, res.front().index.toString(tmp));
        }
        FC_THROWM(Base::RuntimeError, "Failed to deduce bound geometry");
    }
    guard.release();
    return binder;
}
