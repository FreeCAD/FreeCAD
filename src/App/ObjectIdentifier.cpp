/***************************************************************************
 *   Copyright (c) Eivind Kvedalen        (eivind@kvedalen.name) 2015      *
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

#include <boost/algorithm/string/predicate.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/GeometryPyCXX.h>
#include <App/ComplexGeoData.h>
#include "Property.h"
#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "ObjectIdentifier.h"
#include "ExpressionParser.h"
#include <Base/Tools.h>
#include <Base/Interpreter.h>
#include <Base/QuantityPy.h>
#include <App/Link.h>
#include <Base/Console.h>

FC_LOG_LEVEL_INIT("Expression",true,true)

using namespace App;
using namespace Base;

/**
 * @brief Compute a hash value for the object identifier given by \a path.
 * @param path Inputn path
 * @return Hash value
 */

std::size_t App::hash_value(const App::ObjectIdentifier & path)
{
    return path.hash();
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
    std::stringstream output;

    std::string::const_iterator cur = input.begin();
    std::string::const_iterator end = input.end();

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

    return output.str();
}


/**
 * @brief Construct an ObjectIdentifier object, given an owner and a single-value property.
 * @param _owner Owner of property.
 * @param property Name of property.
 */

ObjectIdentifier::ObjectIdentifier(const App::PropertyContainer * _owner, 
        const std::string & property, int index)
    : owner(0)
    , documentNameSet(false)
    , documentObjectNameSet(false)
    , localProperty(false)
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

ObjectIdentifier::ObjectIdentifier(const App::PropertyContainer * _owner, bool localProperty)
    :localProperty(localProperty)
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
    , documentNameSet(false)
    , documentObjectNameSet(false)
    , localProperty(false)
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(prop.getContainer());

    if (!docObj)
        FC_THROWM(Base::TypeError,"Property must be owned by a document object.");

    owner = const_cast<DocumentObject*>(docObj);

    setDocumentObjectName(docObj);

    addComponent(SimpleComponent(String(prop.getName())));
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
 * @param i Index to get
 * @return A component.
 */

const App::ObjectIdentifier::Component &App::ObjectIdentifier::getPropertyComponent(int i) const
{
    ResolveResults result(*this);

    assert(result.propertyIndex + i >=0 && static_cast<std::size_t>(result.propertyIndex) + i < components.size());

    return components[result.propertyIndex + i];
}

App::ObjectIdentifier::Component &App::ObjectIdentifier::getPropertyComponent(int i)
{
    ResolveResults result(*this);
    assert(result.propertyIndex + i >=0 && 
            static_cast<std::size_t>(result.propertyIndex) + i < components.size());
    return components[result.propertyIndex + i];
}

std::vector<ObjectIdentifier::Component> ObjectIdentifier::getPropertyComponents() const {
    if(components.size()<=1 || documentObjectName.getString().empty())
        return components;
    ResolveResults result(*this);
    if(result.propertyIndex==0)
        return components;
    std::vector<ObjectIdentifier::Component> res;
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

    return components.size() - result.propertyIndex;
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

    if(result.propertyIndex >= (int)components.size())
        return _cache;
    
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
            s << documentName.toString() << "#"
              << documentObjectName.toString() << '.';
        else if(result.resolvedDocumentObjectName.getString().size())
            s << documentName.toString() << "#"
              << result.resolvedDocumentObjectName.toString() << '.';
    } else if (documentObjectNameSet && documentObjectName.getString().size()) {
        s << documentObjectName.toString() << '.';
    } else if (result.propertyIndex > 0) {
        components[0].toString(s);
        s << '.';
    }

    if(subObjectName.getString().size())
        s << subObjectName.toString() << '.';

    s << components[result.propertyIndex].getName();
    getSubPathStr(s,result);
    _cache = s.str();
    return _cache;
}

std::string ObjectIdentifier::toPersistentString() const {

    if(!owner)
        return std::string();

    std::ostringstream s;
    ResolveResults result(*this);

    if(result.propertyIndex >= (int)components.size())
        return std::string();
    
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
    } else if (documentNameSet && documentName.getString().size()) {
        if(documentObjectNameSet && documentObjectName.getString().size())
            s << documentName.toString() << "#"
                << documentObjectName.toString() << '.';
        else if(result.resolvedDocumentObjectName.getString().size())
            s << documentName.toString() << "#"
                << result.resolvedDocumentObjectName.toString() << '.';
    } else if (documentObjectNameSet && documentObjectName.getString().size()) {
        s << documentObjectName.toString() << '.';
    } else if (result.propertyIndex > 0) {
        components[0].toString(s);
        s << '.';
    }

    if(subObjectName.getString().size()) {
        const char *subname = subObjectName.getString().c_str();

        std::string exportName = PropertyLinkBase::exportSubName(result.resolvedDocumentObject,subname);
        if(exportName.size())
            s << String(exportName,true).toString() << '.';
        else
            s << subObjectName.toString() << '.';
    }

    s << components[result.propertyIndex].getName();
    getSubPathStr(s,result);
    return s.str();
}

std::size_t ObjectIdentifier::hash() const
{
    if(_hash && _cache.size())
        return _hash;
    _hash = boost::hash_value(toString());
    return _hash;
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

void ObjectIdentifier::getSubPathStr(std::ostream &s, const ResolveResults &result, bool toPython) const
{
    std::vector<Component>::const_iterator i = components.begin() + result.propertyIndex + 1;
    while (i != components.end()) {
        if(i->isSimple())
            s << '.';
        i->toString(s,toPython);
        ++i;
    }
}

std::string ObjectIdentifier::getSubPathStr(bool toPython) const {
    std::ostringstream ss;
    getSubPathStr(ss,ResolveResults(*this),toPython);
    return ss.str();
}

/**
 * @brief Construct a Component part
 * @param _name Name of component
 * @param _type Type; simple, array, range or map
 * @param _begin Array index or begining of a Range, or INT_MAX for other type.
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
        res = pyobj.getAttr(getName());
    } else if(isArray()) {
        if(pyobj.isMapping())
            res = Py::Mapping(pyobj).getItem(Py::Int(begin));
        else
            res = Py::Sequence(pyobj).getItem(begin);
    }else if(isMap())
        res = Py::Mapping(pyobj).getItem(getName());
    else {
        assert(isRange());
        Py::Object slice(PySlice_New(Py::Int(begin).ptr(),
                                    end!=INT_MAX?Py::Int(end).ptr():0,
                                    step!=1?Py::Int(step).ptr():0));
        PyObject *r = PyObject_GetItem(pyobj.ptr(),slice.ptr());
        if(!r)
            Base::PyException::ThrowException();
        res = Py::asObject(r);
    }
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
    }else if(isMap())
        Py::Mapping(pyobj).setItem(getName(),value);
    else {
        assert(isRange());
        Py::Object slice(PySlice_New(Py::Int(begin).ptr(),
                                    end!=INT_MAX?Py::Int(end).ptr():0,
                                    step!=1?Py::Int(step).ptr():0));
        if(PyObject_SetItem(pyobj.ptr(),slice.ptr(),value.ptr())<0)
            Base::PyException::ThrowException();
    }
}

void ObjectIdentifier::Component::del(Py::Object &pyobj) const {
    if(isSimple())
        pyobj.delAttr(getName());
    else if(isArray()) {
        if(pyobj.isMapping())
            Py::Mapping(pyobj).delItem(Py::Int(begin));
        else
            PySequence_DelItem(pyobj.ptr(),begin);
    } else if(isMap())
        Py::Mapping(pyobj).delItem(getName());
    else {
        assert(isRange());
        Py::Object slice(PySlice_New(Py::Int(begin).ptr(),
                                    end!=INT_MAX?Py::Int(end).ptr():0,
                                    step!=1?Py::Int(step).ptr():0));
        if(PyObject_DelItem(pyobj.ptr(),slice.ptr())<0)
            Base::PyException::ThrowException();
    }
}

/**
 * @brief Create a simple component part with the given name
 * @param _component Name of component.
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::SimpleComponent(const char *_component)
{
    return Component(String(_component));
}

/**
 * @brief Create a simple component part with the given name
 * @param _component Name of component.
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::SimpleComponent(const ObjectIdentifier::String &_component)
{
    return Component(_component);
}

ObjectIdentifier::Component ObjectIdentifier::SimpleComponent(ObjectIdentifier::String &&_component)
{
    return Component(std::move(_component));
}

/**
 * @brief Create an array component with given name and index.
 * @param _component Name of component
 * @param _index Index of component
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::ArrayComponent(int _index)
{
    return Component(String(), Component::ARRAY, _index);
}

/**
 * @brief Create a map component with given name and key.
 * @param _component Name of component
 * @param _key Key of component
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::MapComponent(const String & _key)
{
    return Component(_key, Component::MAP);
}

ObjectIdentifier::Component ObjectIdentifier::MapComponent(String &&_key)
{
    return Component(std::move(_key), Component::MAP);
}


/**
 * @brief Create a range component with given begin and end.
 * @param _begin begining index of the range
 * @param _end ending index of the range
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::RangeComponent(int _begin, int _end, int _step)
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
    case SIMPLE:
    case MAP:
        return name == other.name;
    case ARRAY:
        return begin == other.begin;
    case RANGE:
        return begin == other.begin && end == other.end && step==other.step;
    default:
        assert(0);
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
        ss << name.getString();
        break;
    case Component::MAP:
        ss << "[" << name.toString(toPython) << "]";
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
        assert(0);
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
    if(!owner)
        return;

    bool docAmbiguous = false;

    /* Document name specified? */
    if (documentName.getString().size() > 0) {
        results.resolvedDocument = getDocument(documentName,&docAmbiguous);
        results.resolvedDocumentName = documentName;
    }
    else {
        results.resolvedDocument = owner->getDocument();
        results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);
    }

    results.subObjectName = subObjectName;
    results.propertyName = "";
    results.propertyIndex = 0;

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
            results.propertyName = components[0].name.getString();
            results.propertyIndex = 0;
            results.getProperty(*this);
        }
        else
            return;
    }
    else {
        /* Document object name not specified, resolve from path */

        /* One component? */
        if (components.size() == 1 || (components.size()>1 && !components[0].isSimple())) {
            /* Yes -- then this must be a property, so we get the document object's name from the owner */
            results.resolvedDocumentObjectName = String(owner->getNameInDocument(), false, true);
            results.resolvedDocumentObject = owner;
            results.propertyName = components[0].name.getString();
            results.propertyIndex = 0;
            results.getProperty(*this);
        }
        else if (components.size() >= 2) {
            /* No --  */
            if (!components[0].isSimple())
                return;

            results.resolvedDocumentObject = getDocumentObject(
                    results.resolvedDocument, components[0].name, results.flags);

            /* Possible to resolve component to a document object? */
            if (results.resolvedDocumentObject) {
                /* Yes */
                results.resolvedDocumentObjectName = String(
                        components[0].name, false, results.flags.test(ResolveByIdentifier));
                results.propertyName = components[1].name.getString();
                results.propertyIndex = 1;
                results.getProperty(*this);
                if(!results.resolvedProperty) {
                    // If the second component is not a property name, try to
                    // interpret the first component as the property name.
                    DocumentObject *sobj = 0;
                    results.resolvedProperty = resolveProperty(
                            owner,components[0].name,sobj,results.propertyType);
                    if(results.resolvedProperty) {
                        results.propertyName = components[0].name.getString();
                        results.resolvedDocument = owner->getDocument();
                        results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);
                        results.resolvedDocumentObjectName = String(owner->getNameInDocument(), false, true);
                        results.resolvedDocumentObject = owner;
                        results.resolvedSubObject = sobj;
                        results.propertyIndex = 0;
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
                results.propertyName = components[results.propertyIndex].name.getString();
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


enum PseudoPropertyType {
    PseudoNone,
    PseudoShape,
    PseudoPlacement,
    PseudoMatrix,
    PseudoLinkPlacement,
    PseudoLinkMatrix,
    PseudoSelf,
    PseudoApp,
    PseudoPart,
    PseudoRegex,
    PseudoBuiltins,
    PseudoMath,
    PseudoCollections,
    PseudoGui,
    PseudoCadquery,
};

std::pair<DocumentObject*,std::string> ObjectIdentifier::getDep(std::vector<std::string> *labels) const {
    ResolveResults result(*this);
    if(labels) {
        if(documentObjectName.getString().size()) {
            if(documentObjectName.isRealString())
                labels->push_back(documentObjectName.getString());
        } else if(result.propertyIndex == 1)
            labels->push_back(components[0].name.getString());
        if(subObjectName.getString().size()) 
            PropertyLinkBase::getLabelReferences(*labels,subObjectName.getString().c_str());
    }
    if(subObjectName.getString().empty()) {
        if(result.propertyType==PseudoNone) {
            CellAddress addr;
            if(addr.parseAbsoluteAddress(result.propertyName.c_str())) 
                return std::make_pair(result.resolvedDocumentObject,addr.toString(true));
            return std::make_pair(result.resolvedDocumentObject,result.propertyName);
        }else if(result.propertyType == PseudoSelf
                    && result.resolvedDocumentObject
                    && result.propertyIndex+1 < (int)components.size())
        {
            return std::make_pair(result.resolvedDocumentObject,
                    components[result.propertyIndex+1].getName());
        }
    }
    return std::make_pair(result.resolvedDocumentObject,std::string());
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
        if (documentNameSet)
            l.push_back(documentName.toString());

        if (documentObjectNameSet)
            l.push_back(documentObjectName.toString());
    }
    if(subObjectName.getString().size()) {
        l.back() += subObjectName.toString();
    }
    std::vector<Component>::const_iterator i = components.begin();
    while (i != components.end()) {
        std::ostringstream ss;
        i->toString(ss);
        l.push_back(ss.str());
        ++i;
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
    components.push_back(value);
    _cache.clear();
    return *this;
}

ObjectIdentifier &ObjectIdentifier::operator <<(ObjectIdentifier::Component &&value)
{
    components.push_back(std::move(value));
    _cache.clear();
    return *this;
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

Property *ObjectIdentifier::resolveProperty(const App::DocumentObject *obj, 
        const char *propertyName, App::DocumentObject *&sobj, int &ptype) const 
{
    if(obj && subObjectName.getString().size()) {
        sobj = obj->getSubObject(subObjectName);
        obj = sobj;
    }
    if(!obj)
        return 0;

    static std::map<std::string,int> _props = {
        {"_shape",PseudoShape}, 
        {"_pla",PseudoPlacement},
        {"_matrix",PseudoMatrix},
        {"__pla",PseudoLinkPlacement},
        {"__matrix",PseudoLinkMatrix},
        {"_self",PseudoSelf},
        {"_app",PseudoApp},
        {"_part",PseudoPart},
        {"_re",PseudoRegex},
        {"_py", PseudoBuiltins},
        {"_math", PseudoMath},
        {"_coll", PseudoCollections},
        {"_gui",PseudoGui},
        {"_cq",PseudoCadquery},
    };
    auto it = _props.find(propertyName);
    if(it == _props.end())
        ptype = PseudoNone;
    else {
        ptype = it->second;
        if(ptype != PseudoShape && 
           subObjectName.getString().size() &&
           !boost::ends_with(subObjectName.getString(),"."))
        {
            return 0;
        }
        return &const_cast<App::DocumentObject*>(obj)->Label; //fake the property
    }
    
    auto prop = obj->getPropertyByName(propertyName);
    if(prop && !prop->testStatus(Property::Hidden) && !(prop->getType() & PropertyType::Prop_Hidden))
        return prop;

    auto linked = obj->getLinkedObject(true);
    if(!linked || linked==obj) {
#if 0
        auto ext = obj->getExtensionByType<App::LinkBaseExtension>(true);
        if(!ext)
            return prop;
        linked = ext->getTrueLinkedObject(true);
        if(!linked || linked==obj)
            return prop;
#else
        return prop;
#endif
    }

    auto linkedProp = linked->getPropertyByName(propertyName);
    return linkedProp?linkedProp:prop;
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
    subObjectName = std::move(subname);

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
    if (isRealString())
        return quote(str,toPython);
    else
        return str;
}

void ObjectIdentifier::String::checkImport(const App::DocumentObject *owner,
        const App::DocumentObject *obj, String *objName) 
{
    if(owner && owner->getDocument() &&
       str.size() && 
       ExpressionParser::ExpressionImporter::reader()) 
    {
        auto reader = ExpressionParser::ExpressionImporter::reader();
        if(obj || objName) {
            bool restoreLabel = false;
            str = PropertyLinkBase::importSubName(*reader,str.c_str(),restoreLabel);
            if(restoreLabel) {
                if(!obj) {
                    std::bitset<32> flags;
                    obj = getDocumentObject(owner->getDocument(),*objName,flags);
                    if(!obj)
                        FC_ERR("Cannot find object " << objName->toString());
                }
                PropertyLinkBase::restoreLabelReference(obj,str);
            }
        } else if (str.back()!='@')
            str = reader->getName(str.c_str());
        else{
            str.resize(str.size()-1);
            auto mapped = reader->getName(str.c_str());
            auto obj = owner->getDocument()->getObject(mapped);
            if(!obj) 
                FC_ERR("Cannot find object " << str);
            else {
                isString = true;
                forceIdentifier = false;
                str = obj->Label.getValue();
            }
        }
    }
}

Py::Object ObjectIdentifier::access(const ResolveResults &result, Py::Object *value) const
{
    if(!result.resolvedDocumentObject || !result.resolvedProperty ||
       (subObjectName.getString().size() && !result.resolvedSubObject))
    {
        FC_THROWM(Base::RuntimeError, result.resolveErrorString() << std::endl
           << "in '" << toString() << "'");
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
        Base::Matrix4D mat;
        auto obj = result.resolvedDocumentObject;
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
        case PseudoLinkMatrix:
            obj->getLinkedObject(true,&mat,false);
            if(ptype == PseudoLinkPlacement)
                pyobj = Py::Placement(Base::Placement(mat));
            else
                pyobj = Py::Matrix(mat);
            break;
        case PseudoSelf:
            pyobj = Py::Object(obj->getPyObject(),true);
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
            if(container!=result.resolvedDocumentObject && container!=result.resolvedSubObject) {
                obj = obj->getLinkedObject(true);
                assert(obj);
            }
            pyobj = Py::Object(obj->getPyObject(),true);
            idx = result.propertyIndex;
            break;
        }}}
    }
    if(components.empty())
        return pyobj;
    size_t count = components.size();
    if(value) --count;
    assert(idx<=count);
    for(;idx<count;++idx) 
        pyobj = components[idx].get(pyobj);
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

Py::Object ObjectIdentifier::getPyValue(bool pathValue, bool *isPseudoProperty) const
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

    if(rs.resolvedProperty && rs.propertyType==PseudoNone && pathValue) {
        Py::Object res;
        if(rs.resolvedProperty->getPyPathValue(*this,res))
            return res;
    }

    try {
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
    std::stringstream ss;
    ResolveResults rs(*this);
    if(rs.propertyType)
        FC_THROWM(Base::RuntimeError,"Cannot set pseudo property");

    Base::PyGILStateLocker lock;
    try {
        Py::Object pyvalue = pyObjectFromAny(value);
        access(rs,&pyvalue);
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
    if(!result.resolvedSubObject) {
        if(inList.count(result.resolvedDocumentObject))
            FC_THROWM(Base::RuntimeError,"cyclic reference to " 
                    << result.resolvedDocumentObject->getFullName());
    }else{
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
    if(!owner || !owner->getNameInDocument() || isLocalProperty() ||
       (documentObjectNameSet && documentObjectName.getString().size() && 
        (documentObjectName.isRealString() || documentObjectName.isForceIdentifier())))
    {
        return;
    }

    ResolveResults result(*this);
    resolveAmbiguity(result);
}

void ObjectIdentifier::resolveAmbiguity(ResolveResults &result) {

    if(!result.resolvedDocumentObject)
        return;

    if(result.propertyIndex==1)
        components.erase(components.begin());

    String subname = subObjectName;
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
               << resolvedDocumentName.getString() << "'";
        else
            ss << "Document '" << resolvedDocumentName.toString() << "' not found";
    } else if (resolvedDocumentObject == 0) {
        if(flags.test(ResolveAmbiguous))
            ss << "Ambiguous document object name '" 
                << resolvedDocumentObjectName.getString() << "'";
        else
            ss << "Document object '" << resolvedDocumentObjectName.toString() 
                << "' not found";
    } else if (subObjectName.getString().size() && resolvedSubObject == 0) {
        ss << "Sub-object '" << resolvedDocumentObjectName.getString()
            << '.' << subObjectName.toString() << "' not found";
    } else if (resolvedProperty == 0) {
        if(propertyType != PseudoShape && 
           subObjectName.getString().size() &&
           !boost::ends_with(subObjectName.getString(),"."))
        {
            ss << "Non geometry subname reference must end with '.'";
        }else
            ss << "Property '" << propertyName << "' not found";
    }

    return ss.str();
}

void ObjectIdentifier::ResolveResults::getProperty(const ObjectIdentifier &oi) {
    resolvedProperty = oi.resolveProperty(
            resolvedDocumentObject,propertyName.c_str(),resolvedSubObject,propertyType);
}

