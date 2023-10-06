/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef APP_PROPERTYCONTAINER_H
#define APP_PROPERTYCONTAINER_H

#include <map>
#include <cstring>
#include <Base/Persistence.h>

#include "DynamicProperty.h"

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN
	class DOMNode;
	class DOMElement;
//    class DefaultHandler;
//    class SAX2XMLReader;
XERCES_CPP_NAMESPACE_END


namespace Base {
class Writer;
}


namespace App
{
class Property;
class PropertyContainer;
class DocumentObject;
class Extension;

enum PropertyType
{
  Prop_None        = 0, /*!< No special property type */
  Prop_ReadOnly    = 1, /*!< Property is read-only in the editor */
  Prop_Transient   = 2, /*!< Property content won't be saved to file, but still saves name, type and status */
  Prop_Hidden      = 4, /*!< Property won't appear in the editor */
  Prop_Output      = 8, /*!< Modified property doesn't touch its parent container */
  Prop_NoRecompute = 16,/*!< Modified property doesn't touch its container for recompute */
  Prop_NoPersist   = 32,/*!< Property won't be saved to file at all */
};

struct AppExport PropertyData
{
  struct PropertySpec
  {
    const char * Name;
    const char * Group;
    const char * Docu;
    short Offset, Type;

    inline PropertySpec(const char *name, const char *group, const char *doc, short offset, short type)
        :Name(name),Group(group),Docu(doc),Offset(offset),Type(type)
    {}
  };

  //purpose of this struct is to be constructible from all acceptable container types and to
  //be able to return the offset to a property from the accepted containers. This allows to use
  //one function implementation for multiple container types without losing all type safety by
  //accepting void*
  struct OffsetBase
  {
      OffsetBase(const App::PropertyContainer* container) : m_container(container) {}//explicit bombs
      OffsetBase(const App::Extension* container) : m_container(container) {}//explicit bombs

      short int getOffsetTo(const App::Property* prop) const {
            auto *pt = (const char*)prop;
            auto *base = (const char *)m_container;
            if(pt<base || pt>base+SHRT_MAX)
                return -1;
            return (short) (pt-base);
      }
      char* getOffset() const {return (char*) m_container;}

  private:
      const void* m_container;
  };

  // A multi index container for holding the property spec, with the following
  // index,
  // * a sequence, to preserve creation order
  // * hash index on property name
  // * hash index on property pointer offset
  mutable bmi::multi_index_container<
      PropertySpec,
      bmi::indexed_by<
          bmi::sequenced<>,
          bmi::hashed_unique<
              bmi::member<PropertySpec, const char*, &PropertySpec::Name>,
              CStringHasher,
              CStringHasher
          >,
          bmi::hashed_unique<
              bmi::member<PropertySpec, short, &PropertySpec::Offset>
          >
      >
  > propertyData;

  mutable bool parentMerged = false;

  const PropertyData*     parentPropertyData;

  void addProperty(OffsetBase offsetBase,const char* PropName, Property *Prop, const char* PropertyGroup= nullptr, PropertyType = Prop_None, const char* PropertyDocu= nullptr );

  const PropertySpec *findProperty(OffsetBase offsetBase,const char* PropName) const;
  const PropertySpec *findProperty(OffsetBase offsetBase,const Property* prop) const;

  const char* getName         (OffsetBase offsetBase,const Property* prop) const;
  short       getType         (OffsetBase offsetBase,const Property* prop) const;
  short       getType         (OffsetBase offsetBase,const char* name)     const;
  const char* getGroup        (OffsetBase offsetBase,const char* name)     const;
  const char* getGroup        (OffsetBase offsetBase,const Property* prop) const;
  const char* getDocumentation(OffsetBase offsetBase,const char* name)     const;
  const char* getDocumentation(OffsetBase offsetBase,const Property* prop) const;

  Property *getPropertyByName(OffsetBase offsetBase,const char* name) const;
  void getPropertyMap(OffsetBase offsetBase,std::map<std::string,Property*> &Map) const;
  void getPropertyList(OffsetBase offsetBase,std::vector<Property*> &List) const;
  void getPropertyNamedList(OffsetBase offsetBase, std::vector<std::pair<const char*,Property*> > &List) const;

  void merge(PropertyData *other=nullptr) const;
  void split(PropertyData *other);
};


/** Base class of all classes with properties
 */
class AppExport PropertyContainer: public Base::Persistence
{

  TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
  /**
   * A constructor.
   * A more elaborate description of the constructor.
   */
  PropertyContainer();

  /**
   * A destructor.
   * A more elaborate description of the destructor.
   */
  ~PropertyContainer() override;

  unsigned int getMemSize () const override;

  virtual std::string getFullName() const {return {};}

  /// find a property by its name
  virtual Property *getPropertyByName(const char* name) const;
  /// get the name of a property
  virtual const char* getPropertyName(const Property* prop) const;
  /// get all properties of the class (including properties of the parent)
  virtual void getPropertyMap(std::map<std::string,Property*> &Map) const;
  /// get all properties of the class (including properties of the parent)
  virtual void getPropertyList(std::vector<Property*> &List) const;
  /// get all properties with their names, may contain duplicates and aliases
  virtual void getPropertyNamedList(std::vector<std::pair<const char*,Property*> > &List) const;
  /// set the Status bit of all properties at once
  void setPropertyStatus(unsigned char bit,bool value);

  /// get the Type of a Property
  virtual short getPropertyType(const Property* prop) const;
  /// get the Type of a named Property
  virtual short getPropertyType(const char *name) const;
  /// get the Group of a Property
  virtual const char* getPropertyGroup(const Property* prop) const;
  /// get the Group of a named Property
  virtual const char* getPropertyGroup(const char *name) const;
  /// get the Group of a Property
  virtual const char* getPropertyDocumentation(const Property* prop) const;
  /// get the Group of a named Property
  virtual const char* getPropertyDocumentation(const char *name) const;
  /// check if the property is read-only
  bool isReadOnly(const Property* prop) const;
  /// check if the named property is read-only
  bool isReadOnly(const char *name) const;
  /// check if the property is hidden
  bool isHidden(const Property* prop) const;
  /// check if the named property is hidden
  bool isHidden(const char *name) const;
  virtual App::Property* addDynamicProperty(
        const char* type, const char* name=nullptr,
        const char* group=nullptr, const char* doc=nullptr,
        short attr=0, bool ro=false, bool hidden=false);

  DynamicProperty::PropData getDynamicPropertyData(const Property* prop) const {
      return dynamicProps.getDynamicPropertyData(prop);
  }

  bool changeDynamicProperty(const Property *prop, const char *group, const char *doc) {
      return dynamicProps.changeDynamicProperty(prop,group,doc);
  }

  virtual bool removeDynamicProperty(const char* name) {
      return dynamicProps.removeDynamicProperty(name);
  }
  virtual std::vector<std::string> getDynamicPropertyNames() const {
      return dynamicProps.getDynamicPropertyNames();
  }
  virtual App::Property *getDynamicPropertyByName(const char* name) const {
      return dynamicProps.getDynamicPropertyByName(name);
  }

  virtual void onPropertyStatusChanged(const Property &prop, unsigned long oldStatus);

  void Save (Base::Writer &writer) const override;
  void Restore(Base::XMLReader &reader) override;
  void Restore(Base::DocumentReader &reader, XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *containerEl) override;
  virtual void beforeSave() const;

  virtual void editProperty(const char * /*propName*/) {}

  const char *getPropertyPrefix() const {
      return _propertyPrefix.c_str();
  }

  void setPropertyPrefix(const char *prefix) {
      _propertyPrefix = prefix;
  }

  friend class Property;
  friend class DynamicProperty;


protected:
  /// get called by the container when a property has changed
  virtual void onChanged(const Property* /*prop*/){}
  /// get called before the value is changed
  virtual void onBeforeChange(const Property* /*prop*/){}

  //void hasChanged(Property* prop);
  static const  PropertyData * getPropertyDataPtr();
  virtual const PropertyData& getPropertyData() const;

  virtual void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName);
  virtual void handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, Property * prop);
  virtual void handleChangedPropertyName(Base::DocumentReader &reader, const char * TypeName, const char *PropName);
  virtual void handleChangedPropertyType(Base::DocumentReader &reader, const char * TypeName, Property * prop);

public:
  // forbidden
  PropertyContainer(const PropertyContainer&) = delete;
  PropertyContainer& operator = (const PropertyContainer&) = delete;
  void readProperty(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *PropertyDOM);

protected:
  DynamicProperty dynamicProps;

private:
  std::string _propertyPrefix;
  static PropertyData propertyData;
};

/// Property define
#define _ADD_PROPERTY(_name,_prop_, _defaultval_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    this->_prop_.setContainer(this); \
    propertyData.addProperty(static_cast<App::PropertyContainer*>(this), _name, &this->_prop_); \
  } while (0)

#define ADD_PROPERTY(_prop_, _defaultval_) \
    _ADD_PROPERTY(#_prop_, _prop_, _defaultval_)

#define _ADD_PROPERTY_TYPE(_name,_prop_, _defaultval_, _group_,_type_,_Docu_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    this->_prop_.setContainer(this); \
    propertyData.addProperty(static_cast<App::PropertyContainer*>(this), _name, &this->_prop_, (_group_),(_type_),(_Docu_)); \
  } while (0)

#define ADD_PROPERTY_TYPE(_prop_, _defaultval_, _group_,_type_,_Docu_) \
    _ADD_PROPERTY_TYPE(#_prop_,_prop_,_defaultval_,_group_,_type_,_Docu_)


#define PROPERTY_HEADER(_class_) \
  TYPESYSTEM_HEADER(); \
protected: \
  static const App::PropertyData * getPropertyDataPtr(void); \
  virtual const App::PropertyData &getPropertyData(void) const; \
private: \
  static App::PropertyData propertyData

/// Like PROPERTY_HEADER, but with overridden methods declared as such
#define PROPERTY_HEADER_WITH_OVERRIDE(_class_) \
  TYPESYSTEM_HEADER_WITH_OVERRIDE(); \
protected: \
  static const App::PropertyData * getPropertyDataPtr(void); \
  const App::PropertyData &getPropertyData(void) const override; \
private: \
  static App::PropertyData propertyData
///
#define PROPERTY_SOURCE(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_P(_class_)\
const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr(); \
}

#define PROPERTY_SOURCE_ABSTRACT(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_ABSTRACT_P(_class_)\
const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr(); \
}

#define TYPESYSTEM_SOURCE_TEMPLATE(_class_) \
template<> Base::Type _class_::classTypeId = Base::Type::badType();  \
template<> Base::Type _class_::getClassTypeId(void) { return _class_::classTypeId; } \
template<> Base::Type _class_::getTypeId(void) const { return _class_::classTypeId; } \
template<> void * _class_::create(void){\
   return new _class_ ();\
}

#define PROPERTY_SOURCE_TEMPLATE(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_TEMPLATE(_class_)\
template<> App::PropertyData _class_::propertyData = App::PropertyData(); \
template<> const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
template<> const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
template<> void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr(); \
}

} // namespace App

#endif // APP_PROPERTYCONTAINER_H
