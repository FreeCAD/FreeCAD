/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#	include <cassert>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Property.h"
#include "ObjectIdentifier.h"
#include "PropertyContainer.h"
#include <Base/Exception.h>
#include "Application.h"
#include "DocumentObject.h"

using namespace App;


//**************************************************************************
//**************************************************************************
// Property
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::Property , Base::Persistence)

//**************************************************************************
// Construction/Destruction

// Here is the implementation! Description should take place in the header file!
Property::Property()
  :father(0), propertySpec()
{

}

Property::~Property()
{

}

std::string Property::getName(void) const
{
    if(propertySpec)
        return propertySpec->Name;
    return "";
}

std::string Property::getFullName() const {
    std::string name;
    if (!getName().empty()) {
        if(father)
            name = father->getFullName() + ".";
        name += getName();
    }else
        return "?";
    return name;
}

const std::string Property::getGroup(void) const
{
    if(propertySpec)
        return propertySpec->Group;
    return "";
}

const std::string  Property::getDocumentation(void) const
{
    if(propertySpec)
        return propertySpec->Docu;
   return "";
}

void Property::setContainer(PropertyContainer *Father)
{
    father = Father;
}

void Property::setPathValue(const ObjectIdentifier &path, const boost::any &value)
{
    path.setValue(value);
}

const boost::any Property::getPathValue(const ObjectIdentifier &path) const
{
    return path.getValue();
}

void Property::getPaths(std::vector<ObjectIdentifier> &paths) const
{
    paths.emplace_back(getContainer(), getName());
}

ObjectIdentifier Property::canonicalPath(const ObjectIdentifier &p) const
{
    return p;
}

namespace App {
/*!
 * \brief The PropertyCleaner struct
 * Make deleting dynamic property safer by postponing its destruction.
 *
 * Dynamic property can be removed at any time, even during triggering of
 * onChanged() signal of the removing property. This patch introduced
 * static function Property::destroy() to make it safer by queueing any
 * removed property, and only deleting them when no onChanged() call is
 * active.
 */
struct PropertyCleaner {
    PropertyCleaner(Property *p)
        : prop(p)
    {
        ++_PropCleanerCounter;
    }
    ~PropertyCleaner() {
        if(--_PropCleanerCounter)
            return;
        bool found = false;
        while (_RemovedProps.size()) {
            auto p = _RemovedProps.back();
            _RemovedProps.pop_back();
            if(p != prop)
                delete p;
            else
                found = true;
        }

        if (found)
            _RemovedProps.push_back(prop);
    }
    static void add(Property *prop) {
        _RemovedProps.push_back(prop);
    }

    Property *prop;

    static std::vector<Property*> _RemovedProps;
    static int _PropCleanerCounter;
};
}

std::vector<Property*> PropertyCleaner::_RemovedProps;
int PropertyCleaner::_PropCleanerCounter = 0;

void Property::destroy(Property *p) {
    if (p) {
        // Is it necessary to nullify the container? May cause crash if any
        // onChanged() caller assumes a non-null container.
        //
        // p->setContainer(0);

        PropertyCleaner::add(p);
    }
}

void Property::hasSetValue(void)
{
    setStatus(Touched,false);
    setStatus(Touched);
}

void Property::aboutToSetValue(void)
{
    if (father)
        father->onBeforeChange(this);
}

void Property::verifyPath(const ObjectIdentifier &p) const
{
    p.verify(*this);
}

Property *Property::Copy(void) const
{
    // have to be reimplemented by a subclass!
    assert(0);
    return 0;
}

void Property::Paste(const Property& /*from*/)
{
    // have to be reimplemented by a subclass!
    assert(0);
}

void Property::onStatusChanged(const PropertyStatus& status, bool newValue ) {
    (void)newValue;
    if( father )
        father->onPropertyStatusChanged(*this,status);
    if( father && status == Touched && newValue)
    {
        PropertyCleaner guard(this);
        father->onChanged(this);
    }
}

//**************************************************************************
//**************************************************************************
// PropertyListsBase
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void PropertyListsBase::_setPyObject(PyObject *value) {
    std::vector<int> indices;
    std::vector<PyObject *> vals;
    Py::Object pySeq;

    if (PyDict_Check(value)) {
        Py::Dict dict(value);
        auto size = dict.size();
        vals.reserve(size);
        indices.reserve(size);
        int listSize = getSize();
        for(auto it=dict.begin();it!=dict.end();++it) {
            const auto &item = *it;
            PyObject *key = item.first.ptr();
#if PY_MAJOR_VERSION < 3
            if(!PyInt_Check(key))
#else
            if(!PyLong_Check(key))
#endif
                throw Base::TypeError("expect key type to be integer");
            long idx = PyLong_AsLong(key);
            if(idx<-1 || idx>listSize)
                throw Base::ValueError("index out of bound");
            if(idx==-1 || idx==listSize) {
                idx = listSize;
                ++listSize;
            }
            indices.push_back(idx);
            vals.push_back(item.second.ptr());
        }
    } else {
        if (PySequence_Check(value))
            pySeq = value;
        else {
            PyObject *iter = PyObject_GetIter(value);
            if(iter) {
                Py::Object pyIter(iter,true);
                pySeq = Py::asObject(PySequence_Fast(iter,""));
            } else {
                PyErr_Clear();
                vals.push_back(value);
            }
        }
        if(!pySeq.isNone()) {
            Py::Sequence seq(pySeq);
            vals.reserve(seq.size());
            for(auto it=seq.begin();it!=seq.end();++it)
                vals.push_back((*it).ptr());
        }
    }
    setPyValues(vals,indices);
}


//**************************************************************************
//**************************************************************************
// PropertyLists
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLists , App::Property)
