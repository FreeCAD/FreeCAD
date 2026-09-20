// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/


#include <algorithm>
#include <format>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <FCConfig.h>

#ifdef FC_OS_LINUX
# include <unistd.h>
#endif

#include <boost/algorithm/string.hpp>

#include "Parameter.h"
#include "ParameterSchema.h"
#include "Console.h"
#include "Exception.h"
#include "FileInfo.h"
#include "XMLParser.h"
#include "FileLock.h"
#include "Tools.h"

FC_LOG_LEVEL_INIT("Parameter", true, true)

using namespace Base;
namespace
{
constexpr int BASE = 10;
constexpr const char* PARAMETERS_TAG = "FCParameters";
constexpr const char* PARAM_GROUP_TAG = "FCParamGroup";
constexpr const char* BOOL_TAG = "FCBool";
constexpr const char* INT_TAG = "FCInt";
constexpr const char* UINT_TAG = "FCUInt";
constexpr const char* TEXT_TAG = "FCText";
constexpr const char* FLOAT_TAG = "FCFloat";

template<typename Value>
void sortByName(std::vector<std::pair<std::string, Value>>& values)
{
    std::sort(values.begin(), values.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
    });
}
}  // namespace

ParameterGrp::ParameterGrp()
    : groupName("Root")
{}

ParameterGrp::ParameterGrp(Base::XMLElement& GroupNode, const std::string& name, ParameterGrp& Parent)
    : groupNode(&GroupNode)
    , parent(&Parent)
    , groupName(name)
    , manager(Parent.manager)
{
    assert(manager != nullptr);
    GroupNode.attrs["Name"] = name;
}

ParameterGrp::~ParameterGrp()
{
    // Cached groups may outlive the XML document and their manager.
    RebindGroupNodes(nullptr);
}

const char* ParameterGrp::GetGroupName() const
{
    return this->groupName.c_str();
}


void ParameterGrp::copyTo(const Base::Reference<ParameterGrp>& Group)
{
    if (Group == this) {
        return;
    }

    // delete previous content
    Group->Clear(true);

    // copy all
    insertTo(Group);
}

void ParameterGrp::insertTo(const Base::Reference<ParameterGrp>& Group)
{
    if (Group == this) {
        return;
    }
    for (const auto& group : GetGroups()) {
        group->insertTo(Group->GetGroup(group->groupName.c_str()));
    }
    for (const auto& it : GetASCIIMap()) {
        Group->SetASCII(it.first.c_str(), it.second);
    }
    for (const auto& it : GetBoolMap()) {
        Group->SetBool(it.first.c_str(), it.second);
    }
    for (const auto& it : GetIntMap()) {
        Group->SetInt(it.first.c_str(), it.second);
    }
    for (const auto& it : GetFloatMap()) {
        Group->SetFloat(it.first.c_str(), it.second);
    }
    for (const auto& it : GetUnsignedMap()) {
        Group->SetUnsigned(it.first.c_str(), it.second);
    }
}

void ParameterGrp::exportTo(const char* FileName)
{
    auto Mngr = ParameterManager::Create();

    Mngr->CreateDocument();

    // copy all into the new document
    insertTo(Base::Reference<ParameterGrp>(Mngr));

    Mngr->SaveDocument(FileName);
}

void ParameterGrp::insert(const char* FileName)
{
    auto Mngr = ParameterManager::Create();

    if (Mngr->LoadDocument(FileName) != 1) {
        throw FileException("ParameterGrp::insert() cannot load document", FileName);
    }

    Mngr->insertTo(Base::Reference<ParameterGrp>(this));
}

void ParameterGrp::importFrom(const char* FileName)
{
    auto Mngr = ParameterManager::Create();

    if (Mngr->LoadDocument(FileName) != 1) {
        throw FileException("ParameterGrp::import() cannot load document", FileName);
    }

    Mngr->copyTo(Base::Reference<ParameterGrp>(this));
}

void ParameterGrp::revert(const Base::Reference<ParameterGrp>& Group)
{
    if (Group == this) {
        return;
    }

    for (auto& grp : Group->GetGroups()) {
        if (HasGroup(grp->groupName.c_str())) {
            GetGroup(grp->groupName.c_str())->revert(grp);
        }
    }

    for (const auto& v : Group->GetASCIIMap()) {
        if (GetASCII(v.first.c_str(), v.second.c_str()) == v.second) {
            RemoveASCII(v.first.c_str());
        }
    }

    for (const auto& v : Group->GetBoolMap()) {
        if (GetBool(v.first.c_str(), v.second) == v.second) {
            RemoveBool(v.first.c_str());
        }
    }

    for (const auto& v : Group->GetIntMap()) {
        if (GetInt(v.first.c_str(), v.second) == v.second) {
            RemoveInt(v.first.c_str());
        }
    }

    for (const auto& v : Group->GetUnsignedMap()) {
        if (GetUnsigned(v.first.c_str(), v.second) == v.second) {
            RemoveUnsigned(v.first.c_str());
        }
    }

    for (const auto& v : Group->GetFloatMap()) {
        if (GetFloat(v.first.c_str(), v.second) == v.second) {
            RemoveFloat(v.first.c_str());
        }
    }
}

const Base::XMLElement* ParameterGrp::GetRootNode() const
{
    if (this->detached && this->parent) {
        this->parent->GetGroup(this->groupName.c_str());  // Recreate the group
    }
    return this->groupNode;
}

Base::XMLElement* ParameterGrp::GetRootNode()
{
    return const_cast<Base::XMLElement*>(const_cast<const ParameterGrp*>(this)->GetRootNode());
}

void ParameterGrp::RebindGroupNodes(Base::XMLElement* root)
{
    for (auto& [name, group] : groupMap) {
        Base::XMLElement* node = nullptr;
        if (root) {
            for (const auto& child : root->children) {
                if (child->tag == PARAM_GROUP_TAG && child->attrs.contains("Name")
                    && child->attrs.at("Name") == name) {
                    node = child.get();
                    break;
                }
            }
        }

        group->groupNode = node;
        group->detached = node == nullptr;
        group->RebindGroupNodes(node);
        if (!root) {
            group->parent = nullptr;
            group->manager = nullptr;
        }
    }
}

Base::XMLElement* ParameterGrp::CreateElement(
    Base::XMLElement* start,
    const std::string& Type,
    const std::string& Name
)
{
    if (start->tag != PARAM_GROUP_TAG && start->tag != PARAMETERS_TAG) {
        throw Base::TypeError(
            std::format("CreateElement: {} cannot have the element {} of type {}\n", start->tag, Name, Type)
        );
    }

    auto newElement = std::make_unique<XMLElement>();
    newElement->tag = Type;
    newElement->attrs = {{"Name", Name}};
    start->children.emplace_back(std::move(newElement));
    return start->children.back().get();
}

Base::Reference<ParameterGrp> ParameterGrp::GetGroup(const char* Name)
{
    if (!Name) {
        throw Base::ValueError("Empty group name");
    }
    std::string_view NameView(Name);
    if (NameView.empty()) {
        throw Base::ValueError("Empty group name");
    }

    Base::Reference<ParameterGrp> hGrp = this;
    std::string_view path = NameView;
    while (!path.empty()) {
        const auto pos = path.find('/');
        auto token = path.substr(0, pos);

        // trim
        const auto first = token.find_first_not_of(" \t\n\r");
        const auto last = token.find_last_not_of(" \t\n\r");

        if (first != std::string_view::npos) {
            token = token.substr(first, last - first + 1);
            auto nextGroup = hGrp->GetOrCreateGroup(std::string(token));
            if (!nextGroup.isValid()) {
                return {};
            }
            hGrp = nextGroup;
        }

        if (pos == std::string_view::npos) {
            break;
        }

        path.remove_prefix(pos + 1);
    }

    if (hGrp == this) {
        throw Base::ValueError("Empty group name");
    }
    return hGrp;
}

Base::Reference<ParameterGrp> ParameterGrp::GetOrCreateGroup(const std::string& Name)
{
    if (clearing || !GetRootNode()) {
        return {};
    }

    if (groupMap.contains(Name) && groupMap[Name].isValid() && !groupMap[Name]->detached) {
        return groupMap[Name];
    }

    Base::XMLElement* pcTemp = FindElement(GetRootNode(), PARAM_GROUP_TAG, Name);
    const bool created = !pcTemp;
    if (!pcTemp) {
        Base::XMLElement* newGroup = CreateElement(GetRootNode(), PARAM_GROUP_TAG, Name);
        pcTemp = newGroup;
    }

    // Reattach group
    if (groupMap.contains(Name) && groupMap[Name]->detached) {
        groupMap[Name]->groupNode = pcTemp;
        groupMap[Name]->detached = false;
    }

    // create and register handle
    if (!groupMap.contains(Name) || !groupMap[Name].isValid()) {
        auto rParamGrp = Base::Reference<ParameterGrp>(new ParameterGrp(*pcTemp, Name, *this));
        groupMap[Name] = rParamGrp;
    }

    if (created && !detached) {
        NotifyChange(ParamType::FCGroup, Name, Name);
    }

    return groupMap[Name];
}

std::string ParameterGrp::GetPath() const
{
    std::string path;
    if (parent && parent != manager) {
        path = parent->GetPath();
    }
    if (!path.empty() && !groupName.empty()) {
        path += "/";
    }
    path += groupName;
    return path;
}


std::vector<Base::Reference<ParameterGrp>> ParameterGrp::GetGroups()
{
    std::vector<Base::Reference<ParameterGrp>> vrParamGrp;
    auto groups = FindAllElements(GetRootNode(), PARAM_GROUP_TAG);
    vrParamGrp.reserve(groups.size());
    for (const auto& it : groups) {
        vrParamGrp.push_back(GetOrCreateGroup(it->attrs["Name"]));
    }

    return vrParamGrp;
}

/// test if this group is empty
bool ParameterGrp::IsEmpty() const
{
    const auto* root = GetRootNode();
    return !root || root->children.empty();
}

/// test if a special sub group is in this group
bool ParameterGrp::HasGroup(const char* Name) const
{
    const auto it = groupMap.find(Name);
    if (it != groupMap.end() && it->second.isValid()) {
        return true;
    }

    return FindElement(GetRootNode(), PARAM_GROUP_TAG, Name);
}

const char* ParameterGrp::TypeName(ParamType Type)
{
    switch (Type) {
        case ParamType::FCBool:
            return BOOL_TAG;
        case ParamType::FCInt:
            return INT_TAG;
        case ParamType::FCUInt:
            return UINT_TAG;
        case ParamType::FCText:
            return TEXT_TAG;
        case ParamType::FCFloat:
            return FLOAT_TAG;
        case ParamType::FCGroup:
            return PARAM_GROUP_TAG;
        default:
            return nullptr;
    }
}

ParameterGrp::ParamType ParameterGrp::TypeValue(const char* Name)
{
    if (Name) {
        if (boost::equals(Name, BOOL_TAG)) {
            return ParamType::FCBool;
        }
        if (boost::equals(Name, INT_TAG)) {
            return ParamType::FCInt;
        }
        if (boost::equals(Name, UINT_TAG)) {
            return ParamType::FCUInt;
        }
        if (boost::equals(Name, TEXT_TAG)) {
            return ParamType::FCText;
        }
        if (boost::equals(Name, FLOAT_TAG)) {
            return ParamType::FCFloat;
        }
        if (boost::equals(Name, PARAM_GROUP_TAG)) {
            return ParamType::FCGroup;
        }
    }
    return ParamType::FCInvalid;
}

void ParameterGrp::SetAttribute(ParamType Type, const char* Name, const char* Value)
{
    switch (Type) {
        case ParamType::FCBool:
        case ParamType::FCInt:
        case ParamType::FCUInt:
        case ParamType::FCFloat:
            SetAttributeInternal(Type, Name, Value);
            break;
        case ParamType::FCText:
            SetASCII(Name, Value);
            break;
        case ParamType::FCGroup:
            RenameGrp(Name, Value);
            break;
        default:
            break;
    }
}

const char* ParameterGrp::GetAttribute(
    ParamType Type,
    const char* Name,
    std::string& Value,
    const char* Default
) const
{
    const char* T = TypeName(Type);
    if (!T) {
        Value = Default ? Default : "";
        return Value.c_str();
    }

    auto pcElem = FindElement(GetRootNode(), T, Name);
    if (!pcElem) {
        Value = Default ? Default : "";
        return Value.c_str();
    }

    if (Type == ParamType::FCText) {
        Value = GetASCII(Name, Default);
    }
    else if (Type != ParamType::FCGroup) {
        Value = pcElem->attrs.at("Value");
    }
    return Value.c_str();
}

std::vector<std::pair<std::string, std::string>> ParameterGrp::GetAttributeMap(
    ParamType Type,
    const char* sFilter
) const
{
    std::vector<std::pair<std::string, std::string>> res;

    const char* typeName = TypeName(Type);
    if (!typeName) {
        return res;
    }

    std::string Name;

    for (const auto& element : FindAllElements(GetRootNode(), typeName)) {
        Name = element->attrs["Name"];
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
            if (Type == ParamType::FCGroup) {
                res.emplace_back(Name, "");
            }
            else if (Type == ParamType::FCText) {
                res.emplace_back(Name, GetASCII(Name.c_str()));
            }
            else {
                res.emplace_back(Name, element->attrs["Value"]);
            }
        }
    }
    sortByName(res);
    return res;
}

void ParameterGrp::RemoveAttribute(ParamType Type, const char* Name)
{
    switch (Type) {
        case ParamType::FCBool:
            RemoveBool(Name);
            break;
        case ParamType::FCInt:
            RemoveInt(Name);
            break;
        case ParamType::FCUInt:
            RemoveUnsigned(Name);
            break;
        case ParamType::FCText:
            RemoveASCII(Name);
            break;
        case ParamType::FCFloat:
            RemoveFloat(Name);
            break;
        case ParamType::FCGroup:
            RemoveGrp(Name);
            break;
        default:
            break;
    }
}

void ParameterGrp::NotifyChange(ParamType Type, const std::string& Name, const std::string& Value)
{
    this->Manager()->signalParamChanged(this, Type, Name.c_str(), Value.c_str());
}

void ParameterGrp::SetAttributeInternal(ParamType T, const std::string& Name, const std::string& Value)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    const char* Type = TypeName(T);
    if (!Type) {
        return;
    }

    Base::XMLElement* pcElem = FindOrCreateElement(GetRootNode(), Type, Name);
    if (!pcElem->attrs.contains("Value") || pcElem->attrs.at("Value") != Value) {
        {
            pcElem->attrs["Value"] = Value;
            NotifyChange(T, Name, Value);
        }
    }

    // For backward compatibility, old observer gets notified regardless of
    // value changes or not.
    Notify(Name.c_str());
}

bool ParameterGrp::GetBool(const char* Name, bool bPreset) const
{
    const Base::XMLElement* pcElem = FindElement(GetRootNode(), BOOL_TAG, Name);
    return pcElem ? pcElem->attrs.at("Value") == "1" : bPreset;
}

void ParameterGrp::SetBool(const char* Name, bool bValue)
{
    SetAttributeInternal(ParamType::FCBool, Name, bValue ? "1" : "0");
}

std::vector<bool> ParameterGrp::GetBools(const char* sFilter) const
{
    std::vector<bool> vrValues;
    for (const auto& group : FindAllElements(GetRootNode(), BOOL_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.push_back(group->attrs.at("Value") == "1");
        }
    }
    return vrValues;
}

std::vector<std::pair<std::string, bool>> ParameterGrp::GetBoolMap(const char* sFilter) const
{
    std::vector<std::pair<std::string, bool>> vrValues;
    for (const auto& group : FindAllElements(GetRootNode(), BOOL_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(name, group->attrs.at("Value") == "1");
        }
    }

    sortByName(vrValues);
    return vrValues;
}

long ParameterGrp::GetInt(const char* Name, long lPreset) const
{
    const Base::XMLElement* pcElem = FindElement(GetRootNode(), INT_TAG, Name);
    return pcElem ? atol(pcElem->attrs.at("Value").c_str()) : lPreset;
}

void ParameterGrp::SetInt(const char* Name, long lValue)
{
    std::string buf = std::to_string(lValue);
    SetAttributeInternal(ParamType::FCInt, Name, buf);
}

std::vector<long> ParameterGrp::GetInts(const char* sFilter) const
{
    std::vector<long> vrValues;

    for (const auto& group : FindAllElements(GetRootNode(), INT_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(atol(group->attrs.at("Value").c_str()));
        }
    }

    return vrValues;
}

std::vector<std::pair<std::string, long>> ParameterGrp::GetIntMap(const char* sFilter) const
{
    std::vector<std::pair<std::string, long>> vrValues;

    for (const auto& group : FindAllElements(GetRootNode(), INT_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(name, atol(group->attrs.at("Value").c_str()));
        }
    }

    sortByName(vrValues);
    return vrValues;
}

unsigned long ParameterGrp::GetUnsigned(const char* Name, unsigned long lPreset) const
{
    const Base::XMLElement* pcElem = FindElement(GetRootNode(), UINT_TAG, Name);
    return pcElem ? strtoul(pcElem->attrs.at("Value").c_str(), nullptr, ::BASE) : lPreset;
}

void ParameterGrp::SetUnsigned(const char* Name, unsigned long lValue)
{
    std::string buf = std::to_string(lValue);
    SetAttributeInternal(ParamType::FCUInt, Name, buf);
}

std::vector<unsigned long> ParameterGrp::GetUnsigneds(const char* sFilter) const
{
    std::vector<unsigned long> vrValues;

    for (const auto& group : FindAllElements(GetRootNode(), UINT_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(strtoul(group->attrs.at("Value").c_str(), nullptr, ::BASE));
        }
    }

    return vrValues;
}

std::vector<std::pair<std::string, unsigned long>> ParameterGrp::GetUnsignedMap(const char* sFilter) const
{
    std::vector<std::pair<std::string, unsigned long>> vrValues;

    for (const auto& group : FindAllElements(GetRootNode(), UINT_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(name, strtoul(group->attrs.at("Value").c_str(), nullptr, ::BASE));
        }
    }

    sortByName(vrValues);
    return vrValues;
}

double ParameterGrp::GetFloat(const char* Name, double dPreset) const
{
    const Base::XMLElement* pcElem = FindElement(GetRootNode(), FLOAT_TAG, Name);
    return pcElem ? atof(pcElem->attrs.at("Value").c_str()) : dPreset;
}

void ParameterGrp::SetFloat(const char* Name, double dValue)
{
    // use 12 digits after the decimal point instead of the default 6
    // to handle values < 1.0e-6
    std::string buf = std::format("{:.12f}", dValue);
    SetAttributeInternal(ParamType::FCFloat, Name, buf);
}

std::vector<double> ParameterGrp::GetFloats(const char* sFilter) const
{
    std::vector<double> vrValues;

    for (const auto& group : FindAllElements(GetRootNode(), FLOAT_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(atof(group->attrs.at("Value").c_str()));
        }
    }

    return vrValues;
}

std::vector<std::pair<std::string, double>> ParameterGrp::GetFloatMap(const char* sFilter) const
{
    std::vector<std::pair<std::string, double>> vrValues;

    for (const auto& group : FindAllElements(GetRootNode(), FLOAT_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(name, atof(group->attrs.at("Value").c_str()));
        }
    }

    sortByName(vrValues);
    return vrValues;
}


void ParameterGrp::SetASCII(const char* Name, const char* sValue)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    Base::XMLElement* pcElem = FindElement(GetRootNode(), TEXT_TAG, Name);
    if (!pcElem) {
        pcElem = CreateElement(GetRootNode(), TEXT_TAG, Name);
    }
    if (pcElem->content != sValue) {
        pcElem->content = sValue;
        NotifyChange(ParamType::FCText, Name, sValue);
        Notify(Name);
    }
}

std::string ParameterGrp::GetASCII(const char* Name, const char* pPreset) const
{
    const Base::XMLElement* pcElem = FindElement(GetRootNode(), TEXT_TAG, Name);
    std::string preset = (pPreset ? std::string(pPreset) : "");  // Avoid conversion of nullptr to
                                                                 // std::string
    return pcElem ? pcElem->content : preset;
}

std::vector<std::string> ParameterGrp::GetASCIIs(const char* sFilter) const
{
    std::vector<std::string> vrValues;
    for (const auto& group : FindAllElements(GetRootNode(), TEXT_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(group->content);
        }
    }

    return vrValues;
}

std::vector<std::pair<std::string, std::string>> ParameterGrp::GetASCIIMap(const char* sFilter) const
{
    std::vector<std::pair<std::string, std::string>> vrValues;
    for (const auto& group : FindAllElements(GetRootNode(), TEXT_TAG)) {
        std::string name = group->attrs.at("Name");
        if (!sFilter || name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(name, group->content);
        }
    }
    sortByName(vrValues);
    return vrValues;
}

//**************************************************************************
// Access methods

void ParameterGrp::RemoveASCII(const char* Name)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    for (size_t i = 0; i < GetRootNode()->children.size(); i++) {
        if (GetRootNode()->children[i]->tag == TEXT_TAG
            && GetRootNode()->children[i]->attrs.at("Name") == Name) {
            GetRootNode()->children.erase(GetRootNode()->children.begin() + static_cast<int>(i));
            NotifyChange(ParamType::FCText, Name, "");
            Notify(Name);
            return;
        }
    }
}

void ParameterGrp::RemoveBool(const char* Name)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    for (size_t i = 0; i < GetRootNode()->children.size(); i++) {
        if (GetRootNode()->children[i]->tag == BOOL_TAG
            && GetRootNode()->children[i]->attrs.at("Name") == Name) {
            GetRootNode()->children.erase(GetRootNode()->children.begin() + static_cast<int>(i));
            NotifyChange(ParamType::FCBool, Name, "");
            Notify(Name);
            return;
        }
    }
}


void ParameterGrp::RemoveFloat(const char* Name)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    for (size_t i = 0; i < GetRootNode()->children.size(); i++) {
        if (GetRootNode()->children[i]->tag == FLOAT_TAG
            && GetRootNode()->children[i]->attrs.at("Name") == Name) {
            GetRootNode()->children.erase(GetRootNode()->children.begin() + static_cast<int>(i));
            NotifyChange(ParamType::FCFloat, Name, "");
            Notify(Name);
            return;
        }
    }
}

void ParameterGrp::RemoveInt(const char* Name)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    for (size_t i = 0; i < GetRootNode()->children.size(); i++) {
        if (GetRootNode()->children[i]->tag == INT_TAG
            && GetRootNode()->children[i]->attrs.at("Name") == Name) {
            GetRootNode()->children.erase(GetRootNode()->children.begin() + static_cast<int>(i));
            NotifyChange(ParamType::FCInt, Name, "");
            Notify(Name);
            return;
        }
    }
}

void ParameterGrp::RemoveUnsigned(const char* Name)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    for (size_t i = 0; i < GetRootNode()->children.size(); i++) {
        if (GetRootNode()->children[i]->tag == UINT_TAG
            && GetRootNode()->children[i]->attrs.at("Name") == Name) {
            GetRootNode()->children.erase(GetRootNode()->children.begin() + static_cast<int>(i));
            NotifyChange(ParamType::FCUInt, Name, "");
            Notify(Name);
            return;
        }
    }
}

Base::Color ParameterGrp::GetColor(const char* Name, Base::Color lPreset) const
{
    auto packed = GetUnsigned(Name, lPreset.getPackedValue());

    return Color(static_cast<uint32_t>(packed));
}

void ParameterGrp::SetColor(const char* Name, Base::Color lValue)
{
    SetUnsigned(Name, lValue.getPackedValue());
}

std::vector<Base::Color> ParameterGrp::GetColors(const char* sFilter) const
{
    auto packed = GetUnsigneds(sFilter);
    std::vector<Base::Color> result;

    std::ranges::transform(packed, std::back_inserter(result), [](const unsigned long lValue) {
        return Color(static_cast<uint32_t>(lValue));
    });

    return result;
}

std::vector<std::pair<std::string, Base::Color>> ParameterGrp::GetColorMap(const char* sFilter) const
{
    std::vector<std::pair<std::string, Base::Color>> result;
    for (const auto& color : GetUnsignedMap(sFilter)) {
        result.emplace_back(color.first, Color(static_cast<uint32_t>(color.second)));
    }
    sortByName(result);
    return result;
}

void ParameterGrp::RemoveColor(const char* Name)
{
    RemoveUnsigned(Name);
}

void ParameterGrp::RemoveGrp(const char* Name)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    auto it = groupMap.find(Name);
    if (it == groupMap.end()) {
        return;
    }
    Base::Reference<ParameterGrp> removedGroup = it->second;

    // If this or any of its children is referenced by an observer we do not
    // delete the handle, just in case the group is later added again, or else
    // those existing observer won't get any notification. BUT, we DO delete
    // the underlying xml elements, so that we don't save the empty group
    // later.
    removedGroup->Clear();
    removedGroup->detached = true;
    removedGroup->groupNode = nullptr;
    for (size_t i = 0; i < GetRootNode()->children.size(); i++) {
        if (GetRootNode()->children[i]->tag == PARAM_GROUP_TAG
            && GetRootNode()->children[i]->attrs.at("Name") == Name) {
            GetRootNode()->children.erase(GetRootNode()->children.begin() + static_cast<int>(i));
            break;
        }
    }

    // trigger observer
    Notify(Name);
}

bool ParameterGrp::RenameGrp(const char* OldName, const char* NewName)
{
    if (clearing || !GetRootNode()) {
        return false;
    }

    auto it = groupMap.find(OldName);
    if (it == groupMap.end()) {
        return false;
    }
    auto jt = groupMap.find(NewName);
    if (jt != groupMap.end()) {
        return false;
    }

    // rename group handle and attribute
    groupMap[NewName] = groupMap[OldName];
    groupMap.erase(OldName);
    groupMap[NewName]->groupName = NewName;
    groupMap[NewName]->GetRootNode()->attrs["Name"] = NewName;

    NotifyChange(ParamType::FCGroup, NewName, OldName);
    return true;
}

void ParameterGrp::Clear(bool notify)
{
    if (clearing || !GetRootNode()) {
        return;
    }

    Base::StateLocker guard(clearing);


    // early trigger notification of group removal when all its children
    // hierarchies are intact.
    NotifyChange(ParamType::FCGroup, "", "");

    // checking on references
    for (auto it = groupMap.begin(); it != groupMap.end();) {
        // If a group handle is referenced by some observer, then do not remove
        // it but clear it, so that any existing observer can still get
        // notification if the group is later on add back. We do remove the
        // underlying xml element from its parent so that we won't save this
        // empty group.
        it->second->Clear(notify);
        if (!it->second->detached) {
            it->second->detached = true;
            it->second->groupNode = nullptr;
            for (size_t i = 0; i < GetRootNode()->children.size(); i++) {
                if (GetRootNode()->children[i]->tag == PARAM_GROUP_TAG
                    && GetRootNode()->children[i]->attrs.at("Name") == it->second->groupName) {
                    GetRootNode()->children.erase(
                        GetRootNode()->children.begin() + static_cast<int>(i)
                    );
                    break;
                }
            }
        }
        if (!it->second->ShouldRemove()) {
            ++it;
        }
        else {
            it = groupMap.erase(it);
        }
    }

    // Remove the rest of non-group nodes;
    std::vector<std::pair<ParamType, std::string>> params;
    for (auto& child : GetRootNode()->children) {
        ParamType type = TypeValue(child->tag.c_str());
        if (type != ParamType::FCInvalid && type != ParamType::FCGroup) {
            params.emplace_back(type, child->attrs.at("Name"));
        }
    }

    GetRootNode()->children.clear();

    for (const auto& v : params) {
        NotifyChange(v.first, v.second, "");
        if (notify) {
            Notify(v.second.c_str());
        }
    }

    // trigger observer
    Notify("");
}

//**************************************************************************
// Access methods

bool ParameterGrp::ShouldRemove() const
{
    if (this->getRefCount() > 1) {
        return false;
    }

    return std::ranges::all_of(groupMap, [](const auto& it) { return it.second->ShouldRemove(); });
}

const Base::XMLElement* ParameterGrp::FindElement(
    const Base::XMLElement* start,
    const std::string& Type,
    const std::string& Name
) const
{
    if (!start) {
        return nullptr;
    }

    if (start->tag != PARAM_GROUP_TAG && start->tag != PARAMETERS_TAG) {
        Base::Console().warning(
            "FindElement: {} cannot have the element {} of type {}\n",
            start->tag,
            Name,
            Type
        );
        return nullptr;
    }

    for (const auto& childElem : start->children) {
        if (childElem->tag == Type
            && (Name.empty()
                || (childElem->attrs.contains("Name") && childElem->attrs.at("Name") == Name))) {
            return childElem.get();
        }
    }

    return nullptr;
}

Base::XMLElement* ParameterGrp::FindElement(
    const Base::XMLElement* Start,
    const std::string& Type,
    const std::string& Name
)
{
    return const_cast<Base::XMLElement*>(
        const_cast<const ParameterGrp*>(this)->FindElement(Start, Type, Name)
    );
}

std::vector<Base::XMLElement*> ParameterGrp::FindAllElements(
    const Base::XMLElement* start,
    const std::string& Type
) const
{
    std::vector<Base::XMLElement*> allElems;
    if (!start) {
        return allElems;
    }

    for (const auto& childElem : start->children) {
        if (childElem->tag == Type) {
            allElems.emplace_back(childElem.get());
        }
    }
    return allElems;
}

Base::XMLElement* ParameterGrp::FindOrCreateElement(
    Base::XMLElement* Start,
    const std::string& Type,
    const std::string& Name
)
{
    Base::XMLElement* pcElem = FindElement(Start, Type, Name);
    if (!pcElem) {
        pcElem = CreateElement(Start, Type, Name);
    }
    return pcElem ? pcElem : CreateElement(Start, Type, Name);
}


std::optional<std::string> ParameterGrp::FindAttribute(Base::XMLElement& Node, const std::string& Name) const
{
    if (Node.attrs.contains("Name")) {
        return Node.attrs[Name];
    }
    return std::nullopt;
}

std::vector<std::pair<ParameterGrp::ParamType, std::string>> ParameterGrp::GetParameterNames(
    const char* sFilter
) const
{
    std::vector<std::pair<ParameterGrp::ParamType, std::string>> res;
    const auto* root = GetRootNode();
    if (!root) {
        return res;
    }

    for (const auto& childElem : root->children) {
        const auto type = TypeValue(childElem->tag.c_str());
        if (type == ParamType::FCInvalid || type == ParamType::FCGroup) {
            continue;
        }

        const auto name = childElem->attrs.find("Name");
        if (name != childElem->attrs.end()
            && (!sFilter || name->second.find(sFilter) != std::string::npos)) {
            res.emplace_back(type, name->second);
        }
    }
    return res;
}

void ParameterGrp::NotifyAll()
{
    // get all ints and notify
    auto IntMap = GetIntMap();
    for (const auto& it : IntMap) {
        Notify(it.first.c_str());
    }

    // get all booleans and notify
    auto BoolMap = GetBoolMap();
    for (const auto& it : BoolMap) {
        Notify(it.first.c_str());
    }

    // get all Floats and notify
    auto FloatMap = GetFloatMap();
    for (const auto& it : FloatMap) {
        Notify(it.first.c_str());
    }

    // get all strings and notify
    auto StringMap = GetASCIIMap();
    for (const auto& it : StringMap) {
        Notify(it.first.c_str());
    }

    // get all uints and notify
    auto UIntMap = GetUnsignedMap();
    for (const auto& it : UIntMap) {
        Notify(it.first.c_str());
    }
}

/** Destruction
 * complete destruction of the object
 */
ParameterManager::~ParameterManager() = default;

ParameterManager::ParameterManager()
{
    manager = this;
}


Base::Reference<ParameterManager> ParameterManager::Create()
{
    auto mgr = new ParameterManager();
    mgr->CreateDocument();
    return mgr;
}

void ParameterManager::SetFileName(const std::string& name)
{
    fileName = name;
}

bool ParameterManager::HasFileName() const
{
    return fileName.has_value();
}


void ParameterManager::SetIgnoreSave(bool value)
{
    gIgnoreSave = value;
}

bool ParameterManager::IgnoreSave() const
{
    return gIgnoreSave;
}

namespace
{
std::string getLockFile(const Base::FileInfo& file)
{
    return Base::FileInfo::getTempPath() + file.fileName() + ".lock";
}

int getTimeout()
{
    const int timeout = 5000;
    return timeout;
}
}  // namespace

//**************************************************************************
// Document handling

bool ParameterManager::LoadOrCreateDocument(const char* sFileName)
{
    Base::FileInfo file(sFileName);
    if (file.exists()) {
        this->LoadDocument(sFileName);
        return false;
    }

    CreateDocument();
    return true;
}

int ParameterManager::LoadDocument(const char* sFileName)
{
    try {
        Base::FileInfo file(sFileName);
        Base::FileLock lock(getLockFile(file));
        if (!lock.tryLock(getTimeout())) {
            // Continue with empty config
            CreateDocument();
            SetIgnoreSave(true);
            std::cerr << "Failed to access file for reading: " << sFileName << '\n';
            return 1;
        }
        auto document = Base::ParseXMLFile(std::string(sFileName));
        if (!CheckDocument(*document)) {
            throw XMLBaseException("Malformed Parameter document: XSD validation failed");
        }

        auto rootIt = std::find_if(
            document->children.cbegin(),
            document->children.cend(),
            [](const auto& child) {
                const auto name = child->attrs.find("Name");
                return child->tag == PARAM_GROUP_TAG && name != child->attrs.end()
                    && name->second == "Root";
            }
        );
        if (rootIt == document->children.cend()) {
            throw XMLBaseException("Malformed Parameter document: Root group not found");
        }

        auto* root = rootIt->get();
        RebindGroupNodes(root);
        this->XMLDocument = std::move(document);
    }
    catch (const Base::Exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
    catch (...) {
        std::cerr << "An error occurred during parsing\n " << '\n';
        throw;
    }
    return 1;
}

void ParameterManager::SaveDocument(const char* sFileName) const
{
    try {
        Base::FileInfo file(sFileName);
        Base::FileLock lock(getLockFile(file));
        if (!lock.tryLock(getTimeout())) {
            std::cerr << "Failed to access file for writing: " << sFileName << "\n";
            return;
        }
        Base::SaveXMLFile(std::string(sFileName), *XMLDocument);
    }
    catch (XMLBaseException& e) {
        std::cerr << "An error occurred during creation of output transcoder. Msg is:" << '\n'
                  << e.getMessage() << '\n';
    }
}

const std::string& ParameterManager::GetFileName() const
{
    static const std::string _dummy;
    return fileName ? *fileName : _dummy;
}

int ParameterManager::LoadDocument()
{
    if (fileName) {
        return LoadDocument(fileName->c_str());
    }

    return -1;
}

bool ParameterManager::LoadOrCreateDocument()
{
    if (fileName) {
        return LoadOrCreateDocument(fileName->c_str());
    }

    return false;
}

void ParameterManager::SaveDocument() const
{
    if (fileName) {
        SaveDocument(fileName->c_str());
    }
}


void ParameterManager::CreateDocument()
{
    auto document = std::make_unique<XMLElement>();
    document->tag = PARAMETERS_TAG;
    auto root = std::make_unique<XMLElement>();
    root->tag = PARAM_GROUP_TAG;
    root->attrs["Name"] = "Root";
    document->children.emplace_back(std::move(root));

    RebindGroupNodes(document->children.front().get());
    XMLDocument = std::move(document);
}

bool ParameterManager::CheckDocument() const
{
    return CheckDocument(*XMLDocument);
}

bool ParameterManager::CheckDocument(const Base::XMLElement& document) const
{
    auto res = Base::CheckXMLDocument(document, ParameterSchema);
    if (res.has_value()) {
        std::string errors;
        for (const auto& error : res.value()) {
            if (!errors.empty()) {
                errors += '\n';
            }
            errors += error;
        }
<<<<<<< HEAD

        parser.setExternalNoNamespaceSchemaLocation("Parameter.xsd");
        // parser.setExitOnFirstFatalError(true);
        // parser.setValidationConstraintFatal(true);
        parser.cacheGrammarFromParse(true);
        parser.setValidationScheme(XercesDOMParser::Val_Auto);
        parser.setDoNamespaces(true);
        parser.setDoSchema(true);
        parser.setDisableDefaultEntityResolution(true);

        DOMTreeErrorReporter errHandler;
        parser.setErrorHandler(&errHandler);
        parser.parse(xmlFile);

        if (parser.getErrorCount() > 0) {
            Base::Console().error(
                "Unexpected XML structure detected: {} errors\n",
                parser.getErrorCount()
            );
            return false;
        }
    }
    catch (XMLException& e)
    {
        Base::Console().error(
            "An error occurred while checking document:{}\n",
            StrX(e.getMessage()).c_str()
        );
        == == ==
            = Base::Console().error("Parameter document verification failed: %s\n", errors.c_str());
>>>>>>> 2714d730ae (Parameter: use XMLParser abstraction)
        return false;
    }

    return true;
}

const Base::XMLElement* ParameterManager::GetRootNode() const
{
    if (!XMLDocument || XMLDocument->children.empty()) {
        throw Base::RuntimeError("Parameter document has not been created");
    }
    return XMLDocument->children.front().get();
}
