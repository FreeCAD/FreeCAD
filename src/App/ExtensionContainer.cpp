/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2016     *
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
# include <algorithm>
#endif

#include "Extension.h"
#include "DocumentObject.h"
#include "Base/Exception.h"
#include <Base/Console.h>
 
using namespace App;

TYPESYSTEM_SOURCE(App::ExtensionContainer, App::PropertyContainer);

ExtensionContainer::ExtensionContainer() {

};

ExtensionContainer::~ExtensionContainer() {

    //we need to delete all dynamically added extensions
    for(auto entry : _extensions) {            
        if(entry.second->isPythonExtension())
            delete entry.second;
    }
};

void ExtensionContainer::registerExtension(Base::Type extension, Extension* ext) {

    if(ext->getExtendedContainer() != this)
        throw Base::Exception("ExtensionContainer::registerExtension: Extension has not this as base object");
       
    //no duplicate extensions (including base classes)
    if(hasExtension(extension)) {
        for(auto entry : _extensions) {            
            if(entry.first == extension || entry.first.isDerivedFrom(extension)) {
                _extensions.erase(entry.first);
                break;
            }
        }
    }
        
    _extensions[extension] = ext;
}

bool ExtensionContainer::hasExtension(Base::Type t) const {

    //check for the exact type
    bool found = _extensions.find(t) != _extensions.end();
    if(!found) {
        //and for types derived from it, as they can be cast to the extension
        for(auto entry : _extensions) {            
            if(entry.first.isDerivedFrom(t))
                return true;
        }
        return false;
    }
    return true;
}

bool ExtensionContainer::hasExtension(const char* name) const {

    //and for types derived from it, as they can be cast to the extension
    for(auto entry : _extensions) {            
        if(strcmp(entry.second->name(), name) == 0)
            return true;
    }
    return false;
}


Extension* ExtensionContainer::getExtension(Base::Type t) {
   
    auto result = _extensions.find(t);
    if(result == _extensions.end()) {
        //we need to check for derived types
        for(auto entry : _extensions) {            
            if(entry.first.isDerivedFrom(t))
                return entry.second;
        }
        //if we arive hear we don't have anything matching
        throw Base::Exception("ExtensionContainer::getExtension: No extension of given type available");
    }
    
    return result->second;
}

Extension* ExtensionContainer::getExtension(const char* name) {

    //and for types derived from it, as they can be cast to the extension
    for(auto entry : _extensions) {            
        if(strcmp(entry.second->name(), name) == 0)
            return entry.second;
    }
    return nullptr;
}

std::vector< Extension* > ExtensionContainer::getExtensionsDerivedFrom(Base::Type type) const {

    std::vector<Extension*> vec;
    //and for types derived from it, as they can be cast to the extension
    for(auto entry : _extensions) {            
        if(entry.first.isDerivedFrom(type))
            vec.push_back(entry.second);
    }
    return vec;
}

void ExtensionContainer::getPropertyList(std::vector< Property* >& List) const {
    App::PropertyContainer::getPropertyList(List);
    for(auto entry : _extensions)         
        entry.second->extensionGetPropertyList(List);
}

void ExtensionContainer::getPropertyMap(std::map< std::string, Property* >& Map) const {
    App::PropertyContainer::getPropertyMap(Map);
    for(auto entry : _extensions)     
        entry.second->extensionGetPropertyMap(Map);
}

Property* ExtensionContainer::getPropertyByName(const char* name) const {
    auto prop = App::PropertyContainer::getPropertyByName(name);
    if(prop)
        return prop;
    
    for(auto entry : _extensions) {            
        auto prop = entry.second->extensionGetPropertyByName(name);
        if(prop)
            return prop;
    }
    
    return nullptr;
}


short int ExtensionContainer::getPropertyType(const Property* prop) const {
    short int res = App::PropertyContainer::getPropertyType(prop);
    if(res != 0)
        return res;
    
    for(auto entry : _extensions) {            
        res = entry.second->extensionGetPropertyType(prop);
        if(res != 0)
            return res;     
    }
    
    return 0;
}

short int ExtensionContainer::getPropertyType(const char* name) const {
    
    short int res = App::PropertyContainer::getPropertyType(name);
    if(res != 0)
        return res;
    
    for(auto entry : _extensions) {            
        res = entry.second->extensionGetPropertyType(name);
        if(res != 0)
            return res;     
    }
    
    return 0;
}


const char* ExtensionContainer::getPropertyName(const Property* prop) const {
    
    const char* res = App::PropertyContainer::getPropertyName(prop);
    if(res != 0)
        return res;
    
    for(auto entry : _extensions) {            
        res = entry.second->extensionGetPropertyName(prop);
        if(res != 0)
            return res;     
    }
    
    return 0;
}

const char* ExtensionContainer::getPropertyGroup(const Property* prop) const {
    
    const char* res = App::PropertyContainer::getPropertyGroup(prop);
    if(res != 0)
        return res;
    
    for(auto entry : _extensions) {            
        res = entry.second->extensionGetPropertyGroup(prop);
        if(res != 0)
            return res;     
    }
    
    return 0;
}

const char* ExtensionContainer::getPropertyGroup(const char* name) const {
        
    const char* res = App::PropertyContainer::getPropertyGroup(name);
    if(res != 0)
        return res;
    
    for(auto entry : _extensions) {            
        res = entry.second->extensionGetPropertyGroup(name);
        if(res != 0)
            return res;     
    }
    
    return 0;
}


const char* ExtensionContainer::getPropertyDocumentation(const Property* prop) const {
    
    const char* res = App::PropertyContainer::getPropertyDocumentation(prop);
    if(res != 0)
        return res;
    
    for(auto entry : _extensions) {            
        res = entry.second->extensionGetPropertyDocumentation(prop);
        if(res != 0)
            return res;     
    }
    
    return 0;
}

const char* ExtensionContainer::getPropertyDocumentation(const char* name) const {
    
    const char* res = App::PropertyContainer::getPropertyDocumentation(name);
    if(res != 0)
        return res;
    
    for(auto entry : _extensions) {            
        res = entry.second->extensionGetPropertyDocumentation(name);
        if(res != 0)
            return res;     
    }
    
    return 0;
}

void ExtensionContainer::onChanged(const Property* prop) {
    /*
    //we need to make sure that the proxy we use is always the proxy of the extended class. This 
    //is needed as only the extended class proxy (either c++ or python) is managed and ensured to 
    //be the python implementation class
    //Note: this property only exist if the created object is FeaturePythonT<>, this won't work for 
    //any default document object. But this doesnt matter much as there is a proxy object set anyway
    //if a extension gets registered from python. This is only for synchronisation.
    if(strcmp(prop->getName(), "Proxy")) {
        for(auto entry : _extensions)
            entry.second->extensionGetExtensionPyObject().setValue(static_cast<const PropertyPythonObject*>(prop)->getValue());
    }*/
        
    App::PropertyContainer::onChanged(prop);
}
