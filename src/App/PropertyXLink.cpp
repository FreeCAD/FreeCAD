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

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Application.h"
#include "DocInfo.h"
#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "DocumentObserver.h"
#include "ObjectIdentifier.h"
#include "PropertyLinkList.h"
#include "PropertyLinkSub.h"
#include "PropertyLinkSubList.h"
#include "PropertyXLink.h"


FC_LOG_LEVEL_INIT("PropertyXLink", true, true)

namespace App {
using SubSet = std::pair<DocumentObject*, std::vector<std::string>>;

//**************************************************************************
// PropertyXLink
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(PropertyXLink, PropertyLink)

PropertyXLink::PropertyXLink(bool _allowPartial, PropertyLinkBase* parent)
    : parentProp(parent)
{
    setAllowPartial(_allowPartial);
    setAllowExternal(true);
    setSyncSubObject(true);
    if (parent) {
        setContainer(parent->getContainer());
    }
}

PropertyXLink::~PropertyXLink()
{
    unlink();
}

void PropertyXLink::setSyncSubObject(bool enable)
{
    _Flags.set((std::size_t)LinkSyncSubObject, enable);
}

void PropertyXLink::unlink()
{
    if (docInfo) {
        docInfo->remove(this);
        docInfo.reset();
    }
    objectName.clear();
    resetLink();
}

void PropertyXLink::detach()
{
    if (docInfo && _pcLink) {
        aboutToSetValue();
        resetLink();
        updateElementReference(nullptr);
        hasSetValue();
    }
}

void PropertyXLink::aboutToSetValue()
{
    if (parentProp) {
        parentProp->aboutToSetChildValue(*this);
    }
    else {
        PropertyLinkBase::aboutToSetValue();
    }
}

void PropertyXLink::hasSetValue()
{
    if (parentProp) {
        parentProp->hasSetChildValue(*this);
    }
    else {
        PropertyLinkBase::hasSetValue();
    }
}

void PropertyXLink::setSubName(const char* subname)
{
    std::vector<std::string> subs;
    if (subname && subname[0]) {
        subs.emplace_back(subname);
    }
    aboutToSetValue();
    setSubValues(std::move(subs));
    hasSetValue();
}

void PropertyXLink::setSubValues(std::vector<std::string>&& subs, std::vector<ShadowSub>&& shadows)
{
    _SubList = std::move(subs);
    _ShadowSubList.clear();
    if (shadows.size() == _SubList.size()) {
        _ShadowSubList = std::move(shadows);
        onContainerRestored();  // re-register element references
    }
    else {
        updateElementReference(nullptr);
    }
    checkLabelReferences(_SubList);
}

void PropertyXLink::setValue(App::DocumentObject* lValue)
{
    setValue(lValue, nullptr);
}

void PropertyXLink::setValue(App::DocumentObject* lValue, const char* subname)
{
    std::vector<std::string> subs;
    if (subname && subname[0]) {
        subs.emplace_back(subname);
    }
    setValue(lValue, std::move(subs));
}

void PropertyXLink::restoreLink(App::DocumentObject* lValue)
{
    assert(!_pcLink && lValue && docInfo);

    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if (!owner || !owner->isAttachedToDocument()) {
        throw Base::RuntimeError("invalid container");
    }

    bool touched = owner->isTouched();
    setFlag(LinkDetached, false);
    setFlag(LinkRestoring);
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (!owner->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
        lValue->_addBackLink(owner);
    }
#endif
    _pcLink = lValue;
    updateElementReference(nullptr);
    hasSetValue();
    setFlag(LinkRestoring, false);

    if (!touched && owner->isTouched() && docInfo && docInfo->pcDoc
        && stamp == docInfo->pcDoc->LastModifiedDate.getValue()) {
        owner->purgeTouched();
    }
}

void PropertyXLink::setValue(App::DocumentObject* lValue,
                             std::vector<std::string>&& subs,
                             std::vector<ShadowSub>&& shadows)
{
    if (_pcLink == lValue && _SubList == subs) {
        return;
    }

    if (lValue && (!lValue->isAttachedToDocument() || !lValue->getDocument())) {
        throw Base::ValueError("Invalid object");
    }

    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if (!owner || !owner->isAttachedToDocument()) {
        throw Base::RuntimeError("invalid container");
    }

    if (lValue == owner) {
        throw Base::ValueError("self linking");
    }

    aboutToSetValue();

    DocInfoPtr info;
    const char* name = "";
    if (lValue) {
        name = lValue->getNameInDocument();
        if (lValue->getDocument() != owner->getDocument()) {
            if (!docInfo || lValue->getDocument() != docInfo->pcDoc) {
                const char* filename = lValue->getDocument()->getFileName();
                if (!filename || *filename == 0) {
                    throw Base::RuntimeError("Linked document not saved");
                }
                FC_LOG("xlink set to new document " << lValue->getDocument()->getName());
                info = DocInfo::get(filename, owner->getDocument(), this, name);
                assert(info && info->pcDoc == lValue->getDocument());
            }
            else {
                info = docInfo;
            }
        }
    }

    setFlag(LinkDetached, false);
#ifndef USE_OLD_DAG
    if (!owner->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
        if (_pcLink) {
            _pcLink->_removeBackLink(owner);
        }
        if (lValue) {
            lValue->_addBackLink(owner);
        }
    }
#endif
    if (docInfo != info) {
        unlink();
        docInfo = info;
    }
    if (!docInfo) {
        filePath.clear();
    }
    _pcLink = lValue;
    if (docInfo && docInfo->pcDoc) {
        stamp = docInfo->pcDoc->LastModifiedDate.getValue();
    }
    objectName = name;
    setSubValues(std::move(subs), std::move(shadows));
    hasSetValue();
}

void PropertyXLink::setValue(std::string&& filename,
                             std::string&& name,
                             std::vector<std::string>&& subs,
                             std::vector<ShadowSub>&& shadows)
{
    if (name.empty()) {
        setValue(nullptr, std::move(subs), std::move(shadows));
        return;
    }
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if (!owner || !owner->isAttachedToDocument()) {
        throw Base::RuntimeError("invalid container");
    }

    DocumentObject* pObject = nullptr;
    DocInfoPtr info;
    if (!filename.empty()) {
        owner->getDocument()->signalLinkXsetValue(filename);
        info = DocInfo::get(filename.c_str(), owner->getDocument(), this, name.c_str());
        if (info->pcDoc) {
            pObject = info->pcDoc->getObject(name.c_str());
        }
    }
    else {
        pObject = owner->getDocument()->getObject(name.c_str());
    }

    if (pObject) {
        setValue(pObject, std::move(subs), std::move(shadows));
        return;
    }
    setFlag(LinkDetached, false);
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (_pcLink && !owner->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
        _pcLink->_removeBackLink(owner);
    }
#endif
    _pcLink = nullptr;
    if (docInfo != info) {
        unlink();
        docInfo = info;
    }
    if (!docInfo) {
        filePath.clear();
    }
    if (docInfo && docInfo->pcDoc) {
        stamp = docInfo->pcDoc->LastModifiedDate.getValue();
    }
    objectName = std::move(name);
    setSubValues(std::move(subs), std::move(shadows));
    hasSetValue();
}

void PropertyXLink::setValue(App::DocumentObject* link,
                             const std::vector<std::string>& subs,
                             std::vector<ShadowSub>&& shadows)
{
    setValue(link, std::vector<std::string>(subs), std::move(shadows));
}

App::Document* PropertyXLink::getDocument() const
{
    return docInfo ? docInfo->pcDoc : nullptr;
}

const char* PropertyXLink::getDocumentPath() const
{
    return docInfo ? docInfo->filePath() : filePath.c_str();
}

const char* PropertyXLink::getObjectName() const
{
    return objectName.c_str();
}

bool PropertyXLink::upgrade(Base::XMLReader& reader, const char* typeName)
{
    if (strcmp(typeName, App::PropertyLinkGlobal::getClassTypeId().getName()) == 0
        || strcmp(typeName, App::PropertyLink::getClassTypeId().getName()) == 0
        || strcmp(typeName, App::PropertyLinkChild::getClassTypeId().getName()) == 0) {
        PropertyLink::Restore(reader);
        return true;
    }
    FC_ERR("Cannot upgrade from " << typeName);
    return false;
}

int PropertyXLink::checkRestore(std::string* msg) const
{
    if (!docInfo) {
        if (!_pcLink && !objectName.empty()) {
            // this condition means linked object not found
            if (msg) {
                std::ostringstream ss;
                ss << "Link not restored" << std::endl;
                ss << "Object: " << objectName;
                if (!filePath.empty()) {
                    ss << std::endl << "File: " << filePath;
                }
                *msg = ss.str();
            }
            return 2;
        }
        return 0;
    }
    if (!_pcLink) {
        if (testFlag(LinkSilentRestore)) {
            return 0;
        }
        if (testFlag(LinkAllowPartial)
            && (!docInfo->pcDoc || docInfo->pcDoc->testStatus(App::Document::PartialDoc))) {
            return 0;
        }
        if (msg) {
            std::ostringstream ss;
            ss << "Link not restored" << std::endl;
            ss << "Linked object: " << objectName;
            if (docInfo->pcDoc) {
                ss << std::endl << "Linked document: " << docInfo->pcDoc->Label.getValue();
            }
            else if (!filePath.empty()) {
                ss << std::endl << "Linked file: " << filePath;
            }
            *msg = ss.str();
        }
        return 2;
    }
    if (!docInfo->pcDoc || stamp == docInfo->pcDoc->LastModifiedDate.getValue()) {
        return 0;
    }

    if (msg) {
        std::ostringstream ss;
        ss << "Time stamp changed on link " << _pcLink->getFullName();
        *msg = ss.str();
    }
    return 1;
}

void PropertyXLink::afterRestore()
{
    assert(_SubList.size() == _ShadowSubList.size());
    if (!testFlag(LinkRestoreLabel) || !_pcLink || !_pcLink->isAttachedToDocument()) {
        return;
    }
    setFlag(LinkRestoreLabel, false);
    for (size_t i = 0; i < _SubList.size(); ++i) {
        restoreLabelReference(_pcLink, _SubList[i], &_ShadowSubList[i]);
    }
}

void PropertyXLink::onContainerRestored()
{
    if (!_pcLink || !_pcLink->isAttachedToDocument()) {
        return;
    }
    for (size_t i = 0; i < _SubList.size(); ++i) {
        _registerElementReference(_pcLink, _SubList[i], _ShadowSubList[i]);
    }
}

void PropertyXLink::updateElementReference(DocumentObject* feature, bool reverse, bool notify)
{
    if (!updateLinkReference(this,
                             feature,
                             reverse,
                             notify,
                             _pcLink,
                             _SubList,
                             _mapped,
                             _ShadowSubList)) {
        return;
    }
    if (notify) {
        hasSetValue();
    }
}

bool PropertyXLink::referenceChanged() const
{
    return !_mapped.empty();
}

void PropertyXLink::Save(Base::Writer& writer) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if (!owner || !owner->getDocument()) {
        return;
    }

    assert(_SubList.size() == _ShadowSubList.size());

    auto exporting = owner->isExporting();
    if (_pcLink && exporting && _pcLink->isExporting()) {
        // this means, we are exporting the owner and the linked object together.
        // Lets save the export name
        writer.Stream() << writer.ind() << "<XLink name=\"" << _pcLink->getExportName();
    }
    else {
        const char* path = filePath.c_str();
        std::string _path;
        if (exporting) {
            // Here means we are exporting the owner but not exporting the
            // linked object.  Try to use absolute file path for easy transition
            // into document at different directory
            if (docInfo) {
                _path = docInfo->filePath();
            }
            else {
                auto pDoc = owner->getDocument();
                const char* docPath = pDoc->getFileName();
                if (docPath && docPath[0]) {
                    if (!filePath.empty()) {
                        _path = DocInfo::getDocPath(filePath.c_str(), pDoc, false);
                    }
                    else {
                        _path = docPath;
                    }
                }
                else {
                    FC_WARN("PropertyXLink export without saving the document");
                }
            }
            if (!_path.empty()) {
                path = _path.c_str();
            }
        }
        writer.Stream() << writer.ind() << "<XLink file=\"" << encodeAttribute(path)
                        << "\" stamp=\""
                        << (docInfo && docInfo->pcDoc ? docInfo->pcDoc->LastModifiedDate.getValue()
                                                      : "")
                        << "\" name=\"" << objectName;
    }

    if (testFlag(LinkAllowPartial)) {
        writer.Stream() << "\" partial=\"1";
    }

    if (_SubList.empty()) {
        writer.Stream() << "\"/>" << std::endl;
    }
    else if (_SubList.size() == 1) {
        const auto& subName = _SubList[0];
        const auto& shadowSub = _ShadowSubList[0];
        const auto& sub = shadowSub.oldName.empty() ? subName : shadowSub.oldName;
        if (exporting) {
            std::string exportName;
            writer.Stream() << "\" sub=\""
                            << encodeAttribute(exportSubName(exportName, _pcLink, sub.c_str()));
            if (!shadowSub.oldName.empty() && shadowSub.newName == subName) {
                writer.Stream() << "\" " ATTR_MAPPED "=\"1";
            }
        }
        else {
            writer.Stream() << "\" sub=\"" << encodeAttribute(sub);
            if (!sub.empty()) {
                if (sub != subName) {
                    writer.Stream() << "\" " ATTR_SHADOWED "=\"" << encodeAttribute(subName);
                }
                else if (!shadowSub.newName.empty()) {
                    writer.Stream()
                        << "\" " ATTR_SHADOW "=\"" << encodeAttribute(shadowSub.newName);
                }
            }
        }
        writer.Stream() << "\"/>" << std::endl;
    }
    else {
        writer.Stream() << "\" count=\"" << _SubList.size() << "\">" << std::endl;
        writer.incInd();
        for (unsigned int i = 0; i < _SubList.size(); i++) {
            const auto& shadow = _ShadowSubList[i];
            // shadow.oldName stores the old style element name. For backward
            // compatibility reason, we shall store the old name into attribute
            // 'value' whenever possible.
            const auto& sub = shadow.oldName.empty() ? _SubList[i] : shadow.oldName;
            writer.Stream() << writer.ind() << "<Sub value=\"";
            if (exporting) {
                std::string exportName;
                writer.Stream() << encodeAttribute(exportSubName(exportName, _pcLink, sub.c_str()));
                if (!shadow.oldName.empty() && shadow.newName == _SubList[i]) {
                    writer.Stream() << "\" " ATTR_MAPPED "=\"1";
                }
            }
            else {
                writer.Stream() << encodeAttribute(sub);
                if (!_SubList[i].empty()) {
                    if (sub != _SubList[i]) {
                        writer.Stream()
                            << "\" " ATTR_SHADOWED "=\"" << encodeAttribute(_SubList[i]);
                    }
                    else if (!shadow.newName.empty()) {
                        writer.Stream()
                            << "\" " ATTR_SHADOW "=\"" << encodeAttribute(shadow.newName);
                    }
                }
            }
            writer.Stream() << "\"/>" << std::endl;
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</XLink>" << std::endl;
    }
}

void PropertyXLink::Restore(Base::XMLReader& reader)
{
    // read my element
    reader.readElement("XLink");
    std::string stampAttr, file;
    if (reader.hasAttribute("stamp")) {
        stampAttr = reader.getAttribute("stamp");
    }
    if (reader.hasAttribute("file")) {
        file = reader.getAttribute("file");
    }

    setFlag(LinkAllowPartial,
            reader.hasAttribute("partial") && reader.getAttributeAsInteger("partial"));
    std::string name;
    if (file.empty()) {
        name = reader.getName(reader.getAttribute("name"));
    }
    else {
        name = reader.getAttribute("name");
    }

    assert(getContainer()->isDerivedFrom<App::DocumentObject>());
    DocumentObject* object = nullptr;
    if (!name.empty() && file.empty()) {
        DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        Document* document = parent->getDocument();
        object = document ? document->getObject(name.c_str()) : nullptr;
        if (!object) {
            if (reader.isVerbose()) {
                FC_WARN("Lost link to '" << name
                                         << "' while loading, maybe "
                                            "an object was not loaded correctly");
            }
        }
    }

    std::vector<std::string> subs;
    std::vector<ShadowSub> shadows;
    std::vector<int> mapped;
    bool restoreLabel = false;
    if (reader.hasAttribute("sub")) {
        if (reader.hasAttribute(ATTR_MAPPED)) {
            mapped.push_back(0);
        }
        subs.emplace_back();
        auto& subname = subs.back();
        shadows.emplace_back();
        auto& shadow = shadows.back();
        shadow.oldName = importSubName(reader, reader.getAttribute("sub"), restoreLabel);
        if (reader.hasAttribute(ATTR_SHADOWED) && !IGNORE_SHADOW) {
            subname = shadow.newName =
                importSubName(reader, reader.getAttribute(ATTR_SHADOWED), restoreLabel);
        }
        else {
            subname = shadow.oldName;
            if (reader.hasAttribute(ATTR_SHADOW) && !IGNORE_SHADOW) {
                shadow.newName =
                    importSubName(reader, reader.getAttribute(ATTR_SHADOW), restoreLabel);
            }
        }
    }
    else if (reader.hasAttribute("count")) {
        int count = reader.getAttributeAsInteger("count");
        subs.resize(count);
        shadows.resize(count);
        for (int i = 0; i < count; i++) {
            reader.readElement("Sub");
            shadows[i].oldName = importSubName(reader, reader.getAttribute("value"), restoreLabel);
            if (reader.hasAttribute(ATTR_SHADOWED) && !IGNORE_SHADOW) {
                subs[i] = shadows[i].newName =
                    importSubName(reader, reader.getAttribute(ATTR_SHADOWED), restoreLabel);
            }
            else {
                subs[i] = shadows[i].oldName;
                if (reader.hasAttribute(ATTR_SHADOW) && !IGNORE_SHADOW) {
                    shadows[i].newName =
                        importSubName(reader, reader.getAttribute(ATTR_SHADOW), restoreLabel);
                }
            }
            if (reader.hasAttribute(ATTR_MAPPED)) {
                mapped.push_back(i);
            }
        }
        reader.readEndElement("XLink");
    }
    setFlag(LinkRestoreLabel, restoreLabel);

    if (name.empty()) {
        setValue(nullptr);
        return;
    }

    if (!file.empty() || (!object && !name.empty())) {
        this->stamp = stampAttr;
        setValue(std::move(file), std::move(name), std::move(subs), std::move(shadows));
    }
    else {
        setValue(object, std::move(subs), std::move(shadows));
    }
    _mapped = std::move(mapped);
}

Property*
PropertyXLink::CopyOnImportExternal(const std::map<std::string, std::string>& nameMap) const
{
    auto owner = Base::freecad_dynamic_cast<const DocumentObject>(getContainer());
    if (!owner || !owner->getDocument() || !_pcLink || !_pcLink->isAttachedToDocument()) {
        return nullptr;
    }

    auto subs = updateLinkSubs(_pcLink, _SubList, &tryImportSubName, owner->getDocument(), nameMap);
    auto linked = tryImport(owner->getDocument(), _pcLink, nameMap);
    if (subs.empty() && linked == _pcLink) {
        return nullptr;
    }

    std::unique_ptr<PropertyXLink> p(new PropertyXLink);
    copyTo(*p, linked, &subs);
    return p.release();
}

Property* PropertyXLink::CopyOnLinkReplace(const App::DocumentObject* parent,
                                           App::DocumentObject* oldObj,
                                           App::DocumentObject* newObj) const
{
    auto res = tryReplaceLinkSubs(getContainer(), _pcLink, parent, oldObj, newObj, _SubList);
    if (!res.first) {
        return nullptr;
    }
    std::unique_ptr<PropertyXLink> p(new PropertyXLink);
    copyTo(*p, res.first, &res.second);
    return p.release();
}

Property* PropertyXLink::CopyOnLabelChange(App::DocumentObject* obj,
                                           const std::string& ref,
                                           const char* newLabel) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if (!owner || !owner->getDocument() || !_pcLink || !_pcLink->isAttachedToDocument()) {
        return nullptr;
    }
    auto subs = updateLinkSubs(_pcLink, _SubList, &updateLabelReference, obj, ref, newLabel);
    if (subs.empty()) {
        return nullptr;
    }
    std::unique_ptr<PropertyXLink> p(new PropertyXLink);
    copyTo(*p, _pcLink, &subs);
    return p.release();
}

void PropertyXLink::copyTo(PropertyXLink& other,
                           DocumentObject* linked,
                           std::vector<std::string>* subs) const
{
    if (!linked) {
        linked = _pcLink;
    }
    if (linked && linked->isAttachedToDocument()) {
        other.docName = linked->getDocument()->getName();
        other.objectName = linked->getNameInDocument();
        other.docInfo.reset();
        other.filePath.clear();
    }
    else {
        other.objectName = objectName;
        other.docName.clear();
        other.docInfo = docInfo;
        other.filePath = filePath;
    }
    if (subs) {
        other._SubList = std::move(*subs);
    }
    else {
        other._SubList = _SubList;
        other._ShadowSubList = _ShadowSubList;
    }
    other._Flags = _Flags;
}

Property* PropertyXLink::Copy() const
{
    std::unique_ptr<PropertyXLink> p(new PropertyXLink);
    copyTo(*p);
    return p.release();
}

void PropertyXLink::Paste(const Property& from)
{
    if (!from.isDerivedFrom<PropertyXLink>()) {
        throw Base::TypeError("Incompatible property to paste to");
    }

    const auto& other = static_cast<const PropertyXLink&>(from);
    if (!other.docName.empty()) {
        auto doc = GetApplication().getDocument(other.docName.c_str());
        if (!doc) {
            FC_WARN("Document '" << other.docName << "' not found");
            return;
        }
        auto obj = doc->getObject(other.objectName.c_str());
        if (!obj) {
            FC_WARN("Object '" << other.docName << '#' << other.objectName << "' not found");
            return;
        }
        setValue(obj,
                 std::vector<std::string>(other._SubList),
                 std::vector<ShadowSub>(other._ShadowSubList));
    }
    else {
        setValue(std::string(other.filePath),
                 std::string(other.objectName),
                 std::vector<std::string>(other._SubList),
                 std::vector<ShadowSub>(other._ShadowSubList));
    }
    setFlag(LinkAllowPartial, other.testFlag(LinkAllowPartial));
}

bool PropertyXLink::supportXLink(const App::Property* prop)
{
    return prop->isDerivedFrom<PropertyXLink>()
        || prop->isDerivedFrom<PropertyXLinkSubList>()
        || prop->isDerivedFrom<PropertyXLinkContainer>();
}

bool PropertyXLink::hasXLink(const App::Document* doc)
{
    for (auto& v : DocInfo::getMap()) {
        if (v.second->hasXLink(doc)) {
            return true;
        }
    }
    return false;
}

bool PropertyXLink::hasXLink(const std::vector<App::DocumentObject*>& objs,
                             std::vector<App::Document*>* unsaved)
{
    std::set<App::Document*> docs;
    bool ret = false;
    for (auto o : objs) {
        if (o && o->isAttachedToDocument() && docs.insert(o->getDocument()).second) {
            if (!hasXLink(o->getDocument())) {
                continue;
            }
            if (!unsaved) {
                return true;
            }
            ret = true;
            if (!o->getDocument()->isSaved()) {
                unsaved->push_back(o->getDocument());
            }
        }
    }
    return ret;
}

void PropertyXLink::restoreDocument(const App::Document& doc)
{
    DocInfo::restoreDocument(doc);
}

std::map<App::Document*, std::set<App::Document*>>
PropertyXLink::getDocumentOutList(App::Document* doc)
{
    std::map<App::Document*, std::set<App::Document*>> ret;
    for (auto& v : DocInfo::getMap()) {
        for (auto link : v.second->links) {
            if (!v.second->pcDoc || link->getScope() == LinkScope::Hidden
                || link->testStatus(Property::PropTransient)
                || link->testStatus(Property::Transient)
                || link->testStatus(Property::PropNoPersist)) {
                continue;
            }
            auto obj = dynamic_cast<App::DocumentObject*>(link->getContainer());
            if (!obj || !obj->isAttachedToDocument() || !obj->getDocument()) {
                continue;
            }
            if (doc && obj->getDocument() != doc) {
                continue;
            }
            ret[obj->getDocument()].insert(v.second->pcDoc);
        }
    }
    return ret;
}

std::map<App::Document*, std::set<App::Document*>>
PropertyXLink::getDocumentInList(App::Document* doc)
{
    std::map<App::Document*, std::set<App::Document*>> ret;
    for (auto& v : DocInfo::getMap()) {
        if (!v.second->pcDoc || (doc && doc != v.second->pcDoc)) {
            continue;
        }
        auto& docs = ret[v.second->pcDoc];
        for (auto link : v.second->links) {
            if (link->getScope() == LinkScope::Hidden || link->testStatus(Property::PropTransient)
                || link->testStatus(Property::Transient)
                || link->testStatus(Property::PropNoPersist)) {
                continue;
            }
            auto obj = dynamic_cast<App::DocumentObject*>(link->getContainer());
            if (obj && obj->isAttachedToDocument() && obj->getDocument()) {
                docs.insert(obj->getDocument());
            }
        }
    }
    return ret;
}

PyObject* PropertyXLink::getPyObject()
{
    if (!_pcLink) {
        Py_Return;
    }
    const auto& subs = getSubValues(false);
    if (subs.empty()) {
        return _pcLink->getPyObject();
    }
    Py::Tuple ret(2);
    ret.setItem(0, Py::Object(_pcLink->getPyObject(), true));
    PropertyString propString;
    if (subs.size() == 1) {
        propString.setValue(subs.front());
        ret.setItem(1, Py::asObject(propString.getPyObject()));
    }
    else {
        Py::List list(subs.size());
        int i = 0;
        for (auto& sub : subs) {
            propString.setValue(sub);
            list[i++] = Py::asObject(propString.getPyObject());
        }
        ret.setItem(1, list);
    }
    return Py::new_reference_to(ret);
}

void PropertyXLink::setPyObject(PyObject* value)
{
    if (PySequence_Check(value)) {
        Py::Sequence seq(value);
        if (seq.size() != 2) {
            throw Base::ValueError("Expect input sequence of size 2");
        }
        std::vector<std::string> subs;
        Py::Object pyObj(seq[0].ptr());
        Py::Object pySub(seq[1].ptr());
        if (pyObj.isNone()) {
            setValue(nullptr);
            return;
        }
        else if (!PyObject_TypeCheck(pyObj.ptr(), &DocumentObjectPy::Type)) {
            throw Base::TypeError("Expect the first element to be of 'DocumentObject'");
        }
        PropertyString propString;
        if (pySub.isString()) {
            propString.setPyObject(pySub.ptr());
            subs.push_back(propString.getStrValue());
        }
        else if (pySub.isSequence()) {
            Py::Sequence seq(pySub);
            subs.reserve(seq.size());
            for (Py_ssize_t i = 0; i < seq.size(); ++i) {
                Py::Object sub(seq[i]);
                if (!sub.isString()) {
                    throw Base::TypeError("Expect only string inside second argument");
                }
                propString.setPyObject(sub.ptr());
                subs.push_back(propString.getStrValue());
            }
        }
        else {
            throw Base::TypeError("Expect the second element to be a string or sequence of string");
        }
        setValue(static_cast<DocumentObjectPy*>(pyObj.ptr())->getDocumentObjectPtr(),
                 std::move(subs));
    }
    else if (PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        setValue(static_cast<DocumentObjectPy*>(value)->getDocumentObjectPtr());
    }
    else if (Py_None == value) {
        setValue(nullptr);
    }
    else {
        throw Base::TypeError(
            "type must be 'DocumentObject', 'None', or '(DocumentObject, SubName)' or "
            "'DocumentObject, [SubName..])");
    }
}

const char* PropertyXLink::getSubName(bool newStyle) const
{
    if (_SubList.empty() || _ShadowSubList.empty()) {
        return "";
    }
    return getSubNameWithStyle(_SubList[0], _ShadowSubList[0], newStyle, tmpShadow).c_str();
}

void PropertyXLink::getLinks(std::vector<App::DocumentObject*>& objs,
                             bool all,
                             std::vector<std::string>* subs,
                             bool newStyle) const
{
    if ((all || _pcScope != LinkScope::Hidden) && _pcLink && _pcLink->isAttachedToDocument()) {
        objs.push_back(_pcLink);
        if (subs && _SubList.size() == _ShadowSubList.size()) {
            *subs = getSubValues(newStyle);
        }
    }
}

void PropertyXLink::getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                               App::DocumentObject* obj,
                               const char* subname,
                               bool all) const
{
    if (all || _pcScope != LinkScope::Hidden) {
        if (obj && obj == _pcLink) {
            _getLinksTo(identifiers, obj, subname, _SubList, _ShadowSubList);
        }
    }
}

bool PropertyXLink::adjustLink(const std::set<App::DocumentObject*>& inList)
{
    if (_pcScope == LinkScope::Hidden) {
        return false;
    }
    if (!_pcLink || !_pcLink->isAttachedToDocument() || !inList.count(_pcLink)) {
        return false;
    }
    auto subs = _SubList;
    auto link = adjustLinkSubs(this, inList, _pcLink, subs);
    if (link) {
        setValue(link, std::move(subs));
        return true;
    }
    return false;
}

std::vector<std::string> PropertyXLink::getSubValues(bool newStyle) const
{
    assert(_SubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    ret.reserve(_SubList.size());
    std::string tmp;
    for (size_t i = 0; i < _ShadowSubList.size(); ++i) {
        ret.push_back(getSubNameWithStyle(_SubList[i], _ShadowSubList[i], newStyle, tmp));
    }
    return ret;
}

std::vector<std::string> PropertyXLink::getSubValuesStartsWith(const char* starter,
                                                               bool newStyle) const
{
    (void)newStyle;

    std::vector<std::string> temp;
    for (const auto& it : _SubList) {
        if (strncmp(starter, it.c_str(), strlen(starter)) == 0) {
            temp.push_back(it);
        }
    }
    return temp;
}

void PropertyXLink::setAllowPartial(bool enable)
{
    setFlag(LinkAllowPartial, enable);
    if (enable) {
        return;
    }
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if (!owner) {
        return;
    }
    if (!App::GetApplication().isRestoring() && !owner->getDocument()->isPerformingTransaction()
        && !_pcLink && docInfo && !filePath.empty() && !objectName.empty()
        && (!docInfo->pcDoc || docInfo->pcDoc->testStatus(Document::PartialDoc))) {
        auto path = docInfo->getDocPath(filePath.c_str(), owner->getDocument(), false);
        if (!path.empty()) {
            App::GetApplication().openDocument(path.c_str());
        }
    }
}

//**************************************************************************
// PropertyXLinkSub
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyXLinkSub, App::PropertyXLink)
TYPESYSTEM_SOURCE(App::PropertyXLinkSubHidden, App::PropertyXLinkSub)

PropertyXLinkSub::PropertyXLinkSub(bool allowPartial, PropertyLinkBase* parent)
    : PropertyXLink(allowPartial, parent)
{}

PropertyXLinkSub::~PropertyXLinkSub() = default;

bool PropertyXLinkSub::upgrade(Base::XMLReader& reader, const char* typeName)
{
    if (strcmp(typeName, PropertyLinkSubGlobal::getClassTypeId().getName()) == 0
        || strcmp(typeName, PropertyLinkSub::getClassTypeId().getName()) == 0
        || strcmp(typeName, PropertyLinkSubChild::getClassTypeId().getName()) == 0) {
        App::PropertyLinkSub linkProp;
        linkProp.setContainer(getContainer());
        linkProp.Restore(reader);
        setValue(linkProp.getValue(), linkProp.getSubValues());
        return true;
    }
    return PropertyXLink::upgrade(reader, typeName);
}

PyObject* PropertyXLinkSub::getPyObject()
{
    if (!_pcLink) {
        Py_Return;
    }
    Py::Tuple ret(2);
    ret.setItem(0, Py::Object(_pcLink->getPyObject(), true));
    const auto& subs = getSubValues(false);
    Py::List list(subs.size());
    int i = 0;
    PropertyString propString;
    for (auto& sub : subs) {
        propString.setValue(sub);
        list[i++] = Py::asObject(propString.getPyObject());
    }
    ret.setItem(1, list);
    return Py::new_reference_to(ret);
}


//**************************************************************************
// PropertyXLinkSubList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyXLinkSubList, App::PropertyLinkBase)

//**************************************************************************
// Construction/Destruction


PropertyXLinkSubList::PropertyXLinkSubList()
{
    _pcScope = LinkScope::Global;
    setSyncSubObject(true);
}

PropertyXLinkSubList::~PropertyXLinkSubList() = default;

void PropertyXLinkSubList::setSyncSubObject(bool enable)
{
    _Flags.set((std::size_t)LinkSyncSubObject, enable);
}

int PropertyXLinkSubList::getSize() const
{
    return static_cast<int>(_Links.size());
}

void PropertyXLinkSubList::setValue(DocumentObject* lValue, const char* SubName)
{
    std::map<DocumentObject*, std::vector<std::string>> values;
    if (lValue) {
        auto& subs = values[lValue];
        if (SubName) {
            subs.emplace_back(SubName);
        }
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::setValues(const std::vector<DocumentObject*>& lValue,
                                     const std::vector<const char*>& lSubNames)
{
#define CHECK_SUB_SIZE(_l, _r)                                                                     \
    do {                                                                                           \
        if (_l.size() != _r.size())                                                                \
            FC_THROWM(Base::ValueError, "object and subname size mismatch");                       \
    } while (0)
    CHECK_SUB_SIZE(lValue, lSubNames);
    std::map<DocumentObject*, std::vector<std::string>> values;
    int i = 0;
    for (auto& obj : lValue) {
        const char* sub = lSubNames[i++];
        if (sub) {
            values[obj].emplace_back(sub);
        }
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::setValues(const std::vector<DocumentObject*>& lValue,
                                     const std::vector<std::string>& lSubNames)
{
    CHECK_SUB_SIZE(lValue, lSubNames);
    std::map<DocumentObject*, std::vector<std::string>> values;
    int i = 0;
    for (auto& obj : lValue) {
        values[obj].push_back(lSubNames[i++]);
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::setSubListValues(const std::vector<SubSet>& svalues)
{
    std::map<DocumentObject*, std::vector<std::string>> values;
    for (auto& v : svalues) {
        auto& s = values[v.first];
        s.reserve(s.size() + v.second.size());
        s.insert(s.end(), v.second.begin(), v.second.end());
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::setValues(
    const std::map<App::DocumentObject*, std::vector<std::string>>& values)
{
    setValues(std::map<App::DocumentObject*, std::vector<std::string>>(values));
}

void PropertyXLinkSubList::setValues(
    std::map<App::DocumentObject*, std::vector<std::string>>&& values)
{
    for (auto& v : values) {
        if (!v.first || !v.first->isAttachedToDocument()) {
            FC_THROWM(Base::ValueError, "invalid document object");
        }
    }

    atomic_change guard(*this);

    for (auto it = _Links.begin(), itNext = it; it != _Links.end(); it = itNext) {
        ++itNext;
        auto iter = values.find(it->getValue());
        if (iter == values.end()) {
            _Links.erase(it);
            continue;
        }
        it->setSubValues(std::move(iter->second));
        values.erase(iter);
    }

    for (auto& v : values) {
        _Links.emplace_back(testFlag(LinkAllowPartial), this);
        _Links.back().setValue(v.first, std::move(v.second));
    }
    guard.tryInvoke();
}

void PropertyXLinkSubList::addValue(App::DocumentObject* obj,
                                    const std::vector<std::string>& subs,
                                    bool reset)
{
    addValue(obj, std::vector<std::string>(subs), reset);
}

void PropertyXLinkSubList::addValue(App::DocumentObject* obj,
                                    std::vector<std::string>&& subs,
                                    bool reset)
{

    if (!obj || !obj->isAttachedToDocument()) {
        FC_THROWM(Base::ValueError, "invalid document object");
    }

    for (auto& l : _Links) {
        if (l.getValue() == obj) {
            auto s = l.getSubValues();
            if (s.empty() || reset) {
                l.setSubValues(std::move(subs));
            }
            else {
                s.reserve(s.size() + subs.size());
                std::move(subs.begin(), subs.end(), std::back_inserter(s));
                l.setSubValues(std::move(s));
            }
            return;
        }
    }
    atomic_change guard(*this);
    _Links.emplace_back(testFlag(LinkAllowPartial), this);
    _Links.back().setValue(obj, std::move(subs));
    guard.tryInvoke();
}

void PropertyXLinkSubList::setValue(DocumentObject* lValue, const std::vector<std::string>& SubList)
{
    std::map<DocumentObject*, std::vector<std::string>> values;
    if (lValue) {
        values[lValue] = SubList;
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::setValues(const std::vector<DocumentObject*>& values)
{
    atomic_change guard(*this);
    _Links.clear();
    for (auto obj : values) {
        _Links.emplace_back(testFlag(LinkAllowPartial), this);
        _Links.back().setValue(obj);
    }
    guard.tryInvoke();
}

void PropertyXLinkSubList::set1Value(int idx,
                                     DocumentObject* value,
                                     const std::vector<std::string>& SubList)
{
    if (idx < -1 || idx > getSize()) {
        throw Base::RuntimeError("index out of bound");
    }

    if (idx < 0 || idx + 1 == getSize()) {
        if (SubList.empty()) {
            addValue(value, SubList);
            return;
        }
        atomic_change guard(*this);
        _Links.emplace_back(testFlag(LinkAllowPartial), this);
        _Links.back().setValue(value);
        guard.tryInvoke();
        return;
    }

    auto it = _Links.begin();
    for (; idx; --idx) {
        ++it;
    }
    it->setValue(value, SubList);
}

const std::string PropertyXLinkSubList::getPyReprString() const
{
    if (_Links.empty()) {
        return std::string("None");
    }
    std::ostringstream ss;
    ss << '[';
    for (auto& link : _Links) {
        auto obj = link.getValue();
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }
        ss << "(App.getDocument('" << obj->getDocument()->getName() << "').getObject('"
           << obj->getNameInDocument() << "'),  (";
        const auto& subs = link.getSubValues();
        if (subs.empty()) {
            ss << "''";
        }
        else {
            for (auto& sub : subs) {
                ss << "'" << sub << "',";
            }
        }
        ss << ")), ";
    }
    ss << ']';
    return ss.str();
}

DocumentObject* PropertyXLinkSubList::getValue() const
{
    if (!_Links.empty()) {
        return _Links.begin()->getValue();
    }
    return nullptr;
}

int PropertyXLinkSubList::removeValue(App::DocumentObject* lValue)
{
    atomic_change guard(*this, false);
    int ret = 0;
    for (auto it = _Links.begin(); it != _Links.end();) {
        if (it->getValue() != lValue) {
            ++it;
        }
        else {
            guard.aboutToChange();
            it = _Links.erase(it);
            ++ret;
        }
    }
    guard.tryInvoke();
    return ret;
}

PyObject* PropertyXLinkSubList::getPyObject()
{
    Py::List list;
    for (auto& link : _Links) {
        auto obj = link.getValue();
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }

        Py::Tuple tup(2);
        tup[0] = Py::asObject(obj->getPyObject());

        const auto& subs = link.getSubValues();
        Py::Tuple items(subs.size());
        for (std::size_t j = 0; j < subs.size(); j++) {
            items[j] = Py::String(subs[j]);
        }
        tup[1] = items;
        list.append(tup);
    }
    return Py::new_reference_to(list);
}

void PropertyXLinkSubList::setPyObject(PyObject* value)
{
    try {  // try PropertyLinkSub syntax
        PropertyLinkSub dummy;
        dummy.setAllowExternal(true);
        dummy.setPyObject(value);
        this->setValue(dummy.getValue(), dummy.getSubValues());
        return;
    }
    catch (Base::Exception&) {
    }

    if (!PyTuple_Check(value) && !PyList_Check(value)) {
        throw Base::TypeError(
            "Invalid type. Accepts (DocumentObject, (subname...)) or sequence of such type.");
    }
    Py::Sequence seq(value);
    std::map<DocumentObject*, std::vector<std::string>> values;
    try {
        for (Py_ssize_t i = 0; i < seq.size(); ++i) {
            PropertyLinkSub link;
            link.setAllowExternal(true);
            link.setPyObject(seq[i].ptr());
            const auto& subs = link.getSubValues();
            auto& s = values[link.getValue()];
            s.reserve(s.size() + subs.size());
            s.insert(s.end(), subs.begin(), subs.end());
        }
    }
    catch (Base::Exception&) {
        throw Base::TypeError(
            "Invalid type inside sequence. Must be type of (DocumentObject, (subname...))");
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::afterRestore()
{
    for (auto& l : _Links) {
        l.afterRestore();
    }
}

void PropertyXLinkSubList::onContainerRestored()
{
    for (auto& l : _Links) {
        l.onContainerRestored();
    }
}

void PropertyXLinkSubList::updateElementReference(DocumentObject* feature,
                                                  bool reverse,
                                                  bool notify)
{
    for (auto& l : _Links) {
        l.updateElementReference(feature, reverse, notify);
    }
}

bool PropertyXLinkSubList::referenceChanged() const
{
    for (auto& l : _Links) {
        if (l.referenceChanged()) {
            return true;
        }
    }
    return false;
}

void PropertyXLinkSubList::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<XLinkSubList count=\"" << _Links.size();
    if (testFlag(LinkAllowPartial)) {
        writer.Stream() << "\" partial=\"1";
    }
    writer.Stream() << "\">" << std::endl;
    writer.incInd();
    for (auto& l : _Links) {
        l.Save(writer);
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</XLinkSubList>" << std::endl;
}

void PropertyXLinkSubList::Restore(Base::XMLReader& reader)
{
    reader.readElement("XLinkSubList");
    setFlag(LinkAllowPartial,
            reader.hasAttribute("partial") && reader.getAttributeAsInteger("partial"));
    int count = reader.getAttributeAsInteger("count");
    atomic_change guard(*this, false);
    _Links.clear();
    for (int i = 0; i < count; ++i) {
        _Links.emplace_back(false, this);
        _Links.back().Restore(reader);
    }
    reader.readEndElement("XLinkSubList");
    guard.tryInvoke();
}

Property*
PropertyXLinkSubList::CopyOnImportExternal(const std::map<std::string, std::string>& nameMap) const
{
    std::unique_ptr<Property> copy;
    auto it = _Links.begin();
    for (; it != _Links.end(); ++it) {
        copy.reset(it->CopyOnImportExternal(nameMap));
        if (copy) {
            break;
        }
    }
    if (!copy) {
        return nullptr;
    }
    std::unique_ptr<PropertyXLinkSubList> p(new PropertyXLinkSubList);
    for (auto iter = _Links.begin(); iter != it; ++iter) {
        p->_Links.emplace_back();
        iter->copyTo(p->_Links.back());
    }
    p->_Links.emplace_back();
    static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
    for (++it; it != _Links.end(); ++it) {
        p->_Links.emplace_back();
        copy.reset(it->CopyOnImportExternal(nameMap));
        if (copy) {
            static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
        }
        else {
            it->copyTo(p->_Links.back());
        }
    }
    return p.release();
}

Property* PropertyXLinkSubList::CopyOnLabelChange(App::DocumentObject* obj,
                                                  const std::string& ref,
                                                  const char* newLabel) const
{
    std::unique_ptr<Property> copy;
    auto it = _Links.begin();
    for (; it != _Links.end(); ++it) {
        copy.reset(it->CopyOnLabelChange(obj, ref, newLabel));
        if (copy) {
            break;
        }
    }
    if (!copy) {
        return nullptr;
    }
    std::unique_ptr<PropertyXLinkSubList> p(new PropertyXLinkSubList);
    for (auto iter = _Links.begin(); iter != it; ++iter) {
        p->_Links.emplace_back();
        iter->copyTo(p->_Links.back());
    }
    p->_Links.emplace_back();
    static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
    for (++it; it != _Links.end(); ++it) {
        p->_Links.emplace_back();
        copy.reset(it->CopyOnLabelChange(obj, ref, newLabel));
        if (copy) {
            static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
        }
        else {
            it->copyTo(p->_Links.back());
        }
    }
    return p.release();
}

Property* PropertyXLinkSubList::CopyOnLinkReplace(const App::DocumentObject* parent,
                                                  App::DocumentObject* oldObj,
                                                  App::DocumentObject* newObj) const
{
    std::unique_ptr<Property> copy;
    PropertyXLinkSub* copied = nullptr;
    std::set<std::string> subs;
    auto it = _Links.begin();
    for (; it != _Links.end(); ++it) {
        copy.reset(it->CopyOnLinkReplace(parent, oldObj, newObj));
        if (copy) {
            copied = static_cast<PropertyXLinkSub*>(copy.get());
            if (copied->getValue() == newObj) {
                for (auto& sub : copied->getSubValues()) {
                    subs.insert(sub);
                }
            }
            break;
        }
    }
    if (!copy) {
        return nullptr;
    }
    std::unique_ptr<PropertyXLinkSubList> p(new PropertyXLinkSubList);
    for (auto iter = _Links.begin(); iter != it; ++iter) {
        if (iter->getValue() == newObj && copied->getValue() == newObj) {
            // merge subnames in case new object already exists
            for (auto& sub : iter->getSubValues()) {
                if (subs.insert(sub).second) {
                    copied->_SubList.push_back(sub);
                }
            }
        }
        else {
            p->_Links.emplace_back();
            iter->copyTo(p->_Links.back());
        }
    }
    p->_Links.emplace_back();
    copied->copyTo(p->_Links.back());
    copied = &p->_Links.back();
    for (++it; it != _Links.end(); ++it) {
        if ((it->getValue() == newObj || it->getValue() == oldObj)
            && copied->getValue() == newObj) {
            // merge subnames in case new object already exists
            for (auto& sub : it->getSubValues()) {
                if (subs.insert(sub).second) {
                    copied->_SubList.push_back(sub);
                }
            }
            continue;
        }
        p->_Links.emplace_back();
        copy.reset(it->CopyOnLinkReplace(parent, oldObj, newObj));
        if (copy) {
            static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
        }
        else {
            it->copyTo(p->_Links.back());
        }
    }
    return p.release();
}

Property* PropertyXLinkSubList::Copy() const
{
    PropertyXLinkSubList* p = new PropertyXLinkSubList();
    for (auto& l : _Links) {
        p->_Links.emplace_back(testFlag(LinkAllowPartial), p);
        l.copyTo(p->_Links.back());
    }
    return p;
}

void PropertyXLinkSubList::Paste(const Property& from)
{
    if (!from.isDerivedFrom<PropertyXLinkSubList>()) {
        throw Base::TypeError("Incompatible property to paste to");
    }

    aboutToSetValue();
    _Links.clear();
    for (auto& l : static_cast<const PropertyXLinkSubList&>(from)._Links) {
        _Links.emplace_back(testFlag(LinkAllowPartial), this);
        _Links.back().Paste(l);
    }
    hasSetValue();
}

unsigned int PropertyXLinkSubList::getMemSize() const
{
    unsigned int size = 0;
    for (auto& l : _Links) {
        size += l.getMemSize();
    }
    return size;
}

const std::vector<std::string>& PropertyXLinkSubList::getSubValues(App::DocumentObject* obj) const
{
    for (auto& l : _Links) {
        if (l.getValue() == obj) {
            return l.getSubValues();
        }
    }
    FC_THROWM(Base::RuntimeError, "object not found");
}

std::vector<std::string> PropertyXLinkSubList::getSubValues(App::DocumentObject* obj,
                                                            bool newStyle) const
{
    for (auto& l : _Links) {
        if (l.getValue() == obj) {
            return l.getSubValues(newStyle);
        }
    }
    return {};
}

void PropertyXLinkSubList::getLinks(std::vector<App::DocumentObject*>& objs,
                                    bool all,
                                    std::vector<std::string>* subs,
                                    bool newStyle) const
{
    if (all || _pcScope != LinkScope::Hidden) {
        if (!subs) {
            objs.reserve(objs.size() + _Links.size());
            for (auto& l : _Links) {
                auto obj = l.getValue();
                if (obj && obj->isAttachedToDocument()) {
                    objs.push_back(obj);
                }
            }
            return;
        }
        size_t count = 0;
        for (auto& l : _Links) {
            auto obj = l.getValue();
            if (obj && obj->isAttachedToDocument()) {
                count += std::max((int)l.getSubValues().size(), 1);
            }
        }
        if (!count) {
            objs.reserve(objs.size() + _Links.size());
            for (auto& l : _Links) {
                auto obj = l.getValue();
                if (obj && obj->isAttachedToDocument()) {
                    objs.push_back(obj);
                }
            }
            return;
        }

        objs.reserve(objs.size() + count);
        subs->reserve(subs->size() + count);
        for (auto& l : _Links) {
            auto obj = l.getValue();
            if (obj && obj->isAttachedToDocument()) {
                auto subnames = l.getSubValues(newStyle);
                if (subnames.empty()) {
                    subnames.emplace_back("");
                }
                for (auto& sub : subnames) {
                    objs.push_back(obj);
                    subs->push_back(std::move(sub));
                }
            }
        }
    }
}

// Same algorithm as _getLinksTo above, but returns all matches
void PropertyXLinkSubList::_getLinksToList(
    std::vector<App::ObjectIdentifier>& identifiers,
    App::DocumentObject* obj,
    const char* subname,
    const std::vector<std::string>& subs,
    const std::vector<PropertyLinkBase::ShadowSub>& shadows) const
{
    if (!subname) {
        identifiers.emplace_back(*this);
        return;
    }
    App::SubObjectT objT(obj, subname);
    auto subObject = objT.getSubObject();
    auto subElement = objT.getOldElementName();

    int i = -1;
    for (const auto& sub : subs) {
        ++i;
        if (sub == subname) {
            identifiers.emplace_back(*this, i);
            continue;
        }
        if (!subObject) {
            continue;
        }
        // There is a subobject and the subname doesn't match our current entry
        App::SubObjectT sobjT(obj, sub.c_str());
        if (sobjT.getSubObject() == subObject && sobjT.getOldElementName() == subElement) {
            identifiers.emplace_back(*this, i);
            continue;
        }
        // The oldElementName ( short, I.E. "Edge5" ) doesn't match.
        if (i < (int)shadows.size()) {
            const auto& [shadowNewName, shadowOldName] = shadows[i];
            if (shadowNewName == subname || shadowOldName == subname) {
                identifiers.emplace_back(*this, i);
                continue;
            }
            if (!subObject) {
                continue;
            }
            App::SubObjectT shadowobjT(obj,
                                       shadowNewName.empty() ? shadowOldName.c_str()
                                                             : shadowNewName.c_str());
            if (shadowobjT.getSubObject() == subObject
                && shadowobjT.getOldElementName() == subElement) {
                identifiers.emplace_back(*this, i);
                continue;
            }
        }
    }
}

void PropertyXLinkSubList::getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                                      App::DocumentObject* obj,
                                      const char* subname,
                                      bool all) const
{
    if (!all && _pcScope != LinkScope::Hidden) {
        return;
    }
    for (auto& l : _Links) {
        if (obj && obj == l._pcLink) {
            _getLinksToList(identifiers, obj, subname, l._SubList, l._ShadowSubList);
        }
    }
}

void PropertyXLinkSubList::breakLink(App::DocumentObject* obj, bool clear)
{
    if (clear && getContainer() == obj) {
        setValue(nullptr);
        return;
    }
    atomic_change guard(*this, false);
    for (auto& l : _Links) {
        if (l.getValue() == obj) {
            guard.aboutToChange();
            l.setValue(nullptr);
        }
    }
    guard.tryInvoke();
}

bool PropertyXLinkSubList::adjustLink(const std::set<App::DocumentObject*>& inList)
{
    if (_pcScope == LinkScope::Hidden) {
        return false;
    }
    std::map<App::DocumentObject*, std::vector<std::string>> values;
    bool touched = false;
    int count = 0;
    for (auto& l : _Links) {
        auto obj = l.getValue();
        if (!obj || !obj->isAttachedToDocument()) {
            ++count;
            continue;
        }
        if (inList.count(obj) && adjustLinkSubs(this, inList, obj, l._SubList, &values)) {
            touched = true;
        }
    }
    if (touched) {
        decltype(_Links) tmp;
        if (count) {
            // XLink allows detached state, i.e. with closed external document. So
            // we need to preserve empty link
            for (auto it = _Links.begin(), itNext = it; it != _Links.end(); it = itNext) {
                ++itNext;
                if (!it->getValue()) {
                    tmp.splice(tmp.end(), _Links, it);
                }
            }
        }
        setValues(std::move(values));
        _Links.splice(_Links.end(), tmp);
    }
    return touched;
}

int PropertyXLinkSubList::checkRestore(std::string* msg) const
{
    for (auto& l : _Links) {
        int res;
        if ((res = l.checkRestore(msg))) {
            return res;
        }
    }
    return 0;
}

bool PropertyXLinkSubList::upgrade(Base::XMLReader& reader, const char* typeName)
{
    if (strcmp(typeName, PropertyLinkListGlobal::getClassTypeId().getName()) == 0
        || strcmp(typeName, PropertyLinkList::getClassTypeId().getName()) == 0
        || strcmp(typeName, PropertyLinkListChild::getClassTypeId().getName()) == 0) {
        PropertyLinkList linkProp;
        linkProp.setContainer(getContainer());
        linkProp.Restore(reader);
        setValues(linkProp.getValues());
        return true;
    }
    else if (strcmp(typeName, PropertyLinkSubListGlobal::getClassTypeId().getName()) == 0
             || strcmp(typeName, PropertyLinkSubList::getClassTypeId().getName()) == 0
             || strcmp(typeName, PropertyLinkSubListChild::getClassTypeId().getName()) == 0) {
        PropertyLinkSubList linkProp;
        linkProp.setContainer(getContainer());
        linkProp.Restore(reader);
        std::map<DocumentObject*, std::vector<std::string>> values;
        const auto& objs = linkProp.getValues();
        const auto& subs = linkProp.getSubValues();
        assert(objs.size() == subs.size());
        for (size_t i = 0; i < objs.size(); ++i) {
            values[objs[i]].push_back(subs[i]);
        }
        setValues(std::move(values));
        return true;
    }
    _Links.clear();
    _Links.emplace_back(testFlag(LinkAllowPartial), this);
    if (!_Links.back().upgrade(reader, typeName)) {
        _Links.clear();
        return false;
    }
    return true;
}

void PropertyXLinkSubList::setAllowPartial(bool enable)
{
    setFlag(LinkAllowPartial, enable);
    for (auto& l : _Links) {
        l.setAllowPartial(enable);
    }
}

void PropertyXLinkSubList::hasSetChildValue(Property&)
{
    if (!signalCounter) {
        hasSetValue();
    }
}

void PropertyXLinkSubList::aboutToSetChildValue(Property&)
{
    if (!signalCounter || !hasChanged) {
        aboutToSetValue();
        if (signalCounter) {
            hasChanged = true;
        }
    }
}

std::vector<App::DocumentObject*> PropertyXLinkSubList::getValues() const
{
    std::vector<DocumentObject*> xLinks;
    getLinks(xLinks);
    return (xLinks);
}

//**************************************************************************
// PropertyXLinkList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyXLinkList, App::PropertyXLinkSubList)

//**************************************************************************
// Construction/Destruction

PropertyXLinkList::PropertyXLinkList() = default;

PropertyXLinkList::~PropertyXLinkList() = default;

PyObject* PropertyXLinkList::getPyObject()
{
    for (auto& link : _Links) {
        auto obj = link.getValue();
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }
        if (link.hasSubName()) {
            return PropertyXLinkSubList::getPyObject();
        }
    }

    Py::List list;
    for (auto& link : _Links) {
        auto obj = link.getValue();
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }
        list.append(Py::asObject(obj->getPyObject()));
    }
    return Py::new_reference_to(list);
}

void PropertyXLinkList::setPyObject(PyObject* value)
{
    try {  // try PropertyLinkList syntax
        PropertyLinkList dummy;
        dummy.setAllowExternal(true);
        dummy.setPyObject(value);
        this->setValues(dummy.getValues());
        return;
    }
    catch (Base::Exception&) {
    }

    PropertyXLinkSubList::setPyObject(value);
}

//**************************************************************************
// PropertyXLinkContainer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyXLinkContainer, App::PropertyLinkBase)

PropertyXLinkContainer::PropertyXLinkContainer()
{
    _pcScope = LinkScope::Global;
    _LinkRestored = false;
}

PropertyXLinkContainer::~PropertyXLinkContainer() = default;

void PropertyXLinkContainer::afterRestore()
{
    _DocMap.clear();
    if (!_XLinkRestores) {
        return;
    }
    _Deps.clear();
    for (auto& info : *_XLinkRestores) {
        auto obj = info.xlink->getValue();
        if (!obj) {
            continue;
        }
        if (!info.docName.empty()) {
            if (info.docName != obj->getDocument()->getName()) {
                _DocMap[info.docName] = obj->getDocument()->getName();
            }
            if (info.docLabel != obj->getDocument()->Label.getValue()) {
                _DocMap[App::quote(info.docLabel)] = obj->getDocument()->Label.getValue();
            }
        }
        if (_Deps.insert(std::make_pair(obj, info.xlink->getScope() == LinkScope::Hidden)).second) {
            _XLinks[obj->getFullName()] = std::move(info.xlink);
            onAddDep(obj);
        }
    }
    _XLinkRestores.reset();
}

void PropertyXLinkContainer::breakLink(App::DocumentObject* obj, bool clear)
{
    if (!obj || !obj->isAttachedToDocument()) {
        return;
    }
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if (!owner || !owner->isAttachedToDocument()) {
        return;
    }
    if (!clear || obj != owner) {
        auto it = _Deps.find(obj);
        if (it == _Deps.end()) {
            return;
        }
        aboutToSetValue();
        onBreakLink(obj);
        if (obj->getDocument() != owner->getDocument()) {
            _XLinks.erase(obj->getFullName());
        }
        else if (!it->second) {
            obj->_removeBackLink(owner);
        }
        _Deps.erase(it);
        onRemoveDep(obj);
        hasSetValue();
        return;
    }
    if (obj != owner) {
        return;
    }
    for (auto& v : _Deps) {
        auto key = v.first;
        if (!key || !key->isAttachedToDocument()) {
            continue;
        }
        onBreakLink(key);
        if (!v.second && key->getDocument() == owner->getDocument()) {
            key->_removeBackLink(owner);
        }
    }
    _XLinks.clear();
    _Deps.clear();
}

int PropertyXLinkContainer::checkRestore(std::string* msg) const
{
    if (_LinkRestored) {
        for (auto& v : _XLinks) {
            int res = v.second->checkRestore(msg);
            if (res) {
                return res;
            }
        }
    }
    return 0;
}

void PropertyXLinkContainer::Save(Base::Writer& writer) const
{

    writer.Stream() << writer.ind() << "<XLinks count=\"" << _XLinks.size();

    std::map<App::Document*, int> docSet;
    auto owner = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if (owner && !owner->isExporting()) {
        // Document name and label can change on restore, we shall record the
        // current document name and label and pair it with the associated
        // xlink, so that we can restore them correctly.
        int i = -1;
        for (auto& v : _XLinks) {
            ++i;
            auto obj = v.second->getValue();
            if (obj && obj->getDocument()) {
                docSet.insert(std::make_pair(obj->getDocument(), i));
            }
        }

        if (!docSet.empty()) {
            writer.Stream() << "\" docs=\"" << docSet.size();
        }
    }

    std::ostringstream ss;
    int hidden = 0;
    int i = -1;
    for (auto& v : _XLinks) {
        ++i;
        if (v.second->getScope() == LinkScope::Hidden) {
            ss << i << ' ';
            ++hidden;
        }
    }
    if (hidden) {
        writer.Stream() << "\" hidden=\"" << ss.str();
    }

    writer.Stream() << "\">" << std::endl;
    writer.incInd();

    for (auto& v : docSet) {
        writer.Stream() << writer.ind() << "<DocMap "
                        << "name=\"" << v.first->getName() << "\" label=\""
                        << encodeAttribute(v.first->Label.getValue()) << "\" index=\"" << v.second
                        << "\"/>" << std::endl;
    }

    for (auto& v : _XLinks) {
        v.second->Save(writer);
    }
    writer.decInd();

    writer.Stream() << writer.ind() << "</XLinks>" << std::endl;
}

void PropertyXLinkContainer::Restore(Base::XMLReader& reader)
{
    reader.readElement("XLinks");
    auto count = reader.getAttributeAsUnsigned("count");
    _XLinkRestores = std::make_unique<std::vector<RestoreInfo>>(count);

    if (reader.hasAttribute("hidden")) {
        std::istringstream iss(reader.getAttribute("hidden"));
        int index;
        while (iss >> index) {
            if (index >= 0 && index < static_cast<int>(count)) {
                _XLinkRestores->at(index).hidden = true;
            }
        }
    }

    if (reader.hasAttribute("docs")) {
        auto docCount = reader.getAttributeAsUnsigned("docs");
        _DocMap.clear();
        for (unsigned i = 0; i < docCount; ++i) {
            reader.readElement("DocMap");
            auto index = reader.getAttributeAsUnsigned("index");
            if (index >= count) {
                FC_ERR(propertyName(this) << " invalid document map entry");
                continue;
            }
            auto& info = _XLinkRestores->at(index);
            info.docName = reader.getAttribute("name");
            info.docLabel = reader.getAttribute("label");
        }
    }

    for (auto& info : *_XLinkRestores) {
        info.xlink.reset(createXLink());
        if (info.hidden) {
            info.xlink->setScope(LinkScope::Hidden);
        }
        info.xlink->Restore(reader);
    }
    reader.readEndElement("XLinks");
}

void PropertyXLinkContainer::aboutToSetChildValue(App::Property& prop)
{
    auto xlink = dynamic_cast<App::PropertyXLink*>(&prop);
    if (xlink && xlink->testFlag(LinkDetached)) {
        auto obj = const_cast<App::DocumentObject*>(xlink->getValue());
        if (_Deps.erase(obj)) {
            _onBreakLink(xlink->getValue());
            onRemoveDep(obj);
        }
    }
}

void PropertyXLinkContainer::onBreakLink(DocumentObject*)
{}

void PropertyXLinkContainer::_onBreakLink(DocumentObject* obj)
{
    try {
        onBreakLink(obj);
    }
    catch (Base::Exception& e) {
        e.ReportException();
        FC_ERR("Exception on breaking link property " << getFullName());
    }
    catch (std::exception& e) {
        FC_ERR("Exception on breaking link property " << getFullName() << ": " << e.what());
    }
    catch (...) {
        FC_ERR("Exception on breaking link property " << getFullName());
    }
}

PropertyXLink* PropertyXLinkContainer::createXLink()
{
    return new PropertyXLink(false, this);
}

bool PropertyXLinkContainer::isLinkedToDocument(const App::Document& doc) const
{
    auto iter = _XLinks.lower_bound(doc.getName());
    if (iter != _XLinks.end()) {
        size_t len = strlen(doc.getName());
        return iter->first.size() > len && iter->first[len] == '#'
            && boost::starts_with(iter->first, doc.getName());
    }
    return false;
}

void PropertyXLinkContainer::updateDeps(std::map<DocumentObject*, bool>&& newDeps)
{
    auto owner = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if (!owner || !owner->isAttachedToDocument()) {
        return;
    }
    newDeps.erase(owner);

    for (auto& v : newDeps) {
        auto obj = v.first;
        if (obj && obj->isAttachedToDocument()) {
            auto it = _Deps.find(obj);
            if (it != _Deps.end()) {
                if (v.second != it->second) {
                    if (v.second) {
                        obj->_removeBackLink(owner);
                    }
                    else {
                        obj->_addBackLink(owner);
                    }
                }
                _Deps.erase(it);
                continue;
            }
            if (owner->getDocument() != obj->getDocument()) {
                auto& xlink = _XLinks[obj->getFullName()];
                if (!xlink) {
                    xlink.reset(createXLink());
                    xlink->setValue(obj);
                }
                xlink->setScope(v.second ? LinkScope::Hidden : LinkScope::Global);
            }
            else if (!v.second) {
                obj->_addBackLink(owner);
            }

            onAddDep(obj);
        }
    }
    for (auto& v : _Deps) {
        auto obj = v.first;
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }
        if (obj->getDocument() == owner->getDocument()) {
            if (!v.second) {
                obj->_removeBackLink(owner);
            }
        }
        else {
            _XLinks.erase(obj->getFullName());
        }
        onRemoveDep(obj);
    }
    _Deps = std::move(newDeps);

    _LinkRestored = testFlag(LinkRestoring);

    if (!_LinkRestored && !testFlag(LinkDetached)) {
        for (auto it = _XLinks.begin(), itNext = it; it != _XLinks.end(); it = itNext) {
            ++itNext;
            if (!it->second->getValue()) {
                _XLinks.erase(it);
            }
        }
    }
}

void PropertyXLinkContainer::clearDeps()
{
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if (!owner || !owner->isAttachedToDocument()) {
        return;
    }
#ifndef USE_OLD_DAG
    if (!owner->testStatus(ObjectStatus::Destroy)) {
        for (auto& v : _Deps) {
            auto obj = v.first;
            if (!v.second && obj && obj->isAttachedToDocument()
                && obj->getDocument() == owner->getDocument()) {
                obj->_removeBackLink(owner);
            }
        }
    }
#endif
    _Deps.clear();
    _XLinks.clear();
    _LinkRestored = false;
}

void PropertyXLinkContainer::getLinks(std::vector<App::DocumentObject*>& objs,
                                      bool all,
                                      std::vector<std::string>* /*subs*/,
                                      bool /*newStyle*/) const
{
    for (auto& v : _Deps) {
        if (all || !v.second) {
            objs.push_back(v.first);
        }
    }
}

}  // namespace App