/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "Application.h"
#include "Property.h"
#include "PropertyContainer.h"
#include "PropertyLinks.h"

FC_LOG_LEVEL_INIT("App",true,true)

using namespace App;
using namespace Base;
using namespace std;

TYPESYSTEM_SOURCE(App::PropertyContainer,Base::Persistence)


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


App::Property* PropertyContainer::addDynamicProperty(
    const std::string& type, const std::string& name, const std::string& group, const std::string& doc)
{
    return dynamicProps.addDynamicProperty(*this,type,name,group,doc);
}

Property *PropertyContainer::getPropertyByName(const std::string& name) const
{
    auto *prop = dynamicProps.getDynamicPropertyByName(name);
    if(prop) return prop;
    return getPropertyData().getPropertyByName(this,name);
}

void PropertyContainer::getPropertyMap(std::map<std::string,Property*> &Map) const
{
    dynamicProps.getPropertyMap(Map);
    getPropertyData().getPropertyMap(this,Map);
}

void PropertyContainer::getPropertyList(std::vector<Property*> &List) const
{
    dynamicProps.getPropertyList(List);
    getPropertyData().getPropertyList(this,List);
}

void PropertyContainer::setPropertyStatus(PropertyStatus bit,bool value) const
{
    std::vector<Property*> List;
    getPropertyList(List);
    for(std::vector<Property*>::const_iterator it=List.begin();it!=List.end();++it)
    {
        (**it).setStatus(bit,value);
    }
}


bool PropertyContainer::isOwnerOf(const Property & prop) const {
    return prop.getContainer() == this;
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

void PropertyContainer::Save (Base::Writer &writer) const
{
    std::map<std::string,Property*> Map;
    getPropertyMap(Map);

    std::vector<Property*> transients;
    for(auto it=Map.begin();it!=Map.end();) {
        auto prop = it->second;
        if(prop->testStatus(PropertyStatus::Prop_NoPersist)) {
            it = Map.erase(it);
            continue;
        }
        if(!prop->testStatus(PropertyStatus::Prop_Dynamic)
                && (prop->testStatus(PropertyStatus::Transient) ||
                    prop->testStatus(PropertyStatus::Prop_Transient)))
        {
            transients.push_back(prop);
            it = Map.erase(it);
        }else
            ++it;
    }

    writer.incInd(); // indentation for 'Properties Count'
    writer.Stream() << writer.ind() << "<Properties Count=\"" << Map.size()
                    << "\" TransientCount=\"" << transients.size() << "\">" << endl;

    // First store transient properties to persist their status value. We use
    // a new element named "_Property" so that the save file can be opened by
    // older versions of FC.
    writer.incInd();
    for(auto prop : transients) {
        writer.Stream() << writer.ind() << "<_Property name=\"" << prop->getName()
            << "\" type=\"" << prop->getTypeId().getName()
            << "\" status=\"" << prop->getStatus().to_ullong() << "\"/>" << std::endl;
    }
    writer.decInd();

    // Now store normal properties
    for (auto it = Map.begin(); it != Map.end(); ++it)
    {
        writer.incInd(); // indentation for 'Property name'
        writer.Stream() << writer.ind() << "<Property name=\"" << it->first << "\" type=\""
                        << it->second->getTypeId().getName();

        dynamicProps.save(it->second,writer);

        auto status = it->second->getStatus().to_ullong();
        writer.Stream() << "\" status=\"" << status << "\">";

        if(it->second->testStatus(PropertyStatus::Transient)
                || it->second->testStatus(PropertyStatus::Prop_Transient) )
        {
            writer.decInd();
            writer.Stream() << "</Property>" << std::endl;
            continue;
        }

        writer.Stream() << std::endl;

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
    writer.Stream() << writer.ind() << "</Properties>" << endl;
    writer.decInd(); // indentation for 'Properties Count'
}

void PropertyContainer::Restore(Base::XMLReader &reader)
{
    reader.clearPartialRestoreProperty();
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    int transientCount = 0;
    if(reader.hasAttribute("TransientCount"))
        transientCount = reader.getAttributeAsUnsigned("TransientCount");

    for (int i=0;i<transientCount; ++i) {
        reader.readElement("_Property");
        Property* prop = getPropertyByName(reader.getAttribute("name"));
        if(prop)
            FC_TRACE("restore transient '" << prop->getName() << "'");
        if(prop && reader.hasAttribute("status")) {
            auto status= StatusCollection<PropertyStatus>(reader.getAttribute("status"));
            prop->setStatus(status);
        }

    }

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        std::string PropName = reader.getAttribute("name");
        std::string TypeName = reader.getAttribute("type");
        auto prop = dynamicProps.restore(*this,PropName.c_str(),TypeName.c_str(),reader);
        if(!prop)
            prop = getPropertyByName(PropName);

        if(prop && reader.hasAttribute("status")) {
            auto status= StatusCollection<PropertyStatus>(reader.getAttribute("status"));
            prop->setStatus(status);
        }
        // NOTE: We must also check the type of the current property because a
        // subclass of PropertyContainer might change the type of a property but
        // not its name. In this case we would force to read-in a wrong property
        // type and the behaviour would be undefined.
        try {
            // name and type match
            if (prop && strcmp(prop->getTypeId().getName(), TypeName.c_str()) == 0) {
                if (!prop->testStatus(PropertyStatus::Transient)
                    && !prop->testStatus(PropertyStatus::Prop_Transient))
                {
                    FC_TRACE("restore property '" << prop->getName() << "'");
                    prop->Restore(reader);
                }else
                    FC_TRACE("skip transient '" << prop->getName() << "'");
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

void PropertyContainer::onPropertyStatusChanged(const Property &prop, const App::PropertyStatus& status)
{
    (void)prop;
    (void)status;
}

void PropertyData::addProperty(OffsetBase offsetBase, const std::string& PropName, Property *Prop, const std::string& PropertyGroup , const std::string& PropertyDocu)
{
#ifdef FC_DEBUG
    if(!parentMerged)
#endif
    {
        short offset = offsetBase.getOffsetTo(Prop);
        if(offset < 0)
            throw Base::RuntimeError("Invalid static property");
        auto &index = propertyData.get<1>();
        auto it = index.find(PropName);
        if(it == index.end()) {
            if(parentMerged)
                throw Base::RuntimeError("Cannot add static property");
            index.insert(std::shared_ptr<PropertySpec>(new PropertySpec(PropName, PropertyGroup, PropertyDocu, offset)));
        } else{
#ifdef FC_DEBUG
            if((*it)->Offset != offset) {
                FC_ERR("Duplicate property '" << PropName << "'");
            }
#endif
        }
    }
    Prop->setPropertySpec(*(propertyData.get<1>().find(PropName)));
}

void PropertyData::merge(PropertyData *other) const {
    if(!other)
        other = const_cast<PropertyData*>(parentPropertyData);
    if(other == parentPropertyData) {
        if(parentMerged)
            return;
        parentMerged = true;
    }
    if(other)  {
        other->merge();
        auto &index = propertyData.get<0>();
        for(auto &spec : other->propertyData.get<0>())
            index.push_back(spec);
    }
}

void PropertyData::split(PropertyData *other) {
    if(other == parentPropertyData) {
        if(!parentMerged)
            return;
        parentMerged = false;
    }
    if(other)  {
        auto &index = propertyData.get<2>();
        for(const auto &spec : other->propertyData.get<0>()) {
            const short offset = spec->Offset;
            index.erase(offset);
        }

    }
}

std::shared_ptr<PropertySpec> PropertyData::findProperty(OffsetBase offsetBase,const std::string& PropName) const
{
    (void)offsetBase;
    merge();
    auto &index = propertyData.get<1>();
    auto it = index.find(PropName);
    if(it != index.end())
        return *it;
    return 0;
}

Property *PropertyData::getPropertyByName(OffsetBase offsetBase,const std::string& name) const
{
   auto Spec = findProperty(offsetBase,name);

  if(Spec)
    return (Property *) (Spec->Offset + offsetBase.getOffset());
  else
    return 0;
}

void PropertyData::getPropertyMap(OffsetBase offsetBase,std::map<std::string,Property*> &Map) const
{
    merge();
    for(auto &spec : propertyData.get<0>())
        Map[spec->Name] = (Property *) (spec->Offset + offsetBase.getOffset());
}

void PropertyData::getPropertyList(OffsetBase offsetBase,std::vector<Property*> &List) const
{
    merge();
    size_t base = List.size();
    List.reserve(base+propertyData.size());
    for (auto &spec : propertyData.get<0>())
        List.push_back((Property *) (spec->Offset + offsetBase.getOffset()));
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
