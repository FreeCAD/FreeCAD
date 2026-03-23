// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <map>
#include <string>

#include "PropertyContainer.h"
#include <Base/SmartPtrPy.h>

namespace App {

class ExtensionContainer;

// clang-format off
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
  Base::Type getExtensionTypeId(void) const override; \
  static void init(void);\
  static void *create(void);\
private: \
  static Base::Type classTypeId

/// define to implement a subclass of Base::BaseClass
#define EXTENSION_TYPESYSTEM_SOURCE_P(_class_) \
Base::Type _class_::getExtensionClassTypeId(void) { return _class_::classTypeId; } \
Base::Type _class_::getExtensionTypeId(void) const { return _class_::classTypeId; } \
Base::Type _class_::classTypeId = Base::Type::BadType; \
void * _class_::create(void){\
   return new _class_ ();\
}

/// define to implement a subclass of Base::BaseClass
#define EXTENSION_TYPESYSTEM_SOURCE_ABSTRACT_P(_class_) \
Base::Type _class_::getExtensionClassTypeId(void) { return _class_::classTypeId; } \
Base::Type _class_::getExtensionTypeId(void) const { return _class_::classTypeId; } \
Base::Type _class_::classTypeId = Base::Type::BadType; \
void * _class_::create(void){return 0;}

/// define to implement a subclass of Base::BaseClass
#define EXTENSION_TYPESYSTEM_SOURCE(_class_, _parentclass_) \
EXTENSION_TYPESYSTEM_SOURCE_P(_class_)\
void _class_::init(void){\
  initExtensionSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
}

#define EXTENSION_TYPESYSTEM_SOURCE_TEMPLATE(_class_) \
template<> Base::Type _class_::classTypeId = Base::Type::BadType; \
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
  const App::PropertyData &extensionGetPropertyData(void) const override; \
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
// clang-format on

/**
 * @brief A base class for extensions for a DocumentObject.
 * @ingroup ExtensionFramework
 *
 * This class provides a base class for extensions for a DocumentObject.  It is
 * used to hold extensions of a document object.  For a more high-level
 * discussion of extensions see the topic @ref ExtensionFramework "Extension Framework".
 */
class AppExport Extension
{

    //The cass does not have properties itself, but it is important to provide the property access
    //functions. see cpp file for details
    EXTENSION_PROPERTY_HEADER(App::Extension);

public:
    /// Construct an extension object.
    Extension() = default;
    /// Destruct an extension object.
    virtual ~Extension();

    /**
     * @brief Initialize the extension.
     *
     * This function is called by the ExtensionContainer to initialize the
     * extension.
     *
     * @param[in,out] obj The ExtensionContainer object to which this extension
     *                    is attached.
     *
     * @throw Base::RuntimeError If the extension type is not set.
     */
    virtual void initExtension(App::ExtensionContainer* obj);

    /**
     * @brief Get the container of the extension.
     *
     * @return The ExtensionContainer object to which this extension belongs.
     */
    App::ExtensionContainer*       getExtendedContainer() {return m_base;}

    /**
     * @brief Get the container of the extension.
     *
     * @return The ExtensionContainer object to which this extension belongs.
     */
    const App::ExtensionContainer* getExtendedContainer() const {return m_base;}

    /**
     * @brief Get the name of the extension.
     *
     * The name is the class name of the extension without the namespace.
     *
     * @return The name of the extension.
     */
    std::string name() const;

    /**
     * @brief Whether the extension is a Python extension.
     *
     * @return True if the extension is a Python extension, false otherwise.
     */
    bool isPythonExtension() {return m_isPythonExtension;}

    /**
     * @brief Get the PyObject of the extension.
     *
     * @return The PyObject of the extension.
     */
    virtual PyObject* getExtensionPyObject();


    /** @name Access properties
     *
     * @{
     */

    /**
     * @copydoc PropertyContainer::getPropertyByName()
     */
    virtual Property *extensionGetPropertyByName(const char* name) const;

    /**
     * @copydoc PropertyContainer::getPropertyName()
     */
    virtual const char* extensionGetPropertyName(const Property* prop) const;

    /**
     * @copydoc PropertyContainer::getPropertyMap()
     */
    virtual void extensionGetPropertyMap(std::map<std::string,Property*> &map) const;

    /**
     * @copydoc PropertyContainer::visitProperties()
     */
    virtual void extensionVisitProperties(const std::function<void(Property*)>& visitor) const;

    /**
     * @copydoc PropertyContainer::getPropertyList()
     */
    virtual void extensionGetPropertyList(std::vector<Property*> &List) const;

    /**
     * @copydoc PropertyContainer::getPropertyType(const Property*) const
     */
    virtual short extensionGetPropertyType(const Property* prop) const;

    /**
     * @copydoc PropertyContainer::getPropertyType(const char*) const
     */
    virtual short extensionGetPropertyType(const char *name) const;

    /**
     * @copydoc PropertyContainer::getPropertyGroup(const Property*) const
     */
    virtual const char* extensionGetPropertyGroup(const Property* prop) const;

    /**
     * @copydoc PropertyContainer::getPropertyGroup(const char*) const
     */
    virtual const char* extensionGetPropertyGroup(const char *name) const;

    /**
     * @copydoc PropertyContainer::getPropertyDocumentation(const Property*) const
     */
    virtual const char* extensionGetPropertyDocumentation(const Property* prop) const;

    /**
     * @copydoc PropertyContainer::getPropertyDocumentation(const char*) const
     */
    virtual const char* extensionGetPropertyDocumentation(const char *name) const;
    /// @}

    /** @name Persistence
     * @{
     */

    /**
     * @copydoc Base::Persistence::Save
     */
    virtual void extensionSave(Base::Writer&) const {}

    /**
     * @copydoc Base::Persistence::Restore
     */
    virtual void extensionRestore(Base::XMLReader&) {}
    /// @}

    /**
     * @copydoc Base::Type::isDerivedFrom()
     */
    bool extensionIsDerivedFrom(const Base::Type type) const {return getExtensionTypeId().isDerivedFrom(type);}

protected:
    /**
     * @brief Helper function to register the parent.
     *
     * This is used in the macros.  It creates a new type in @p toInit based on
     * the parent.  @p toInit should be an uninitialized type.
     *
     * @param[out] toInit The type to initialize.
     * @param[in] ClassName The name of the class.
     * @param[in] ParentName The name of the parent class.
     * @param[in] method The instantiation method.
     */
    static void initExtensionSubclass(Base::Type &toInit,const char* ClassName, const char *ParentName,
                                      Base::Type::instantiationMethod method=nullptr);

    /**
     * @copydoc PropertyContainer::onChanged()
     */
    virtual void extensionOnChanged([[maybe_unused]] const Property* prop) {}

    /**
     * @copydoc PropertyContainer::handleChangedPropertyName()
     *
     * @return True if the property name change was handled by the extension,
     *         false otherwise.
     */
    virtual bool extensionHandleChangedPropertyName(Base::XMLReader &reader, const char * typeName, const char *propName);

    /**
     * @copydoc PropertyContainer::handleChangedPropertyType()
     *
     * @return True if the property type change was handled by the extension,
     *         false otherwise.
     */
    virtual bool extensionHandleChangedPropertyType(Base::XMLReader &reader, const char * typeName, Property * prop);

    /// Provide access to the ExtensionContainer.
    friend class App::ExtensionContainer;

protected:
    /**
     * @brief Initialize the extension type.
     *
     * This function is to be called by the constructor to initialize the
     * extension type.
     *
     * @param[in] type The type of the extension.
     *
     * @throw Base::RuntimeError If the extension type is not set.
     */
    void initExtensionType(Base::Type type);

    /// Whether the extension is a Python extension.
    bool m_isPythonExtension = false;

    /// The Python object of the extension.
    Py::SmartPtr ExtensionPythonObject;

private:
    Base::Type                    m_extensionType;
    App::ExtensionContainer*      m_base = nullptr;
};

// clang-format off
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
// clang-format on


} // namespace App
