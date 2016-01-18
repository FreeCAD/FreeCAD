/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
#endif

#include "DynamicProperty.h"
#include "Property.h"
#include "PropertyContainer.h"
#include "Application.h"
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>


using namespace App;


DynamicProperty::DynamicProperty(PropertyContainer* p) : pc(p)
{
}

DynamicProperty::~DynamicProperty()
{
}

void DynamicProperty::getPropertyList(std::vector<Property*> &List) const
{
    // get the properties of the base class first and insert the dynamic properties afterwards
    this->pc->PropertyContainer::getPropertyList(List);
    for (std::map<std::string,PropData>::const_iterator it = props.begin(); it != props.end(); ++it)
        List.push_back(it->second.property);
}

void DynamicProperty::getPropertyMap(std::map<std::string,Property*> &Map) const
{
    // get the properties of the base class first and insert the dynamic properties afterwards
    this->pc->PropertyContainer::getPropertyMap(Map);
    for (std::map<std::string,PropData>::const_iterator it = props.begin(); it != props.end(); ++it)
        Map[it->first] = it->second.property;
}

Property *DynamicProperty::getPropertyByName(const char* name) const
{
    std::map<std::string,PropData>::const_iterator it = props.find(name);
    if (it != props.end())
        return it->second.property;
    return this->pc->PropertyContainer::getPropertyByName(name);
}

Property *DynamicProperty::getDynamicPropertyByName(const char* name) const
{
    std::map<std::string,PropData>::const_iterator it = props.find(name);
    if (it != props.end())
        return it->second.property;
    return 0;
}

std::vector<std::string> DynamicProperty::getDynamicPropertyNames() const
{
    std::vector<std::string> names;
    for (std::map<std::string,PropData>::const_iterator it = props.begin(); it != props.end(); ++it) {
        names.push_back(it->first);
    }
    return names;
}

void DynamicProperty::addDynamicProperties(const PropertyContainer* cont)
{
    std::vector<std::string> names = cont->getDynamicPropertyNames();
    for (std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it) {
        App::Property* prop = cont->getDynamicPropertyByName(it->c_str());
        if (prop) {
            addDynamicProperty(
                prop->getTypeId().getName(),
                prop->getName(),
                prop->getGroup(),
                prop->getDocumentation(),
                prop->getType(),
                cont->isReadOnly(prop),
                cont->isHidden(prop));
        }
    }
}

const char* DynamicProperty::getPropertyName(const Property* prop) const
{
    for (std::map<std::string,PropData>::const_iterator it = props.begin(); it != props.end(); ++it) {
        if (it->second.property == prop)
            return it->first.c_str();
    }
    return this->pc->PropertyContainer::getPropertyName(prop);
}

unsigned int DynamicProperty::getMemSize (void) const
{
    std::map<std::string,Property*> Map;
    getPropertyMap(Map);
    std::map<std::string,Property*>::const_iterator It;
    unsigned int size = 0;
    for (It = Map.begin(); It != Map.end();++It)
        size += It->second->getMemSize();
    return size;
}

short DynamicProperty::getPropertyType(const Property* prop) const
{
    for (std::map<std::string,PropData>::const_iterator it = props.begin(); it != props.end(); ++it) {
        if (it->second.property == prop) {
            short attr = it->second.attr;
            if (it->second.hidden)
                attr |= Prop_Hidden;
            if (it->second.readonly)
                attr |= Prop_ReadOnly;
            return attr;
        }
    }
    return this->pc->PropertyContainer::getPropertyType(prop);
}

short DynamicProperty::getPropertyType(const char *name) const
{
    std::map<std::string,PropData>::const_iterator it = props.find(name);
    if (it != props.end()) {
        short attr = it->second.attr;
        if (it->second.hidden)
            attr |= Prop_Hidden;
        if (it->second.readonly)
            attr |= Prop_ReadOnly;
        return attr;
    }
    return this->pc->PropertyContainer::getPropertyType(name);
}

const char* DynamicProperty::getPropertyGroup(const Property* prop) const
{
    for (std::map<std::string,PropData>::const_iterator it = props.begin(); it != props.end(); ++it) {
        if (it->second.property == prop)
            return it->second.group.c_str();
    }
    return this->pc->PropertyContainer::getPropertyGroup(prop);
}

const char* DynamicProperty::getPropertyGroup(const char *name) const
{
    std::map<std::string,PropData>::const_iterator it = props.find(name);
    if (it != props.end())
        return it->second.group.c_str();
    return this->pc->PropertyContainer::getPropertyGroup(name);
}

const char* DynamicProperty::getPropertyDocumentation(const Property* prop) const
{
    for (std::map<std::string,PropData>::const_iterator it = props.begin(); it != props.end(); ++it) {
        if (it->second.property == prop)
            return it->second.doc.c_str();
    }
    return this->pc->PropertyContainer::getPropertyDocumentation(prop);
}

const char* DynamicProperty::getPropertyDocumentation(const char *name) const
{
    std::map<std::string,PropData>::const_iterator it = props.find(name);
    if (it != props.end())
        return it->second.doc.c_str();
    return this->pc->PropertyContainer::getPropertyDocumentation(name);
}

Property* DynamicProperty::addDynamicProperty(const char* type, const char* name, const char* group,
                                              const char* doc, short attr, bool ro, bool hidden)
{
    Base::BaseClass* base = static_cast<Base::BaseClass*>(Base::Type::createInstanceByName(type,true));
    if (!base)
        return 0;
    if (!base->getTypeId().isDerivedFrom(Property::getClassTypeId())) {
        delete base;
        std::stringstream str;
        str << "'" << type << "' is not a property type";
        throw Base::Exception(str.str());
    }

    // get unique name
    Property* pcProperty = static_cast<Property*>(base);
    std::string ObjectName;
    if (name && *name != '\0')
        ObjectName = getUniquePropertyName(name);
    else
        ObjectName = getUniquePropertyName(type);

    pcProperty->setContainer(this->pc);
    PropData data;
    data.property = pcProperty;
    data.group = (group ? group : "");
    data.doc = (doc ? doc : "");
    data.attr = attr;
    data.readonly = ro;
    data.hidden = hidden;
    props[ObjectName] = data;

    GetApplication().signalAppendDynamicProperty(*pcProperty);

    return pcProperty;
}

bool DynamicProperty::removeDynamicProperty(const char* name)
{
    std::map<std::string,PropData>::iterator it = props.find(name);
    if (it != props.end()) {
        GetApplication().signalRemoveDynamicProperty(*it->second.property);
        delete it->second.property;
        props.erase(it);
        return true;
    }

    return false;
}

std::string DynamicProperty::getUniquePropertyName(const char *Name) const
{
    std::string CleanName = Base::Tools::getIdentifier(Name);

    // name in use?
    std::map<std::string,Property*> objectProps;
    getPropertyMap(objectProps);
    std::map<std::string,Property*>::const_iterator pos;
    pos = objectProps.find(CleanName);

    if (pos == objectProps.end()) {
        // if not, name is OK
        return CleanName;
    }
    else {
        std::vector<std::string> names;
        names.reserve(objectProps.size());
        for (pos = objectProps.begin();pos != objectProps.end();++pos) {
            names.push_back(pos->first);
        }
        return Base::Tools::getUniqueName(CleanName, names);
    }
}

std::string DynamicProperty::encodeAttribute(const std::string& str) const
{
    std::string tmp;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (*it == '<')
            tmp += "&lt;";
        else if (*it == '"')
            tmp += "&quot;";
        else if (*it == '\'')
            tmp += "&apos;";
        else if (*it == '&')
            tmp += "&amp;";
        else if (*it == '>')
            tmp += "&gt;";
        else if (*it == '\r')
            tmp += "&#xD;";
        else if (*it == '\n')
            tmp += "&#xA;";
        else
            tmp += *it;
    }

    return tmp;
}

void DynamicProperty::Save (Base::Writer &writer) const 
{
    std::map<std::string,Property*> Map;
    getPropertyMap(Map);

    writer.incInd(); // indentation for 'Properties Count'
    writer.Stream() << writer.ind() << "<Properties Count=\"" << Map.size() << "\">" << std::endl;
    std::map<std::string,Property*>::iterator it;
    for (it = Map.begin(); it != Map.end(); ++it)
    {
        writer.incInd(); // indentation for 'Property name'
        // check whether a static or dynamic property
        std::map<std::string,PropData>::const_iterator pt = props.find(it->first);
        if (pt == props.end()) {
            writer.Stream() << writer.ind() << "<Property name=\"" << it->first << "\" type=\"" 
                            << it->second->getTypeId().getName() << "\">" << std::endl;
        }
        else {
            writer.Stream() << writer.ind() << "<Property name=\"" << it->first
                            << "\" type=\"" << it->second->getTypeId().getName()
                            << "\" group=\"" << encodeAttribute(pt->second.group)
                            << "\" doc=\"" << encodeAttribute(pt->second.doc)
                            << "\" attr=\"" << pt->second.attr << "\" ro=\"" << pt->second.readonly
                            << "\" hide=\"" << pt->second.hidden << "\">" << std::endl;
        }

        writer.incInd(); // indentation for the actual property
        try {
            // We must make sure to handle all exceptions accordingly so that
            // the project file doesn't get invalidated. In the error case this
            // means to proceed instead of aborting the write operation.

            // Don't write transient properties 
            if (!(getPropertyType(it->second) & Prop_Transient))
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
            Base::Console().Error("DynamicProperty::Save: Unknown C++ exception thrown. Try to continue...\n");
        }
#endif
        writer.decInd(); // indentation for the actual property
        writer.Stream() << writer.ind() << "</Property>" << std::endl;
        writer.decInd(); // indentation for 'Property name'
    }
    writer.Stream() << writer.ind() << "</Properties>" << std::endl;
    writer.decInd(); // indentation for 'Properties Count'
}

void DynamicProperty::Restore(Base::XMLReader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        const char* PropName = reader.getAttribute("name");
        const char* TypeName = reader.getAttribute("type");
        Property* prop = getPropertyByName(PropName);
        try {
            if (!prop) {
                short attribute = 0;
                bool readonly = false, hidden = false;
                const char *group=0, *doc=0, *attr=0, *ro=0, *hide=0;
                if (reader.hasAttribute("group"))
                    group = reader.getAttribute("group");
                if (reader.hasAttribute("doc"))
                    doc = reader.getAttribute("doc");
                if (reader.hasAttribute("attr")) {
                    attr = reader.getAttribute("attr");
                    if (attr) attribute = attr[0]-48;
                }
                if (reader.hasAttribute("ro")) {
                    ro = reader.getAttribute("ro");
                    if (ro) readonly = (ro[0]-48) != 0;
                }
                if (reader.hasAttribute("hide")) {
                    hide = reader.getAttribute("hide");
                    if (hide) hidden = (hide[0]-48) != 0;
                }
                prop = addDynamicProperty(TypeName, PropName, group, doc, attribute, readonly, hidden);
            }
        }
        catch(const Base::Exception& e) {
            // only handle this exception type
            Base::Console().Warning(e.what());
        }

        //NOTE: We must also check the type of the current property because a subclass of
        //PropertyContainer might change the type of a property but not its name. In this
        //case we would force to read-in a wrong property type and the behaviour would be
        //undefined.

        // Don't read transient properties
        if (!(getPropertyType(prop) & Prop_Transient)) {
            if (prop && strcmp(prop->getTypeId().getName(), TypeName) == 0) {
                try {
                    prop->Restore(reader);
                }
                catch (const Base::XMLParseException&) {
                    throw; // re-throw
                }
                catch (const Base::Exception &e) {
                    Base::Console().Error("%s\n", e.what());
                }
                catch (const std::exception &e) {
                    Base::Console().Error("%s\n", e.what());
                }
#ifndef FC_DEBUG
                catch (...) {
                    Base::Console().Error("DynamicProperty::Restore: Unknown C++ exception thrown");
                }
#endif
            }
            else if (prop) {
                Base::Console().Warning("%s: Overread data for property %s of type %s, expected type is %s\n",
                    pc->getTypeId().getName(), prop->getName(), prop->getTypeId().getName(), TypeName);
            }
            else {
                Base::Console().Warning("%s: No property found with name %s and type %s\n",
                    pc->getTypeId().getName(), PropName, TypeName);
            }
        }
        reader.readEndElement("Property");
    }

    reader.readEndElement("Properties");
}
