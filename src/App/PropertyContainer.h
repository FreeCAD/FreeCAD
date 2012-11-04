/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2005     *
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
#include <Base/Persistence.h>

namespace Base {
class Writer;
}


namespace App
{
class Property;
class PropertyContainer;
class DocumentObject;

enum PropertyType 
{
  Prop_None     = 0,
  Prop_ReadOnly = 1,
  Prop_Transient= 2,
  Prop_Hidden   = 4,
  Prop_Output   = 8
};

struct AppExport PropertyData
{
  struct PropertySpec
  {
    const char* Name;
    const char * Group;
    const char * Docu;
    short Offset,Type;
  };
  // vector of all properties
  std::vector<PropertySpec> propertyData;
  const PropertyData *parentPropertyData;

  void addProperty(const PropertyContainer *container,const char* PropName, Property *Prop, const char* PropertyGroup= 0, PropertyType = Prop_None, const char* PropertyDocu= 0 );

  const PropertySpec *findProperty(const PropertyContainer *container,const char* PropName) const;
  const PropertySpec *findProperty(const PropertyContainer *container,const Property* prop) const;
  
  const char* getName         (const PropertyContainer *container,const Property* prop) const;
  short       getType         (const PropertyContainer *container,const Property* prop) const;
  short       getType         (const PropertyContainer *container,const char* name)     const;
  const char* getGroup        (const PropertyContainer *container,const char* name)     const;
  const char* getGroup        (const PropertyContainer *container,const Property* prop) const;
  const char* getDocumentation(const PropertyContainer *container,const char* name)     const;
  const char* getDocumentation(const PropertyContainer *container,const Property* prop) const;

  Property *getPropertyByName(const PropertyContainer *container,const char* name) const;
  void getPropertyMap(const PropertyContainer *container,std::map<std::string,Property*> &Map) const;
  void getPropertyList(const PropertyContainer *container,std::vector<Property*> &List) const;
};


/** Base class of all classes with properties
 */
class AppExport PropertyContainer: public Base::Persistence
{

  TYPESYSTEM_HEADER();

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
  virtual ~PropertyContainer();

  virtual unsigned int getMemSize (void) const;

  /// find a property by its name
  virtual Property *getPropertyByName(const char* name) const;
  /// get the name of a property
  virtual const char* getName(const Property* prop) const;
  /// get all properties of the class (including properties of the parent)
  virtual void getPropertyMap(std::map<std::string,Property*> &Map) const;
  /// get all properties of the class (including properties of the parent)
  virtual void getPropertyList(std::vector<Property*> &List) const;
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
  virtual bool isReadOnly(const Property* prop) const;
  /// check if the nameed property is read-only
  virtual bool isReadOnly(const char *name) const;
  /// check if the property is hidden
  virtual bool isHidden(const Property* prop) const;
  /// check if the named property is hidden
  virtual bool isHidden(const char *name) const;
  virtual App::Property* addDynamicProperty(
        const char* type, const char* name=0,
        const char* group=0, const char* doc=0,
        short attr=0, bool ro=false, bool hidden=false){
        return 0;
  }
  virtual bool removeDynamicProperty(const char* name) {
      return false;
  }
  virtual std::vector<std::string> getDynamicPropertyNames() const {
      return std::vector<std::string>();
  }
  virtual App::Property *getDynamicPropertyByName(const char* name) const {
      return 0;
  }
  virtual void addDynamicProperties(const PropertyContainer*) {
  }

  virtual void Save (Base::Writer &writer) const;
  virtual void Restore(Base::XMLReader &reader);


  friend class Property;


protected: 
  /// get called by the container when a property has changed
  virtual void onChanged(const Property* /*prop*/){};
  /// get called before the value is changed
  virtual void onBeforeChange(const Property* /*prop*/){};

  //void hasChanged(Propterty* prop);
  static const  PropertyData * getPropertyDataPtr(void); 
  virtual const PropertyData& getPropertyData(void) const; 

private:
  // forbidden
  PropertyContainer(const PropertyContainer&);
  PropertyContainer& operator = (const PropertyContainer&);

private: 
  static PropertyData propertyData; 
};

/// Property define 
#define ADD_PROPERTY(_prop_, _defaultval_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    this->_prop_.setContainer(this); \
    propertyData.addProperty(this, #_prop_, &this->_prop_); \
  } while (0)

#define ADD_PROPERTY_TYPE(_prop_, _defaultval_, _group_,_type_,_Docu_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    this->_prop_.setContainer(this); \
    propertyData.addProperty(this, #_prop_, &this->_prop_, (_group_),(_type_),(_Docu_)); \
  } while (0)



#define PROPERTY_HEADER(_class_) \
  TYPESYSTEM_HEADER(); \
protected: \
  static const App::PropertyData * getPropertyDataPtr(void); \
  virtual const App::PropertyData &getPropertyData(void) const; \
private: \
  static App::PropertyData propertyData 

/// 
#define PROPERTY_SOURCE(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_P(_class_);\
const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr();\
}

#define PROPERTY_SOURCE_ABSTRACT(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_ABSTRACT_P(_class_);\
const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr();\
}

#define TYPESYSTEM_SOURCE_TEMPLATE(_class_) \
template<> Base::Type _class_::getClassTypeId(void) { return _class_::classTypeId; } \
template<> Base::Type _class_::getTypeId(void) const { return _class_::classTypeId; } \
template<> Base::Type _class_::classTypeId = Base::Type::badType();  \
template<> void * _class_::create(void){\
   return new _class_ ();\
}

#define PROPERTY_SOURCE_TEMPLATE(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_TEMPLATE(_class_);\
template<> const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
template<> const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
template<> App::PropertyData _class_::propertyData = App::PropertyData(); \
template<> void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr();\
}


} // namespace App

#endif // APP_PROPERTYCONTAINER_H
