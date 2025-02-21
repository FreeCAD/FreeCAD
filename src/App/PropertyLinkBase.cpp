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
    #include <unordered_map>
#endif

#include <Base/Console.h>

#include "DocInfo.h"
#include "Document.h"
#include "DocumentObject.h"
#include "GeoFeature.h"
#include "ObjectIdentifier.h"
#include "PropertyLinkBase.h"

FC_LOG_LEVEL_INIT("PropertyLinkBase", true, true)

namespace App {

//**************************************************************************
//**************************************************************************
// PropertyLinkBase
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLinkBase, App::Property)

// clang-format off
static std::unordered_map<std::string, std::set<PropertyLinkBase*>> _LabelMap;
static std::unordered_map<App::DocumentObject*, std::unordered_set<PropertyLinkBase*>> _ElementRefMap;
// clang-format on

PropertyLinkBase::PropertyLinkBase() = default;

PropertyLinkBase::~PropertyLinkBase()
{
    unregisterLabelReferences();
    unregisterElementReference();
}

void PropertyLinkBase::setAllowExternal(bool allow)
{
    setFlag(LinkAllowExternal, allow);
}

void PropertyLinkBase::setSilentRestore(bool allow)
{
    setFlag(LinkSilentRestore, allow);
}

void PropertyLinkBase::setReturnNewElement(bool enable)
{
    setFlag(LinkNewElement, enable);
}

void PropertyLinkBase::hasSetValue()
{
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if (owner) {
        owner->clearOutListCache();
    }
    Property::hasSetValue();
}

bool PropertyLinkBase::isSame(const Property& other) const
{
    if (&other == this) {
        return true;
    }
    if (other.isDerivedFrom<PropertyLinkBase>()
        || getScope() != static_cast<const PropertyLinkBase*>(&other)->getScope()) {
        return false;
    }

    static std::vector<App::DocumentObject*> ret;
    static std::vector<std::string> subs;
    static std::vector<App::DocumentObject*> ret2;
    static std::vector<std::string> subs2;

    ret.clear();
    subs.clear();
    ret2.clear();
    subs2.clear();
    getLinks(ret, true, &subs, false);
    static_cast<const PropertyLinkBase*>(&other)->getLinks(ret2, true, &subs2, true);

    return ret == ret2 && subs == subs2;
}

void PropertyLinkBase::unregisterElementReference()
{
    for (auto obj : _ElementRefs) {
        auto it = _ElementRefMap.find(obj);
        if (it != _ElementRefMap.end()) {
            it->second.erase(this);
            if (it->second.empty()) {
                _ElementRefMap.erase(it);
            }
        }
    }
    _ElementRefs.clear();
}

void PropertyLinkBase::unregisterLabelReferences()
{
    for (auto& label : _LabelRefs) {
        auto it = _LabelMap.find(label);
        if (it != _LabelMap.end()) {
            it->second.erase(this);
            if (it->second.empty()) {
                _LabelMap.erase(it);
            }
        }
    }
    _LabelRefs.clear();
}

void PropertyLinkBase::getLabelReferences(std::vector<std::string>& subs, const char* subname)
{
    const char* dot;
    for (; (subname = strchr(subname, '$')) != nullptr; subname = dot + 1) {
        ++subname;
        dot = strchr(subname, '.');
        if (!dot) {
            break;
        }
        subs.emplace_back(subname, dot - subname);
    }
}

void PropertyLinkBase::registerLabelReferences(std::vector<std::string>&& labels, bool reset)
{
    if (reset) {
        unregisterLabelReferences();
    }
    for (auto& label : labels) {
        auto res = _LabelRefs.insert(std::move(label));
        if (res.second) {
            _LabelMap[*res.first].insert(this);
        }
    }
}

void PropertyLinkBase::checkLabelReferences(const std::vector<std::string>& subs, bool reset)
{
    if (reset) {
        unregisterLabelReferences();
    }
    std::vector<std::string> labels;
    for (auto& sub : subs) {
        labels.clear();
        getLabelReferences(labels, sub.c_str());
        registerLabelReferences(std::move(labels), false);
    }
}

std::string PropertyLinkBase::updateLabelReference(const App::DocumentObject* parent,
                                                   const char* subname,
                                                   App::DocumentObject* obj,
                                                   const std::string& ref,
                                                   const char* newLabel)
{
    if (!obj || !obj->isAttachedToDocument() || !parent || !parent->isAttachedToDocument()) {
        return {};
    }

    // Because the label is allowed to be the same across different
    // hierarchies, we have to search for all occurrences, and make sure the
    // referenced sub-object at the found hierarchy is actually the given
    // object.
    for (const char* pos = subname; ((pos = strstr(pos, ref.c_str())) != nullptr);
         pos += ref.size()) {
        auto sub = std::string(subname, pos + ref.size() - subname);
        auto sobj = parent->getSubObject(sub.c_str());
        if (sobj == obj) {
            sub = subname;
            sub.replace(pos + 1 - subname, ref.size() - 2, newLabel);
            return sub;
        }
    }
    return {};
}

std::vector<std::pair<Property*, std::unique_ptr<Property>>>
PropertyLinkBase::updateLabelReferences(App::DocumentObject* obj, const char* newLabel)
{
    std::vector<std::pair<Property*, std::unique_ptr<Property>>> ret;
    if (!obj || !obj->isAttachedToDocument()) {
        return ret;
    }
    auto it = _LabelMap.find(obj->Label.getStrValue());
    if (it == _LabelMap.end()) {
        return ret;
    }
    std::string ref("$");
    ref += obj->Label.getValue();
    ref += '.';
    std::vector<PropertyLinkBase*> props;
    props.reserve(it->second.size());
    props.insert(props.end(), it->second.begin(), it->second.end());
    for (auto prop : props) {
        if (!prop->getContainer()) {
            continue;
        }
        std::unique_ptr<Property> copy(prop->CopyOnLabelChange(obj, ref, newLabel));
        if (copy) {
            ret.emplace_back(prop, std::move(copy));
        }
    }
    return ret;
}

const std::unordered_set<PropertyLinkBase*>&
PropertyLinkBase::getElementReferences(DocumentObject* feature)
{
    static std::unordered_set<PropertyLinkBase*> none;

    auto it = _ElementRefMap.find(feature);
    if (it == _ElementRefMap.end()) {
        return none;
    }

    return it->second;
}

void PropertyLinkBase::updateElementReferences(DocumentObject* feature, bool reverse)
{
    if (!feature || !feature->getNameInDocument()) {
        return;
    }
    auto it = _ElementRefMap.find(feature);
    if (it == _ElementRefMap.end()) {
        return;
    }
    std::vector<PropertyLinkBase*> props;
    props.reserve(it->second.size());
    props.insert(props.end(), it->second.begin(), it->second.end());
    for (auto prop : props) {
        if (prop->getContainer()) {
            try {
                prop->updateElementReference(feature, reverse, true);
            }
            catch (Base::Exception& e) {
                e.ReportException();
                FC_ERR("Failed to update element reference of " << propertyName(prop));
            }
            catch (std::exception& e) {
                FC_ERR("Failed to update element reference of " << propertyName(prop) << ": "
                                                                << e.what());
            }
        }
    }
}

void PropertyLinkBase::_registerElementReference(App::DocumentObject* obj,
                                                 std::string& sub,
                                                 ShadowSub& shadow)
{
    if (!obj || !obj->getNameInDocument() || sub.empty()) {
        return;
    }
    if (shadow.newName.empty()) {
        _updateElementReference(nullptr, obj, sub, shadow, false);
        return;
    }
    GeoFeature* geo = nullptr;
    const char* element = nullptr;
    ShadowSub elementName;
    GeoFeature::resolveElement(obj,
                               sub.c_str(),
                               elementName,
                               true,
                               GeoFeature::ElementNameType::Export,
                               nullptr,
                               &element,
                               &geo);
    if (!geo || !element || !element[0]) {
        return;
    }

    if (_ElementRefs.insert(geo).second) {
        _ElementRefMap[geo].insert(this);
    }
}

void PropertyLinkBase::restoreLabelReference(const DocumentObject* obj,
                                             std::string& subname,
                                             ShadowSub* shadow)
{
    std::ostringstream ss;
    char* sub = &subname[0];
    char* next = sub;
    for (char* dot = strchr(next, '.'); dot; next = dot + 1, dot = strchr(next, '.')) {
        if (dot != next && dot[-1] != '@') {
            continue;
        }
        DocumentObject* sobj;
        try {
            StringGuard guard(dot - 1);
            sobj = obj->getSubObject(subname.c_str());
            if (!sobj) {
                FC_ERR("Failed to restore label reference " << obj->getFullName() << '.'
                                                            << ss.str());
                return;
            }
        }
        catch (...) {
            throw;
        }
        ss.write(sub, next - sub);
        ss << '$' << sobj->Label.getStrValue() << '.';
        sub = dot + 1;
    }
    if (sub == subname.c_str()) {
        return;
    }

    size_t count = sub - subname.c_str();
    const auto& newSub = ss.str();
    if (shadow && shadow->oldName.size() >= count) {
        shadow->oldName = newSub + (shadow->oldName.c_str() + count);
    }
    if (shadow && shadow->newName.size() >= count) {
        shadow->newName = newSub + (shadow->newName.c_str() + count);
    }
    subname = newSub + sub;
}

std::string PropertyLinkBase::propertyName(const Property* prop)
{
    if (!prop) {
        return {};
    }
    if (!prop->getContainer() || !prop->hasName()) {
        auto xlink = Base::freecad_dynamic_cast<const PropertyXLink>(prop);
        if (xlink) {
            return propertyName(xlink->parent());
        }
    }
    return prop->getFullName();
}

bool PropertyLinkBase::_updateElementReference(DocumentObject* feature,
                                               App::DocumentObject* obj,
                                               std::string& sub,
                                               ShadowSub& shadow,
                                               bool reverse,
                                               bool notify)
{
    if (!obj || !obj->getNameInDocument()) {
        return false;
    }
    ShadowSub elementName;
    const char* subname;
    if (shadow.newName.size()) {
        subname = shadow.newName.c_str();
    }
    else if (shadow.oldName.size()) {
        subname = shadow.oldName.c_str();
    }
    else {
        subname = sub.c_str();
    }
    GeoFeature* geo = nullptr;
    const char* element = nullptr;
    auto ret = GeoFeature::resolveElement(obj,
                                          subname,
                                          elementName,
                                          true,
                                          GeoFeature::ElementNameType::Export,
                                          feature,
                                          &element,
                                          &geo);
    if (!ret || !geo || !element || !element[0]) {
        if (elementName.oldName.size()) {
            shadow.oldName.swap(elementName.oldName);
        }
        return false;
    }

    if (_ElementRefs.insert(geo).second) {
        _ElementRefMap[geo].insert(this);
    }

    if (!reverse) {
        if (elementName.newName.empty()) {
            shadow.oldName.swap(elementName.oldName);
            return false;
        }
        if (shadow == elementName) {
            return false;
        }
    }

    bool missing = GeoFeature::hasMissingElement(elementName.oldName.c_str());
    if (feature == geo && (missing || reverse)) {
        // If the referenced element is missing, or we are generating element
        // map for the first time, or we are re-generating the element map due
        // to version change, i.e. 'reverse', try search by geometry first
        const char* oldElement = Data::findElementName(shadow.oldName.c_str());
        if (!Data::hasMissingElement(oldElement)) {
            const auto& names = geo->searchElementCache(oldElement);
            if (names.size()) {
                missing = false;
                std::string newsub(subname, strlen(subname) - strlen(element));
                newsub += names.front();
                GeoFeature::resolveElement(obj,
                                           newsub.c_str(),
                                           elementName,
                                           true,
                                           GeoFeature::ElementNameType::Export,
                                           feature);
                const auto& oldName = shadow.newName.size() ? shadow.newName : shadow.oldName;
                const auto& newName =
                    elementName.newName.size() ? elementName.newName : elementName.oldName;
                if (oldName != newName) {
                    FC_WARN(propertyName(this)
                            << " auto change element reference " << ret->getFullName() << " "
                            << oldName << " -> " << newName);
                }
            }
        }
    }

    if (notify) {
        aboutToSetValue();
    }

    auto updateSub = [&](const std::string& newSub) {
        if (sub != newSub) {
            // signalUpdateElementReference(sub, newSub);
            sub = newSub;
        }
    };

    if (missing) {
        FC_WARN(propertyName(this)
                << " missing element reference " << ret->getFullName() << " "
                << (elementName.newName.size() ? elementName.newName : elementName.oldName));
        shadow.oldName.swap(elementName.oldName);
    }
    else {
        FC_TRACE(propertyName(this) << " element reference shadow update " << ret->getFullName()
                                    << " " << shadow.newName << " -> " << elementName.newName);
        shadow.swap(elementName);
        if (shadow.newName.size() && Data::hasMappedElementName(sub.c_str())) {
            updateSub(shadow.newName);
        }
    }

    if (reverse) {
        if (shadow.newName.size() && Data::hasMappedElementName(sub.c_str())) {
            updateSub(shadow.newName);
        }
        else {
            updateSub(shadow.oldName);
        }
        return true;
    }
    if (missing) {
        if (sub != shadow.newName) {
            updateSub(shadow.oldName);
        }
        return true;
    }
    auto pos2 = shadow.newName.rfind('.');
    if (pos2 == std::string::npos) {
        return true;
    }
    ++pos2;
    auto pos = sub.rfind('.');
    if (pos == std::string::npos) {
        pos = 0;
    }
    else {
        ++pos;
    }
    if (pos == pos2) {
        if (sub.compare(pos, sub.size() - pos, &shadow.newName[pos2]) != 0) {
            FC_LOG("element reference update " << sub << " -> " << shadow.newName);
            std::string newSub(sub);
            newSub.replace(pos, sub.size() - pos, &shadow.newName[pos2]);
            updateSub(newSub);
        }
    }
    else if (sub != shadow.oldName) {
        FC_LOG("element reference update " << sub << " -> " << shadow.oldName);
        updateSub(shadow.oldName);
    }
    return true;
}

std::pair<DocumentObject*, std::string>
PropertyLinkBase::tryReplaceLink(const PropertyContainer* owner,
                                 DocumentObject* obj,
                                 const DocumentObject* parent,
                                 DocumentObject* oldObj,
                                 DocumentObject* newObj,
                                 const char* subname)
{
    std::pair<DocumentObject*, std::string> res;
    res.first = 0;
    if (!obj) {
        return res;
    }

    if (oldObj == obj) {
        if (owner == parent) {
            res.first = newObj;
            if (subname) {
                res.second = subname;
            }
            return res;
        }
        return res;
    }
    else if (newObj == obj) {
        // This means the new object is already sub-object of this parent
        // (consider a case of swapping the tool and base object of the Cut
        // feature). We'll swap the old and new object.
        return tryReplaceLink(owner, obj, parent, newObj, oldObj, subname);
    }
    if (!subname || !subname[0]) {
        return res;
    }

    App::DocumentObject* prev = obj;
    std::size_t prevPos = 0;
    std::string sub = subname;
    for (auto pos = sub.find('.'); pos != std::string::npos; pos = sub.find('.', pos)) {
        ++pos;
        char c = sub[pos];
        if (c == '.') {
            continue;
        }
        sub[pos] = 0;
        auto sobj = obj->getSubObject(sub.c_str());
        sub[pos] = c;
        if (!sobj) {
            break;
        }
        if (sobj == oldObj) {
            if (prev == parent) {
                if (sub[prevPos] == '$') {
                    sub.replace(prevPos + 1, pos - 1 - prevPos, newObj->Label.getValue());
                }
                else {
                    sub.replace(prevPos, pos - 1 - prevPos, newObj->getNameInDocument());
                }
                res.first = obj;
                res.second = std::move(sub);
                return res;
            }
            break;
        }
        else if (sobj == newObj) {
            return tryReplaceLink(owner, obj, parent, newObj, oldObj, subname);
        }
        else if (prev == parent) {
            break;
        }
        prev = sobj;
        prevPos = pos;
    }
    return res;
}

std::pair<DocumentObject*, std::vector<std::string>>
PropertyLinkBase::tryReplaceLinkSubs(const PropertyContainer* owner,
                                     DocumentObject* obj,
                                     const DocumentObject* parent,
                                     DocumentObject* oldObj,
                                     DocumentObject* newObj,
                                     const std::vector<std::string>& subs)
{
    std::pair<DocumentObject*, std::vector<std::string>> res;
    res.first = 0;
    if (!obj) {
        return res;
    }

    auto r = tryReplaceLink(owner, obj, parent, oldObj, newObj);
    if (r.first) {
        res.first = r.first;
        res.second = subs;
        return res;
    }
    for (auto it = subs.begin(); it != subs.end(); ++it) {
        auto r = tryReplaceLink(owner, obj, parent, oldObj, newObj, it->c_str());
        if (r.first) {
            if (!res.first) {
                res.first = r.first;
                res.second.insert(res.second.end(), subs.begin(), it);
            }
            res.second.push_back(std::move(r.second));
        }
        else if (res.first) {
            res.second.push_back(*it);
        }
    }
    return res;
}


void PropertyLinkBase::breakLinks(App::DocumentObject* link,
                                  const std::vector<App::DocumentObject*>& objs,
                                  bool clear)
{
    std::vector<Property*> props;
    for (auto obj : objs) {
        props.clear();
        obj->getPropertyList(props);
        for (auto prop : props) {
            auto linkProp = dynamic_cast<PropertyLinkBase*>(prop);
            if (linkProp) {
                linkProp->breakLink(link, clear);
            }
        }
    }
    DocInfo::breakLinks(link, clear);
}

}  // namespace App