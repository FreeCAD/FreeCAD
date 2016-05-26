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
 
TYPESYSTEM_SOURCE(App::Extension, App::PropertyContainer);

using namespace App;

//**************************************************************************
// Construction/Destruction

// here the implemataion! description should take place in the header file!
Extension::Extension() {}

Extension::~Extension()
{

}

void Extension::setExtendedObject(DocumentObject* obj) {

    if(m_extensionType.isBad())
        throw Base::Exception("Extension::setExtendedObject: Extension type not set");
    
    m_base = obj;
    obj->registerExtension( m_extensionType, this );
}



TYPESYSTEM_SOURCE(App::ExtensionContainer, App::PropertyContainer);

ExtensionContainer::ExtensionContainer() {

};

ExtensionContainer::~ExtensionContainer() {

};

void ExtensionContainer::registerExtension(Base::Type extension, Extension* ext) {

    if(hasExtension(extension))
        throw Base::Exception("ExtensionContainer::registerExtension: Such a extension is already registered");
    
    if(ext->getExtendedObject() != this)
        throw Base::Exception("ExtensionContainer::registerExtension: Extension has not this as base object");
    
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
