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
 
/* We do not use a standart property macro for type initiation. The reason is that we want to expose all property functions, 
 * to allow the derived classes to access the private property data, but we do not want to have our
 * property data a reference to the parent data. That is because the extension is used in  a multi 
 * inheritance way, and hence our propertydata partent data would point to the same property data
 * as any other parent of the inherited class. It makes more sense to create a total unrelated line
 * of property datas which are added as additional parent to the extended class.
 */
TYPESYSTEM_SOURCE_P(App::Extension);
const App::PropertyData * App::Extension::getPropertyDataPtr(void){return &propertyData;}
const App::PropertyData & App::Extension::getPropertyData(void) const{return propertyData;}
App::PropertyData App::Extension::propertyData;
void App::Extension::init(void){
  initSubclass(App::Extension::classTypeId, "App::Extension" , "App::PropertyContainer", &(App::Extension::create) );
}

using namespace App;

Extension::Extension() 
{
}

Extension::~Extension()
{

}

void Extension::initExtension(Base::Type type) {

    m_extensionType = type;
    if(m_extensionType.isBad())
        throw Base::Exception("Extension: Extension type not set");
}

void Extension::initExtension(DocumentObject* obj) {

    if(m_extensionType.isBad())
        throw Base::Exception("Extension: Extension type not set");
 
    m_base = obj;
    m_base->registerExtension( m_extensionType, this );
}


PyObject* Extension::getExtensionPyObject(void) {

    return nullptr;
}

const char* Extension::name() {
    
    if(m_extensionType.isBad())
        throw Base::Exception("Extension::setExtendedObject: Extension type not set");
    
    std::string temp(m_extensionType.getName());
    std::string::size_type pos = temp.find_last_of(":");

    if(pos != std::string::npos)
        return temp.substr(pos+1).c_str();
    else
        return std::string().c_str();
}

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::ExtensionPython, App::ExtensionPython::Inherited)

// explicit template instantiation
template class AppExport ExtensionPythonT<Extension>;
}