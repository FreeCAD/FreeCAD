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
#include <map>
#include <vector>
#include <string>
#endif

#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/UniqueNameManager.h>
#include <Base/Writer.h>

#include "DynamicProperty.h"
#include "Application.h"
#include "Property.h"
#include "PropertyContainer.h"


FC_LOG_LEVEL_INIT("Property", true, true)


using namespace App;


DynamicProperty::DynamicProperty() = default;

DynamicProperty::~DynamicProperty()
{
    clear();
}

void DynamicProperty::clear()
{
    auto& index = props.get<0>();
    for (auto& v : index) {
        delete v.property;
    }
    index.clear();
}

void DynamicProperty::getPropertyList(std::vector<Property*>& List) const
{
    for (auto& v : props.get<0>()) {
        List.push_back(v.property);
    }
}

void DynamicProperty::getPropertyNamedList(
    std::vector<std::pair<const char*, Property*>>& List) const
{
    for (auto& v : props.get<0>()) {
        List.emplace_back(v.getName(), v.property);
    }
}

void DynamicProperty::visitProperties(const std::function<void(Property*)>& visitor) const {
    for (auto& v : props.get<0>()) {
        visitor(v.property);
    }
}

void DynamicProperty::getPropertyMap(std::map<std::string,Property*>& Map) const
{
    for (auto& v : props.get<0>()) {
        Map[v.name] = v.property;
    }
}

Property* DynamicProperty::getDynamicPropertyByName(const char* name) const
{
    auto& index = props.get<0>();
    auto it = index.find(name);
    if (it != index.end()) {
        return it->property;
    }
    return nullptr;
}

std::vector<std::string> DynamicProperty::getDynamicPropertyNames() const
{
    std::vector<std::string> names;
    auto& index = props.get<0>();
    names.reserve(index.size());
    for (auto& v : index) {
        names.push_back(v.name);
    }
    return names;
}

short DynamicProperty::getPropertyType(const Property* prop) const
{
    return prop ? prop->getType() : 0;
}

short DynamicProperty::getPropertyType(const char* name) const
{
    auto& index = props.get<0>();
    auto it = index.find(name);
    if (it != index.end()) {
        short attr = it->attr;
        if (it->hidden) {
            attr |= Prop_Hidden;
        }
        if (it->readonly) {
            attr |= Prop_ReadOnly;
        }
        return attr;
    }
    return 0;
}

const char* DynamicProperty::getPropertyGroup(const Property* prop) const
{
    auto& index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if (it != index.end()) {
        return it->group.c_str();
    }
    return nullptr;
}

const char* DynamicProperty::getPropertyGroup(const char* name) const
{
    auto& index = props.get<0>();
    auto it = index.find(name);
    if (it != index.end()) {
        return it->group.c_str();
    }
    return nullptr;
}

const char* DynamicProperty::getPropertyDocumentation(const Property* prop) const
{
    auto& index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if (it != index.end()) {
        return it->doc.c_str();
    }
    return nullptr;
}

const char* DynamicProperty::getPropertyDocumentation(const char* name) const
{
    auto& index = props.get<0>();
    auto it = index.find(name);
    if (it != index.end()) {
        return it->doc.c_str();
    }
    return nullptr;
}

Property* DynamicProperty::addDynamicProperty(PropertyContainer& pc,
                                              const char* type,
                                              const char* name,
                                              const char* group,
                                              const char* doc,
                                              short attr,
                                              bool ro,
                                              bool hidden)
{
    if (!type) {
        type = "<null>";
    }

    std::string _name;

    static ParameterGrp::handle hGrp =
        GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");
    if (hGrp->GetBool("AutoNameDynamicProperty", false)) {
        if (!name || !name[0]) {
            name = type;
        }
        _name = getUniquePropertyName(pc, name);
        if (_name != name) {
            FC_WARN(pc.getFullName()
                    << " rename dynamic property from '" << name << "' to '" << _name << "'");
        }
        name = _name.c_str();
    }
    else if (!name) {
        name = "<null>";  // setting a bad name to trigger exception
    }

    auto prop = pc.getPropertyByName(name);
    if (prop && prop->getContainer() == &pc) {
        FC_THROWM(Base::NameError,
                  "Property " << pc.getFullName() << '.' << name << " already exists");
    }

    if (Base::Tools::getIdentifier(name) != name) {
        FC_THROWM(Base::NameError, "Invalid property name '" << name << "'");
    }

    Base::Type propType =
        Base::Type::getTypeIfDerivedFrom(type, App::Property::getClassTypeId(), true);
    if (propType.isBad()) {
        FC_THROWM(Base::TypeError,
                  "Invalid type " << type << " for property " << pc.getFullName() << '.' << name);
    }

    void* propInstance = propType.createInstance();
    if (!propInstance) {
        FC_THROWM(Base::RuntimeError,
                  "Failed to create property " << pc.getFullName() << '.' << name << " of type "
                                               << type);
    }

    Property* pcProperty = static_cast<Property*>(propInstance);

    auto res = props.get<0>().emplace(pcProperty, name, nullptr, group, doc, attr, ro, hidden);

    pcProperty->setContainer(&pc);
    pcProperty->myName = res.first->name.c_str();

    if (ro) {
        attr |= Prop_ReadOnly;
    }
    if (hidden) {
        attr |= Prop_Hidden;
    }

    pcProperty->syncType(attr);
    pcProperty->StatusBits.set((size_t)Property::PropDynamic);

    GetApplication().signalAppendDynamicProperty(*pcProperty);

    return pcProperty;
}

bool DynamicProperty::addProperty(Property* prop)
{
    if (!prop || !prop->hasName()) {
        return false;
    }
    auto& index = props.get<0>();
#if BOOST_VERSION < 107500
    if (index.count(prop->getName())) {
        return false;
    }
#else
    if (index.contains(prop->getName())) {
        return false;
    }
#endif
    index.emplace(prop,
                  std::string(),
                  prop->getName(),
                  prop->getGroup(),
                  prop->getDocumentation(),
                  prop->getType(),
                  false,
                  false);
    return true;
}

bool DynamicProperty::removeProperty(const Property* prop)
{
    auto& index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if (it != index.end()) {
        index.erase(it);
        return true;
    }
    return false;
}

bool DynamicProperty::removeDynamicProperty(const char* name)
{
    auto& index = props.get<0>();
    auto it = index.find(name);
    if (it != index.end()) {
        if (it->property->testStatus(Property::LockDynamic)) {
            throw Base::RuntimeError("property is locked");
        }
        else if (!it->property->testStatus(Property::PropDynamic)) {
            throw Base::RuntimeError("property is not dynamic");
        }
        Property* prop = it->property;
        GetApplication().signalRemoveDynamicProperty(*prop);

        // Handle possible recursive calls of removeDynamicProperty
        if (prop->myName) {
            Property::destroy(prop);
            index.erase(it);
            // memory of myName has been freed
            prop->myName = nullptr;
        }
        return true;
    }

    return false;
}

std::string DynamicProperty::getUniquePropertyName(const PropertyContainer& pc, const char* Name) const
{
    std::string cleanName = Base::Tools::getIdentifier(Name);

    // We test if the property already exists by finding it, which is not much more expensive than
    // having a separate propertyExists(name) method. This avoids building the UniqueNameManager
    // (which could also tell if the name exists) except in the relatively rare condition of
    // the name already existing.
    if (pc.getPropertyByName(cleanName.c_str()) == nullptr) {
        return cleanName;
    }
    Base::UniqueNameManager names;
    // Build the index of existing names.
    pc.visitProperties([&](Property* prop) {
        names.addExactName(prop->getName());
    });
    return names.makeUniqueName(cleanName);
}

void DynamicProperty::save(const Property* prop, Base::Writer& writer) const
{
    auto& index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if (it != index.end()) {
        auto& data = *it;
        writer.Stream() << "\" group=\"" << Base::Persistence::encodeAttribute(data.group)
                        << "\" doc=\"" << Base::Persistence::encodeAttribute(data.doc)
                        << "\" attr=\"" << data.attr << "\" ro=\"" << data.readonly << "\" hide=\""
                        << data.hidden;
    }
}

Property* DynamicProperty::restore(PropertyContainer& pc,
                                   const char* PropName,
                                   const char* TypeName,
                                   const Base::XMLReader& reader)
{
    if (!reader.hasAttribute("group")) {
        return nullptr;
    }

    short attribute = 0;
    bool readonly = false, hidden = false;
    const char *group = nullptr, *doc = nullptr, *attr = nullptr, *ro = nullptr, *hide = nullptr;
    group = reader.getAttribute<const char*>("group");
    if (reader.hasAttribute("doc")) {
        doc = reader.getAttribute<const char*>("doc");
    }
    if (reader.hasAttribute("attr")) {
        attr = reader.getAttribute<const char*>("attr");
        if (attr) {
            std::istringstream str(attr);
            str >> attribute;
        }
    }
    if (reader.hasAttribute("ro")) {
        ro = reader.getAttribute<const char*>("ro");
        if (ro) {
            readonly = (ro[0] - 48) != 0;
        }
    }
    if (reader.hasAttribute("hide")) {
        hide = reader.getAttribute<const char*>("hide");
        if (hide) {
            hidden = (hide[0] - 48) != 0;
        }
    }

    return addDynamicProperty(pc, TypeName, PropName, group, doc, attribute, readonly, hidden);
}

DynamicProperty::PropData DynamicProperty::getDynamicPropertyData(const Property* prop) const
{
    auto& index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if (it != index.end()) {
        return *it;
    }
    return {};
}

bool DynamicProperty::changeDynamicProperty(const Property* prop,
                                            const char* group,
                                            const char* doc)
{
    auto& index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if (it == index.end()) {
        return false;
    }
    if (group) {
        it->group = group;
    }
    if (doc) {
        it->doc = doc;
    }
    return true;
}

bool DynamicProperty::renameDynamicProperty(Property* prop,
                                            const char* newName)
{
    auto& propIndex = props.get<1>();
    auto propIt = propIndex.find(prop);
    if (propIt == propIndex.end()) {
        return false;
    }
    const PropData& data = *propIt;

    if (propIt->property->testStatus(Property::LockDynamic)) {
        FC_THROWM(Base::RuntimeError, "Property " << prop->getName() << " is locked");
    }

    PropertyContainer* container = prop->getContainer();
    if (container->getPropertyByName(newName) != nullptr) {
        FC_THROWM(Base::NameError,
                  "Property " << container->getFullName() << '.' << newName << " already exists");
    }

    if (Base::Tools::getIdentifier(newName) != newName) {
        FC_THROWM(Base::NameError, "Invalid property name '" << newName << "'");
    }

    std::string oldName{data.getName()};
    auto& nameIndex = props.get<0>();
    auto nameIt = nameIndex.find(data.getName());
    if (nameIt == nameIndex.end()) {
        // This should never happen
        FC_THROWM(Base::RuntimeError,
                  "Property " << data.getName() << " not found in index");
    }
    nameIndex.modify(nameIt, [&](PropData& d) {
        d.name = newName;
        d.pName = nullptr;
        // make sure that the property's name points to PropData.name that
        // manages the memory.
        d.property->myName = d.name.c_str();
    });

    GetApplication().signalRenameDynamicProperty(*prop, oldName.c_str());

    return true;
}

const char* DynamicProperty::getPropertyName(const Property* prop) const
{
    auto& index = props.get<1>();
    auto it = index.find(const_cast<Property*>(prop));
    if (it != index.end()) {
        return it->getName();
    }
    return nullptr;
}
