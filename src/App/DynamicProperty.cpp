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
#include "LegacyPropertyStatus.h"
#include "PropertyContainer.h"
#include "Application.h"
#include "ExtensionContainer.h"
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

FC_LOG_LEVEL_INIT("DynamicProperty",true,true)


using namespace App;


DynamicProperty::DynamicProperty()
{
}

DynamicProperty::~DynamicProperty()
{
    clear();
}

void DynamicProperty::clear() {
    auto &index = props.get<0>();
    for(auto &v : index)
        delete v->property;
    index.clear();
}

void DynamicProperty::getPropertyList(std::vector<Property*> &List) const
{
    for (auto &v : props.get<0>())
        List.push_back(v->property);
}

void DynamicProperty::getPropertyMap(std::map<std::string,Property*> &Map) const
{
    for (auto &v : props.get<0>())
        Map[v->Name] = v->property;
}

Property *DynamicProperty::getDynamicPropertyByName(const std::string& name) const
{
    auto &index = props.get<0>();
    auto it = index.find(name);
    if (it != index.end())
        return (*it)->property;
    return 0;
}

std::vector<std::string> DynamicProperty::getDynamicPropertyNames() const
{
    std::vector<std::string> names;
    auto &index = props.get<0>();
    names.reserve(index.size());
    for(auto &v : index)
        names.push_back(v->Name);
    return names;
}

Property* DynamicProperty::addDynamicProperty(PropertyContainer &pc, const std::string& type,
        const std::string& name, const std::string& group, const std::string& doc)
{
    Base::BaseClass* base = static_cast<Base::BaseClass*>(Base::Type::createInstanceByName(type.c_str(),true));
    if (!base)
        return 0;
    if (!base->getTypeId().isDerivedFrom(Property::getClassTypeId())) {
        delete base;
        std::stringstream str;
        str << "'" << type << "' is not a property type";
        throw Base::ValueError(str.str());
    }

    // get unique name
    Property* pcProperty = static_cast<Property*>(base);
    std::string decidedName;
    if(name.empty()) //TODO: remove this
        decidedName=type;
    else
        decidedName=name;
    auto res = props.get<0>().insert(
         std::shared_ptr<DynamicProperty::PropData>
         (
          new DynamicProperty::PropData(pcProperty, getUniquePropertyName(pc,decidedName), group, doc)
          )
    );

    pcProperty->setContainer(&pc);
    pcProperty->setPropertySpec(*res.first);

    pcProperty->setStatus(Prop_Dynamic);

    GetApplication().signalAppendDynamicProperty(*pcProperty);

    return pcProperty;
}

bool DynamicProperty::removeProperty(const Property *prop)
{
    auto &index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if (it != index.end()) {
        index.erase(it);
        return true;
    }
    return false;
}

bool DynamicProperty::removeDynamicProperty(const std::string& name)
{
    auto &index = props.get<0>();
    auto it = index.find(name);
    if (it != index.end()) {
        if((*it)->property->testStatus(PropertyStatus::LockDynamic))
            throw Base::RuntimeError("property is locked");
        else if(!(*it)->property->testStatus(PropertyStatus::Prop_Dynamic))
            throw Base::RuntimeError("property is not dynamic");
        Property *prop = (*it)->property;
        GetApplication().signalRemoveDynamicProperty(*prop);
        Property::destroy(prop);
        index.erase(it);
        return true;
    }

    return false;
}

std::string DynamicProperty::getUniquePropertyName(PropertyContainer &pc, const std::string& Name) const
{
    std::string CleanName = Base::Tools::getIdentifier(Name);

    // name in use?
    std::map<std::string,Property*> objectProps;
    pc.getPropertyMap(objectProps);
    auto pos = objectProps.find(CleanName);

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

void DynamicProperty::save(const Property *prop, Base::Writer &writer) const
{
    auto &index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if(it != index.end()) {
        writer.Stream() << "\" group=\"" << Base::Persistence::encodeAttribute(prop->getGroup())
                        << "\" doc=\"" << Base::Persistence::encodeAttribute(prop->getDocumentation())
                        << "\" ro=\"" << prop->testStatus(Prop_ReadOnly)
                        << "\" hide=\"" << prop->testStatus(Prop_Hidden);
    }
}

Property *DynamicProperty::restore(PropertyContainer &pc,
        const char *PropName, const char *TypeName, Base::XMLReader &reader)
{
    if (!reader.hasAttribute("group"))
        return 0;

    StatusCollection<App::LegacyPropertyStatus::Value> attribute ;
    bool readonly = false, hidden = false;
    std::string group, doc;
    const char *attr=0, *ro=0, *hide=0;
    group = reader.getAttribute("group");
    if (reader.hasAttribute("doc"))
        doc = reader.getAttribute("doc");
    if (reader.hasAttribute("attr")) {
        attr = reader.getAttribute("attr");
        if (attr) attribute = StatusCollection<App::LegacyPropertyStatus::Value> (attr[0]-48);
    }
    if (reader.hasAttribute("ro")) {
        ro = reader.getAttribute("ro");
        if (ro) readonly = (ro[0]-48) != 0;
    }
    if (reader.hasAttribute("hide")) {
        hide = reader.getAttribute("hide");
        if (hide) hidden = (hide[0]-48) != 0;
    }
    Property * prop = addDynamicProperty(pc,TypeName, PropName, group, doc);
    if (prop) {
        prop->setStatus(App::fromLegacyAttributes(attribute));
        prop->setStatus(App::PropertyStatus::Prop_ReadOnly, readonly);
        prop->setStatus(App::PropertyStatus::Prop_Hidden, hidden);
    }
    return prop;
}
