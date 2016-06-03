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


#ifndef APP_EXTENSIONCONTAINER_H
#define APP_EXTENSIONCONTAINER_H

#include "Extension.h"
#include "PropertyContainer.h"
#include "PropertyPythonObject.h"
#include "DynamicProperty.h"
#include <CXX/Objects.hxx>

#include <boost/preprocessor/seq/for_each.hpp>

namespace App {
    
class AppExport ExtensionContainer : public virtual App::PropertyContainer
{

    TYPESYSTEM_HEADER();

public:
    
    typedef std::map<Base::Type, App::Extension*>::iterator ExtensionIterator;

    ExtensionContainer();
    virtual ~ExtensionContainer();

    void registerExtension(Base::Type extension, App::Extension* ext);
    bool hasExtension(Base::Type) const;
    bool hasExtension(const char* name) const; //this version does not check derived classes
    App::Extension* getExtension(Base::Type);
    App::Extension* getExtension(const char* name); //this version does not check derived classes
    template<typename ExtensionT>
    ExtensionT* getExtensionByType() {
        return dynamic_cast<ExtensionT*>(getExtension(ExtensionT::getClassTypeId()));
    };
    
    //get all extensions which have the given base class
    std::vector<Extension*> getExtensionsDerivedFrom(Base::Type type) const;
    template<typename ExtensionT>
    std::vector<ExtensionT*> getExtensionsDerivedFromType() const {
        auto vec = getExtensionsDerivedFrom(ExtensionT::getClassTypeId());
        std::vector<ExtensionT*> typevec;
        for(auto ext : vec)
            typevec.push_back(dynamic_cast<ExtensionT*>(ext));
        
        return typevec;
    };
    
    ExtensionIterator extensionBegin() {return _extensions.begin();};
    ExtensionIterator extensionEnd() {return _extensions.end();};
       
    
    /** @name Access properties */
    //@{
    /// find a property by its name
    virtual Property *getPropertyByName(const char* name) const override;
    /// get the name of a property
    virtual const char* getPropertyName(const Property* prop) const override;
    /// get all properties of the class (including properties of the parent)
    virtual void getPropertyMap(std::map<std::string,Property*> &Map) const override;
    /// get all properties of the class (including properties of the parent)
    virtual void getPropertyList(std::vector<Property*> &List) const override;

    /// get the Type of a Property
    virtual short getPropertyType(const Property* prop) const override;
    /// get the Type of a named Property
    virtual short getPropertyType(const char *name) const override;
    /// get the Group of a Property
    virtual const char* getPropertyGroup(const Property* prop) const override;
    /// get the Group of a named Property
    virtual const char* getPropertyGroup(const char *name) const override;
    /// get the Group of a Property
    virtual const char* getPropertyDocumentation(const Property* prop) const override;
    /// get the Group of a named Property
    virtual const char* getPropertyDocumentation(const char *name) const override;
    //@}
    
    virtual void onChanged(const Property*);
    
private:
    //stored extensions
    std::map<Base::Type, App::Extension*> _extensions;
};


#define PROPERTY_HEADER_WITH_EXTENSIONS(_class_) \
  PROPERTY_HEADER(_class)

//helper macro to add parent to property data
#define ADD_PARENT(r, data, elem)\
    data::propertyData.parentPropertyData.push_back(elem::getPropertyDataPtr());

/// 
#define PROPERTY_SOURCE_WITH_EXTENSIONS(_class_, _parentclass_, _extensions_) \
TYPESYSTEM_SOURCE_P(_class_);\
const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  ADD_PARENT(0, _class_, _parentclass_)\
  BOOST_PP_SEQ_FOR_EACH(ADD_PARENT, _class_, _extensions_)\
}

} //App

#endif // APP_EXTENSIONCONTAINER_H
