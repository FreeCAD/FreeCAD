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
#include "Property.h"
#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "ObjectIdentifier.h"
#include "Expression.h"
#include <Base/Tools.h>
#include <Base/Interpreter.h>
#include <Base/QuantityPy.h>

using namespace App;
using namespace Base;

/**
 * @brief Compute a hash value for the object identifier given by \a path.
 * @param path Inputn path
 * @return Hash value
 */

std::size_t App::hash_value(const App::ObjectIdentifier & path)
{
    return boost::hash_value(path.toString());
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

ObjectIdentifier::ObjectIdentifier(const App::PropertyContainer * _owner, const std::string & property)
    : owner(_owner)
    , documentNameSet(false)
    , documentObjectNameSet(false)
{
    if (owner) {
        const DocumentObject * docObj = freecad_dynamic_cast<const DocumentObject>(owner);
        if (!docObj)
            throw Base::RuntimeError("Property must be owned by a document object.");

        if (property.size() > 0) {
            const Document * doc = docObj->getDocument();

            documentName = String(doc->getName(), false, true);
            documentObjectName = String(docObj->getNameInDocument(), false, true);
        }
    }
    if (property.size() > 0)
        addComponent(Component::SimpleComponent(property));
}

/**
 * @brief Construct an ObjectIdentifier object given a property. The property is assumed to be single-valued.
 * @param prop Property to construct object identifier for.
 */

ObjectIdentifier::ObjectIdentifier(const Property &prop)
    : owner(prop.getContainer())
    , documentNameSet(false)
    , documentObjectNameSet(false)
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(prop.getContainer());

    if (!docObj)
        throw Base::TypeError("Property must be owned by a document object.");

    Document * doc = docObj->getDocument();

    documentName = String(doc->getName(), false, true);
    documentObjectName = String(docObj->getNameInDocument(), false, true);

    addComponent(Component::SimpleComponent(String(owner->getPropertyName(&prop))));
}

/**
 * @brief Get the name of the property.
 * @return Name
 */

const std::string App::ObjectIdentifier::getPropertyName() const
{
    ResolveResults result(*this);

    assert(result.propertyIndex >=0 && static_cast<std::size_t>(result.propertyIndex) < components.size());

    return components[result.propertyIndex].toString();
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
    ResolveResults result1(*this);
    ResolveResults result2(other);

    if (owner != other.owner)
        return false;
    if (result1.resolvedDocumentName != result2.resolvedDocumentName)
        return false;
    if (result1.resolvedDocumentObjectName != result2.resolvedDocumentObjectName)
        return false;
    if (components != other.components)
        return false;
    return true;
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
    ResolveResults result1(*this);
    ResolveResults result2(other);

    if (result1.resolvedDocumentName < result2.resolvedDocumentName)
        return true;

    if (result1.resolvedDocumentName > result2.resolvedDocumentName)
        return false;

    if (result1.resolvedDocumentObjectName < result2.resolvedDocumentObjectName)
        return true;

    if (result1.resolvedDocumentObjectName > result2.resolvedDocumentObjectName)
        return false;

    if (components.size() < other.components.size())
        return true;

    if (components.size() > other.components.size())
        return false;

    for (std::size_t i = 0; i < components.size(); ++i) {
        if (components[i].name < other.components[i].name)
            return true;
        if (components[i].name > other.components[i].name)
            return false;
        if (components[i].type < other.components[i].type)
            return true;
        if (components[i].type > other.components[i].type)
            return false;
        if (components[i].isArray()) {
            if (components[i].index < other.components[i].index)
                return true;
            if (components[i].index > other.components[i].index)
                return false;
        }
        else if (components[i].isMap()) {
            if (components[i].key < other.components[i].key)
                return true;
            if (components[i].key > other.components[i].key)
                return false;
        }
    }
    return false;
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

std::string ObjectIdentifier::toString() const
{
    std::stringstream s;
    ResolveResults result(*this);

    if (documentNameSet)
        s << documentName.toString() << "#";

    if (documentObjectNameSet)
        s << documentObjectName.toString() << ".";
    else if (result.propertyIndex > 0)
        s << components[0].toString() << ".";

    s << getPropertyName() << getSubPathStr();

    return s.str();
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

bool ObjectIdentifier::renameDocumentObject(const std::string &oldName, const std::string &newName)
{
    if (oldName == newName)
        return false;

    if (documentObjectNameSet && documentObjectName == oldName) {
        if (ExpressionParser::isTokenAnIndentifier(newName))
            documentObjectName = newName;
        else
            documentObjectName = ObjectIdentifier::String(newName, true);

        return true;
    }
    else {
        ResolveResults result(*this);

        if (result.propertyIndex == 1 && result.resolvedDocumentObjectName == oldName) {
            if (ExpressionParser::isTokenAnIndentifier(newName))
                components[0].name = newName;
            else
                components[0].name = ObjectIdentifier::String(newName, true);

            return true;
        }
    }

    // If object identifier uses the label then resolving the document object will fail.
    // So, it must be checked if using the new label will succeed
    if (!components.empty() && components[0].getName() == oldName) {
        ObjectIdentifier id(*this);
        id.components[0].name = newName;

        ResolveResults result(id);

        if (result.propertyIndex == 1 && result.resolvedDocumentObjectName == newName) {
            if (ExpressionParser::isTokenAnIndentifier(newName))
                components[0].name = newName;
            else
                components[0].name = ObjectIdentifier::String(newName, true);

            return true;
        }
    }
    return false;
}


/**
 * @brief Check whether a rename call with the same arguments would actually cause a rename.
 * @param oldName Name of current document object
 * @param newName New name of document object
 */

bool ObjectIdentifier::validDocumentObjectRename(const std::string &oldName, const std::string &newName)
{
    if (oldName == newName)
        return false;

    if (documentObjectNameSet && documentObjectName == oldName)
        return true;
    else {
        ResolveResults result(*this);

        if (result.propertyIndex == 1 && result.resolvedDocumentObjectName == oldName)
            return true;
    }

    // If object identifier uses the label then resolving the document object will fail.
    // So, it must be checked if using the new label will succeed
    if (!components.empty() && components[0].getName() == oldName) {
        ObjectIdentifier id(*this);
        id.components[0].name = newName;

        ResolveResults result(id);

        if (result.propertyIndex == 1 && result.resolvedDocumentObjectName == newName)
            return true;
    }
    return false;
}

/**
 * @brief Modify object identifier given that the document \a oldName has changed name to \a newName.
 * @param oldName Name of current document
 * @param newName New name of document
 */

bool ObjectIdentifier::renameDocument(const std::string &oldName, const std::string &newName)
{
    if (oldName == newName)
        return false;

    if (documentNameSet && documentName == oldName) {
        documentName = newName;
        return true;
    }
    else {
        ResolveResults result(*this);

        if (result.resolvedDocumentName == oldName) {
            documentName = newName;
            return true;
        }
    }
    return false;
}

/**
 * @brief Check whether a rename call with the same arguments would actually cause a rename.
 * @param oldName Name of current document
 * @param newName New name of document
 */

bool ObjectIdentifier::validDocumentRename(const std::string &oldName, const std::string &newName)
{
    if (oldName == newName)
        return false;

    if (documentNameSet && documentName == oldName)
        return true;
    else {
        ResolveResults result(*this);

        if (result.resolvedDocumentName == oldName)
            return true;
    }

    return false;
}

/**
 * @brief Get sub field part of a property as a string.
 * @return String representation of path.
 */

std::string ObjectIdentifier::getSubPathStr() const
{
    ResolveResults result(*this);

    std::stringstream s;
    std::vector<Component>::const_iterator i = components.begin() + result.propertyIndex + 1;
    while (i != components.end()) {
        s << "." << i->toString();
        ++i;
    }

    return s.str();
}

/**
 * @brief Construct a Component part
 * @param _component Name of component
 * @param _type Type; simple, array, or map
 * @param _index Array index, if type is array, or -1 if simple or map.
 * @param _key Key index, if type is map, ir empty if simple or array.
 */

ObjectIdentifier::Component::Component(const String &_component, ObjectIdentifier::Component::typeEnum _type, int _index, String _key)
    : name(_component)
    , type(_type)
    , index(_index)
    , key(_key)
    , keyIsString(false)
{
}

/**
 * @brief Create a simple component part with the given name
 * @param _component Name of component.
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::Component::SimpleComponent(const char *_component)
{
    return Component(String(_component));
}

/**
 * @brief Create a simple component part with the given name
 * @param _component Name of component.
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::Component::SimpleComponent(const ObjectIdentifier::String &_component)
{
    return Component(_component);
}

/**
 * @brief Create an array component with given name and index.
 * @param _component Name of component
 * @param _index Index of component
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::Component::ArrayComponent(const ObjectIdentifier::String &_component, int _index)
{
    return Component(_component, ARRAY, _index);
}

/**
 * @brief Create a map component with given name and key.
 * @param _component Name of component
 * @param _key Key of component
 * @return A new Component object.
 */

ObjectIdentifier::Component ObjectIdentifier::Component::MapComponent(const ObjectIdentifier::String &_component, const String & _key)
{
    return Component(_component, MAP, -1, _key);
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

    if (name != other.name)
        return false;

    switch (type) {
    case SIMPLE:
        return true;
    case ARRAY:
        return index == other.index;
    case MAP:
        return key == other.key;
    default:
        assert(0);
        return false;
    }
}

/**
 * @brief Create a string representation of a component.
 * @return A string representing the component.
 */

std::string ObjectIdentifier::Component::toString() const
{
    std::stringstream s;

    s << name.toString();
    switch (type) {
    case Component::SIMPLE:
        break;
    case Component::MAP:
        s << "[" << key.toString() << "]";
        break;
    case Component::ARRAY:
        s << "[" << index << "]";
        break;
    default:
        assert(0);
    }

    return s.str();
}


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

App::DocumentObject * ObjectIdentifier::getDocumentObject(const App::Document * doc, const String & name, bool & byIdentifier) const
{
    DocumentObject * objectById = 0;
    DocumentObject * objectByLabel = 0;
    std::vector<DocumentObject*> docObjects = doc->getObjects();

    // No object found with matching label, try using name directly
    objectById = doc->getObject(static_cast<const char*>(name));

    if (name.isForceIdentifier()) {
        byIdentifier = true;
        return objectById;
    }

    for (std::vector<DocumentObject*>::iterator j = docObjects.begin(); j != docObjects.end(); ++j) {
        if (strcmp((*j)->Label.getValue(), static_cast<const char*>(name)) == 0) {
            // Found object with matching label
            if (objectByLabel != 0)
                return 0;
            objectByLabel = *j;
        }
    }

    if (objectByLabel == 0 && objectById == 0) // Not found at all
        return 0;
    else if (objectByLabel == 0) { // Found by name
        byIdentifier = true;
        return objectById;
    }
    else if (objectById == 0) { // Found by label
        byIdentifier = false;
        return objectByLabel;
    }
    else if (objectByLabel == objectById) { // Found by both name and label, same object
        byIdentifier = false;
        return objectByLabel;
    }
    else
        return 0; // Found by both name and label, two different objects
}

/**
 * @brief Resolve the object identifier to a concrete document, documentobject, and property.
 *
 * This method is a helper method that fills out data in the given ResolveResults object.
 *
 */

void ObjectIdentifier::resolve(ResolveResults &results) const
{
    if (freecad_dynamic_cast<DocumentObject>(owner) == 0)
        return;

    /* Document name specified? */
    if (documentName.getString().size() > 0) {
        results.resolvedDocument = getDocument(documentName);
        results.resolvedDocumentName = documentName;
    }
    else {
        results.resolvedDocument = freecad_dynamic_cast<DocumentObject>(owner)->getDocument();
        results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);
    }

    results.propertyName = "";
    results.propertyIndex = 0;

    // Assume document name and object name from owner if not found
    if (results.resolvedDocument == 0) {
        if (documentName.getString().size() > 0)
            return;

        results.resolvedDocument = freecad_dynamic_cast<DocumentObject>(owner)->getDocument();
        if (results.resolvedDocument == 0)
            return;
    }

    results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);

    /* Document object name specified? */
    if (documentObjectName.getString().size() > 0) {
        bool dummy;

        results.resolvedDocumentObjectName = documentObjectName;
        results.resolvedDocumentObject = getDocumentObject(results.resolvedDocument, documentObjectName, dummy);
        if (!results.resolvedDocumentObject)
            return;
        if (components.size() > 0) {
            results.propertyName = components[0].name.getString();
            results.propertyIndex = 0;
            results.resolvedProperty = results.resolvedDocumentObject->getPropertyByName(results.propertyName.c_str());
        }
        else
            return;
    }
    else {
        /* Document object name not specified, resolve from path */

        /* One component? */
        if (components.size() == 1) {
            /* Yes -- then this must be a property, so we get the document object's name from the owner */
            bool byIdentifier;

            results.resolvedDocumentObjectName = String(static_cast<const DocumentObject*>(owner)->getNameInDocument(), false, true);
            results.resolvedDocumentObject = getDocumentObject(results.resolvedDocument, results.resolvedDocumentObjectName, byIdentifier);
            results.propertyName = components[0].name.getString();
            if (results.resolvedDocumentObject)
                results.resolvedProperty = results.resolvedDocumentObject->getPropertyByName(results.propertyName.c_str());
            results.propertyIndex = 0;
        }
        else if (components.size() >= 2) {
            /* No --  */
            bool byIdentifier;

            if (!components[0].isSimple())
                return;

            results.resolvedDocumentObject = getDocumentObject(results.resolvedDocument, components[0].name, byIdentifier);

            /* Possible to resolve component to a document object? */
            if (results.resolvedDocumentObject) {
                /* Yes */
                results.resolvedDocumentObjectName = String(components[0].name, false, byIdentifier);
                results.propertyName = components[1].name.getString();
                results.resolvedProperty = results.resolvedDocumentObject->getPropertyByName(results.propertyName.c_str());
                results.propertyIndex = 1;
            }
            else {

                /* Document name set explicitly? */
                if (documentName.getString().size() > 0) {
                    /* Yes; then document object must follow */
                    results.resolvedDocumentObjectName = String(components[0].name, false, false);
                    results.resolvedDocumentObject = results.resolvedDocument->getObject(static_cast<const DocumentObject*>(owner)->getNameInDocument());
                    results.propertyIndex = 1;
                }
                else {
                    /* No, assume component is a property, and get document object's name from owner */
                    const DocumentObject * docObj = static_cast<const DocumentObject*>(owner);
                    results.resolvedDocument = docObj->getDocument();
                    results.resolvedDocumentName = String(results.resolvedDocument->getName(), false, true);
                    results.resolvedDocumentObjectName = String(docObj->getNameInDocument(), false, true);
                    results.resolvedDocumentObject = docObj->getDocument()->getObject(docObj->getNameInDocument());
                    results.propertyIndex = 0;
                }
                results.propertyName = components[results.propertyIndex].name.getString();
                if (results.resolvedDocumentObject)
                    results.resolvedProperty = results.resolvedDocumentObject->getPropertyByName(results.propertyName.c_str());
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
    bool dummy;

    if (!doc)
        return 0;

    ResolveResults result(*this);

    return getDocumentObject(doc, result.resolvedDocumentObjectName, dummy);
}

/**
 * @brief Get components as a string list.
 * @return List of strings.
 */

std::vector<std::string> ObjectIdentifier::getStringList() const
{
    std::vector<std::string> l;
    ResolveResults result(*this);

    if (documentNameSet)
        l.push_back(result.resolvedDocumentName.toString());
    if (documentObjectNameSet)
        l.push_back(result.resolvedDocumentObjectName.toString());

    std::vector<Component>::const_iterator i = components.begin();
    while (i != components.end()) {
        l.push_back(i->toString());
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
    return *this;
}

/**
 * @brief Get pointer to property pointed to by this object identifier.
 * @return Point to property if it is uniquely defined, or 0 otherwise.
 */

Property *ObjectIdentifier::getProperty() const
{
    ResolveResults result(*this);

    return result.resolvedProperty;
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
    // Simplify input path by ensuring that components array only has property + optional sub-properties first.
    ObjectIdentifier simplified(getDocumentObject());

    ResolveResults result(*this);

    for (std::size_t i = result.propertyIndex; i < components.size(); ++i)
        simplified << components[i];

    Property * prop = getProperty();

    // Invoke properties canonicalPath method, to let the property do the rest of the job.

    return prop ? prop->canonicalPath(simplified) : simplified;
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
}

/**
 * @brief Get the document name from this object identifier
 *
 * @return Document name as a String object.
 */

const ObjectIdentifier::String ObjectIdentifier::getDocumentName() const
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

void ObjectIdentifier::setDocumentObjectName(const ObjectIdentifier::String &name, bool force)
{
    documentObjectName = name;
    documentObjectNameSet = force;
}

/**
 * @brief Get the document object name
 * @return String with name of document object as resolved by object identifier.
 */

const ObjectIdentifier::String ObjectIdentifier::getDocumentObjectName() const
{
    ResolveResults result(*this);

    return result.resolvedDocumentObjectName;
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

std::string ObjectIdentifier::getPythonAccessor() const
{
    std::stringstream s;
    DocumentObject * docObj = getDocumentObject();

    s << "App.getDocument('" << getDocumentName() << "')."
      << "getObject('" << docObj->getNameInDocument() << "')."
      << getPropertyName() << getSubPathStr();

    return s.str();
}

/**
 * @brief Get the value of the property or field pointed to by this object identifier.
 *
 * Only a limited number of types are supported: Int, Float, String, Unicode String, and Quantities.
 *
 * @return The value of the property or field.
 */

boost::any ObjectIdentifier::getValue() const
{
    std::string s = "_path_value_temp_ = " + getPythonAccessor();
    PyObject * pyvalue = Base::Interpreter().getValue(s.c_str(), "_path_value_temp_");

    class destructor {
    public:
        destructor(PyObject * _p) : p(_p) { }
        ~destructor() { Py_DECREF(p); }
    private:
        PyObject * p;
    };

    destructor d1(pyvalue);

    Base::PyGILStateLocker locker;
    if (!pyvalue)
        throw Base::RuntimeError("Failed to get property value.");
#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(pyvalue))
        return boost::any(PyInt_AsLong(pyvalue));
#else
    if (PyLong_Check(pyvalue))
        return boost::any(PyLong_AsLong(pyvalue));
#endif
    else if (PyFloat_Check(pyvalue))
        return boost::any(PyFloat_AsDouble(pyvalue));
#if PY_MAJOR_VERSION < 3
    else if (PyString_Check(pyvalue))
        return boost::any(PyString_AsString(pyvalue));
#endif
    else if (PyUnicode_Check(pyvalue)) {
#if PY_MAJOR_VERSION >= 3
        return boost::any(PyUnicode_AsUTF8(pyvalue));
#else
        PyObject * s = PyUnicode_AsUTF8String(pyvalue);
        destructor d2(s);

        return boost::any(PyString_AsString(s));
#endif
    }
    else if (PyObject_TypeCheck(pyvalue, &Base::QuantityPy::Type)) {
        Base::QuantityPy * qp = static_cast<Base::QuantityPy*>(pyvalue);
        Base::Quantity * q = qp->getQuantityPtr();

        return boost::any(*q);
    }
    else {
        throw Base::TypeError("Invalid property type.");
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

    ss << getPythonAccessor() + " = ";

    if (value.type() == typeid(Base::Quantity))
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

/** Construct and initialize a ResolveResults object, given an ObjectIdentifier instance.
 *
 * The constructor will invoke the ObjectIdentifier's resolve() method to initialize the object's data.
 */

ObjectIdentifier::ResolveResults::ResolveResults(const ObjectIdentifier &oi)
    : propertyIndex(-1)
    , resolvedDocument(0)
    , resolvedDocumentName()
    , resolvedDocumentObject(0)
    , resolvedDocumentObjectName()
    , resolvedProperty(0)
    , propertyName()
{
    oi.resolve(*this);
}

std::string ObjectIdentifier::ResolveResults::resolveErrorString() const
{
    if (resolvedDocument == 0)
        return std::string("Document not found: ") + resolvedDocumentName.toString();
    else if (resolvedDocumentObject == 0)
        return std::string("Document object not found: ") + resolvedDocumentObjectName.toString();
    else if (resolvedProperty == 0)
        return std::string("Property not found: ") + propertyName;

    assert(false);

    return "";
}
