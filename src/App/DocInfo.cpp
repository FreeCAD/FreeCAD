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
    #include <string>
#endif

#include <QDir>
#include <QString>

#include <Base/Console.h>

#include "Application.h"
#include "DocInfo.h"
#include "Document.h"
#include "DocumentObject.h"
#include "PropertyXLink.h"

FC_LOG_LEVEL_INIT("DocInfo", true, true)

namespace {
App::DocInfoMap _DocInfoMap;
}

namespace App {

DocInfoMap DocInfo::getMap()
{
    return _DocInfoMap;
}

std::string DocInfo::getDocPath(const char* filename,
                                App::Document* pDoc,
                                bool relative,
                                QString* fullPath)
{
    bool absolute;
    // The path could be an URI, in that case
    // TODO: build a far much more resilient approach to test for an URI
    QString path = QString::fromUtf8(filename);
    if (path.startsWith(QLatin1String("https://"))) {
        // We do have an URI
        if (fullPath) {
            *fullPath = path;
        }
        return std::string(filename);
    }

    // make sure the filename is absolute path
    path = QDir::cleanPath(path);
    if ((absolute = QFileInfo(path).isAbsolute())) {
        if (fullPath) {
            *fullPath = path;
        }
        if (!relative) {
            return std::string(path.toUtf8().constData());
        }
    }

    const char* docPath = pDoc->getFileName();
    if (!docPath || *docPath == 0) {
        throw Base::RuntimeError("Owner document not saved");
    }

    QDir docDir(QFileInfo(QString::fromUtf8(docPath)).absoluteDir());
    if (!absolute) {
        path = QDir::cleanPath(docDir.absoluteFilePath(path));
        if (fullPath) {
            *fullPath = path;
        }
    }

    if (relative) {
        return std::string(docDir.relativeFilePath(path).toUtf8().constData());
    }
    else {
        return std::string(path.toUtf8().constData());
    }
}


DocInfoPtr DocInfo::get(const char* filename,
                    App::Document* pDoc,
                    PropertyXLink* l,
                    const char* objName)
{
    QString path;
    l->filePath = getDocPath(filename, pDoc, true, &path);

    FC_LOG("finding doc " << filename);

    auto it = _DocInfoMap.find(path);
    DocInfoPtr info;
    if (it != _DocInfoMap.end()) {
        info = it->second;
        if (!info->pcDoc) {
            QString fullpath(info->getFullPath());
            if (fullpath.size()
                && App::GetApplication().addPendingDocument(
                        fullpath.toUtf8().constData(),
                        objName,
                        l->testFlag(PropertyLinkBase::LinkAllowPartial))
                    == 0) {
                for (App::Document* doc : App::GetApplication().getDocuments()) {
                    if (getFullPath(doc->getFileName()) == fullpath) {
                        info->attach(doc);
                        break;
                    }
                }
            }
        }
    }
    else {
        info = std::make_shared<DocInfo>();
        auto ret = _DocInfoMap.insert(std::make_pair(path, info));
        info->init(ret.first, objName, l);
    }

    if (info->pcDoc) {
        // make sure to attach only external object
        auto owner = Base::freecad_dynamic_cast<DocumentObject>(l->getContainer());
        if (owner && owner->getDocument() == info->pcDoc) {
            return info;
        }
    }

    info->links.insert(l);
    return info;
}


QString DocInfo::getFullPath(const char* p)
{
    QString path = QString::fromUtf8(p);
    if (path.isEmpty()) {
        return path;
    }

    if (path.startsWith(QLatin1String("https://"))) {
        return path;
    }
    else {
        return QFileInfo(path).absoluteFilePath();
    }
}


QString DocInfo::getFullPath() const
{
    QString path = myPos->first;
    if (path.startsWith(QLatin1String("https://"))) {
        return path;
    }
    else {
        return QFileInfo(myPos->first).absoluteFilePath();
    }
}

void DocInfo::deinit()
{
    FC_LOG("deinit " << (pcDoc ? pcDoc->getName() : filePath()));
    assert(links.empty());
    connFinishRestoreDocument.disconnect();
    connPendingReloadDocument.disconnect();
    connDeleteDocument.disconnect();
    connSaveDocument.disconnect();
    connDeletedObject.disconnect();

    auto me = shared_from_this();
    _DocInfoMap.erase(myPos);
    myPos = _DocInfoMap.end();
    myPath.clear();
    pcDoc = nullptr;
}

void DocInfo::init(DocInfoMap::iterator pos, const char* objName, PropertyXLink* l)
{
    myPos = pos;
    myPath = myPos->first.toUtf8().constData();
    App::Application& app = App::GetApplication();
    // NOLINTBEGIN
    connFinishRestoreDocument = app.signalFinishRestoreDocument.connect(
        std::bind(&DocInfo::slotFinishRestoreDocument, this, std::placeholders::_1));
    connPendingReloadDocument = app.signalPendingReloadDocument.connect(
        std::bind(&DocInfo::slotFinishRestoreDocument, this, std::placeholders::_1));
    connDeleteDocument =
        app.signalDeleteDocument.connect(std::bind(&DocInfo::slotDeleteDocument, this, std::placeholders::_1));
    connSaveDocument =
        app.signalSaveDocument.connect(std::bind(&DocInfo::slotSaveDocument, this, std::placeholders::_1));
    // NOLINTEND

    QString fullpath(getFullPath());
    if (fullpath.isEmpty()) {
        FC_ERR("document not found " << filePath());
    }
    else {
        for (App::Document* doc : App::GetApplication().getDocuments()) {
            if (getFullPath(doc->getFileName()) == fullpath) {
                if (doc->testStatus(App::Document::PartialDoc) && !doc->getObject(objName)) {
                    break;
                }
                attach(doc);
                return;
            }
        }
        FC_LOG("document pending " << filePath());
        app.addPendingDocument(fullpath.toUtf8().constData(),
                                objName,
                                l->testFlag(PropertyLinkBase::LinkAllowPartial));
    }
}


void DocInfo::attach(Document* doc)
{
    assert(!pcDoc);
    pcDoc = doc;
    FC_LOG("attaching " << doc->getName() << ", " << doc->getFileName());
    std::map<App::PropertyLinkBase*, std::vector<App::PropertyXLink*>> parentLinks;
    for (auto it = links.begin(), itNext = it; it != links.end(); it = itNext) {
        ++itNext;
        auto link = *it;
        if (link->_pcLink) {
            continue;
        }
        if (link->parentProp) {
            parentLinks[link->parentProp].push_back(link);
            continue;
        }
        auto obj = doc->getObject(link->objectName.c_str());
        if (obj) {
            link->restoreLink(obj);
        }
        else if (doc->testStatus(App::Document::PartialDoc)) {
            App::GetApplication().addPendingDocument(doc->FileName.getValue(),
                                                        link->objectName.c_str(),
                                                        false);
            FC_WARN("reloading partial document '" << doc->FileName.getValue()
                                                    << "' due to object " << link->objectName);
        }
        else {
            FC_WARN("object '" << link->objectName << "' not found in document '"
                                << doc->getName() << "'");
        }
    }
    for (auto& v : parentLinks) {
        v.first->setFlag(PropertyLinkBase::LinkRestoring);
        v.first->aboutToSetValue();
        for (auto link : v.second) {
            auto obj = doc->getObject(link->objectName.c_str());
            if (obj) {
                link->restoreLink(obj);
            }
            else if (doc->testStatus(App::Document::PartialDoc)) {
                App::GetApplication().addPendingDocument(doc->FileName.getValue(),
                                                            link->objectName.c_str(),
                                                            false);
                FC_WARN("reloading partial document '"
                        << doc->FileName.getValue() << "' due to object " << link->objectName);
            }
            else {
                FC_WARN("object '" << link->objectName << "' not found in document '"
                                    << doc->getName() << "'");
            }
        }
        v.first->hasSetValue();
        v.first->setFlag(PropertyLinkBase::LinkRestoring, false);
    }
}

void DocInfo::restoreDocument(const App::Document& doc)
{
    auto it = _DocInfoMap.find(getFullPath(doc.FileName.getValue()));
    if (it == _DocInfoMap.end()) {
        return;
    }
    it->second->slotFinishRestoreDocument(doc);
}

void DocInfo::slotFinishRestoreDocument(const App::Document& doc)
{
    if (pcDoc) {
        return;
    }
    QString fullpath(getFullPath());
    if (!fullpath.isEmpty() && getFullPath(doc.getFileName()) == fullpath) {
        attach(const_cast<App::Document*>(&doc));
    }
}

void DocInfo::slotSaveDocument(const App::Document& doc)
{
    if (!pcDoc) {
        slotFinishRestoreDocument(doc);
        return;
    }
    if (&doc != pcDoc) {
        return;
    }

    QFileInfo info(myPos->first);
    QString path(info.absoluteFilePath());
    const char* filename = doc.getFileName();
    QString docPath(getFullPath(filename));

    if (path.isEmpty() || path != docPath) {
        FC_LOG("document '" << doc.getName() << "' path changed");
        auto me = shared_from_this();
        auto ret = _DocInfoMap.insert(std::make_pair(docPath, me));
        if (!ret.second) {
            // is that even possible?
            FC_WARN("document '" << doc.getName() << "' path exists, detach");
            slotDeleteDocument(doc);
            return;
        }
        _DocInfoMap.erase(myPos);
        myPos = ret.first;

        std::set<PropertyXLink*> tmp;
        tmp.swap(links);
        for (auto link : tmp) {
            auto owner = static_cast<DocumentObject*>(link->getContainer());
            // adjust file path for each PropertyXLink
            DocInfo::get(filename, owner->getDocument(), link, link->objectName.c_str());
        }
    }

    // time stamp changed, touch the linking document.
    std::set<Document*> docs;
    for (auto link : links) {
        auto linkdoc = static_cast<DocumentObject*>(link->getContainer())->getDocument();
        auto ret = docs.insert(linkdoc);
        if (ret.second) {
            // This will signal the Gui::Document to call setModified();
            FC_LOG("touch document " << linkdoc->getName() << " on time stamp change of "
                                        << link->getFullName());
            linkdoc->Comment.touch();
        }
    }
}

void DocInfo::slotDeleteDocument(const App::Document& doc)
{
    for (auto it = links.begin(), itNext = it; it != links.end(); it = itNext) {
        ++itNext;
        auto link = *it;
        auto obj = dynamic_cast<DocumentObject*>(link->getContainer());
        if (obj && obj->getDocument() == &doc) {
            links.erase(it);
            // must call unlink here, so that PropertyLink::resetLink can
            // remove back link before the owner object is marked as being
            // destroyed
            link->unlink();
        }
    }
    if (links.empty()) {
        deinit();
        return;
    }
    if (pcDoc != &doc) {
        return;
    }
    std::map<App::PropertyLinkBase*, std::vector<App::PropertyXLink*>> parentLinks;
    for (auto link : links) {
        link->setFlag(PropertyLinkBase::LinkDetached);
        if (link->parentProp) {
            parentLinks[link->parentProp].push_back(link);
        }
        else {
            parentLinks[nullptr].push_back(link);
        }
    }
    for (auto& v : parentLinks) {
        if (v.first) {
            v.first->setFlag(PropertyLinkBase::LinkDetached);
            v.first->aboutToSetValue();
        }
        for (auto l : v.second) {
            l->detach();
        }
        if (v.first) {
            v.first->hasSetValue();
            v.first->setFlag(PropertyLinkBase::LinkDetached, false);
        }
    }
    pcDoc = nullptr;
}

bool DocInfo::hasXLink(const App::Document* doc) const
{
    for (auto link : links) {
        auto obj = dynamic_cast<DocumentObject*>(link->getContainer());
        if (obj && obj->getDocument() == doc) {
            return true;
        }
    }
    return false;
}

void DocInfo::breakLinks(App::DocumentObject* obj, bool clear)
{
    auto doc = obj->getDocument();
    for (auto itD = _DocInfoMap.begin(), itDNext = itD; itD != _DocInfoMap.end();
            itD = itDNext) {
        ++itDNext;
        auto docInfo = itD->second;
        if (docInfo->pcDoc != doc) {
            continue;
        }
        auto& links = docInfo->links;
        std::set<PropertyLinkBase*> parentLinks;
        for (auto it = links.begin(), itNext = it; it != links.end(); it = itNext) {
            ++itNext;
            auto link = *it;
            if (link->_pcLink != obj && !(clear && link->getContainer() == obj)) {
                continue;
            }
            if (link->parentProp) {
                parentLinks.insert(link->parentProp);
            }
            else {
                link->breakLink(obj, clear);
            }
        }
        for (auto link : parentLinks) {
            link->breakLink(obj, clear);
        }
    }
}

}  // namespace App