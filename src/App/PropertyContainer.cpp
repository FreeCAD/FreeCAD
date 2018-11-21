/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <functional>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "Property.h"
#include "PropertyContainer.h"
#include "PropertyLinks.h"

using namespace App;
using namespace Base;
using namespace std;

TYPESYSTEM_SOURCE(App::PropertyContainer,Base::Persistence);


//**************************************************************************
// Construction/Destruction

// Here's the implementation! Description should take place in the header file!
PropertyContainer::PropertyContainer()
{
    propertyData.parentPropertyData = 0;
}

PropertyContainer::~PropertyContainer()
{

}

unsigned int PropertyContainer::getMemSize (void) const
{
    std::map<std::string,Property*> Map;
    getPropertyMap(Map);
    std::map<std::string,Property*>::const_iterator It;
    unsigned int size = 0;
    for (It = Map.begin(); It != Map.end();++It)
        size += It->second->getMemSize();
    return size;
}

Property *PropertyContainer::getPropertyByName(const char* name) const
{
    return getPropertyData().getPropertyByName(this,name);
}

void PropertyContainer::getPropertyMap(std::map<std::string,Property*> &Map) const
{
    getPropertyData().getPropertyMap(this,Map);
}

void PropertyContainer::getPropertyList(std::vector<Property*> &List) const
{
    getPropertyData().getPropertyList(this,List);
}

void PropertyContainer::setPropertyStatus(unsigned char bit,bool value)
{
    std::vector<Property*> List;
    getPropertyList(List);
    for(std::vector<Property*>::const_iterator it=List.begin();it!=List.end();++it)
        (**it).StatusBits.set(bit,value);
}

short PropertyContainer::getPropertyType(const Property* prop) const
{
    return getPropertyData().getType(this,prop);
}

short PropertyContainer::getPropertyType(const char *name) const
{
    return getPropertyData().getType(this,name);
}

const char* PropertyContainer::getPropertyGroup(const Property* prop) const
{
    return getPropertyData().getGroup(this,prop);
}

const char* PropertyContainer::getPropertyGroup(const char *name) const
{
    return getPropertyData().getGroup(this,name);
}

const char* PropertyContainer::getPropertyDocumentation(const Property* prop) const
{
    return getPropertyData().getDocumentation(this,prop);
}

const char* PropertyContainer::getPropertyDocumentation(const char *name) const
{
    return getPropertyData().getDocumentation(this,name);
}

bool PropertyContainer::isReadOnly(const Property* prop) const
{
    return (getPropertyType(prop) & Prop_ReadOnly) == Prop_ReadOnly;
}

bool PropertyContainer::isReadOnly(const char *name) const
{
    return (getPropertyType(name) & Prop_ReadOnly) == Prop_ReadOnly;
}

bool PropertyContainer::isHidden(const Property* prop) const
{
    return (getPropertyType(prop) & Prop_Hidden) == Prop_Hidden;
}

bool PropertyContainer::isHidden(const char *name) const
{
    return (getPropertyType(name) & Prop_Hidden) == Prop_Hidden;
}

const char* PropertyContainer::getPropertyName(const Property* prop)const
{
    return getPropertyData().getName(this,prop);
}


const PropertyData * PropertyContainer::getPropertyDataPtr(void){return &propertyData;}
const PropertyData & PropertyContainer::getPropertyData(void) const{return propertyData;}

/**
 * @brief PropertyContainer::handleChangedPropertyName is called during restore to possibly
 * fix reading of older versions of this property container. This method is typically called
 * if the property on file has changed its name in more recent versions.
 *
 * The default implementation does nothing.
 *
 * @param reader The XML stream to read from.
 * @param TypeName Name of property type on file.
 * @param PropName Name of property on file that does not exist in the container anymore.
 */

void PropertyContainer::handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName)
{
    (void)reader;
    (void)TypeName;
    (void)PropName;
}

/**
 * @brief PropertyContainer::handleChangedPropertyType is called during restore to possibly
 * fix reading of older versions of the property container. This method is typically called
 * if the property on file has changed its type in more recent versions.
 *
 * The default implementation does nothing.
 *
 * @param reader The XML stream to read from.
 * @param TypeName Name of property type on file.
 * @param prop Pointer to property to restore. Its type differs from TypeName.
 */

void PropertyContainer::handleChangedPropertyType(XMLReader &reader, const char *TypeName, Property *prop)
{
    (void)reader;
    (void)TypeName;
    (void)prop;
}

PropertyData PropertyContainer::propertyData;

/**
 * Binary function to query the flags for use with generic STL functions.
 */
template <class TCLASS>
class PropertyAttribute : public std::binary_function<TCLASS, typename App::PropertyType, bool>
{
public:
    PropertyAttribute(const PropertyContainer* c) : cont(c) {}
    bool operator () (const TCLASS& prop, typename App::PropertyType attr) const
    { return (cont->getPropertyType(prop.second) & attr) == attr; }
private:
    const PropertyContainer* cont;
};

void PropertyContainer::Save (Base::Writer &writer) const
{
    std::map<std::string,Property*> Map;
    getPropertyMap(Map);

    // ignore the properties we won't store
    size_t ct = std::count_if(Map.begin(), Map.end(), std::bind2nd(PropertyAttribute
        <std::pair<std::string,Property*> >(this), Prop_Transient));
    size_t size = Map.size() - ct;

    writer.incInd(); // indentation for 'Properties Count'
    writer.Stream() << writer.ind() << "<Properties Count=\"" << size << "\">" << endl;
    std::map<std::string,Property*>::iterator it;
    for (it = Map.begin(); it != Map.end(); ++it)
    {
        // Don't write transient properties
        if (!(getPropertyType(it->second) & Prop_Transient))
        {
            writer.incInd(); // indentation for 'Property name'
            writer.Stream() << writer.ind() << "<Property name=\"" << it->first << "\" type=\""
                            << it->second->getTypeId().getName() << "\">" << endl;;
            writer.incInd(); // indentation for the actual property
            try {
                // We must make sure to handle all exceptions accordingly so that
                // the project file doesn't get invalidated. In the error case this
                // means to proceed instead of aborting the write operation.
                it->second->Save(writer);
            }
            catch (const Base::Exception &e) {
                Base::Console().Error("%s\n", e.what());
            }
            catch (const std::exception &e) {
                Base::Console().Error("%s\n", e.what());
            }
            catch (const char* e) {
                Base::Console().Error("%s\n", e);
            }
#ifndef FC_DEBUG
            catch (...) {
                Base::Console().Error("PropertyContainer::Save: Unknown C++ exception thrown. Try to continue...\n");
            }
#endif
            writer.decInd(); // indentation for the actual property
            writer.Stream() << writer.ind() << "</Property>" << endl;
            writer.decInd(); // indentation for 'Property name'
        }
    }
    writer.Stream() << writer.ind() << "</Properties>" << endl;
    writer.decInd(); // indentation for 'Properties Count'
}

void PropertyContainer::Restore(Base::XMLReader &reader)
{
    reader.clearPartialRestoreProperty();
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        std::string PropName = reader.getAttribute("name");
        std::string TypeName = reader.getAttribute("type");
        Property* prop = getPropertyByName(PropName.c_str());
        // NOTE: We must also check the type of the current property because a
        // subclass of PropertyContainer might change the type of a property but
        // not its name. In this case we would force to read-in a wrong property
        // type and the behaviour would be undefined.
        try {
            // name and type match
            if (prop && strcmp(prop->getTypeId().getName(), TypeName.c_str()) == 0) {
                prop->Restore(reader);
            }
            // name matches but not the type
            else if (prop) {
                handleChangedPropertyType(reader, TypeName.c_str(), prop);
            }
            // name doesn't match, the sub-class then has to know
            // if the property has been renamed or removed
            else {
                handleChangedPropertyName(reader, TypeName.c_str(), PropName.c_str());
            }

            if (reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInProperty)) {
                Base::Console().Error("Property %s of type %s was subject to a partial restore.\n",PropName.c_str(),TypeName.c_str());
                reader.clearPartialRestoreProperty();
            }
        }
        catch (const Base::XMLParseException&) {
            throw; // re-throw
        }
        catch (const Base::RestoreError &) {
            reader.setPartialRestore(true);
            reader.clearPartialRestoreProperty();
            Base::Console().Error("Property %s of type %s was subject to a partial restore.\n",PropName.c_str(),TypeName.c_str());
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const char* e) {
            Base::Console().Error("%s\n", e);
        }
#ifndef FC_DEBUG
        catch (...) {
            Base::Console().Error("PropertyContainer::Restore: Unknown C++ exception thrown\n");
        }
#endif

        reader.readEndElement("Property");
    }
    reader.readEndElement("Properties");
}

void PropertyData::addProperty(OffsetBase offsetBase,const char* PropName, Property *Prop, const char* PropertyGroup , PropertyType Type, const char* PropertyDocu)
{
  bool IsIn = false;
  for (vector<PropertySpec>::const_iterator It = propertyData.begin(); It != propertyData.end(); ++It)
    if(strcmp(It->Name,PropName)==0)
      IsIn = true;

  if( !IsIn )
  {
    PropertySpec temp;
    temp.Name   = PropName;
    temp.Offset = offsetBase.getOffsetTo(Prop);
    assert(temp.Offset>=0);
    temp.Group  = PropertyGroup;
    temp.Type   = Type;
    temp.Docu   = PropertyDocu;
    propertyData.push_back(temp);
  }
}

const PropertyData::PropertySpec *PropertyData::findProperty(OffsetBase offsetBase,const char* PropName) const
{
  for (vector<PropertyData::PropertySpec>::const_iterator It = propertyData.begin(); It != propertyData.end(); ++It)
    if(strcmp(It->Name,PropName)==0)
      return &(*It);

  if(parentPropertyData)
      return parentPropertyData->findProperty(offsetBase,PropName);

  return 0;
}

const PropertyData::PropertySpec *PropertyData::findProperty(OffsetBase offsetBase,const Property* prop) const
{
  const int diff = offsetBase.getOffsetTo(prop);
  if(diff<0)
      return 0;

  for (vector<PropertyData::PropertySpec>::const_iterator It = propertyData.begin(); It != propertyData.end(); ++It)
    if(diff == It->Offset)
        return &(*It);

  if(parentPropertyData)
      return parentPropertyData->findProperty(offsetBase,prop);

  return 0;
}

const char* PropertyData::getName(OffsetBase offsetBase,const Property* prop) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,prop);

  if(Spec)
    return Spec->Name;
  else
    return 0;
  /*

  for(std::map<std::string,PropertySpec>::const_iterator pos = propertyData.begin();pos != propertyData.end();++pos)
    if(pos->second.Offset == diff)
      return pos->first.c_str();

  if(parentPropertyData)
    return parentPropertyData->getName(container,prop);

  return 0;
  */
}

short PropertyData::getType(OffsetBase offsetBase,const Property* prop) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,prop);

  if(Spec)
    return Spec->Type;
  else
    return 0;

  /*
  const int diff = (int) ((char*)prop - (char*)container);

  for(std::map<std::string,PropertySpec>::const_iterator pos = propertyData.begin();pos != propertyData.end();++pos)
    if(pos->second.Offset == diff)
      return pos->second.Type;

  if(parentPropertyData)
    return parentPropertyData->getType(container,prop);

  return 0;
  */
}

short PropertyData::getType(OffsetBase offsetBase,const char* name) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,name);

  if(Spec)
    return Spec->Type;
  else
    return 0;
}

const char* PropertyData::getGroup(OffsetBase offsetBase,const Property* prop) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,prop);

  if(Spec)
    return Spec->Group;
  else
    return 0;

  /*
  const int diff = (int) ((char*)prop - (char*)container);

  for(std::map<std::string,PropertySpec>::const_iterator pos = propertyData.begin();pos != propertyData.end();++pos)
    if(pos->second.Offset == diff)
      return pos->second.Group;

  if(parentPropertyData)
    return parentPropertyData->getGroup(container,prop);

  return 0;
  */
}

const char* PropertyData::getGroup(OffsetBase offsetBase,const char* name) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,name);

  if(Spec)
    return Spec->Group;
  else
    return 0;
}

const char* PropertyData::getDocumentation(OffsetBase offsetBase,const Property* prop) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,prop);

  if(Spec)
    return Spec->Docu;
  else
    return 0;
}

const char* PropertyData::getDocumentation(OffsetBase offsetBase,const char* name) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,name);

  if(Spec)
    return Spec->Docu;
  else
    return 0;
}



Property *PropertyData::getPropertyByName(OffsetBase offsetBase,const char* name) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,name);

  if(Spec)
    return (Property *) (Spec->Offset + offsetBase.getOffset());
  else
    return 0;
/*
  std::map<std::string,PropertySpec>::const_iterator pos = propertyData.find(name);

  if(pos != propertyData.end())
  {
    // calculate propterty by offset
    return (Property *) (pos->second.Offset + (char *)container);
  }else{
    if(parentPropertyData)
      return parentPropertyData->getPropertyByName(container,name);
    else
      return 0;
  }*/
}

void PropertyData::getPropertyMap(OffsetBase offsetBase,std::map<std::string,Property*> &Map) const
{
  for (vector<PropertyData::PropertySpec>::const_iterator It = propertyData.begin(); It != propertyData.end(); ++It)
    Map[It->Name] = (Property *) (It->Offset + offsetBase.getOffset());
/*
  std::map<std::string,PropertySpec>::const_iterator pos;

  for(pos = propertyData.begin();pos != propertyData.end();++pos)
  {
    Map[pos->first] = (Property *) (pos->second.Offset + (char *)container);
  }
  */

  if(parentPropertyData)
      parentPropertyData->getPropertyMap(offsetBase,Map);

}

void PropertyData::getPropertyList(OffsetBase offsetBase,std::vector<Property*> &List) const
{
  for (vector<PropertyData::PropertySpec>::const_iterator It = propertyData.begin(); It != propertyData.end(); ++It)
    List.push_back((Property *) (It->Offset + offsetBase.getOffset()) );

/*  std::map<std::string,PropertySpec>::const_iterator pos;

  for(pos = propertyData.begin();pos != propertyData.end();++pos)
  {
    List.push_back((Property *) (pos->second.Offset + (char *)container) );
  }*/
  if(parentPropertyData)
      parentPropertyData->getPropertyList(offsetBase,List);

}



/** \defgroup PropFrame Property framework
    \ingroup APP
    \brief System to access object properties
\section Introduction
The property framework introduces the ability to access attributes (member variables) of a class by name without
knowing the class type. It's like the reflection mechanism of Java or C#.
This ability is introduced by the App::PropertyContainer class and can be used by all derived classes.

This makes it possible in the first place to make an automatic mapping to python (e.g. in App::FeaturePy) and
abstract editing properties in Gui::PropertyEditor.

\section Examples

Here some little examples how to use it:

\code
// search in PropertyList
Property *prop = _pcFeature->getPropertyByName(attr);
if(prop)
{
  return prop->getPyObject();
}
\endcode

or:

\code
void PropertyContainer::Restore(Base::Reader &reader)
{
  reader.readElement("Properties");
  int Cnt = reader.getAttributeAsInteger("Count");

  for(int i=0 ;i<Cnt ;i++)
  {
    reader.readElement("Property");
    string PropName = reader.getAttribute("name");
    Property* prop = getPropertyByName(PropName.c_str());
    if(prop)
      prop->Restore(reader);

    reader.readEndElement("Property");
  }
  reader.readEndElement("Properties");
}
\endcode

*/

