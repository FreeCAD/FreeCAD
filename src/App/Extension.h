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


#ifndef APP_DOCUMENTOBJECTEXTENSION_H
#define APP_DOCUMENTOBJECTEXTENSION_H

#include "PropertyContainer.h"
#include "PropertyPythonObject.h"
#include <CXX/Objects.hxx>

namespace App {
    
/**
 * @brief Base class for all extension that can be added to a DocumentObject
 * 
 */
class AppExport Extension : public virtual App::PropertyContainer
{

  TYPESYSTEM_HEADER();

public:
  /**
   * A constructor.
   * A more elaborate description of the constructor.
   */
  Extension();

  /**
   * A destructor.
   * A more elaborate description of the destructor.
   */
  virtual ~Extension();

  App::DocumentObject*       getExtendedObject() {return m_base;};
  const App::DocumentObject* getExtendedObject() const {return m_base;};
  void                       setExtendedObject(App::DocumentObject* obj);
 
protected:     
  Base::Type           m_extensionType;
  App::DocumentObject* m_base;
};

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
    /**
    * A constructor.
    * A more elaborate description of the constructor.
    */
    ExtensionContainer();

    /**
    * A destructor.
    * A more elaborate description of the destructor.
    */
    virtual ~ExtensionContainer();

    void registerExtension(Base::Type extension, App::Extension* ext);
    bool hasExtension(Base::Type) const;
    App::Extension* getExtension(Base::Type);
    template<typename Extension>
    Extension* getExtensionByType() {
        return dynamic_cast<Extension*>(getExtension(Extension::getClassTypeId()));
    };
    
private:
    //stored extensions
    std::map<Base::Type, App::Extension*> _extensions;
};

} //App

#endif // APP_DOCUMENTOBJECTEXTENSION_H
