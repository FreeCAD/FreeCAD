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

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Property.h"
#include "PropertyContainer.h"
#include <Base/DocumentReader.h>

#ifndef _PreComp_
# include <xercesc/dom/DOM.hpp>
#endif


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
    propertyData.parentPropertyData = nullptr;
}

PropertyContainer::~PropertyContainer() = default;

unsigned int PropertyContainer::getMemSize () const
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
    const char* type, const char* name, const char* group, const char* doc,
    short attr, bool ro, bool hidden)
{
    return dynamicProps.addDynamicProperty(*this,type,name,group,doc,attr,ro,hidden);
}

Property *PropertyContainer::getPropertyByName(const char* name) const
{
    auto prop = dynamicProps.getDynamicPropertyByName(name);
    if(prop){
        return prop;
    }
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

void PropertyContainer::getPropertyNamedList(std::vector<std::pair<const char*, Property*> > &List) const
{
    dynamicProps.getPropertyNamedList(List);
    getPropertyData().getPropertyNamedList(this,List);
}

void PropertyContainer::setPropertyStatus(unsigned char bit,bool value)
{
    std::vector<Property*> List;
    getPropertyList(List);
    for(auto it : List)
        it->StatusBits.set(bit,value);
}

short PropertyContainer::getPropertyType(const Property* prop) const
{
    return prop?prop->getType():0;
}

short PropertyContainer::getPropertyType(const char *name) const
{
    return getPropertyType(getPropertyByName(name));
}

const char* PropertyContainer::getPropertyGroup(const Property* prop) const
{
    auto group = dynamicProps.getPropertyGroup(prop);
    if(group)
        return group;
    return getPropertyData().getGroup(this,prop);
}

const char* PropertyContainer::getPropertyGroup(const char *name) const
{
    auto group = dynamicProps.getPropertyGroup(name);
    if(group)
        return group;
    return getPropertyData().getGroup(this,name);
}

const char* PropertyContainer::getPropertyDocumentation(const Property* prop) const
{
    auto doc = dynamicProps.getPropertyDocumentation(prop);
    if(doc)
        return doc;
    return getPropertyData().getDocumentation(this,prop);
}

const char* PropertyContainer::getPropertyDocumentation(const char *name) const
{
    auto doc = dynamicProps.getPropertyDocumentation(name);
    if(doc)
        return doc;
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
    auto res = dynamicProps.getPropertyName(prop);
    if(!res)
        res = getPropertyData().getName(this,prop);
    return res;
}

const PropertyData * PropertyContainer::getPropertyDataPtr(){return &propertyData;}
const PropertyData & PropertyContainer::getPropertyData() const{return propertyData;}


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

void PropertyContainer::handleChangedPropertyName(Base::DocumentReader &reader, const char * TypeName, const char *PropName)
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

void PropertyContainer::handleChangedPropertyType(DocumentReader &reader, const char *TypeName, Property *prop)
{
    (void)reader;
    (void)TypeName;
    (void)prop;
}

PropertyData PropertyContainer::propertyData;

void PropertyContainer::beforeSave() const
{
    std::map<std::string, Property*> Map;
    getPropertyMap(Map);
    for (auto& entry : Map) {
        auto prop = entry.second;
        if (!prop->testStatus(Property::PropDynamic)
            && (prop->testStatus(Property::Transient)
                || ((getPropertyType(prop) & Prop_Transient) != 0))) {
            // Nothing
        }
        else {
            prop->beforeSave();
        }
    }
}

void PropertyContainer::Save (Base::Writer &writer) const
{
    std::map<std::string,Property*> Map;
    getPropertyMap(Map);

    std::vector<Property*> transients;
    for(auto it=Map.begin();it!=Map.end();) {
        auto prop = it->second;
        if(prop->testStatus(Property::PropNoPersist)) {
            it = Map.erase(it);
            continue;
        }
        if(!prop->testStatus(Property::PropDynamic)
                && (prop->testStatus(Property::Transient) ||
                    getPropertyType(prop) & Prop_Transient))
        {
            transients.push_back(prop);
            it = Map.erase(it);
        } else {
            ++it;
        }
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
            << "\" status=\"" << prop->getStatus() << "\"/>" << std::endl;
    }
    writer.decInd();

    // Now store normal properties
    for (const auto& it : Map)
    {
        writer.incInd(); // indentation for 'Property name'
        writer.Stream() << writer.ind() << "<Property name=\"" << it.first << "\" type=\""
                        << it.second->getTypeId().getName();

        dynamicProps.save(it.second,writer);

        auto status = it.second->getStatus();
        if(status)
            writer.Stream() << "\" status=\"" << status;
        writer.Stream() << "\">";

        if(it.second->testStatus(Property::Transient)
                || it.second->getType() & Prop_Transient)
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
            it.second->Save(writer);
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
        if(prop && reader.hasAttribute("status"))
            prop->setStatusValue(reader.getAttributeAsUnsigned("status"));
    }

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        std::string PropName = reader.getAttribute("name");
        std::string TypeName = reader.getAttribute("type");
        // NOTE: We must also check the type of the current property because a
        // subclass of PropertyContainer might change the type of a property but
        // not its name. In this case we would force to read-in a wrong property
        // type and the behaviour would be undefined.
        try {
            auto prop = getPropertyByName(PropName.c_str());
            if(!prop || prop->getContainer() != this){
                prop = dynamicProps.restore(*this,PropName.c_str(),TypeName.c_str(),reader);
            }

            decltype(Property::StatusBits) status;
            if(reader.hasAttribute("status")) {
                status = decltype(status)(reader.getAttributeAsUnsigned("status"));
                if(prop)
                    prop->setStatusValue(status.to_ulong());
            }
            // name and type match
            if (prop && strcmp(prop->getTypeId().getName(), TypeName.c_str()) == 0) {
                if (!prop->testStatus(Property::Transient)
                        && !status.test(Property::Transient)
                        && !status.test(Property::PropTransient)
                        && !prop->testStatus(Property::PropTransient))
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

void PropertyContainer::Restore(Base::DocumentReader &reader, XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *containerEl)
{
	reader.clearPartialRestoreProperty();
	
	auto PropertiesDOM = reader.FindElement(containerEl,"Properties");
	const char* count_cstr = reader.GetAttribute(PropertiesDOM,"Count");
	if(count_cstr){
		long Cnt = reader.ContentToInt( count_cstr );
		const char* TransientCount_cstr = reader.GetAttribute(PropertiesDOM,"TransientCount");
		int transientCount = 0;
		if(TransientCount_cstr)
			transientCount = reader.ContentToUnsigned( TransientCount_cstr );
		
		for (int i=0;i<transientCount; ++i) {
			auto _PropertyDOM = reader.FindElement(PropertiesDOM,"_Property");
			const char* name_cstr = reader.GetAttribute(_PropertyDOM,"name");
			const char* status_cstr = reader.GetAttribute(_PropertyDOM,"status");
			Property* prop = getPropertyByName(name_cstr);
			if(prop)
		        FC_TRACE("restore transient '" << prop->getName() << "'");
		    if(prop && status_cstr){
		    	int u_status = reader.ContentToUnsigned( status_cstr );
		        prop->setStatusValue(u_status);
		    }
    	}
    	if(Cnt){
    		auto prev_PropertyDOM = reader.FindElement(PropertiesDOM,"Property");
			readProperty(reader,prev_PropertyDOM);
			for (int i=1 ;i<Cnt ;i++) {
				auto PropertyDOM_i = reader.FindNextElement(prev_PropertyDOM,"Property");
				readProperty(reader,PropertyDOM_i);
				prev_PropertyDOM = PropertyDOM_i;
		    }
		    
    	}
	}
}

// NOTE: We must also check the type of the current property because a
// subclass of PropertyContainer might change the type of a property but
// not its name. In this case we would force to read-in a wrong property
// type and the behaviour would be undefined.
void PropertyContainer::readProperty(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *PropertyDOM){
	const char* name_cstr = reader.GetAttribute(PropertyDOM,"name");
	const char* type_cstr = reader.GetAttribute(PropertyDOM,"type");
	try {
		
	    auto prop = getPropertyByName(name_cstr);
	    if(!prop || prop->getContainer() != this){
	        prop = dynamicProps.restore(*this,name_cstr,type_cstr,reader,PropertyDOM);
	    }
	    
	    decltype(Property::StatusBits) status;
	    const char* status_cstr = reader.GetAttribute(PropertyDOM,"status");
	    if(status_cstr){
	    	int u_status = reader.ContentToUnsigned( status_cstr );
	    	status = decltype(status)(u_status);
	    	if(prop)
	            prop->setStatusValue(status.to_ulong());
	    }
	    // name and type match
	    if (prop && strcmp(prop->getTypeId().getName(), type_cstr) == 0) {
	        if (!prop->testStatus(Property::Transient)
	                && !status.test(Property::Transient)
	                && !status.test(Property::PropTransient)
	                && !prop->testStatus(Property::PropTransient))
	        {
	            FC_TRACE("restore property '" << prop->getName() << "'");
	            prop->Restore(reader,PropertyDOM);
	        }else
	            FC_TRACE("skip transient '" << prop->getName() << "'");
	    }
	    // name matches but not the type
	    else if (prop) {
	        handleChangedPropertyType(reader, type_cstr, prop);
	    }
	    // name doesn't match, the sub-class then has to know
	    // if the property has been renamed or removed
	    else {
	        handleChangedPropertyName(reader, type_cstr, name_cstr);
	    }
	    
	    if (reader.testStatus(Base::DocumentReader::ReaderStatus::PartialRestoreInProperty)) {
	        Base::Console().Error("Property %s of type %s was subject to a partial restore.\n",name_cstr,type_cstr);
	        reader.clearPartialRestoreProperty();
	    }
	}
	catch (const Base::XMLParseException&) {
	    throw; // re-throw
	}
	catch (const Base::RestoreError &) {
	    reader.setPartialRestore(true);
	    reader.clearPartialRestoreProperty();
	    Base::Console().Error("Property %s of type %s was subject to a partial restore.\n",name_cstr,type_cstr);
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
}

void PropertyContainer::onPropertyStatusChanged(const Property &prop, unsigned long oldStatus)
{
    (void)prop;
    (void)oldStatus;
}

void PropertyData::addProperty(OffsetBase offsetBase,const char* PropName, Property *Prop, const char* PropertyGroup , PropertyType Type, const char* PropertyDocu)
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
            index.emplace(PropName, PropertyGroup, PropertyDocu, offset, Type);
        } else{
#ifdef FC_DEBUG
            if(it->Offset != offset) {
                FC_ERR("Duplicate property '" << PropName << "'");
            }
#endif
        }
    }

    Prop->syncType(Type);
    Prop->myName = PropName;
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
        for(const auto &spec : other->propertyData.get<0>())
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
        for(const auto &spec : other->propertyData.get<0>())
            index.erase(spec.Offset);
    }
}

const PropertyData::PropertySpec *PropertyData::findProperty(OffsetBase offsetBase,const char* PropName) const
{
    (void)offsetBase;
    merge();
    auto &index = propertyData.get<1>();
    auto it = index.find(PropName);
    if(it != index.end())
        return &(*it);
    return nullptr;
}

const PropertyData::PropertySpec *PropertyData::findProperty(OffsetBase offsetBase,const Property* prop) const
{
    merge();
    int diff = offsetBase.getOffsetTo(prop);
    if(diff<0)
        return nullptr;

    auto &index = propertyData.get<2>();
    auto it = index.find(diff);
    if(it!=index.end())
        return &(*it);

    return nullptr;
}

const char* PropertyData::getName(OffsetBase offsetBase,const Property* prop) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,prop);

  if(Spec)
    return Spec->Name;
  else
    return nullptr;
}

short PropertyData::getType(OffsetBase offsetBase,const Property* prop) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,prop);

  if(Spec)
    return Spec->Type;
  else
    return 0;
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
    return nullptr;
}

const char* PropertyData::getGroup(OffsetBase offsetBase,const char* name) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,name);

  if(Spec)
    return Spec->Group;
  else
    return nullptr;
}

const char* PropertyData::getDocumentation(OffsetBase offsetBase,const Property* prop) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,prop);

  if(Spec)
    return Spec->Docu;
  else
    return nullptr;
}

const char* PropertyData::getDocumentation(OffsetBase offsetBase,const char* name) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,name);

  if(Spec)
    return Spec->Docu;
  else
    return nullptr;
}

Property *PropertyData::getPropertyByName(OffsetBase offsetBase,const char* name) const
{
  const PropertyData::PropertySpec* Spec = findProperty(offsetBase,name);

  if(Spec)
    return reinterpret_cast<Property *>(Spec->Offset + offsetBase.getOffset());
  else
    return nullptr;
}

void PropertyData::getPropertyMap(OffsetBase offsetBase,std::map<std::string,Property*> &Map) const
{
    merge();
    for(auto &spec : propertyData.get<0>())
        Map[spec.Name] = reinterpret_cast<Property *>(spec.Offset + offsetBase.getOffset());
}

void PropertyData::getPropertyList(OffsetBase offsetBase,std::vector<Property*> &List) const
{
    merge();
    size_t base = List.size();
    List.reserve(base+propertyData.size());
    for (auto &spec : propertyData.get<0>())
        List.push_back(reinterpret_cast<Property *>(spec.Offset + offsetBase.getOffset()));
}

void PropertyData::getPropertyNamedList(OffsetBase offsetBase,
        std::vector<std::pair<const char*,Property*> > &List) const
{
    merge();
    size_t base = List.size();
    List.reserve(base+propertyData.size());
    for (auto &spec : propertyData.get<0>()) {
        auto prop = reinterpret_cast<Property *>(spec.Offset + offsetBase.getOffset());
        List.emplace_back(prop->getName(),prop);
    }
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
