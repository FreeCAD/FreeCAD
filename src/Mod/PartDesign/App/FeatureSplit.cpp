/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <boost/range.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "Body.h"
#include "FeatureSplit.h"
#include "FeatureSolid.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <Mod/Part/App/PartPyCXX.h>

typedef boost::iterator_range<const char*> CharRange;

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Split, PartDesign::Feature)

Split::Split()
{
    ADD_PROPERTY_TYPE(Mode,((long)0),"Part Design", App::Prop_None,
            "Standard: wires, shells, compsolids remain in one piece.\n"
            "Split: wires, shells, compsolids are split.\n"
            "CompSolid: make compsolid from solid fragments.");
    static const char *ModeEnums[]= {"Standard","Split","CompSolid",NULL};
    Mode.setEnums(ModeEnums);

    ADD_PROPERTY_TYPE(Tolerance, (0), "Part Design", App::Prop_None,
        "Tolerance when intersecting (fuzzy value). In addition to tolerances of the shapes.");

    ADD_PROPERTY_TYPE(Fragment,(false),"Part Design",App::Prop_None,
            "Fragmentation will split both the base and tool shape with each other");

    ADD_PROPERTY_TYPE(Tools, (), "Part Design", App::Prop_None, "List of tool features");

    ADD_PROPERTY_TYPE(Solids, (), "Part Design",
            (App::PropertyType)(App::Prop_Hidden|App::Prop_ReadOnly), "List of solids");
    Solids.setScope(App::LinkScope::Hidden);
}

static inline PyObject* loadAPI(const char *name) {
    PyObject *mod = PyImport_ImportModule("BOPTools.SplitAPI");
    if(!mod) {
        PyErr_Clear();
    } else {
        Py::Object pyMod = Py::asObject(mod);
        if(pyMod.hasAttr(name))
            return Py::new_reference_to(pyMod.getAttr(name));
    }
    return Py_None;
}

App::DocumentObjectExecReturn *Split::execute(void)
{
    newSolidCount = 0;
    auto body = Body::findBodyOf(this);
    if(!body)
        return App::DocumentObject::StdReturn;

    auto shape = getBaseShape();
    if(Tools.getSize()) {
        std::vector<Part::TopoShape> tools;
        for(auto obj : Tools.getValues()) {
            tools.push_back(Part::Feature::getTopoShape(obj));
            if(tools.back().isNull())
                return new App::DocumentObjectExecReturn("Null tool shape");
        }
        Base::PyGILStateLocker lock;
        PyObject *func;
        Py::Object args;
        if(Fragment.getValue()) {
            static PyObject *_func;
            if(!_func) 
                _func = loadAPI("booleanFragments");
            func = _func;
            Py::List list(tools.size()+1);
            int i = 0;
            list.setItem(i++,Part::shape2pyshape(shape));
            for(auto &s : tools)
                list.setItem(i++,Part::shape2pyshape(s));
            Py::Tuple tuple(3);
            tuple.setItem(0,list);
            tuple.setItem(1,Py::String(Mode.getValueAsString()));
            tuple.setItem(2,Py::Float(Tolerance.getValue()));
            args = tuple;
        } else {
            static PyObject *_func;
            if(!_func) 
                _func = loadAPI("slice");
            func = _func;
            Py::List list(tools.size());
            int i = 0;
            for(auto &s : tools)
                list.setItem(i++,Part::shape2pyshape(s));
            Py::Tuple tuple(4);
            tuple.setItem(0,Part::shape2pyshape(shape));
            tuple.setItem(1,list);
            tuple.setItem(2,Py::String(Mode.getValueAsString()));
            tuple.setItem(3,Py::Float(Tolerance.getValue()));
            args = tuple;
        }
        if(func == Py_None)
            return new App::DocumentObjectExecReturn("Failed to load SplitAPI");
        try {
            Py::Object res = Py::Callable(func).apply(Py::Tuple(args));
            if(!PyObject_TypeCheck(res.ptr(),&Part::TopoShapePy::Type))
                return new App::DocumentObjectExecReturn("No shape returned");
            shape = *static_cast<Part::TopoShapePy*>(res.ptr())->getTopoShapePtr();
        } catch (Py::Exception &) {
            Base::PyException e;
            e.ReportException();
            return new App::DocumentObjectExecReturn("Split failed");
        }
    }

    auto solids = shape.getSubTopoShapes(TopAbs_SOLID);
    auto children = Solids.getValues();

    bool children_changed = false;
    for(auto it=children.begin();it!=children.end();) {
        auto child = *it;
        if(it-children.begin()>=(int)solids.size()
                || !child 
                || !child->getNameInDocument()
                || !child->isDerivedFrom(Solid::getClassTypeId()))
        {
            children_changed = true;
            if(child && child->getNameInDocument())
                getDocument()->removeObject(child->getNameInDocument());
            it = children.erase(it);
        } else
            ++it;
    }
    if(solids.empty())
        return new App::DocumentObjectExecReturn("No solid found");

    std::size_t oldCount = children.size();
    children.reserve(solids.size());
    std::string prefix(getNameInDocument());
    prefix += "_i";
    std::vector<Part::TopoShape> activeShapes;
    for(std::size_t i=0;i<solids.size();++i) {
        Solid *child;
        if(i<children.size()) {
            child = static_cast<Solid*>(children[i]);
            if(child->Active.getValue())
                activeShapes.push_back(solids[i]);
        } else {
            std::string name(prefix);
            name += std::to_string(i);
            child = static_cast<PartDesign::Solid*>(
                    getDocument()->addObject("PartDesign::Solid", name.c_str()));
            body->addObject(child);
            children.push_back(child);
            children_changed = true;
        }
        child->Parent.setValue(this);
        child->Shape.setValue(solids[i]);
    }
    if(activeShapes.empty()) {
        activeShapes.push_back(solids[0]);
        auto solid = static_cast<Solid*>(children[0]);
        Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
                App::Property::User3, &solid->Active);
        solid->Active.setValue(true);
    }

    newSolidCount = children.size() - oldCount;
    if(children_changed)
        Solids.setValues(children);

    Shape.setValue(TopoShape().makECompound(activeShapes,0,false));
    return App::DocumentObject::StdReturn;
}

void Split::afterRecompute() {
    const auto &solids = Solids.getValues();
    if(newSolidCount <= solids.size()) {
        auto count = solids.size();
        for(std::size_t i=count-newSolidCount;i<count;++i)
            solids[i]->purgeTouched();
    }
}

void Split::updateActiveSolid(Solid *caller) {
    if(Shape.isTouched()) {
        touch();
        return;
    }

    std::vector<Part::TopoShape> activeShapes;
    Solid *firstChild = nullptr;
    for(auto obj : Solids.getValues()) {
        auto child = Base::freecad_dynamic_cast<Solid>(obj);
        if(!child || child->Shape.getShape().isNull())
            continue;
        if(child->Active.getValue()) {
            auto shape = child->Shape.getShape();
            shape.Tag = 0;
            activeShapes.push_back(shape);
        } else if(!firstChild && child!=caller)
            firstChild = child;

    }
    if(activeShapes.empty()) {
        if(firstChild) {
            firstChild->Active.setValue(true);
            return;
        }
        // No active shapes found, touch to trigger recompute
        Solids.touch();
        return;
    }

    Shape.setValue(TopoShape().makECompound(activeShapes,0,false));

    if(Suppress.getValue()) {
        updateSuppressedShape();
        SuppressedShape.purgeTouched();
    }

    Shape.purgeTouched();
}

bool Split::isElementGenerated(const TopoShape &shape, const char *name) const
{
    bool res = false;
    long tag = 0;
    int depth = 2;
    shape.traceElement(name,
        [&] (const std::string &, size_t, long tag2) {
            if(tag && tag2!=tag) {
                if(--depth == 0)
                    return true;
            }
            for(auto obj : this->Tools.getValues()) {
                if(tag2 == obj->getID()) {
                    res = true;
                    return true;
                }
            }
            tag = tag2;
            return false;
        });

    return res;
}

bool Split::isToolAllowed(App::DocumentObject *obj, bool inside) const {
    if(obj->isDerivedFrom(PartDesign::Solid::getClassTypeId()))
        return false;
    if(!inside && Tools.find(obj->getNameInDocument()))
        return false;
    auto body = Body::findBodyOf(obj);
    if(!body)
        return false;
    if(body != Body::findBodyOf(obj))
        return false;
    if(!Body::isSolidFeature(obj))
        return true;
    return !body->isSibling(this,obj);
}

void Split::onChanged(const App::Property* prop) {
    if(!isRestoring() && !getDocument()->isPerformingTransaction()) {
        if(prop == &Tools) {
            if(!Tools.testStatus(App::Property::User3)) {
                Base::ObjectStatusLocker<App::Property::Status, App::Property>
                    guard(App::Property::User3, &Tools);
                Tools.removeIf([=](App::DocumentObject *obj){
                    return !this->isToolAllowed(obj,true);
                });
            }
        }
    }
    PartDesign::Feature::onChanged(prop);
}

App::DocumentObject *Split::getSubObject(const char *subname,
        PyObject **pyObj, Base::Matrix4D *mat, bool transform, int depth) const
{
    auto ret = PartDesign::Feature::getSubObject(subname,pyObj,mat,transform,depth);
    if(ret)
        return ret;

    const char *dot=0;
    if(!subname || !(dot=strchr(subname,'.'))) {
        ret = const_cast<Split*>(this);
    }else if(subname[0]=='$') {
        CharRange name(subname+1,dot);
        for(auto obj : Solids.getValues()) {
            if(boost::equals(name, obj->Label.getValue())) {
                ret = obj;
                break;
            }
        }
    } else if (Solids.getSize()<=10) {
        CharRange name(subname,dot);
        for(auto obj : Solids.getValues()) {
            if(obj && obj->getNameInDocument() && boost::equals(name,obj->getNameInDocument())) {
                ret = obj;
                break;
            }
        }
    } else
        ret = Solids.find(std::string(subname,dot));
    if(ret && dot)
        return ret->getSubObject(dot+1,pyObj,mat,true,depth+1);
    return ret;
}

int Split::isElementVisible(const char * element) const
{
    auto solid = Base::freecad_dynamic_cast<Solid>(
            Solids.find(element));
    if (solid)
        return solid->Active.getValue() ? 1 : 0;
    return -1;
}

int Split::setElementVisible(const char * element, bool visible)
{
    auto solid = Base::freecad_dynamic_cast<Solid>(
            Solids.find(element));
    if (solid) {
        if (!App::GetApplication().getActiveTransaction()) {
            App::AutoTransaction guard("Toggle split solid");
            solid->Active.setValue(visible);
        } else
            solid->Active.setValue(visible);
        return 1;
    }
    return 0;
}
