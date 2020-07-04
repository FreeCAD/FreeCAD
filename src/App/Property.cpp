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

#include <atomic>

#include <boost/algorithm/string/predicate.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Property.h"
#include "ObjectIdentifier.h"
#include "PropertyContainer.h"
#include "Transactions.h"
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/Console.h>
#include "Application.h"
#include "DocumentParams.h"
#include "DocumentObject.h"

FC_LOG_LEVEL_INIT("App",true,true)

using namespace App;


//**************************************************************************
//**************************************************************************
// Property
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::Property , Base::Persistence)

//**************************************************************************
// Construction/Destruction

static std::atomic<long> _PropID;

// Here is the implementation! Description should take place in the header file!
Property::Property()
  :father(0), myName(0), _id(++_PropID)
{
}

Property::~Property()
{
    Transaction::removePendingProperty(this);
}

const char* Property::getName(void) const
{
    return myName;
}

std::string Property::getFullName(bool python) const {
    if(!myName || (python && !father)) 
        return std::string(python?"None":"?");
    std::ostringstream ss;
    if(father)
        ss << father->getFullName(python) 
            << '.' << father->getPropertyPrefix();
    ss << myName;
    return ss.str();
}

std::string Property::getFileName(const char *postfix, const char *prefix) const {
    std::ostringstream ss;
    if(prefix)
        ss << prefix;
    if(!myName)
        ss << "Property";
    else {
        std::string name = getFullName();
        auto pos = name.find('#');
        if(pos == std::string::npos)
            ss << name;
        else
            ss << (name.c_str()+pos+1);
    }
    if(postfix)
        ss << postfix;
    return ss.str();
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
        if(type & Prop_##_name) _StatusBits.set((size_t)Prop##_name);\
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

void Property::setPathValue(const ObjectIdentifier &path, const App::any &value)
{
    path.setValue(value);
}

App::any Property::getPathValue(const ObjectIdentifier &path) const
{
    return path.getValue();
}

void Property::getPaths(std::vector<ObjectIdentifier> &paths) const
{
    (void)paths;
    // paths.emplace_back(getContainer(), getName());
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
    if(GetApplication().isClosingAll())
        return;

    PropertyCleaner guard(this);
    _StatusBits.set(Touched);
    if (getName() && father && !Transaction::isApplying(this)) {
        father->onEarlyChange(this);
        father->onChanged(this);
        if(!testStatus(Busy)) {
            Base::BitsetLocker<StatusBits> guard(_StatusBits,Busy);
            signalChanged(*this);
        }
    }
}

void Property::setReadOnly(bool readOnly)
{
    this->setStatus(App::Property::ReadOnly, readOnly);
}

void Property::hasSetValue(void)
{
    if (father && _old) {
        if(isSame(*_old)) {
            FC_LOG("no change of " << getFullName());
            return;
        }
        _old.reset();
    }
    touch();
}

void Property::aboutToSetValue(void)
{
    PropertyCleaner guard(this);
    if (father) {
        if(!_old && DocumentParams::OptimizeRecompute())
            _old.reset(copyBeforeChange());
        father->onBeforeChange(this);
    }
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

unsigned long Property::getStatus() const
{
    StatusBits bits = _StatusBits;
    if(!bits.test(Touched) && isTouched())
        bits.set(Touched);
    return bits.to_ulong();
}

void Property::setStatus(const StatusBits &bits, bool on)
{
    if(on)
        setStatusValue((_StatusBits | bits).to_ulong());
    else
        setStatusValue((_StatusBits & ~bits).to_ulong());
}

void Property::setStatusValue(unsigned long status) {
    static const unsigned long mask = 
        (1<<PropDynamic)
        |(1<<PropNoRecompute)
        |(1<<PropReadOnly)
        |(1<<PropTransient)
        |(1<<PropOutput)
        |(1<<PropHidden)
        |(1<<PropNoPersist)
        |(1<<Busy);

    status &= ~mask;
    status |= _StatusBits.to_ulong() & mask;
    auto oldStatus = _StatusBits.to_ulong();
    _StatusBits = StatusBits(status);

    if(father) {
        static unsigned long _signalMask = (1<<ReadOnly) | (1<<Hidden);
        if((status & _signalMask) != (oldStatus & _signalMask))
            father->onPropertyStatusChanged(*this,oldStatus);
    }
}

void Property::setStatus(Status pos, bool on) {
    if(pos == Touched) {
        if(on)
            touch();
        else
            purgeTouched();
        return;
    }
    auto bits = _StatusBits;
    bits.set(pos,on);
    setStatusValue(bits.to_ulong());
}

bool Property::testStatus(Status pos) const
{
    if(pos == Touched)
        return isTouched();
    return _StatusBits.test(static_cast<size_t>(pos));
}

bool Property::testStatus(const StatusBits &bits, const StatusBits &mask) const
{
    auto copy = _StatusBits;
    if(mask.test(Touched) || bits.test(Touched))
        copy.set(Touched, isTouched());
    if((copy & mask).any())
        return false;
    return (copy & bits) == bits;
}

bool Property::isSameContent(const Property &other) const {
    if(other.getTypeId() != getTypeId() || getMemSize() != other.getMemSize())
        return false;

    FC_STATIC Base::StringWriter writer,writer2;
    writer.clear();
    Save(writer);
    writer2.clear();
    other.Save(writer2);
    return writer.getString() == writer2.getString();
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

const char *PropertyLists::xmlName() const {
    const char *name = getTypeId().getName();
    const char *start = strstr(name,"::");
    if(!start) {
        assert(false);
        return name;
    }
    start += 2;
    if(boost::starts_with(start,"Property"))
        start += 8;
    return start;
}

void PropertyLists::Save (Base::Writer &writer) const
{
    const char *element = xmlName();

    if (!getSize() && canSaveStream(writer)) {
        // for backward compatibility, we still need to add attribute 'file' if empty
        writer.Stream() << writer.ind() << '<' << element << " file=\"\"/>\n";
        return;
    }

    if (writer.isForceXML() || !canSaveStream(writer)) {
        writer.Stream() << writer.ind() << '<' << element << " count=\"" <<  getSize() <<"\" ";
        if(!saveXML(writer))
            writer.Stream() << writer.ind() << "</" << element << ">\n";
    } else {
        writer.Stream() << writer.ind() << '<' << element << " file=\"" 
            << writer.addFile(getFileName(writer.isPreferBinary()?".bin":".txt"), this) 
            << "\"/>\n";
    }
}

void PropertyLists::Restore(Base::XMLReader &reader)
{
    reader.readElement(xmlName());
    std::string file (reader.getAttribute("file","") );
    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(),this);
    }else if(reader.hasAttribute("count")) {
        restoreXML(reader);
    }else if(getSize()) {
        setSize(0);
    }
}

void PropertyLists::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream(),writer.isPreferBinary());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    saveStream(str);
}

void PropertyLists::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader, !boost::ends_with(reader.getFileName(),".txt"));
    uint32_t uCt=0;
    str >> uCt;
    restoreStream(str,uCt);
}
