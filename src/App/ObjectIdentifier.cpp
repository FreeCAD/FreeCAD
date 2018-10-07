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

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/GeometryPyCXX.h>
#include "Property.h"
#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "ObjectIdentifier.h"
#include "Expression.h"
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

std::string App::quote(const std::string &input)
{
    std::stringstream output;

    std::string::const_iterator cur = input.begin();
    std::string::const_iterator end = input.end();

    output << "<<";
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
            output << "\\>";
            break;
        default:
            output << *cur;
        }
        ++cur;
    }
    output << ">>";

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
{
    if (_owner) {
        const DocumentObject * docObj = freecad_dynamic_cast<const DocumentObject>(_owner);
        if (!docObj)
            throw Base::RuntimeError("Property must be owned by a document object.");
        owner = const_cast<DocumentObject*>(docObj);

        if (property.size() > 0)
            setDocumentObjectName(docObj);
    }
    if (property.size() > 0) {
        addComponent(SimpleComponent(property));
        if(index!=INT_MAX)
            addComponent(ArrayComponent(index));
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
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(prop.getContainer());

    if (!docObj)
        throw Base::TypeError("Property must be owned by a document object.");

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
    
    if(result.resolvedProperty && 
       result.resolvedDocumentObject==owner && 
       components.size()>1 && 
       components[1].isSimple() &&
       result.propertyIndex==0) 
    {
        s << '.';
    }else{
        if (documentNameSet && documentName.getString().size())
            s << documentName.toString() << "#";
        if (documentObjectNameSet && documentObjectName.getString().size())
            s << documentObjectName.toString() << ".";
        else if (result.propertyIndex > 0) {
            components[0].toString(s);
            s << ".";
        }
    }

    if(subObjectName.getString().size())
        s << subObjectName.toString() << '.';

    s << components[result.propertyIndex].getName();
    getSubPathStr(s,result);
    _cache = s.str();
    return _cache;
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

/**
 * @brief Modify object identifier given that document object \a oldName gets the new name \a newName.
 * @param oldName Name of current document object
 * @param newName New name of document object
 */

bool ObjectIdentifier::renameDocumentObject(const App::DocumentObject *obj, ExpressionVisitor *v)
{
    if(!owner)
        return false;

    if(documentName.getString().size()) {
        if(documentName.getString()!=obj->getDocument()->getName())
            return false;
    }else if(owner->getDocument()!=obj->getDocument())
        return false;

    if(documentObjectName.getString().size()) {
        if(documentObjectName.isForceIdentifier())
            return false;

        if(!documentObjectName.isRealString() && 
           documentObjectName.getString()==obj->getNameInDocument())
            return false;

        if(documentObjectName!=obj->getOldLabel())
            return false;

        if(v) v->setExpressionChanged();
        documentObjectName = ObjectIdentifier::String(obj->Label.getStrValue(), true);

        _cache.clear();
        return true;
    }

    ResolveResults result(*this);
    if (result.resolvedDocumentObject==obj && 
        result.propertyIndex == 1 && 
        result.resolvedDocumentObjectName==obj->getOldLabel()) 
    {
        if(v) v->setExpressionChanged();
        components[0].name = ObjectIdentifier::String(obj->Label.getStrValue(), true);
        _cache.clear();
        return true;
    }

    // If object identifier uses the label then resolving the document object will fail.
    // So, it must be checked if using the new label will succeed
    if (components.size()>1 && components[0].getName()==obj->getOldLabel()) {
        ObjectIdentifier id(*this);
        id.components[0].name = obj->Label.getStrValue();

        ResolveResults result(id);

        if (result.propertyIndex == 1 && result.resolvedDocumentObject == obj) {
            components[0].name = ObjectIdentifier::String(obj->Label.getStrValue(), true);
            _cache.clear();
            return true;
        }
    }
    return false;
}

/**
 * @brief Modify object identifier given that the document \a oldName has changed name to \a newName.
 * @param oldName Name of current document
 * @param newName New name of document
 */

bool ObjectIdentifier::renameDocument(
        const std::string &oldName, const std::string &newName, ExpressionVisitor *v)
{
    if (oldName == newName)
        return false;

    if (documentNameSet && documentName == oldName) {
        if(v) v->setExpressionChanged();
        documentName = newName;
        _cache.clear();
        return true;
    }
    else {
        ResolveResults result(*this);

        if (result.resolvedDocumentName == oldName) {
            if(v) v->setExpressionChanged();
            documentName = newName;
            _cache.clear();
            return true;
        }
    }
    return false;
}

/**
 * @brief Get sub field part of a property as a string.
 * @return String representation of path.
 */

void ObjectIdentifier::getSubPathStr(std::ostringstream &s, const ResolveResults &result, bool toPython) const
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

ObjectIdentifier::Component::Component(const String &_name, ObjectIdentifier::Component::typeEnum _type, int _begin, int _end)
    : name(_name)
    , type(_type)
    , begin(_begin)
    , end(_end)
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
    FC_THROWM(Base::RuntimeError, "Array out of bound: " << begin << ", " << count);
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

/**
 * @brief Create a range component with given begin and end.
 * @param _begin begining index of the range
 * @param _end ending index of the range
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::RangeComponent(int _begin, int _end)
{
    return Component(String(), Component::RANGE, _begin, _end);
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
        return begin == other.begin && end == other.end;
    default:
        assert(0);
        return false;
    }
}

/**
 * @brief Create a string representation of a component.
 * @return A string representing the component.
 */

void ObjectIdentifier::Component::toString(std::ostringstream &ss, bool toPython) const
{
    switch (type) {
    case Component::SIMPLE:
        ss << name.getString();
        break;
    case Component::MAP:
        if(toPython)
            ss << "['" << name.getString() << "']";
        else
            ss << "[" << name.toString() << "]";
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
        const String & name, std::bitset<32> &flags) const
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

    /* Document name specified? */
    if (documentName.getString().size() > 0) {
        results.resolvedDocument = getDocument(documentName);
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
        if (documentName.getString().size() > 0)
            return;

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

Document * ObjectIdentifier::getDocument(String name) const
{
    if (name.getString().size() == 0)
        name = getDocumentName();

    App::Document * docById = App::GetApplication().getDocument(name);

    if (name.isForceIdentifier())
        return docById;

    App::Document * docByLabel = 0;
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();

    for (std::vector<App::Document*>::const_iterator i = docs.begin(); i != docs.end(); ++i) {
        if ((*i)->Label.getValue() == name.getString()) {
            /* Multiple hits for same label? */
            if (docByLabel != 0)
                return 0;
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
        return docByLabel == docById ? docById : 0;
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
};

std::pair<DocumentObject*,std::string> ObjectIdentifier::getDep() const {
    ResolveResults result(*this);
    if(result.propertyType==PseudoNone && subObjectName.getString().empty())
        return std::make_pair(result.resolvedDocumentObject,result.propertyName);
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
            l.push_back(result.resolvedDocumentName.toString());

        if (documentObjectNameSet)
            l.push_back(result.resolvedDocumentObjectName.toString());
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
        result.setDocumentName(thisresult.resolvedDocumentName, true);
    if (otherresult.resolvedDocumentObject != thisresult.resolvedDocumentObject)
        result.setDocumentObjectName(thisresult.resolvedDocumentObjectName, true);

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
        throw Base::RuntimeError("Invalid property specification.");
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
        {"shape",PseudoShape}, 
        {"placement",PseudoPlacement},
        {"matrix",PseudoMatrix},
        {"placement_",PseudoLinkPlacement},
        {"matrix_",PseudoLinkMatrix},
        {"self",PseudoSelf},
        {"app",PseudoApp},
        {"part",PseudoPart},
        {"re",PseudoRegex},
    };
    auto it = _props.find(propertyName);
    if(it == _props.end())
        ptype = PseudoNone;
    else {
        ptype = it->second;
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

/**
 * @brief Set the document name for this object identifier.
 *
 * If force is true, the document name will always be included in the string representation.
 *
 * @param name Name of document object.
 * @param force Force name to be set
 */

void ObjectIdentifier::setDocumentName(const ObjectIdentifier::String &name, bool force)
{
    documentName = name;
    documentNameSet = force;
    _cache.clear();
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

void ObjectIdentifier::setDocumentObjectName(const ObjectIdentifier::String &name, bool force, 
        const ObjectIdentifier::String &subname)
{
    documentObjectName = name;
    documentObjectNameSet = force;
    subObjectName = subname;

    if(subObjectName.str.size() && subObjectName.str.back()!='.')
        subObjectName.str += '.';
    _cache.clear();
}

void ObjectIdentifier::setDocumentObjectName(const App::DocumentObject *obj, bool force,
        const ObjectIdentifier::String &subname)
{
    if(!owner || !obj || !obj->getNameInDocument() || !obj->getDocument())
        throw Base::RuntimeError("invalid object");
    if(obj == owner)
        force = false;
    if(obj->getDocument() == owner->getDocument())
        setDocumentName(String());
    else {
        documentNameSet = force;
        documentName = String(obj->getDocument()->getName(),false,true);
    }
    documentObjectNameSet = force;
    documentObjectName = String(obj->getNameInDocument(),false,true);
    subObjectName = subname;
    if(subObjectName.str.size() && subObjectName.str.back()!='.')
        subObjectName.str += '.';
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

bool ObjectIdentifier::hasDocumentObjectName() const {
    return !documentObjectName.getString().empty();
}

/**
 * @brief Get a string representation of this object identifier.
 * @return String representation.
 */

std::string ObjectIdentifier::String::toString() const
{
    if (isRealString())
        return quote(str);
    else
        return str;
}

/**
 * @brief Return a string that can be used to access the property or field pointed to by
 *        this object identifier.
 * @return Python code as a string
 */

std::string ObjectIdentifier::getPythonAccessor(const ResolveResults &result, PythonVariables &vars) const
{
    std::ostringstream ss;
    if(!result.resolvedDocumentObject || !result.resolvedProperty ||
       (subObjectName.getString().size() && !result.resolvedSubObject))
    {
        throw RuntimeError(result.resolveErrorString());
    }

    int ptype = result.propertyType;
    if(ptype == PseudoApp)
        ss << "App";
    else if(ptype == PseudoPart) {
        static bool imported = false;
        if(!imported) {
            imported = true;
            Base::Interpreter().runString("import Part");
        }
        ss << "Part";
    } else if(ptype == PseudoRegex) {
        static bool imported = false;
        if(!imported) {
            imported = true;
            Base::Interpreter().runString("import re");
        }
        ss << "re";
    } else if(ptype == PseudoShape) {
        Type::importModule("Part::");
        ss << "Part.getShape(App.getDocument('" << result.resolvedDocument->getName()
            << "').getObject("  << result.resolvedDocumentObject->getID() << ')';
        if(result.subObjectName.getString().size())
            ss << ",'" << result.subObjectName.getString() << "',needSubElement=True";
        ss << ')';
    }else{
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
            ss << vars.add(Py::Placement(Base::Placement(mat)));
            break;
        case PseudoMatrix:
            ss << vars.add(Py::Matrix(mat));
            break;
        case PseudoLinkPlacement:
        case PseudoLinkMatrix:
            obj->getLinkedObject(true,&mat,false);
            if(ptype == PseudoLinkPlacement)
                ss << vars.add(Py::Placement(Base::Placement(mat)));
            else
                ss << vars.add(Py::Matrix(mat));
            break;
        case PseudoSelf:
            ss << vars.add(Py::Object(obj->getPyObject(),true));
            break;
        default: {
            auto container = result.resolvedProperty->getContainer();
            if(container!=result.resolvedDocumentObject && container!=result.resolvedSubObject) {
                obj = obj->getLinkedObject(true);
                assert(obj);
            }
            ss << vars.add(Py::Object(obj->getPyObject(),true)) << '.' << result.propertyName;
            break;
        }}
    }
    getSubPathStr(ss,result,true);
    return ss.str();
}

/**
 * @brief Get the value of the property or field pointed to by this object identifier.
 *
 * All type of objects are supported. Some types are casted to FC native
 * type, including: Int, Float, String, Unicode String, and Quantities. Others
 * are just kept as Python object wrapped by boost::any.
 *
 * @param pathValue: if true, calls the property's getPathValue(), which is
 * necessary for Qunatities to work.
 *
 * @return The value of the property or field.
 */

boost::any ObjectIdentifier::getValue(bool pathValue) const
{
    ResolveResults rs(*this);

    if(rs.resolvedProperty && rs.propertyType==PseudoNone && pathValue)
        return rs.resolvedProperty->getPathValue(*this);

    PythonVariables vars;
    std::string name = vars.add(Py::Object());
    std::ostringstream ss;
    ss << name << '=' << getPythonAccessor(rs,vars);
    PyObject * pyvalue = Base::Interpreter().getValue(ss.str().c_str(), name.c_str());
    if (!pyvalue)
        FC_THROWM(Base::RuntimeError,"Failed to get value from " << toString());
    return pyObjectToAny(Py::Object(pyvalue,true));
}

boost::any ObjectIdentifier::pyObjectToAny(Py::Object value) {
    Base::PyGILStateLocker lock;

    if(value.isNone())
        return boost::any();

    PyObject *pyvalue = value.ptr();

#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(pyvalue))
        return boost::any(PyInt_AsLong(pyvalue));
#endif
    if (PyLong_Check(pyvalue))
        return boost::any(PyLong_AsLong(pyvalue));
    else if (PyFloat_Check(pyvalue))
        return boost::any(PyFloat_AsDouble(pyvalue));
#if PY_MAJOR_VERSION < 3
    else if (PyString_Check(pyvalue))
        return boost::any(std::string(PyString_AsString(pyvalue)));
#endif
    else if (PyUnicode_Check(pyvalue)) {
        PyObject * s = PyUnicode_AsUTF8String(pyvalue);
        if(!s) 
            FC_THROWM(Base::ValueError,"Invalid unicode string");
        Py::Object o(s,true);

#if PY_MAJOR_VERSION >= 3
        return boost::any(std::string(PyUnicode_AsUTF8(s)));
#else
        return boost::any(std::string(PyString_AsString(s)));
#endif
    }
    else if (PyObject_TypeCheck(pyvalue, &Base::QuantityPy::Type)) {
        Base::QuantityPy * qp = static_cast<Base::QuantityPy*>(pyvalue);
        Base::Quantity * q = qp->getQuantityPtr();

        return boost::any(*q);
    }
    else {
        return boost::any(value);
    }
}

/**
 * @brief Set value of a property or field pointed to by this object identifier.
 *
 * This method uses Python to do the actual work. and a limited set of types that
 * can be in the boost::any variable is supported: Base::Quantity, double,
 * char*, const char*, int, unsigned int, short, unsigned short, char, and unsigned char.
 *
 * @param value Value to set
 */

void ObjectIdentifier::setValue(const boost::any &value) const
{
    std::stringstream ss;
    ResolveResults rs(*this);
    if(rs.propertyType)
        FC_THROWM(Base::RuntimeError,"Cannot set pseudo property");

    PythonVariables vars;
    ss << getPythonAccessor(rs,vars) + " = ";

    if (value.type() == typeid(Py::Object)) {
        ss <<  vars.add(boost::any_cast<Py::Object>(value));
    }else if (value.type() == typeid(Base::Quantity))
        ss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << boost::any_cast<Base::Quantity>(value).getValue();
    else if (value.type() == typeid(double))
        ss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << boost::any_cast<double>(value);
    else if (value.type() == typeid(char*))
        ss << '\'' << Base::Tools::escapedUnicodeFromUtf8(boost::any_cast<char*>(value)) << '\'';
    else if (value.type() == typeid(const char*))
        ss << '\'' << Base::Tools::escapedUnicodeFromUtf8(boost::any_cast<const char*>(value)) << '\'';
    else if (value.type() == typeid(std::string))
        ss << '\'' << Base::Tools::escapedUnicodeFromUtf8(boost::any_cast<std::string>(value).c_str()) << '\'';
    else if (value.type() == typeid(int))
        ss << boost::any_cast<int>(value);
    else if (value.type() == typeid(unsigned int))
        ss << boost::any_cast<unsigned int >(value);
    else if (value.type() == typeid(short))
        ss << boost::any_cast<short>(value);
    else if (value.type() == typeid(unsigned short))
        ss << boost::any_cast<unsigned short>(value);
    else if (value.type() == typeid(char))
        ss << boost::any_cast<char>(value);
    else if (value.type() == typeid(unsigned char))
        ss << boost::any_cast<unsigned char>(value);
    else
        throw std::bad_cast();

    Base::Interpreter().runString(ss.str().c_str());
}

bool ObjectIdentifier::adjustLinks(const std::set<App::DocumentObject *> &inList, ExpressionVisitor *v) {
    ResolveResults result(*this);
    if(!result.resolvedDocumentObject)
        return false;
    if(!result.resolvedSubObject) {
        if(inList.count(result.resolvedDocumentObject)) {
            std::ostringstream ss;
            ss << "cyclic reference to " << result.resolvedDocumentObject->getExportName(true);
            THROWM(Base::RuntimeError,ss.str().c_str());
        }
    }else{
        PropertyLinkSub prop;
        prop.setValue(result.resolvedDocumentObject, {subObjectName.getString()});
        if(prop.adjustLink(inList)) {
            if(v) v->setExpressionChanged();
            documentObjectName = String(prop.getValue()->getNameInDocument(),false,true);
            subObjectName = prop.getSubValues().front();
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
    if(!owner || !owner->getNameInDocument() ||
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
        setDocumentObjectName(owner,false,subname);
    }else if(result.flags.test(ResolveByIdentifier))
        setDocumentObjectName(result.resolvedDocumentObject,true,subname);
    else
        setDocumentObjectName(
                String(result.resolvedDocumentObject->Label.getStrValue(),true,false),true,subname);

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
    if (resolvedDocument == 0)
        return std::string("Document not found: ") + resolvedDocumentName.toString();
    else if (resolvedDocumentObject == 0) {
        if(flags.test(ResolveAmbiguous))
            return std::string("Ambiguous document object name: ") + resolvedDocumentObjectName.getString();
        return std::string("Document object not found: ") + resolvedDocumentObjectName.toString();
    } else if (subObjectName.getString().size() && resolvedSubObject == 0)
        return std::string("Sub-object not found: ") + resolvedDocumentObjectName.getString()
            + '.' + subObjectName.toString();
    else if (resolvedProperty == 0)
        return std::string("Property not found: ") + propertyName;

    assert(false);

    return "";
}

void ObjectIdentifier::ResolveResults::getProperty(const ObjectIdentifier &oi) {
    resolvedProperty = oi.resolveProperty(
            resolvedDocumentObject,propertyName.c_str(),resolvedSubObject,propertyType);
}

