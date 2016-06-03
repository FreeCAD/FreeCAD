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
#include "Base/Interpreter.h"
#include <CXX/Objects.hxx>

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

  void initExtension(App::DocumentObject* obj);
    
  App::DocumentObject*       getExtendedObject() {return m_base;};
  const App::DocumentObject* getExtendedObject() const {return m_base;};
 
  //get extension name without namespace
  const char* name();

  bool isPythonExtension() {return m_isPythonExtension;};
  
  virtual PyObject* getExtensionPyObject(void);

protected:     
  void initExtension(Base::Type type);
  bool m_isPythonExtension = false;
  Py::Object ExtensionPythonObject;
  
private:
  Base::Type           m_extensionType;
  App::DocumentObject* m_base = nullptr;
};


/**
 * Generic Python extension class which allows to behave every extension
 * derived class as Python extension -- simply by subclassing.
 */
template <class ExtensionT>
class ExtensionPythonT : public ExtensionT
{
    PROPERTY_HEADER(App::ExtensionPythonT<ExtensionT>);

public:
    typedef ExtensionT Inherited;
    
    ExtensionPythonT() {
        ExtensionT::m_isPythonExtension = true;
        
        ADD_PROPERTY(Proxy,(Py::Object()));
    }
    virtual ~ExtensionPythonT() {
    }

    PropertyPythonObject Proxy;
};

typedef ExtensionPythonT<App::Extension> ExtensionPython;

//helper macros to define python extensions
#define EXTENSION_PROXY_FIRST(function) \
    Base::PyGILStateLocker lock;\
    Py::Object result;\
    try {\
        Property* proxy = this->getPropertyByName("Proxy");\
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {\
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();\
            if (feature.hasAttr(std::string("function"))) {\
                if (feature.hasAttr("__object__")) {\
                    Py::Callable method(feature.getAttr(std::string("function")));
                    
                    
                    

#define EXTENSION_PROXY_SECOND(function)\
                    result = method.apply(args);\
                }\
                else {\
                    Py::Callable method(feature.getAttr(std::string("function")));
                    
#define EXTENSION_PROXY_THIRD()\
                    result = method.apply(args);\
                }\
            }\
        }\
    }\
    catch (Py::Exception&) {\
        Base::PyException e;\
        e.ReportException();\
    }
    
#define EXTENSION_PROXY_NOARG(function)\
    EXTENSION_PROXY_FIRST(function) \
    Py::Tuple args;\
    EXTENSION_PROXY_SECOND(function) \
    Py::Tuple args(1);\
    args.setItem(0, Py::Object(this->getExtensionPyObject(), true));\
    EXTENSION_PROXY_THIRD()

#define EXTENSION_PROXY_ONEARG(function, arg)\
    EXTENSION_PROXY_FIRST(function) \
    Py::Tuple args;\
    args.setItem(0, arg); \
    EXTENSION_PROXY_SECOND(function) \
    Py::Tuple args(2);\
    args.setItem(0, Py::Object(this->getExtensionPyObject(), true));\
    args.setItem(1, arg); \
    EXTENSION_PROXY_THIRD()

#define EXTENSION_PYTHON_OVERRIDE_VOID_NOARGS(function)\
    virtual void function() override {\
        EXTENSION_PROXY_NOARGS(function)\
    };
    
#define EXTENSION_PYTHON_OVERRIDE_OBJECT_NOARGS(function)\
    virtual PyObject* function() override {\
        EXTENSION_PROXY_NOARGS(function)\
        return res.ptr();\
    };
    
}; //App

#endif // APP_EXTENSION_H
