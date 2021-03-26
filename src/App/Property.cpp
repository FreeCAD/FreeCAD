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
  :father(0), myName(0)
{

}

Property::~Property()
{

}

const char* Property::getName(void) const
{
    return myName;
}

std::string Property::getFullName() const {
    std::string name;
    if(myName) {
        if(father)
            name = father->getFullName() + ".";
        name += myName;
    }else
        return "?";
    return name;
}

short Property::getType(void) const
{
    short type = 0;
#define GET_PTYPE(_name) do {\
        if(testStatus(App::Property::Prop##_name)) type|=Prop_##_name;\
    }while(0)
    GET_PTYPE(ReadOnly);
    GET_PTYPE(Hidden);
    GET_PTYPE(Output);
    GET_PTYPE(Transient);
    GET_PTYPE(NoRecompute);
    GET_PTYPE(NoPersist);
    return type;
}

void Property::syncType(unsigned type) {
#define SYNC_PTYPE(_name) do{\
        if(type & Prop_##_name) StatusBits.set((size_t)Prop##_name);\
    }while(0)
    SYNC_PTYPE(ReadOnly);
    SYNC_PTYPE(Transient);
    SYNC_PTYPE(Hidden);
    SYNC_PTYPE(Output);
    SYNC_PTYPE(NoRecompute);
    SYNC_PTYPE(NoPersist);
}

const char* Property::getGroup(void) const
{
    return father->getPropertyGroup(this);
}

const char* Property::getDocumentation(void) const
{
    return father->getPropertyDocumentation(this);
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

void Property::touch()
{
    PropertyCleaner guard(this);
    if (father)
        father->onChanged(this);
    StatusBits.set(Touched);
}

void Property::setReadOnly(bool readOnly)
{
    this->setStatus(App::Property::ReadOnly, readOnly);
}

void Property::hasSetValue(void)
{
    PropertyCleaner guard(this);
    if (father)
        father->onChanged(this);
    StatusBits.set(Touched);
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

void Property::setStatusValue(unsigned long status) {
    static const unsigned long mask =
        (1<<PropDynamic)
        |(1<<PropNoRecompute)
        |(1<<PropReadOnly)
        |(1<<PropTransient)
        |(1<<PropOutput)
        |(1<<PropHidden);

    status &= ~mask;
    status |= StatusBits.to_ulong() & mask;
    unsigned long oldStatus = StatusBits.to_ulong();
    StatusBits = decltype(StatusBits)(status);

    if(father) {
        static unsigned long _signalMask = (1<<ReadOnly) | (1<<Hidden);
        if((status & _signalMask) != (oldStatus & _signalMask))
            father->onPropertyStatusChanged(*this,oldStatus);
    }
}

void Property::setStatus(Status pos, bool on) {
    auto bits = StatusBits;
    bits.set(pos,on);
    setStatusValue(bits.to_ulong());
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
