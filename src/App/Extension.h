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


#ifndef APP_EXTENSION_H
#define APP_EXTENSION_H

#include "PropertyContainer.h"
#include "PropertyPythonObject.h"
#include <CXX/Objects.hxx>

#include <boost/preprocessor/seq/for_each.hpp>

namespace App {
    
/**
 * @brief Base class for all extension that can be added to a DocumentObject
 * 
 */
class AppExport Extension : public virtual App::PropertyContainer
{

  //The cass does not have properties itself, but it is important to provide the property access
  //functions. see cpp file for details
  PROPERTY_HEADER(App::Extension);

public:

  Extension();
  virtual ~Extension();

  App::DocumentObject*       getExtendedObject() {return m_base;};
  const App::DocumentObject* getExtendedObject() const {return m_base;};
 
  const char* name();
  
  virtual PyObject* getExtensionPyObject(void);

protected:     
  void          initExtension(Base::Type type);
  void          initExtension(App::DocumentObject* obj);
  Py::Object    ExtensionPythonObject;
  
private:
  Base::Type           m_extensionType;
  App::DocumentObject* m_base = nullptr;
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

template<typename ExtensionT>
class ExtensionPython : public ExtensionT {
    
    PROPERTY_HEADER(App::ExtensionPython<ExtensionT>);
    
public:
    ExtensionPython() {
        ADD_PROPERTY(Proxy,(Py::Object()));
    }
    
    //we actually don't need to override any extension methods by default as dynamic properties 
    //should not be supportet.
protected:
    PropertyPythonObject Proxy;
};

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
       
private:
    //stored extensions
    std::map<Base::Type, App::Extension*> _extensions;
};

} //App

#endif // APP_EXTENSION_H
