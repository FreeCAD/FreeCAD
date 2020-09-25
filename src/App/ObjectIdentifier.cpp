/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#include <limits>
#include <iomanip>
#include <unordered_map>

#include <boost/algorithm/string.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/GeometryPyCXX.h>
#include <Base/Tools.h>
#include <Base/Interpreter.h>
#include <Base/QuantityPy.h>
#include <Base/Console.h>
#include <App/DocumentObjectPy.h>
#include "ComplexGeoData.h"
#include "Property.h"
#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "ObjectIdentifier.h"
#include "ExpressionParser.h"
#include "Link.h"

FC_LOG_LEVEL_INIT("Expression",true,true)

using namespace App;
using namespace Base;

template<class InputIterator>
static void _quote(std::ostream &output, InputIterator cur, InputIterator end, bool toPython=false)
{
    output << (toPython?"'":"<<");
    while (cur != end) {
        switch (*cur) {
        case '\t':
            output << "\\t";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '\'':
            output << "\\'";
            break;
        case '"':
            output << "\\\"";
            break;
        case '>':
            output << (toPython?">":"\\>");
            break;
        default:
            output << *cur;
        }
        ++cur;
    }
    output << (toPython?"'":">>");
}

template<class InputIterator>
static inline std::string _quote(InputIterator cur, InputIterator end, bool toPython=false) {
    std::ostringstream os;
    _quote(os, cur, end, toPython);
    return os.str();
}

// Path class

/**
 * @brief Quote input string according to quoting rules for an expression: because " and ' are
 * used to designate inch and foot units, strings are quoted as <<string>>.
 *
 * @param input
 * @return
 */
std::string App::quote(const std::string &input, bool toPython)
{
    return _quote(input.begin(), input.end(), toPython);
}

enum PseudoPropertyType {
    PseudoNone,
    PseudoShape,
    PseudoPlacement,
    PseudoMatrix,
    PseudoLinkPlacement,
    PseudoLinkMatrix,
    PseudoSelf,
    PseudoRef,
    PseudoApp,
    PseudoPart,
    PseudoRegex,
    PseudoBuiltins,
    PseudoMath,
    PseudoCollections,
    PseudoGui,
    PseudoCadquery,
    PseudoSubObject,
};

/**
 * @brief Construct an ObjectIdentifier object, given an owner and a single-value property.
 * @param _owner Owner of property.
 * @param property Name of property.
 */

ObjectIdentifier::ObjectIdentifier(const App::PropertyContainer * _owner,
        const std::string & property, int index)
    : owner(0)
    , subObjectName("",true)
    , documentNameSet(false)
    , documentObjectNameSet(false)
    , localProperty(false)
    , _hash(0)
{
    if (_owner) {
        const DocumentObject * docObj = freecad_dynamic_cast<const DocumentObject>(_owner);
        if (!docObj)
            FC_THROWM(Base::RuntimeError,"Property must be owned by a document object.");
        owner = const_cast<DocumentObject*>(docObj);

        if (property.size() > 0) {
            setDocumentObjectName(docObj);
        }
    }
    if (property.size() > 0) {
        addComponent(SimpleComponent(property));
        if(index!=INT_MAX)
            addComponent(ArrayComponent(index));
    }
}

ObjectIdentifier::ObjectIdentifier(const App::PropertyContainer * _owner, 
                                   const char *property, int index)
    : owner(0)
    , subObjectName("",true)
    , documentNameSet(false)
    , documentObjectNameSet(false)
    , localProperty(false)
{
    if (_owner) {
        const DocumentObject * docObj = freecad_dynamic_cast<const DocumentObject>(_owner);
        if (!docObj)
            FC_THROWM(Base::RuntimeError,"Property must be owned by a document object.");
        owner = const_cast<DocumentObject*>(docObj);

        if (property && property[0]) {
            setDocumentObjectName(docObj);
        }
    }
    if (property && property[0]) {
        addComponent(SimpleComponent(property));
        if(index!=INT_MAX)
            addComponent(ArrayComponent(index));
    }
}

ObjectIdentifier::ObjectIdentifier(const App::PropertyContainer * _owner, bool localProperty)
    : owner(0)
    , subObjectName("",true)
    , documentNameSet(false)
    , documentObjectNameSet(false)
    , localProperty(localProperty)
    , _hash(0)
{
    if (_owner) {
        const DocumentObject * docObj = freecad_dynamic_cast<const DocumentObject>(_owner);
        if (!docObj)
            FC_THROWM(Base::RuntimeError,"Property must be owned by a document object.");
        owner = const_cast<DocumentObject*>(docObj);
    }
}

/**
 * @brief Construct an ObjectIdentifier object given a property. The property is assumed to be single-valued.
 * @param prop Property to construct object identifier for.
 */

ObjectIdentifier::ObjectIdentifier(const Property &prop, int index)
    : owner(0)
    , subObjectName("",true)
    , documentNameSet(false)
    , documentObjectNameSet(false)
    , localProperty(false)
    , _hash(0)
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(prop.getContainer());

    if (!docObj)
        FC_THROWM(Base::TypeError,"Property must be owned by a document object.");

    owner = const_cast<DocumentObject*>(docObj);

    setDocumentObjectName(docObj);

    addComponent(SimpleComponent(prop.getName()));
    if(index!=INT_MAX)
        addComponent(ArrayComponent(index));
}

/**
 * @brief Get the name of the property.
 * @return Name
 */

std::string App::ObjectIdentifier::getPropertyName() const
{
    ResolveResults result(*this);

    assert(result.propertyIndex >=0 && static_cast<std::size_t>(result.propertyIndex) < components.size());

    return components[result.propertyIndex].getName();
}

/**
 * @brief Get Component at given index \a i.
 * @param i: Index to get
 * @param idx: optional return of adjusted component index
 * @return A component.
 */

const App::ObjectIdentifier::Component &App::ObjectIdentifier::getPropertyComponent(int i, int *idx) const
{
    ResolveResults result(*this);

    i += result.propertyIndex;
    if (i < 0 || i >= static_cast<int>(components.size()))
        FC_THROWM(Base::ValueError, "Invalid property component index");

    if (idx)
        *idx = i;

    return components[i];
}

void App::ObjectIdentifier::setComponent(int idx, Component &&comp)
{
    if (idx < 0 || idx >= static_cast<int>(components.size()))
        FC_THROWM(Base::ValueError, "Invalid component index");
    components[idx] = std::move(comp);
    _cache.clear();
}

void App::ObjectIdentifier::setComponent(int idx, const Component &comp)
{
    setComponent(idx, Component(comp));
}

std::vector<ObjectIdentifier::Component> ObjectIdentifier::getPropertyComponents(int i) const {
    ResolveResults result(*this);
    result.propertyIndex += i;
    if(result.propertyIndex==0)
        return components;
    std::vector<ObjectIdentifier::Component> res;
    if(result.propertyIndex < (int)components.size())
        res.insert(res.end(),components.begin()+result.propertyIndex,components.end());
    return res;
}

/**
 * @brief Compare object identifier with \a other.
 * @param other Other object identifier.
 * @return true if they are equal.
 */

bool ObjectIdentifier::operator ==(const ObjectIdentifier &other) const
{
    return owner==other.owner && toString() == other.toString();
}

/**
 * @brief Compare object identifier with \a other.
 * @param other Other object identifier
 * @return true if they differ from each other.
 */

bool ObjectIdentifier::operator !=(const ObjectIdentifier &other) const
{
    return !(operator==)(other);
}

/**
 * @brief Compare object identifier with other.
 * @param other Other object identifier.
 * @return true if this object is less than the other.
 */

bool ObjectIdentifier::operator <(const ObjectIdentifier &other) const
{
    if(owner < other.owner)
        return true;
    if(owner > other.owner)
        return false;
    return toString() < other.toString();
}

/**
 * @brief Return number of components.
 * @return Number of components in this identifier.
 */

int ObjectIdentifier::numComponents() const
{
    return components.size();
}

/**
 * @brief Compute number of sub components, i.e excluding the property.
 * @return Number of components.
 */

int ObjectIdentifier::numSubComponents() const
{
    ResolveResults result(*this);

    return (int)components.size() - result.propertyIndex;
}

bool ObjectIdentifier::verify(const App::Property &prop, bool silent) const {
    ResolveResults result(*this);
    if(components.size() - result.propertyIndex != 1) {
        if(silent) return false;
        FC_THROWM(Base::ValueError,"Invalid property path: single component expected");
    }
    if(!components[result.propertyIndex].isSimple()) {
        if(silent) return false;
        FC_THROWM(Base::ValueError,"Invalid property path: simple component expected");
    }
    const std::string &name = components[result.propertyIndex].getName();
    CellAddress addr;
    bool isAddress = addr.parseAbsoluteAddress(name.c_str());
    if((isAddress && addr.toString(true) != prop.getName()) ||
       (!isAddress && name!=prop.getName()))
    {
        if(silent) return false;
        FC_THROWM(Base::ValueError,"Invalid property path: name mismatch");
    }
    return true;
}

/**
 * @brief Create a string representation of this object identifier.
 *
 * An identifier is written as document#documentobject.property.subproperty1...subpropertyN
 * document# may be dropped; it is assumed to be within owner's document. If documentobject is dropped,
 * the property is assumed to be owned by the owner specified in the object identifiers constructor.
 *
 * @return A string
 */

const std::string &ObjectIdentifier::toString() const
{
    if(_cache.size() || !owner)
        return _cache;

    std::ostringstream s;
    ResolveResults result(*this);

    auto itComp = components.begin();
    
    if(localProperty ||
       (result.resolvedProperty &&
        result.resolvedDocumentObject==owner &&
        components.size()>1 &&
        components[1].isSimple() &&
        result.propertyIndex==0))
    {
        s << '.';
    }else if (documentNameSet && documentName.getString().size()) {
        if(documentObjectNameSet && documentObjectName.getString().size())
            s << documentName << "#"
              << documentObjectName << '.';
        else if(result.resolvedDocumentObjectName.getString().size())
            s << documentName << "#"
              << result.resolvedDocumentObjectName << '.';
    } else if (documentObjectNameSet && documentObjectName.getString().size()) {
        s << documentObjectName << '.';
    }

    if(subObjectName.getString().size())
        s << subObjectName << '.';

    bool first = true;
    for(;itComp!=components.end();++itComp) {
        if(first)
            first = false;
        else if(itComp->isSimple() || itComp->isLabel())
            s << '.';
        itComp->toString(s);
    }

    const_cast<ObjectIdentifier*>(this)->_cache = s.str();
    return _cache;
}

std::string ObjectIdentifier::toPersistentString() const {

    if(!owner)
        return std::string();

    std::ostringstream s;
    ResolveResults result(*this);

    if(result.propertyIndex >= (int)components.size())
        return std::string();

    auto itComp = components.begin() + result.propertyIndex;
    
    if(localProperty ||
       (result.resolvedProperty &&
        result.resolvedDocumentObject==owner &&
        components.size()>1 &&
        components[1].isSimple() &&
        result.propertyIndex==0))
    {
        s << '.';
    }else if(result.resolvedDocumentObject &&
        result.resolvedDocumentObject!=owner &&
        result.resolvedDocumentObject->isExporting())
    {
        s << result.resolvedDocumentObject->getExportName(true);
        if(documentObjectName.isRealString())
            s << '@';
        s << '.';
    } else if ((owner->isExporting() || documentNameSet)
                && result.resolvedDocumentObject != owner
                && (documentName.getString().size()
                    || result.resolvedDocumentName.getString().size()))
    {
        if(documentName.getString().size())
            s << documentName;
        else
            s << result.resolvedDocumentName;
        s << '#';
        if(documentObjectNameSet && documentObjectName.getString().size())
            s << documentObjectName;
        else
            s << result.resolvedDocumentObjectName;
        s << '.';
    } else if (documentObjectNameSet && documentObjectName.getString().size()) {
        s << documentObjectName << '.';
    } else if (result.propertyIndex > 0) {
        components[0].toString(s);
        s << '.';
    }

    if(subObjectName.getString().size()) {
        const char *subname = subObjectName.getString().c_str();
        std::string exportName;
        s << String(PropertyLinkBase::exportSubName(exportName,
                        result.resolvedDocumentObject,subname),true) << '.';
    }

    bool first = true;
    for(;itComp!=components.end();++itComp) {
        if(first)
            first = false;
        else if(itComp->isSimple() || itComp->isLabel())
            s << '.';
        itComp->toString(s);
    }
    return s.str();
}

std::size_t ObjectIdentifier::hash() const
{
    if(_hash && _cache.size())
        return _hash;
    const_cast<ObjectIdentifier*>(this)->_hash = boost::hash_value(toString());
    return _hash;
}

bool ObjectIdentifier::replaceObject(ObjectIdentifier &res, const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const
{
    ResolveResults result(*this);

    if(!result.resolvedDocumentObject)
        return false;

    auto r = PropertyLinkBase::tryReplaceLink(owner, result.resolvedDocumentObject,
            parent, oldObj, newObj, subObjectName.getString().c_str());

    if(!r.first)
        return false;

    res = *this;
    if(r.first != result.resolvedDocumentObject) {
        if(r.first->getDocument()!=owner->getDocument()) {
            auto doc = r.first->getDocument();
            bool useLabel = res.documentName.isRealString();
            const char *name = useLabel?doc->Label.getValue():doc->getName();
            res.setDocumentName(String(name, useLabel), true);
        }
        if(documentObjectName.isRealString())
            res.documentObjectName = String(r.first->Label.getValue(),true);
        else
            res.documentObjectName = String(r.first->getNameInDocument(),false,true);
    }
    res.subObjectName = String(r.second,true);
    res._cache.clear();
    res.shadowSub.first.clear();
    res.shadowSub.second.clear();
    return true;
}

/**
 * @brief Escape toString representation so it is suitable for being embedded in a python command.
 * @return Escaped string.
 */

std::string ObjectIdentifier::toEscapedString() const
{
    return Base::Tools::escapedUnicodeFromUtf8(toString().c_str());
}

bool ObjectIdentifier::updateLabelReference(
        App::DocumentObject *obj, const std::string &ref, const char *newLabel)
{
    if(!owner)
        return false;

    ResolveResults result(*this);

    if(subObjectName.getString().size() && result.resolvedDocumentObject) {
        std::string sub = PropertyLinkBase::updateLabelReference(
                result.resolvedDocumentObject, subObjectName.getString().c_str(), obj,ref,newLabel);
        if(sub.size()) {
            subObjectName = String(sub,true);
            _cache.clear();
            return true;
        }
    }

    if(result.resolvedDocument != obj->getDocument())
        return false;

    if(documentObjectName.getString().size()) {
        if(documentObjectName.isForceIdentifier())
            return false;

        if(!documentObjectName.isRealString() &&
           documentObjectName.getString()==obj->getNameInDocument())
            return false;

        if(documentObjectName.getString()!=obj->Label.getValue())
            return false;

        documentObjectName = ObjectIdentifier::String(newLabel, true);

        _cache.clear();
        return true;
    }

    if (result.resolvedDocumentObject==obj &&
        result.propertyIndex == 1 &&
        result.resolvedDocumentObjectName.isRealString() &&
        result.resolvedDocumentObjectName.getString()==obj->Label.getValue())
    {
        components[0].name = ObjectIdentifier::String(newLabel, true);
        _cache.clear();
        return true;
    }

    // If object identifier uses the label then resolving the document object will fail.
    // So, it must be checked if using the new label will succeed
    if (components.size()>1 && components[0].getName()==obj->Label.getValue()) {
        ObjectIdentifier id(*this);
        id.components[0].name.str = newLabel;

        ResolveResults result(id);

        if (result.propertyIndex == 1 && result.resolvedDocumentObject == obj) {
            components[0].name = id.components[0].name;
            _cache.clear();
            return true;
        }
    }

    return false;
}

bool ObjectIdentifier::relabeledDocument(ExpressionVisitor &v,
        const std::string &oldLabel, const std::string &newLabel)
{
    if (documentNameSet && documentName.isRealString() && documentName.getString()==oldLabel) {
        v.aboutToChange();
        documentName = String(newLabel,true);
        _cache.clear();
        return true;
    }
    return false;
}

/**
 * @brief Get sub field part of a property as a string.
 * @return String representation of path.
 */

void ObjectIdentifier::getSubPathStr(std::ostream &s, const ResolveResults &result, bool toPython, bool prefix) const
{
    if(result.propertyIndex >= (int)components.size())
        return;
    std::vector<Component>::const_iterator i = components.begin() + result.propertyIndex + 1;
    bool first = !prefix;
    while (i != components.end()) {
        if(i->isSimple() || i->isLabel()) {
            if(first)
                first = false;
            else
                s << '.';
        }
        i->toString(s,toPython);
        ++i;
    }
}

std::string ObjectIdentifier::getSubPathStr(bool toPython, bool prefix) const {
    std::ostringstream ss;
    getSubPathStr(ss,ResolveResults(*this),toPython, prefix);
    return ss.str();
}


/**
 * @brief Construct a Component part
 * @param _name Name of component
 * @param _type Type; simple, array, range or map
 * @param _begin Array index or beginning of a Range, or INT_MAX for other type.
 * @param _end ending of a Range, or INT_MAX for other type.
 */

ObjectIdentifier::Component::Component(const String &_name,
        ObjectIdentifier::Component::typeEnum _type, int _begin, int _end, int _step)
    : name(_name)
    , type(_type)
    , begin(_begin)
    , end(_end)
    , step(_step)
{
}

ObjectIdentifier::Component::Component(String &&_name,
        ObjectIdentifier::Component::typeEnum _type, int _begin, int _end, int _step)
    : name(std::move(_name))
    , type(_type)
    , begin(_begin)
    , end(_end)
    , step(_step)
{
}

size_t ObjectIdentifier::Component::getIndex(size_t count) const {
    if(begin>=0) {
        if(begin<(int)count)
            return begin;
    }else {
        int idx = begin + (int)count;
        if(idx >= 0)
            return idx;
    }
    FC_THROWM(Base::IndexError, "Array out of bound: " << begin << ", " << count);
}

Py::Object ObjectIdentifier::Component::get(const Py::Object &pyobj) const {
    Py::Object res;
    if(isSimple()) {
        if(!pyobj.hasAttr(getName()))
            FC_THROWM(Base::AttributeError, "No attribute named '" << getName() << "'");
        res = pyobj.getAttr(getName());
    } else if(isArray()) {
        if(pyobj.isMapping())
            res = Py::Mapping(pyobj).getItem(Py::Int(begin));
        else
            res = Py::Sequence(pyobj).getItem(begin);
    }else if(isMap() || isLabel())
        res = Py::Mapping(pyobj).getItem(getName());
    else if(isRange()){
        Py::Object slice(PySlice_New(Py::Int(begin).ptr(),
                                    end!=INT_MAX?Py::Int(end).ptr():0,
                                    step!=1?Py::Int(step).ptr():0),true);
        PyObject *r = PyObject_GetItem(pyobj.ptr(),slice.ptr());
        if(!r)
            Base::PyException::ThrowException();
        res = Py::asObject(r);
    } else
        FC_THROWM(Base::RuntimeError, "Invalid component: " << getName());
    if(!res.ptr())
        Base::PyException::ThrowException();
    if(PyModule_Check(res.ptr()) && !ExpressionParser::isModuleImported(res.ptr()))
        FC_THROWM(Base::RuntimeError, "Module '" << getName() << "' access denied.");
    return res;
}

void ObjectIdentifier::Component::set(Py::Object &pyobj, const Py::Object &value) const {
    if(isSimple()) {
        if(PyObject_SetAttrString(*pyobj, getName().c_str(), *value ) == -1)
            Base::PyException::ThrowException();
    } else if(isArray()) {
        if(pyobj.isMapping())
            Py::Mapping(pyobj).setItem(Py::Int(begin),value);
        else
            Py::Sequence(pyobj).setItem(begin,value);
    }else if(isMap() || isLabel())
        Py::Mapping(pyobj).setItem(getName(),value);
    else if(isRange()) {
        Py::Object slice(PySlice_New(Py::Int(begin).ptr(),
                                    end!=INT_MAX?Py::Int(end).ptr():0,
                                    step!=1?Py::Int(step).ptr():0),true);
        if(PyObject_SetItem(pyobj.ptr(),slice.ptr(),value.ptr())<0)
            Base::PyException::ThrowException();
    } else
        FC_THROWM(Base::RuntimeError, "Invalid component: " << getName());
}

void ObjectIdentifier::Component::del(Py::Object &pyobj) const {
    if(isSimple())
        pyobj.delAttr(getName());
    else if(isArray()) {
        if(pyobj.isMapping())
            Py::Mapping(pyobj).delItem(Py::Int(begin));
        else
            PySequence_DelItem(pyobj.ptr(),begin);
    } else if(isMap() || isLabel())
        Py::Mapping(pyobj).delItem(getName());
    else if(isRange()) {
        Py::Object slice(PySlice_New(Py::Int(begin).ptr(),
                                    end!=INT_MAX?Py::Int(end).ptr():0,
                                    step!=1?Py::Int(step).ptr():0),true);
        if(PyObject_DelItem(pyobj.ptr(),slice.ptr())<0)
            Base::PyException::ThrowException();
    }
    FC_THROWM(Base::RuntimeError, "Invalid component: " << getName());
}

/**
 * @brief Create a simple component part with the given name
 * @param _component Name of component.
 * @return A new Component object.
 */

ObjectIdentifier::Component
ObjectIdentifier::Component::SimpleComponent(const char *_component)
{
    return Component(String(_component));
}

/**
 * @brief Create a simple component part with the given name
 * @param _component Name of component.
 * @return A new Component object.
 */
ObjectIdentifier::Component
ObjectIdentifier::Component::SimpleComponent(const ObjectIdentifier::String &_component)
{
    return Component(String(_component.getString(),false));
}

ObjectIdentifier::Component
ObjectIdentifier::Component::SimpleComponent(ObjectIdentifier::String &&_component)
{
    return Component(String(std::move(_component.getString()), false));
}

ObjectIdentifier::Component
ObjectIdentifier::Component::LabelComponent(const char *_component)
{
    return Component(String(_component, true), LABEL);
}

ObjectIdentifier::Component
ObjectIdentifier::Component::LabelComponent(std::string &&_component)
{
    return Component(String(std::move(_component), true), LABEL);
}

ObjectIdentifier::Component
ObjectIdentifier::Component::LabelComponent(const std::string &_component)
{
    return Component(String(_component, true), LABEL);
}

/**
 * @brief Create an array component with given name and index.
 * @param _component Name of component
 * @param _index Index of component
 * @return A new Component object.
 */

ObjectIdentifier::Component
ObjectIdentifier::Component::ArrayComponent(int _index)
{
    return Component(String(), Component::ARRAY, _index);
}

/**
 * @brief Create a map component with given name and key.
 * @param _component Name of component
 * @param _key Key of component
 * @return A new Component object.
 */

ObjectIdentifier::Component
ObjectIdentifier::Component::MapComponent(const String & _key)
{
    return Component(_key, Component::MAP);
}

ObjectIdentifier::Component
ObjectIdentifier::Component::MapComponent(String &&_key)
{
    return Component(std::move(_key), Component::MAP);
}


/**
 * @brief Create a range component with given begin and end.
 * @param _begin begining index of the range
 * @param _end ending index of the range
 * @return A new Component object.
 */

ObjectIdentifier::Component
ObjectIdentifier::Component::RangeComponent(int _begin, int _end, int _step)
{
    return Component(String(), Component::RANGE, _begin, _end, _step);
}

/**
 * @brief Comparison operator for Component objects.
 * @param other The object we want to compare to.
 * @return true if they are equal, false if not.
 */

bool ObjectIdentifier::Component::operator ==(const ObjectIdentifier::Component &other) const
{
    if (type != other.type)
        return false;

    switch (type) {
    case LABEL:
    case SIMPLE:
    case MAP:
        return name == other.name;
    case ARRAY:
        return begin == other.begin;
    case RANGE:
        return begin == other.begin && end == other.end && step==other.step;
    default:
        return false;
    }
}

/**
 * @brief Create a string representation of a component.
 * @return A string representing the component.
 */

void ObjectIdentifier::Component::toString(std::ostream &ss, bool toPython) const
{
    switch (type) {
    case Component::SIMPLE:
        ss << name;
        break;
    case Component::LABEL:
        name.toString(ss,toPython);
        break;
    case Component::MAP:
        ss << "[";
        name.toString(ss,toPython);
        ss << "]";
        break;
    case Component::ARRAY:
        ss << "[" << begin << "]";
        break;
    case Component::RANGE:
        ss << '[';
        if(begin!=INT_MAX)
            ss << begin;
        ss << ':';
        if(end!=INT_MAX)
            ss << end;
        if(step!=1)
            ss << ':' << step;
        ss << ']';
        break;
    default:
        break;
    }
}

enum ResolveFlags {
    ResolveByIdentifier,
    ResolveByLabel,
    ResolveAmbiguous,
};

/**
 * @brief Search for the document object given by name in doc.
 *
 * Name might be the internal name or a label. In any case, it must uniquely define
 * the document object.
 *
 * @param doc Document to search
 * @param name Name to search for.
 * @return Pointer to document object if a unique pointer is found, 0 otherwise.
 */

App::DocumentObject * ObjectIdentifier::getDocumentObject(const App::Document * doc,
        const String & name, std::bitset<32> &flags)
{
    DocumentObject * objectById = 0;
    DocumentObject * objectByLabel = 0;

    if(!name.isRealString()) {
        // No object found with matching label, try using name directly
        objectById = doc->getObject(static_cast<const char*>(name));

        if (objectById) {
            flags.set(ResolveByIdentifier);
            return objectById;
        }
        if(name.isForceIdentifier())
            return 0;
    }

    std::vector<DocumentObject*> docObjects = doc->getObjects();
    for (std::vector<DocumentObject*>::iterator j = docObjects.begin(); j != docObjects.end(); ++j) {
        if (strcmp((*j)->Label.getValue(), static_cast<const char*>(name)) == 0) {
            // Found object with matching label
            if (objectByLabel != 0)  {
                FC_WARN("duplicate object label " << doc->getName() << '#' << name);
                return 0;
            }
            objectByLabel = *j;
        }
    }

    if (objectByLabel == 0 && objectById == 0) // Not found at all
        return 0;
    else if (objectByLabel == 0) { // Found by name
        flags.set(ResolveByIdentifier);
        return objectById;
    }
    else if (objectById == 0) { // Found by label
        flags.set(ResolveByLabel);
        return objectByLabel;
    }
    else if (objectByLabel == objectById) { // Found by both name and label, same object
        flags.set(ResolveByIdentifier);
        flags.set(ResolveByLabel);
        return objectByLabel;
    }
    else {
        flags.set(ResolveAmbiguous);
        return 0; // Found by both name and label, two different objects
    }
}

/**
 * @brief Resolve the object identifier to a concrete document, documentobject, and property.
 *
 * This method is a helper method that fills out data in the given ResolveResults object.
 *
 */

void ObjectIdentifier::resolve(ResolveResults &results) const
{
    if(!owner || !owner->getNameInDocument())
        return;

    bool docAmbiguous = false;

    results.subObjectName = subObjectName;
    results.propertyName = "";
    results.propertyIndex = 0;

    if(isLocalProperty()) {
        results.resolvedDocument = owner->getDocument();
        results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);
        results.resolvedDocumentObjectName = String(owner->getNameInDocument(), false, true);
        results.resolvedDocumentObject = owner;
        results.getProperty(*this);
        return;
    }

    /* Document name specified? */
    if (documentName.getString().size() > 0) {
        results.resolvedDocument = getDocument(documentName,&docAmbiguous);
        results.resolvedDocumentName = documentName;
    }
    else {
        results.resolvedDocument = owner->getDocument();
        results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);
    }

    // Assume document name and object name from owner if not found
    if (results.resolvedDocument == 0) {
        if (documentName.getString().size() > 0) {
            if(docAmbiguous)
                results.flags.set(ResolveAmbiguous);
            return;
        }

        results.resolvedDocument = owner->getDocument();
        if (results.resolvedDocument == 0)
            return;
    }

    results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);

    /* Document object name specified? */
    if (documentObjectName.getString().size() > 0) {
        results.resolvedDocumentObjectName = documentObjectName;
        results.resolvedDocumentObject = getDocumentObject(
                results.resolvedDocument, documentObjectName, results.flags);

        if (!results.resolvedDocumentObject)
            return;

        if (components.size() > 0) {
            results.propertyIndex = 0;
            results.getProperty(*this);
        }
        else
            return;
    }
    else {
        /* Document object name not specified, resolve from path */

        /* One component? */
        if (components.size() == 1 && components[0].isSimple()) {
            /* Yes -- then this must be a property, so we get the document object's name from the owner */
            results.resolvedDocumentObjectName = String(owner->getNameInDocument(), false, true);
            results.resolvedDocumentObject = owner;
            results.propertyIndex = 0;
            results.getProperty(*this);
        }
        else if (components.size()) {
            /* No --  */
            if (!components[0].isSimple() && !components[0].isLabel())
                return;

            results.resolvedDocumentObject = getDocumentObject(
                    results.resolvedDocument, components[0].name, results.flags);

            /* Possible to resolve component to a document object? */
            if (results.resolvedDocumentObject) {
                if(components.size()==1)
                    return;

                /* Yes */
                if(components[0].isLabel())
                    results.resolvedDocumentObjectName = components[0].name;
                else
                    results.resolvedDocumentObjectName = String(
                        components[0].name, false, results.flags.test(ResolveByIdentifier));
                results.propertyIndex = 1;
                results.getProperty(*this);
                if(!results.resolvedProperty) {
                    // If the second component is not a property name, try to
                    // interpret the first component as the property name.
                    DocumentObject *sobj = 0;
                    int pindex = 0;
                    String tmpSub = subObjectName;
                    int ptype;
                    Property *prop = resolveProperty(owner,tmpSub,pindex,sobj,ptype);
                    if(prop) {
                        // If we found sub object in previous attemp, look further
                        // to see which one has the better match
                        if(results.propertyType == PseudoSubObject && pindex+1 < (int)components.size()) {
                            Base::PyGILStateLocker lock;
                            try {
                                Py::Object obj = Py::asObject(prop->getPyObject());
                                for(int i=pindex+1; i<=results.propertyIndex; ++i)
                                    obj = components[i].get(obj);
                            } catch (Py::Exception &) {
                                Base::PyException e;
                                return;
                            } catch (...) {
                                // Exception here means the sub-object
                                // intepretation is a better match, so return
                                // early. 
                                return;
                            }
                        }

                        results.resolvedProperty = prop;
                        results.propertyType = ptype;
                        results.subObjectName = std::move(tmpSub);
                        if(pindex < (int)components.size())
                            results.propertyName = components[pindex].name.getString();
                        else
                            results.propertyName.clear();
                        results.resolvedDocument = owner->getDocument();
                        results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);
                        results.resolvedDocumentObjectName = String(owner->getNameInDocument(), false, true);
                        results.resolvedDocumentObject = owner;
                        results.resolvedSubObject = sobj;
                        results.propertyIndex = pindex;
                    }
                }
            }
            else if (documentName.getString().empty()) {
                /* No, assume component is a property, and get document object's name from owner */
                results.resolvedDocument = owner->getDocument();
                results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);
                results.resolvedDocumentObjectName = String(owner->getNameInDocument(), false, true);
                results.resolvedDocumentObject = owner->getDocument()->getObject(owner->getNameInDocument());
                results.propertyIndex = 0;
                results.getProperty(*this);
            }
        }
        else
            return;
    }
}

/**
 * @brief Find a document with the given name.
 * @param name Name of document
 * @return Pointer to document, or 0 if it is not found or not uniquely defined by name.
 */

Document * ObjectIdentifier::getDocument(String name, bool *ambiguous) const
{
    if (name.getString().size() == 0)
        name = getDocumentName();

    App::Document * docById = 0;

    if(!name.isRealString()) {
        docById = App::GetApplication().getDocument(name);
        if (name.isForceIdentifier())
            return docById;
    }

    App::Document * docByLabel = 0;
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();

    for (std::vector<App::Document*>::const_iterator i = docs.begin(); i != docs.end(); ++i) {
        if ((*i)->Label.getValue() == name.getString()) {
            /* Multiple hits for same label? */
            if (docByLabel != 0) {
                if(ambiguous) *ambiguous = true;
                return 0;
            }
            docByLabel = *i;
        }
    }

    /* Not found on id? */
    if (docById == 0)
        return docByLabel; // Either not found at all, or on label
    else {
        /* Not found on label? */
        if (docByLabel == 0) /* Then return doc by id */
            return docById;

        /* docByLabel and docById could be equal; that is ok */
        if(docByLabel==docById)
            return docById;
        if(ambiguous) *ambiguous = true;
        return 0;
    }
}

/**
 * @brief Get the document object for the object identifier.
 * @return Pointer to document object, or 0 if not found or uniquely defined.
 */

DocumentObject *ObjectIdentifier::getDocumentObject() const
{
    const App::Document * doc = getDocument();
    std::bitset<32> dummy;

    if (!doc)
        return 0;

    ResolveResults result(*this);

    return getDocumentObject(doc, result.resolvedDocumentObjectName, dummy);
}

void ObjectIdentifier::getDepLabels(std::vector<std::string> &labels) const {
    getDepLabels(ResolveResults(*this),labels);
}

void ObjectIdentifier::getDepLabels(
        const ResolveResults &result, std::vector<std::string> &labels) const
{
    if(documentObjectName.getString().size()) {
        if(documentObjectName.isRealString())
            labels.push_back(documentObjectName.getString());
    } else if(result.propertyIndex == 1)
        labels.push_back(components[0].name.getString());
    if(subObjectName.getString().size()) 
        PropertyLinkBase::getLabelReferences(labels,subObjectName.getString().c_str());
}

ObjectIdentifier::Dependencies
ObjectIdentifier::getDep(bool needProps, std::vector<std::string> *labels) const 
{
    Dependencies deps;
    getDep(deps,needProps,labels);
    return deps;
}

void ObjectIdentifier::getDep(
        Dependencies &deps, bool needProps, std::vector<std::string> *labels) const 
{
    ResolveResults result(*this);
    if(labels) 
        getDepLabels(result,*labels);

    if(!result.resolvedDocumentObject)
       return;

    if(!needProps) {
        deps[result.resolvedDocumentObject];
        return;
    }

    if(!result.resolvedProperty) {
        if(result.propertyName.size())
            deps[result.resolvedDocumentObject].insert(result.propertyName);
        return;
    }

    Base::PyGILStateLocker lock;
    try {
        access(result,0,&deps);
    }catch(Py::Exception &) {
        Base::PyException e;
    }catch(...){
    }
}

/**
 * @brief Get components as a string list.
 * @return List of strings.
 */

std::vector<std::string> ObjectIdentifier::getStringList() const
{
    std::vector<std::string> l;
    ResolveResults result(*this);

    if(!result.resolvedProperty || result.resolvedDocumentObject != owner) {
        if (documentNameSet) {
            l.push_back(documentName.toString() + "#");
            l.push_back(documentObjectName.toString());
        } else if (documentObjectNameSet)
            l.push_back(documentObjectName.toString());
    }
    if(subObjectName.getString().size()) {
        const std::string &s = subObjectName.getString();
        boost::iterator_range<std::string::const_iterator> range(s.begin(), s.end());
        std::vector<boost::iterator_range<std::string::const_iterator> > tokens;
        boost::algorithm::split(tokens, range, boost::is_any_of("."), boost::algorithm::token_compress_on);

        std::string sub;
        if(!result.resolvedSubObject && tokens.size()==1 
                                     && tokens[0].size() 
                                     && tokens[0][0]!='$') 
        {
            l.push_back(std::string(".") + _quote(boost::begin(tokens[0]), boost::end(tokens[0])));
        } else {
            for(size_t i=0; i<tokens.size(); ++i) {
                auto &t = tokens[i];
                if(t.empty())
                    continue;
                sub = ".";
                if(t[0] == '$') {
                    if(t.size() == 1)
                        continue;
                    sub += _quote(boost::begin(t)+1, boost::end(t));
                } else {
                    sub.insert(sub.end(),boost::begin(t),boost::end(t));
                    // This ending dot is for ExpressionCompleter to
                    // disambiguate the sub object reference with
                    // property/attribute reference
                    if(i+1 != tokens.size())
                        sub += "."; 
                }
                l.push_back(std::move(sub));
            }
        }
    }

    bool addDot = l.size() || isLocalProperty();
    std::ostringstream ss;
    for(auto &comp : components) {
        ss.str("");
        if(!addDot)
            addDot = true;
        else if (comp.isSimple() || comp.isLabel())
            ss << '.';
        comp.toString(ss);
        l.push_back(ss.str());
    }
    return l;
}

/**
 * @brief Construct the simplest possible object identifier relative to another.
 * @param other The other object identifier.
 * @return A new simplified object identifier.
 */

ObjectIdentifier ObjectIdentifier::relativeTo(const ObjectIdentifier &other) const
{
    ObjectIdentifier result(owner);
    ResolveResults thisresult(*this);
    ResolveResults otherresult(other);

    if (otherresult.resolvedDocument != thisresult.resolvedDocument)
        result.setDocumentName(std::move(thisresult.resolvedDocumentName), true);
    if (otherresult.resolvedDocumentObject != thisresult.resolvedDocumentObject)
        result.setDocumentObjectName(
                std::move(thisresult.resolvedDocumentObjectName), true, String(subObjectName));

    for (std::size_t i = thisresult.propertyIndex; i < components.size(); ++i)
        result << components[i];

    return result;
}

/**
 * @brief Parse a string to create an object identifier.
 *
 * This method throws an exception if the string is invalid.
 *
 * @param docObj Document object that will own this object identifier.
 * @param str String to parse
 * @return A new object identifier.
 */

ObjectIdentifier ObjectIdentifier::parse(const DocumentObject *docObj, const std::string &str)
{
    std::unique_ptr<Expression> expr(ExpressionParser::parse(docObj, str.c_str()));
    VariableExpression * v = freecad_dynamic_cast<VariableExpression>(expr.get());

    if (v)
        return v->getPath();
    else
        FC_THROWM(Base::RuntimeError,"Invalid property specification.");
}

std::string ObjectIdentifier::resolveErrorString() const
{
    ResolveResults result(*this);

    return result.resolveErrorString();
}

/**
 * @brief << operator, used to add a component to the object identifier.
 * @param value Component object
 * @return Reference to itself.
 */

ObjectIdentifier &ObjectIdentifier::operator <<(const ObjectIdentifier::Component &value)
{
    addComponent(value);
    return *this;
}

ObjectIdentifier &ObjectIdentifier::operator <<(ObjectIdentifier::Component &&value)
{
    addComponent(std::move(value));
    return *this;
}

void ObjectIdentifier::addComponent(ObjectIdentifier::Component &&c) {
    std::string &name = c.name.str;

    if(c.isSimple() || c.isLabel()) {
        if(name.empty())
            return;
    }

    _cache.clear();

    if(!isLocalProperty() 
            && documentNameSet
            && !documentObjectNameSet
            && components.empty())
    {
        if(c.isSimple() || c.isLabel()) {
            documentObjectNameSet = true;
            documentObjectName = std::move(c.name);
            return;
        }
    }

    if(!c.isLabel()) {
        components.push_back(std::move(c));
        return;
    }

    if(!documentObjectNameSet && !isLocalProperty()) {
        if(components.empty()) {
            documentObjectNameSet = true;
            documentObjectName = std::move(c.name);
            return;
        }
       
        if(components.front().isSimple() || components.front().isLabel()) {
            documentObjectNameSet = true;
            documentObjectName = std::move(components.front().name);
            components.erase(components.begin());
        }
    }

    // If a property is found by now, do not try to concatenate label
    // components as sub-object name, because the label may be used as key to a
    // mapping.
    int ptype;
    if(getProperty(&ptype) && ptype!=PseudoSubObject) {
        components.push_back(std::move(c));
        return;
    }

    // A label component indicates this and all existing components is part of
    // the subname component
    for(auto &component : components) {
        if(!component.isSimple()) {
            // There is already something not 'Simple' in the components, this
            // label component is invalid, just add as it is.
            components.push_back(std::move(c));
            return;
        }
    }

    subObjectName.isString = true;
    std::string &subname = subObjectName.str;
    std::ostringstream os;
    if(subname.size() && subname[0]!='$' && subname.find('.') == std::string::npos)
        os << '$';
    os << subname;
    if(subname.size() && !boost::ends_with(subname,"."))
        os << '.';
    for(auto &component : components) 
        os << component.name.str << '.';

    components.clear();

    if(name[0]!='$' && name.find('.') == std::string::npos)
        os << '$';
    if(name[0] == '.')
        os << (name.c_str()+1);
    else
        os << name;
    subname = os.str();
}

void ObjectIdentifier::addComponent(const ObjectIdentifier::Component &c) {
    addComponent(Component(c));
}

void ObjectIdentifier::popComponents(int count) {
    if(count <= 0)
        return;
    _cache.clear();
    if(count <= (int)components.size()) {
        components.resize(components.size() - count);
        return;
    }
    components.clear();
    count -= (int)components.size();

    std::string &subname = subObjectName.str;
    while(count) {
        size_t pos = subname.rfind('.');
        if(pos == std::string::npos) {
            if(subname.size()) {
                subname.clear();
                --count;
            }
            break;
        }
        if(pos+1 != subname.size()) {
            --count;
            ++pos;
        }
        subname.resize(pos);
    }

    if(count && documentObjectNameSet) {
        documentObjectNameSet = false;
        --count;
        if(count && documentNameSet)
            documentNameSet = false;
    }
}

/**
 * @brief Get pointer to property pointed to by this object identifier.
 * @return Point to property if it is uniquely defined, or 0 otherwise.
 */

Property *ObjectIdentifier::getProperty(int *ptype) const
{
    ResolveResults result(*this);
    if(ptype)
        *ptype = result.propertyType;
    return result.resolvedProperty;
}

const std::vector<std::pair<const char *, App::Property*> > &ObjectIdentifier::getPseudoProperties()
{
    static PropertyContainer dummy;
    static std::vector<std::pair<const char *, App::Property*> > pseudoProps;
    if(pseudoProps.empty()) {
        auto addProp = [](PropertyContainer &pc, std::vector<std::pair<const char *, App::Property *> > &props,
                          const char *name, const char *doc, int type)
        {
            auto prop = static_cast<PropertyInteger*>(pc.addDynamicProperty("App::PropertyInteger", name, 0, doc));
            prop->setValue(type);
            props.emplace_back(name, prop);
        };
        addProp(dummy, pseudoProps,
                "_shape",  "Return a geometry shape of the (sub)object using Part.getShape()", PseudoShape); 
        addProp(dummy, pseudoProps,
                "_pla",    "Return the accumulated placement of the (sub)object", PseudoPlacement);
        addProp(dummy, pseudoProps,
                "_matrix", "Return the accumulated transformation matrix of the (sub)object", PseudoMatrix);
        addProp(dummy, pseudoProps,
                "__pla",   "Return the accumulated placement of the (sub)object including any App::Link", PseudoLinkPlacement);
        addProp(dummy, pseudoProps,
                "__matrix","Return the accumulated transformation matrix of the (sub)object including any App::Link", PseudoLinkMatrix);
        addProp(dummy, pseudoProps,
                "_self",   "Return the object itself in order to access its Python attributes", PseudoSelf);
        addProp(dummy, pseudoProps,
                "_ref",   "Return a (sub)object reference that is suitable for assigning to a link type property", PseudoRef);
        addProp(dummy, pseudoProps,
                "_app",    "Return the FreeCAD Python module", PseudoApp);
        addProp(dummy, pseudoProps,
                "_part",   "Return the Part Python module", PseudoPart);
        addProp(dummy, pseudoProps,
                "_re",     "Return the Python regex module", PseudoRegex);
        addProp(dummy, pseudoProps,
                "_py",     "Return the Python builtin module", PseudoBuiltins);
        addProp(dummy, pseudoProps,
                "_math",   "Return the Python math module", PseudoMath);
        addProp(dummy, pseudoProps,
                "_coll",   "Return the Python collections module", PseudoCollections);
        addProp(dummy, pseudoProps,
                "_gui",    "Return the FreeCADGui Python module", PseudoGui);
        addProp(dummy, pseudoProps,
               "_cq",     "Return the CadQuery Python module", PseudoCadquery);
    };
    return pseudoProps;
}

bool ObjectIdentifier::isPseudoProperty(const App::Property *prop) {
    static std::unordered_set<const App::Property*> propSet;
    if(propSet.empty()) {
        for(auto &v : getPseudoProperties())
            propSet.insert(v.second);
    }
    return propSet.count(prop)!=0;
}

Property *ObjectIdentifier::resolveProperty(const App::DocumentObject *obj, 
                                            String &_subname,
                                            int &propertyIndex,
                                            App::DocumentObject *&sobj,
                                            int &ptype) const 
{
    std::string &subname = _subname.str;
    sobj = nullptr;

    static std::unordered_map<const char*,int, CStringHasher, CStringHasher> _props;
    if(_props.empty()) {
        for(auto &info : getPseudoProperties())
            _props[info.first] = static_cast<PropertyInteger*>(info.second)->getValue();
    }

    auto getSubObject = [](const DocumentObject *obj, const char *s) -> DocumentObject* {
        if(!obj || !obj->getNameInDocument())
            return nullptr;
        if(s && s[0] == '.')
            ++s;
        return obj->getSubObject(s);
    };

    App::Property *prop = 0;
    std::string sub;
    bool foundSobj = false;
    while(1) {
        const char *name = 0;
        if(propertyIndex >= 0 && propertyIndex < (int)components.size()) {
            auto &component = components[propertyIndex];
            if(!component.isSimple())
                return nullptr;
            name = component.getName().c_str();

            auto it = _props.find(name);
            if(it == _props.end())
                ptype = PseudoNone;
            else
                ptype = it->second;
        }

        if(!sobj && subname.size()) {
            if(!obj)
                return nullptr;

            std::string _subname;
            const char *s = subname.c_str();

            // If no middle dot found, try interprete it as label
            if(ptype!=PseudoShape && subname[0]!='$' && subname.find('.') == std::string::npos) {
                _subname = "$";
                _subname += subname;
                _subname += ".";
                s = _subname.c_str();
            }

            sobj = getSubObject(obj, s);
            if(sobj) {
                if(ptype!=PseudoShape && !boost::ends_with(s, "."))
                    return nullptr;
            } else {
                if(_subname.empty())
                    _subname = subname;
                if(!boost::ends_with(_subname,".")) {
                    // No ending '.', try interpret as sub-object
                    // name reference instead of element reference.
                    _subname += ".";
                    s = _subname.c_str();
                } else
                    return nullptr;
                sobj = getSubObject(obj, s);
                if(!sobj)
                    return nullptr;
            }
            obj = sobj;
            if(s == _subname.c_str()) {
                subname = std::move(_subname);
                foundSobj = true;
            }
        }

        if(ptype != PseudoNone)
            return &const_cast<App::DocumentObject*>(obj)->Label; //fake the property

        if(!name)
            return prop;

        prop = obj->getPropertyByName(name);
        if(prop) {
            ptype = PseudoNone;
            return prop;
        }

        sub = name;
        if(!boost::ends_with(name,"."))
            sub += ".";

        auto ssobj = getSubObject(obj, sub.c_str());
        if(!ssobj || !ssobj->getNameInDocument()) {
            if(foundSobj)
                ptype = PseudoSubObject;
            return nullptr;
        }

        foundSobj = true;

        obj = sobj = ssobj;
        subname += sub;

        ++propertyIndex;

        if(propertyIndex == (int)components.size()) {
            prop = &sobj->Label;
            ptype = PseudoSubObject;
            return prop;
        }
    }
}

/**
 * @brief Create a canonical representation of an object identifier.
 *
 * The main work is actually done by the property's virtual canonicalPath(...) method,
 * which is invoked by this call.
 *
 * @return A new object identifier.
 */

ObjectIdentifier ObjectIdentifier::canonicalPath() const
{
    ObjectIdentifier res(*this);
    ResolveResults result(res);
    if(result.resolvedDocumentObject && result.resolvedDocumentObject!=owner) {
        res.owner = result.resolvedDocumentObject;
        res._cache.clear();
    }
    res.resolveAmbiguity(result);
    if(!result.resolvedProperty || result.propertyType!=PseudoNone)
        return res;
    return result.resolvedProperty->canonicalPath(res);
}

static const std::map<std::string,std::string> *_DocumentMap;
ObjectIdentifier::DocumentMapper::DocumentMapper(const std::map<std::string,std::string> &map)
{
    assert(!_DocumentMap);
    _DocumentMap = &map;
}

ObjectIdentifier::DocumentMapper::~DocumentMapper()
{
    _DocumentMap = 0;
}

/**
 * @brief Set the document name for this object identifier.
 *
 * If force is true, the document name will always be included in the string representation.
 *
 * @param name Name of document object.
 * @param force Force name to be set
 */

void ObjectIdentifier::setDocumentName(ObjectIdentifier::String &&name, bool force)
{
    if(name.getString().empty())
        force = false;
    documentNameSet = force;
    _cache.clear();
    if(name.getString().size() && _DocumentMap) {
        if(name.isRealString()) {
            auto iter = _DocumentMap->find(name.toString());
            if(iter!=_DocumentMap->end()) {
                documentName = String(iter->second,true);
                return;
            }
        }else{
            auto iter = _DocumentMap->find(name.getString());
            if(iter!=_DocumentMap->end()) {
                documentName = String(iter->second,false,true);
                return;
            }
        }
    }
    documentName = std::move(name);
}

/**
 * @brief Get the document name from this object identifier
 *
 * @return Document name as a String object.
 */

ObjectIdentifier::String ObjectIdentifier::getDocumentName() const
{
    ResolveResults result(*this);

    return result.resolvedDocumentName;
}

/**
 * @brief Set the document object name of this object identifier.
 *
 * If force is true, the document object will not be resolved dynamically from the
 * object identifier's components, but used as given by this method.
 *
 * @param name Name of document object.
 * @param force Force name to be set.
 */

void ObjectIdentifier::setDocumentObjectName(ObjectIdentifier::String &&name, bool force,
        ObjectIdentifier::String &&subname, bool checkImport)
{
    if(checkImport) {
        name.checkImport(owner);
        subname.checkImport(owner,0,&name);
    }

    documentObjectName = std::move(name);
    documentObjectNameSet = force;
    subObjectName = String(std::move(subname.getString()), true);

    _cache.clear();
}

void ObjectIdentifier::setDocumentObjectName(const App::DocumentObject *obj, bool force,
        ObjectIdentifier::String &&subname, bool checkImport)
{
    if(!owner || !obj || !obj->getNameInDocument() || !obj->getDocument())
        FC_THROWM(Base::RuntimeError,"invalid object");

    if(checkImport)
        subname.checkImport(owner,obj);

    if(obj == owner)
        force = false;
    else
        localProperty = false;
    if(obj->getDocument() == owner->getDocument())
        setDocumentName(String());
    else if(!documentNameSet) {
        if(obj->getDocument() == owner->getDocument())
            setDocumentName(String());
        else {
            documentNameSet = true;
            documentName = String(obj->getDocument()->getName(),false,true);
        }
    }else if(documentName.isRealString())
        documentName = String(obj->getDocument()->Label.getStrValue(),true);
    else
        documentName = String(obj->getDocument()->getName(),false,true);

    documentObjectNameSet = force;
    documentObjectName = String(obj->getNameInDocument(),false,true);
    subObjectName = std::move(subname);

    _cache.clear();
}


/**
 * @brief Get the document object name
 * @return String with name of document object as resolved by object identifier.
 */

ObjectIdentifier::String ObjectIdentifier::getDocumentObjectName() const
{
    ResolveResults result(*this);

    return result.resolvedDocumentObjectName;
}

bool ObjectIdentifier::hasDocumentObjectName(bool forced) const {
    return !documentObjectName.getString().empty() && (!forced || documentObjectNameSet);
}

/**
 * @brief Get a string representation of this object identifier.
 * @return String representation.
 */

std::string ObjectIdentifier::String::toString(bool toPython) const
{
    if(str.empty())
        return str;
    if (isRealString())
        return quote(str,toPython);
    else
        return str;
}

void ObjectIdentifier::String::toString(std::ostream &s, bool toPython) const
{
    if(str.empty())
        return;
    if (isRealString())
        _quote(s, str.begin(), str.end(), toPython);
    else
        s << str;
}

void ObjectIdentifier::String::checkImport(const App::DocumentObject *owner,
        const App::DocumentObject *obj, String *objName)
{
    if(owner && owner->getDocument() && str.size() &&
       ExpressionParser::ExpressionImporter::reader()) {
        auto reader = ExpressionParser::ExpressionImporter::reader();
        if (obj || objName) {
            bool restoreLabel = false;
            str = PropertyLinkBase::importSubName(*reader,str.c_str(),restoreLabel);
            if (restoreLabel) {
                if (!obj) {
                    std::bitset<32> flags;
                    obj = getDocumentObject(owner->getDocument(),*objName,flags);
                    if (!obj) {
                        FC_ERR("Cannot find object " << objName->toString());
                    }
                }

                if (obj) {
                    PropertyLinkBase::restoreLabelReference(obj,str);
                }
            }
        }
        else if (str.back()!='@') {
            str = reader->getName(str.c_str());
        }
        else {
            str.resize(str.size()-1);
            auto mapped = reader->getName(str.c_str());
            auto obj = owner->getDocument()->getObject(mapped);
            if (!obj) {
                FC_ERR("Cannot find object " << str);
            }
            else {
                isString = true;
                forceIdentifier = false;
                str = obj->Label.getValue();
            }
        }
    }
}

Py::Object ObjectIdentifier::access(const ResolveResults &result,
        Py::Object *value, Dependencies *deps) const
{
    if(!result.resolvedDocumentObject || !result.resolvedProperty ||
       (subObjectName.getString().size() && !result.resolvedSubObject))
    {
        FC_THROWM(Base::RuntimeError, result.resolveErrorString()
           << " in '" << toString() << "'");
    }

    Py::Object pyobj;
    int ptype = result.propertyType;

    // NOTE! We do not keep reference of the imported module, assuming once
    // imported they'll live (because of sys.modules) till the application
    // dies.
#define GET_MODULE(_name) do {\
        static PyObject *pymod;\
        if(!pymod) {\
           pymod = PyImport_ImportModule(#_name);\
            if(!pymod)\
                Base::PyException::ThrowException();\
            else\
                Py_DECREF(pymod);\
        }\
        pyobj = Py::Object(pymod);\
    }while(0)

    size_t idx = result.propertyIndex+1;
    switch(ptype) {
    case PseudoApp:
        GET_MODULE(FreeCAD);
        break;
    case PseudoGui:
        GET_MODULE(FreeCADGui);
        break;
    case PseudoPart:
        GET_MODULE(Part);
        break;
    case PseudoCadquery:
        GET_MODULE(freecad.fc_cadquery);
        break;
    case PseudoRegex:
        GET_MODULE(re);
        break;
    case PseudoBuiltins:
#if PY_MAJOR_VERSION < 3
        GET_MODULE(__builtin__);
#else
        GET_MODULE(builtins);
#endif
        break;
    case PseudoMath:
        GET_MODULE(math);
        break;
    case PseudoCollections:
        GET_MODULE(collections);
        break;
    case PseudoShape: {
        GET_MODULE(Part);
        Py::Callable func(pyobj.getAttr("getShape"));
        Py::Tuple tuple(1);
        tuple.setItem(0,Py::Object(result.resolvedDocumentObject->getPyObject(),true));
        if(result.subObjectName.getString().empty())
            pyobj = func.apply(tuple);
        else{
            Py::Dict dict;
            dict.setItem("subname",Py::String(result.subObjectName.getString()));
            dict.setItem("needSubElement",Py::True());
            pyobj = func.apply(tuple,dict);
        }
        break;
    } default: { 
        auto obj = result.resolvedDocumentObject;

        if(ptype == PseudoRef) {
            if(subObjectName.getString().size()) {
                PropertyString tmp;
                tmp.setValue(subObjectName.getString().c_str());
                pyobj = Py::TupleN(Py::asObject(obj->getPyObject()),Py::asObject(tmp.getPyObject()));
            } else
                pyobj = Py::Object(obj->getPyObject(),true);
            break;
        }
        Base::Matrix4D mat;

        switch(ptype) {
        case PseudoPlacement:
        case PseudoMatrix:
        case PseudoLinkPlacement:
        case PseudoLinkMatrix:
            obj->getSubObject(result.subObjectName.getString().c_str(),0,&mat);
            break;
        default:
            break;
        }

        if(result.resolvedSubObject)
            obj = result.resolvedSubObject;

        switch(ptype) {
        case PseudoPlacement:
            pyobj = Py::Placement(Base::Placement(mat));
            break;
        case PseudoMatrix:
            pyobj = Py::Matrix(mat);
            break;
        case PseudoLinkPlacement:
        case PseudoLinkMatrix: {
            auto linked = obj->getLinkedObject(true,&mat,false);
            if(!linked || linked==obj) {
                auto ext = obj->getExtensionByType<App::LinkBaseExtension>(true);
                if(ext)
                    ext->getTrueLinkedObject(true,&mat);
            }
            if(ptype == PseudoLinkPlacement)
                pyobj = Py::Placement(Base::Placement(mat));
            else
                pyobj = Py::Matrix(mat);
            break;
        }
        case PseudoSelf:
        case PseudoSubObject:
            pyobj = Py::Object(obj->getPyObject(),true);
            break;
        case PseudoRef:
            break;
        default: {
            // NOTE! We cannot directly call Property::getPyObject(), but
            // instead, must obtain the property's python object through
            // DocumentObjectPy::getAttr(). Because, PyObjectBase has internal
            // attribute tracking only if we obtain attribute through
            // getAttr(). Without attribute tracking, we can't do things like
            //
            //      obj.Placement.Base.x = 10.
            //
            // What happens is that the when Python interpreter calls
            //
            //      Base.setAttr('x', 10),
            //
            // PyObjectBase will lookup Base's parent, i.e. Placement, and call
            //
            //      Placement.setAttr('Base', Base),
            //
            // and in turn calls
            //
            //      obj.setAttr('Placement',Placement)
            //
            // The tracking logic is implemented in PyObjectBase::__getattro/__setattro

            auto container = result.resolvedProperty->getContainer();
            if(container
                    && container!=result.resolvedDocumentObject
                    && container!=result.resolvedSubObject)
            {
                if(!container->isDerivedFrom(DocumentObject::getClassTypeId()))
                    FC_WARN("Invalid property container");
                else
                    obj = static_cast<DocumentObject*>(container);
            }
            pyobj = Py::Object(obj->getPyObject(),true);
            idx = result.propertyIndex;
            break;
        }}}
    }

    auto setPropDep = [deps](DocumentObject *obj, Property *prop, const char *propName) {
        if(!deps || !obj)
            return;
        if(prop && prop->getContainer()!=obj) {
            auto linkTouched = Base::freecad_dynamic_cast<PropertyBool>(
                    obj->getPropertyByName("_LinkTouched"));
            if(linkTouched) 
                propName = linkTouched->getName();
            else {
                auto propOwner = Base::freecad_dynamic_cast<DocumentObject>(prop->getContainer());
                if(propOwner) 
                    obj = propOwner;
                else 
                    propName = 0;
            }
        }
        auto &propset = (*deps)[obj];
        // inserting a null name in the propset indicates the dependency is
        // on all properties of the corresponding object.
        if(propset.size()!=1 || propset.begin()->size()) {
            if(!propName) {
                propset.clear();
                propName = "";
            }
            propset.insert(propName);
        }
        return;
    };

    App::DocumentObject *lastObj = result.resolvedDocumentObject;
    if(result.resolvedSubObject) {
        setPropDep(lastObj,0,0);
        lastObj = result.resolvedSubObject;
    }
    if(ptype == PseudoNone)
        setPropDep(lastObj, result.resolvedProperty, result.resolvedProperty->getName());
    else
        setPropDep(lastObj,0,0);
    lastObj = 0;

    if(idx >= components.size())
        return pyobj;

    size_t count = components.size();
    if(value) --count;

    for(;idx<count;++idx)  {
        if(PyObject_TypeCheck(*pyobj, &DocumentObjectPy::Type))
            lastObj = static_cast<DocumentObjectPy*>(*pyobj)->getDocumentObjectPtr();

        if(lastObj) {
            const char *attr = components[idx].getName().c_str();
            auto prop = lastObj->getPropertyByName(attr);
            if(!prop && pyobj.hasAttr(attr))
                attr = 0;
            setPropDep(lastObj,prop,attr);
            if(value && prop && idx+1 < components.size()) {
                ObjectIdentifier path(*prop);
                path.components.insert(path.components.end(), components.begin()+idx+1, components.end());
                if(prop->setPyPathValue(path, *value))
                    return Py::Object();
            }
            lastObj = 0;
        }
        pyobj = components[idx].get(pyobj);
    }
    if(value) {
        components[idx].set(pyobj,*value);
        return Py::Object();
    }
    return pyobj;
}

/**
 * @brief Get the value of the property or field pointed to by this object identifier.
 *
 * All type of objects are supported. Some types are casted to FC native
 * type, including: Int, Float, String, Unicode String, and Quantities. Others
 * are just kept as Python object wrapped by App::any.
 *
 * @param pathValue: if true, calls the property's getPathValue(), which is
 * necessary for Qunatities to work.
 *
 * @return The value of the property or field.
 */

App::any ObjectIdentifier::getValue(bool pathValue, bool *isPseudoProperty) const
{
    ResolveResults rs(*this);

    if(isPseudoProperty) {
        *isPseudoProperty = rs.propertyType!=PseudoNone;
        if(rs.propertyType == PseudoSelf
                && isLocalProperty()
                && rs.propertyIndex+1 < (int)components.size()
                && owner->getPropertyByName(components[rs.propertyIndex+1].getName().c_str()))
        {
            *isPseudoProperty = false;
        }
    }

    if(rs.resolvedProperty && rs.propertyType==PseudoNone && pathValue)
        return rs.resolvedProperty->getPathValue(*this);

    Base::PyGILStateLocker lock;
    try {
        return pyObjectToAny(access(rs));
    }catch(Py::Exception &) {
        Base::PyException::ThrowException();
    }
    return App::any();
}

Py::Object ObjectIdentifier::getPyValue(bool pathValue, bool *isPseudoProperty, bool *isReadOnly) const
{
    ResolveResults rs(*this);

    if(isPseudoProperty || isReadOnly) {
        bool pseudo = rs.propertyType!=PseudoNone;
        if(rs.propertyType == PseudoSelf
                && isLocalProperty()
                && rs.propertyIndex+1 < (int)components.size()
                && owner->getPropertyByName(components[rs.propertyIndex+1].getName().c_str()))
        {
            pseudo = false;
        }
        if(isPseudoProperty)
            *isPseudoProperty = pseudo;
        if(isReadOnly) {
            *isReadOnly = pseudo || (rs.resolvedProperty
                    && (rs.resolvedProperty->testStatus(Property::Immutable)
                        || rs.resolvedProperty->testStatus(Property::PropReadOnly)));
        }
    }

    try {
        if(rs.resolvedProperty && rs.propertyType==PseudoNone && pathValue) {
            Py::Object res;
            if(rs.resolvedProperty->getPyPathValue(*this,res))
                return res;
        }
        return access(rs);
    }catch(Py::Exception &) {
        Base::PyException::ThrowException();
    }
    return Py::Object();
}

/**
 * @brief Set value of a property or field pointed to by this object identifier.
 *
 * This method uses Python to do the actual work. and a limited set of types that
 * can be in the App::any variable is supported: Base::Quantity, double,
 * char*, const char*, int, unsigned int, short, unsigned short, char, and unsigned char.
 *
 * @param value Value to set
 */

void ObjectIdentifier::setValue(const App::any &value) const
{
    Base::PyGILStateLocker lock;
    setPyValue(pyObjectFromAny(value));
}

void ObjectIdentifier::setPyValue(Py::Object value) const
{
    ResolveResults rs(*this);
    if(rs.propertyType)
        FC_THROWM(Base::RuntimeError,"Cannot set pseudo property " << toString());
    if(!rs.resolvedProperty)
        FC_THROWM(Base::RuntimeError,"Property not found " << toString());
    if(rs.resolvedProperty->testStatus(Property::Immutable)
            || rs.resolvedProperty->testStatus(Property::PropReadOnly))
        FC_THROWM(Base::RuntimeError,"Cannot set read-only property: " << toString());

    try {
        access(rs,&value);
    }catch(Py::Exception &) {
        Base::PyException::ThrowException();
    }
}

const std::string &ObjectIdentifier::getSubObjectName(bool newStyle) const {
    if(newStyle && shadowSub.first.size())
        return shadowSub.first;
    if(shadowSub.second.size())
        return shadowSub.second;
    return subObjectName.getString();
}

const std::string &ObjectIdentifier::getSubObjectName() const {
    return subObjectName.getString();
}

void ObjectIdentifier::importSubNames(const ObjectIdentifier::SubNameMap &subNameMap)
{
    if(!owner || !owner->getDocument())
        return;
    ResolveResults result(*this);
    auto it = subNameMap.find(std::make_pair(result.resolvedDocumentObject,std::string()));
    if(it!=subNameMap.end()) {
        auto obj = owner->getDocument()->getObject(it->second.c_str());
        if(!obj) {
            FC_ERR("Failed to find import object " << it->second << " from "
                    << result.resolvedDocumentObject->getFullName());
            return;
        }
        documentNameSet = false;
        documentName.str.clear();
        if(documentObjectName.isRealString())
            documentObjectName.str = obj->Label.getValue();
        else
            documentObjectName.str = obj->getNameInDocument();
        _cache.clear();
    }
    if(subObjectName.getString().empty())
        return;
    it = subNameMap.find(std::make_pair(
                result.resolvedDocumentObject,subObjectName.str));
    if(it==subNameMap.end())
        return;
    subObjectName = String(it->second,true);
    _cache.clear();
    shadowSub.first.clear();
    shadowSub.second.clear();
}

bool ObjectIdentifier::updateElementReference(ExpressionVisitor &v,
        App::DocumentObject *feature, bool reverse)
{
    assert(v.getPropertyLink());
    if(subObjectName.getString().empty())
        return false;

    ResolveResults result(*this);
    if(!result.resolvedSubObject)
        return false;
    if(v.getPropertyLink()->_updateElementReference(
            feature,result.resolvedDocumentObject,subObjectName.str,shadowSub,reverse)) {
        _cache.clear();
        v.aboutToChange();
        return true;
    }
    return false;
}

bool ObjectIdentifier::adjustLinks(ExpressionVisitor &v, const std::set<App::DocumentObject *> &inList) {
    ResolveResults result(*this);
    if(!result.resolvedDocumentObject)
        return false;
    if(result.resolvedSubObject) {
        PropertyLinkSub prop;
        prop.setValue(result.resolvedDocumentObject, {subObjectName.getString()});
        if(prop.adjustLink(inList)) {
            v.aboutToChange();
            documentObjectName = String(prop.getValue()->getNameInDocument(),false,true);
            subObjectName = String(prop.getSubValues().front(),true);
            _cache.clear();
            return true;
        }
    }
    return false;
}

bool ObjectIdentifier::isTouched() const {
    try {
        ResolveResults result(*this);
        if(result.resolvedProperty) {
            if(result.propertyType==PseudoNone)
                return result.resolvedProperty->isTouched();
            else
                return result.resolvedDocumentObject->isTouched();
        }
    }catch(...) {}
    return false;
}

void ObjectIdentifier::resolveAmbiguity() {
    if(!owner || !owner->getNameInDocument())
        return;

    if(subObjectName.getString().empty() && components.size()==1) {
       if(isLocalProperty()
               || (documentObjectNameSet && documentObjectName.getString().size()
                   && (documentObjectName.isRealString() || documentObjectName.isForceIdentifier())))
        {
            return;
        }
    }

    ResolveResults result(*this);
    resolveAmbiguity(result);
}

void ObjectIdentifier::resolveAmbiguity(ResolveResults &result) {

    if(!result.resolvedDocumentObject)
        return;

    _cache.clear();

    if(result.propertyIndex >= (int)components.size()) {
        components.clear();
    } else if (result.propertyIndex > 0) {
        components.erase(components.begin(), components.begin() + result.propertyIndex);
    }

    std::string s = result.subObjectName;
    if(s.size() && s[0] == '.')
        s.erase(s.begin());
    String subname(std::move(s), true);
    if(result.resolvedDocumentObject == owner) {
        setDocumentObjectName(owner,false,std::move(subname));
    }else if(result.flags.test(ResolveByIdentifier))
        setDocumentObjectName(std::move(result.resolvedDocumentObject),true,std::move(subname));
    else
        setDocumentObjectName(
                String(result.resolvedDocumentObject->Label.getStrValue(),true,false),true,std::move(subname));

    if(result.resolvedDocumentObject->getDocument() == owner->getDocument())
        setDocumentName(String());
}

/** Construct and initialize a ResolveResults object, given an ObjectIdentifier instance.
 *
 * The constructor will invoke the ObjectIdentifier's resolve() method to initialize the object's data.
 */

ObjectIdentifier::ResolveResults::ResolveResults(const ObjectIdentifier &oi)
    : propertyIndex(0)
    , resolvedDocument(0)
    , resolvedDocumentName()
    , resolvedDocumentObject(0)
    , resolvedDocumentObjectName()
    , resolvedSubObject(0)
    , resolvedProperty(0)
    , propertyName()
    , propertyType(PseudoNone)
{
    oi.resolve(*this);
}

std::string ObjectIdentifier::ResolveResults::resolveErrorString() const
{
    std::ostringstream ss;
    if (resolvedDocument == 0) {
        if(flags.test(ResolveAmbiguous)) 
            ss << "Ambiguous document name/label '" 
               << resolvedDocumentName << "'";
        else
            ss << "Document '" << resolvedDocumentName << "' not found";
    } else if (resolvedDocumentObject == 0) {
        if(flags.test(ResolveAmbiguous))
            ss << "Ambiguous document object name '" 
                << resolvedDocumentObjectName << "'";
        else
            ss << "Document object '" << resolvedDocumentObjectName 
                << "' not found";
    } else if (subObjectName.getString().size() && resolvedSubObject == 0) {
        ss << "Sub-object '" << resolvedDocumentObjectName.getString()
            << '.' << subObjectName << "' not found";
    } else if (resolvedProperty == 0) {
        if(propertyType != PseudoShape &&
           subObjectName.getString().size() &&
           !boost::ends_with(subObjectName.getString(),"."))
        {
            ss << "Non geometry subname reference must end with '.'\n"
               << "Or use '_shape' to access the geometry reference,\n"
               << resolvedDocumentObjectName.toString() 
               << '.' << subObjectName.toString() << "._shape\n";
        }else
            ss << "Property '" << propertyName << "' not found";
    }

    return ss.str();
}

void ObjectIdentifier::ResolveResults::getProperty(const ObjectIdentifier &oi) {
    resolvedProperty = oi.resolveProperty(resolvedDocumentObject,
                                          subObjectName,
                                          propertyIndex,
                                          resolvedSubObject,
                                          propertyType);
    if(propertyIndex < (int)oi.components.size())
        propertyName = oi.components[propertyIndex].getName();
}

