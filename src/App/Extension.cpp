/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <cassert>
#endif

#include <Base/PyObjectBase.h>

#include "Extension.h"
#include "ExtensionContainer.h"
#include "ExtensionPython.h"
#include <ExtensionPy.h>

/* We do not use a standard property macro for type initiation. The reason is that we have the first
 * PropertyData in the extension chain, there is no parent property data.
 */
EXTENSION_TYPESYSTEM_SOURCE_P(App::Extension)
const App::PropertyData * App::Extension::extensionGetPropertyDataPtr(){return &propertyData;}
const App::PropertyData & App::Extension::extensionGetPropertyData() const{return propertyData;}
App::PropertyData App::Extension::propertyData;
void App::Extension::init(){

    assert(Extension::classTypeId == Base::Type::badType() && "don't init() twice!");

    /* Set up entry in the type system. */
    Extension::classTypeId = Base::Type::createType(Base::Type::badType(), "App::Extension",
                                Extension::create);
}

using namespace App;

Extension::~Extension()
{
    if (!ExtensionPythonObject.is(Py::_None())){
        // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed
        // Python object or not. In the constructor we forced the wrapper to own the object so we need
        // not to dec'ref the Python object any more.
        // But we must still invalidate the Python object because it need not to be
        // destructed right now because the interpreter can own several references to it.
        Base::PyObjectBase* obj = static_cast<Base::PyObjectBase*>(ExtensionPythonObject.ptr());
        // Call before decrementing the reference counter, otherwise a heap error can occur
        obj->setInvalid();
    }
}

void Extension::initExtensionType(Base::Type type) {

    m_extensionType = type;
    if (m_extensionType.isBad())
        throw Base::RuntimeError("Extension: Extension type not set");
}

void Extension::initExtension(ExtensionContainer* obj) {
    if (m_extensionType.isBad())
        throw Base::RuntimeError("Extension: Extension type not set");

    //all properties are initialised without PropertyContainer father. Now that we know it we can
    //finally finish the property initialisation
    std::vector<Property*> list;
    extensionGetPropertyData().getPropertyList(this, list);
    for(Property* prop : list)
        prop->setContainer(obj);

    m_base = obj;
    m_base->registerExtension( m_extensionType, this );
}


PyObject* Extension::getExtensionPyObject() {

    if (ExtensionPythonObject.is(Py::_None())){
        // ref counter is set to 1
        auto grp = new ExtensionPy(this);
        ExtensionPythonObject = Py::Object(grp,true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

std::string Extension::name() const {

    if (m_extensionType.isBad())
        throw Base::RuntimeError("Extension::name: Extension type not set");

    std::string temp(m_extensionType.getName());
    std::string::size_type pos = temp.find_last_of(':');

    if (pos != std::string::npos)
        return temp.substr(pos+1);
    return {};
}

Property* Extension::extensionGetPropertyByName(const char* name) const {

    return extensionGetPropertyData().getPropertyByName(this, name);
}

short int Extension::extensionGetPropertyType(const Property* prop) const {

    return extensionGetPropertyData().getType(this, prop);
}

short int Extension::extensionGetPropertyType(const char* name) const {

    return extensionGetPropertyData().getType(this, name);
}

const char* Extension::extensionGetPropertyName(const Property* prop) const {

    return extensionGetPropertyData().getName(this,prop);
}

const char* Extension::extensionGetPropertyGroup(const Property* prop) const {

    return extensionGetPropertyData().getGroup(this,prop);
}

const char* Extension::extensionGetPropertyGroup(const char* name) const {

    return extensionGetPropertyData().getGroup(this,name);
}


const char* Extension::extensionGetPropertyDocumentation(const Property* prop) const {

    return extensionGetPropertyData().getDocumentation(this, prop);
}

const char* Extension::extensionGetPropertyDocumentation(const char* name) const {

    return extensionGetPropertyData().getDocumentation(this, name);
}

void Extension::extensionGetPropertyList(std::vector< Property* >& List) const {

    extensionGetPropertyData().getPropertyList(this, List);
}

void Extension::extensionGetPropertyMap(std::map< std::string, Property* >& Map) const {

    extensionGetPropertyData().getPropertyMap(this, Map);
}

void Extension::initExtensionSubclass(Base::Type& toInit, const char* ClassName, const char* ParentName,
                             Base::Type::instantiationMethod method) {

    // don't init twice!
    assert(toInit == Base::Type::badType());
    // get the parent class
    Base::Type parentType(Base::Type::fromName(ParentName));
    // forgot init parent!
    assert(parentType != Base::Type::badType() );

    // create the new type
    toInit = Base::Type::createType(parentType, ClassName, method);
}


bool Extension::extensionHandleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName)
{
    (void) reader;
    (void) TypeName;
    (void) PropName;

    return false;
};

bool Extension::extensionHandleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, Property * prop)
{
    (void) reader;
    (void) TypeName;
    (void) prop;

    return false;
};

bool Extension::extensionHandleChangedPropertyName(Base::DocumentReader &reader, const char * TypeName, const char *PropName)
{
    (void) reader;
    (void) TypeName;
    (void) PropName;

    return false;
};

bool Extension::extensionHandleChangedPropertyType(Base::DocumentReader &reader, const char * TypeName, Property * prop)
{
    (void) reader;
    (void) TypeName;
    (void) prop;

    return false;
};

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::ExtensionPython, App::ExtensionPython::Inherited)

// explicit template instantiation
template class AppExport ExtensionPythonT<Extension>;
}
