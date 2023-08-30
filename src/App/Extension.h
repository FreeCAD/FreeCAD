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


#ifndef APP_EXTENSION_H
#define APP_EXTENSION_H

#include "PropertyContainer.h"
#include <Base/SmartPtrPy.h>

namespace App {

class ExtensionContainer;

/// define Extension types
#define EXTENSION_TYPESYSTEM_HEADER() \
public: \
  static Base::Type getExtensionClassTypeId(void); \
  virtual Base::Type getExtensionTypeId(void) const; \
  static void init(void);\
  static void *create(void);\
private: \
  static Base::Type classTypeId

/// Like EXTENSION_TYPESYSTEM_HEADER, with getExtensionTypeId declared override
#define EXTENSION_TYPESYSTEM_HEADER_WITH_OVERRIDE() \
public: \
  static Base::Type getExtensionClassTypeId(void); \
  virtual Base::Type getExtensionTypeId(void) const override; \
  static void init(void);\
  static void *create(void);\
private: \
  static Base::Type classTypeId

/// define to implement a subclass of Base::BaseClass
#define EXTENSION_TYPESYSTEM_SOURCE_P(_class_) \
Base::Type _class_::getExtensionClassTypeId(void) { return _class_::classTypeId; } \
Base::Type _class_::getExtensionTypeId(void) const { return _class_::classTypeId; } \
Base::Type _class_::classTypeId = Base::Type::badType();  \
void * _class_::create(void){\
   return new _class_ ();\
}

/// define to implement a subclass of Base::BaseClass
#define EXTENSION_TYPESYSTEM_SOURCE_ABSTRACT_P(_class_) \
Base::Type _class_::getExtensionClassTypeId(void) { return _class_::classTypeId; } \
Base::Type _class_::getExtensionTypeId(void) const { return _class_::classTypeId; } \
Base::Type _class_::classTypeId = Base::Type::badType();  \
void * _class_::create(void){return 0;}

/// define to implement a subclass of Base::BaseClass
#define EXTENSION_TYPESYSTEM_SOURCE(_class_, _parentclass_) \
EXTENSION_TYPESYSTEM_SOURCE_P(_class_)\
void _class_::init(void){\
  initExtensionSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
}

#define EXTENSION_TYPESYSTEM_SOURCE_TEMPLATE(_class_) \
template<> Base::Type _class_::classTypeId = Base::Type::badType();  \
template<> Base::Type _class_::getExtensionClassTypeId(void) { return _class_::classTypeId; } \
template<> Base::Type _class_::getExtensionTypeId(void) const { return _class_::classTypeId; } \
template<> void * _class_::create(void){\
   return new _class_ ();\
}

// init property stuff
#define EXTENSION_PROPERTY_HEADER(_class_) \
  EXTENSION_TYPESYSTEM_HEADER(); \
protected: \
  static const App::PropertyData * extensionGetPropertyDataPtr(void); \
  virtual const App::PropertyData &extensionGetPropertyData(void) const; \
private: \
  static App::PropertyData propertyData

/// Like EXTENSION_PROPERTY_HEADER, but with override declarations.
#define EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(_class_) \
  EXTENSION_TYPESYSTEM_HEADER_WITH_OVERRIDE(); \
protected: \
  static const App::PropertyData * extensionGetPropertyDataPtr(void); \
  virtual const App::PropertyData &extensionGetPropertyData(void) const override; \
private: \
  static App::PropertyData propertyData

#define EXTENSION_PROPERTY_SOURCE(_class_, _parentclass_) \
EXTENSION_TYPESYSTEM_SOURCE_P(_class_)\
const App::PropertyData * _class_::extensionGetPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::extensionGetPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initExtensionSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::extensionGetPropertyDataPtr();\
}

#define EXTENSION_PROPERTY_SOURCE_TEMPLATE(_class_, _parentclass_) \
EXTENSION_TYPESYSTEM_SOURCE_TEMPLATE(_class_)\
template<> App::PropertyData _class_::propertyData = App::PropertyData(); \
template<> const App::PropertyData * _class_::extensionGetPropertyDataPtr(void){return &propertyData;} \
template<> const App::PropertyData & _class_::extensionGetPropertyData(void) const{return propertyData;} \
template<> void _class_::init(void){\
  initExtensionSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::extensionGetPropertyDataPtr();\
}

/**
 * @brief Base class for all extension that can be added to a DocumentObject
 *
 * For general documentation on why extension system exists and how to use it see the ExtensionContainer
 * documentation. Following is a description howto create custom extensions.
 *
 * Extensions are like every other FreeCAD object and based on properties. All information storage
 * and persistence should be achieved by use of those. Additional any number of methods can be
 * added to provide functionality around the properties. There are 3 small difference to normal objects:
 * 1. They must be derived from Extension class
 * 2. Properties must be handled with special extension macros
 * 3. Extensions must be initialised
 * This works as simple as
 * @code
 * class MyExtension : public Extension {
 *   EXTENSION_PROPERTY_HEADER(MyExtension);
 *   PropertyInt MyProp;
 *   virtual bool overridableMethod(DocumentObject* obj) {};
 * };
 *
 * EXTENSION_PROPERTY_SOURCE(App::MyExtension, App::Extension)
 * MyExtension::MyExtension() {
 *
 *     EXTENSION_ADD_PROPERTY(MyProp, (0)) *
 *     initExtension(MyExtension::getExtensionClassTypeId());
 * }
 * using MyExtensionPython = ExtensionPythonT<MyExtension>;
 * @endcode
 *
 * The special python extension type created above is important, as only those python extensions
 * can be added to an object from python. It does not work to add the c++ version directly there.
 *
 * Note that every method of the extension becomes part of the extended object when added from c++.
 * This means one should carefully design the API and make only necessary methods public or protected.
 * Every internal method should be private.
 *
 * The automatic availability of methods in the class does not hold for the python interface, only
 * for c++ classes. This is like every where else in FreeCAD, there is no automatic creation of python
 * API from c++ classes. Hence the extension creator must also create a custom python object of its
 * extension, which works exactly like the normal FreeCAD python object workflow. There is nothing
 * special at all for extension python objects, the normal xml + imp.cpp approach is used. It must
 * only be taken care that the objects father is the correct extension base class. Of course also
 * make sure your extension returns the correct python object in its "getPyObject" call.
 * Every method you create in the extensions python will be later added to an extended object. This
 * happens automatically for both, c++ and python extension, if "getPyObject" returns the correct
 * python object. No extra work needs to be done.
 *
 * A special case that needs to be handled for extensions is the possibility of overridden methods.
 * Often it is desired to customise extension behaviour by allowing the user to override methods
 * provided by the extension. On c++ side this is trivial, such methods are simply marked as "virtual"
 * and can than be overridden in any derived class. This is more involved for the python interface and
 * here special care needs to be taken.
 *
 * As already seen above one needs to create a special ExtensionPythonT<> object for extension from
 * python. This is done exactly for the purpose of allowing to have overridable methods. The
 * ExtensionPythonT wrapper adds a proxy property which holds a PyObject which itself will contain
 * the implementations for the overridden methods. This design is equal to the ObjectPythonT<> design
 * of normal document objects.
 * As this wrapper inherits the c++ extension class it can also override the virtual functions the
 * user designed to be overridden. What it should do at a call of the virtual method is to check if
 * this method is implemented in the proxy object and if so call it, and if not call the normal
 * c++ version. It is the extensions creators responsibility to implement this check and call behaviour
 * for every overridable method.
 * This is done by creating a custom wrapper just like ExtensionPythonT<> and overriding all virtual
 * methods.
 * @code
 * template<typename ExtensionT> class MyExtensionPythonT : public ExtensionT {
 * public:
 *
 *   MyExtensionPythonT() {}
 *   virtual ~MyExtensionPythonT() {}
 *
 *   virtual bool overridableMethod(DocumentObject* obj)  override {
 *       Py::Object pyobj = Py::asObject(obj->getPyObject());
 *       EXTENSION_PROXY_ONEARG(allowObject, pyobj);
 *
 *       if(result.isNone())
 *           ExtensionT::allowObject(obj);
 *
 *       if(result.isBoolean())
 *           return result.isTrue();
 *
 *       return false;
 *   };
 * };
 * @endcode
 * @Note As seen in the code there are multiple helper macros to ease the repetitive work of querying
 * and calling methods of the proxy object. See the macro documentation for how to use them.
 *
 * To ensure that your wrapper is used when a extension is created from python the extension type must
 * be exposed as follows:
 * @code
 * using MyExtensionPython = ExtensionPythonT<MyExtensionPythonT<MyExtension>>;
 * @endcode
 *
 * This boilerplate is absolutely necessary to allow overridable methods in python and it is the
 * extension creator's responsibility to ensure full implementation.
 *
 */
class AppExport Extension
{

    //The cass does not have properties itself, but it is important to provide the property access
    //functions. see cpp file for details
    EXTENSION_PROPERTY_HEADER(App::Extension);

public:

    Extension() = default;
    virtual ~Extension();

    virtual void initExtension(App::ExtensionContainer* obj);

    App::ExtensionContainer*       getExtendedContainer() {return m_base;}
    const App::ExtensionContainer* getExtendedContainer() const {return m_base;}

    //get extension name without namespace
    std::string name() const;

    bool isPythonExtension() {return m_isPythonExtension;}

    virtual PyObject* getExtensionPyObject();


    /** @name Access properties */
    //@{
    /// find a property by its name
    virtual Property *extensionGetPropertyByName(const char* name) const;
    /// get the name of a property
    virtual const char* extensionGetPropertyName(const Property* prop) const;
    /// get all properties of the class (including properties of the parent)
    virtual void extensionGetPropertyMap(std::map<std::string,Property*> &Map) const;
    /// get all properties of the class (including properties of the parent)
    virtual void extensionGetPropertyList(std::vector<Property*> &List) const;

    /// get the Type of a Property
    virtual short extensionGetPropertyType(const Property* prop) const;
    /// get the Type of a named Property
    virtual short extensionGetPropertyType(const char *name) const;
    /// get the Group of a Property
    virtual const char* extensionGetPropertyGroup(const Property* prop) const;
    /// get the Group of a named Property
    virtual const char* extensionGetPropertyGroup(const char *name) const;
    /// get the Group of a Property
    virtual const char* extensionGetPropertyDocumentation(const Property* prop) const;
    /// get the Group of a named Property
    virtual const char* extensionGetPropertyDocumentation(const char *name) const;
    //@}

    /** @name Persistence */
    //@{
    virtual void extensionSave(Base::Writer&) const {}
    virtual void extensionRestore(Base::XMLReader&) {}
    virtual void extensionRestore(Base::DocumentReader&) {}
    //@}

    /** @name TypeHandling */
    //@{
    bool extensionIsDerivedFrom(const Base::Type type) const {return getExtensionTypeId().isDerivedFrom(type);}
protected:
    static void initExtensionSubclass(Base::Type &toInit,const char* ClassName, const char *ParentName,
                                      Base::Type::instantiationMethod method=nullptr);
    //@}

    virtual void extensionOnChanged(const Property* p) {(void)(p);}

    /// returns true if the property name change was handled by the extension.
    virtual bool extensionHandleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName);
    /// returns true if the property type change was handled by the extension.
    virtual bool extensionHandleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, Property * prop);
    /// returns true if the property name change was handled by the extension.
    virtual bool extensionHandleChangedPropertyName(Base::DocumentReader &reader, const char * TypeName, const char *PropName);
    /// returns true if the property type change was handled by the extension.
    virtual bool extensionHandleChangedPropertyType(Base::DocumentReader &reader, const char * TypeName, Property * prop);

    friend class App::ExtensionContainer;

protected:
    void initExtensionType(Base::Type type);
    bool m_isPythonExtension = false;
    Py::SmartPtr ExtensionPythonObject;

private:
    Base::Type                    m_extensionType;
    App::ExtensionContainer*      m_base = nullptr;
};

// Property define
#define _EXTENSION_ADD_PROPERTY(_name, _prop_, _defaultval_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    propertyData.addProperty(static_cast<App::Extension*>(this), _name, &this->_prop_); \
  } while (0)


#define EXTENSION_ADD_PROPERTY(_prop_, _defaultval_) \
    _EXTENSION_ADD_PROPERTY(#_prop_, _prop_, _defaultval_)

#define _EXTENSION_ADD_PROPERTY_TYPE(_name, _prop_, _defaultval_, _group_,_type_,_Docu_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    propertyData.addProperty(static_cast<App::Extension*>(this), _name, &this->_prop_, (_group_),(_type_),(_Docu_)); \
  } while (0)

#define EXTENSION_ADD_PROPERTY_TYPE(_prop_, _defaultval_, _group_,_type_,_Docu_) \
    _EXTENSION_ADD_PROPERTY_TYPE(#_prop_, _prop_, _defaultval_, _group_,_type_,_Docu_)


} //App

#endif // APP_EXTENSION_H
