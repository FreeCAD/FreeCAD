// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include <array>
#include <set>

#include <QApplication>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/Link.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include "Selection.h"
#include "SelectionObject.h"
#include "Application.h"
#include "Document.h"
#include "Macro.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "SelectionFilter.h"
#include "Tree.h"
#include "ViewProvider.h"


FC_LOG_LEVEL_INIT("Selection", false, true, true)

using namespace Gui;
using namespace std;
namespace sp = std::placeholders;

SelectionGateFilterExternal::SelectionGateFilterExternal(const char* docName, const char* objName)
{
    if (docName) {
        DocName = docName;
        if (objName) {
            ObjName = objName;
        }
    }
}

bool SelectionGateFilterExternal::allow(App::Document* doc, App::DocumentObject* obj, const char*)
{
    if (!doc || !obj) {
        return true;
    }
    if (!DocName.empty() && doc->getName() != DocName) {
        notAllowedReason = "Cannot select external object";
    }
    else if (!ObjName.empty() && ObjName == obj->getNameInDocument()) {
        notAllowedReason = "Cannot select self";
    }
    else {
        return true;
    }
    return false;
}

bool SelectionSingleton::hasSelection() const
{
    return !_SelList.empty();
}
bool SelectionSingleton::hasPreselection() const
{
    return !CurrentPreselection.Object.getObjectName().empty();
}

std::vector<SelectionSingleton::SelObj> SelectionSingleton::getCompleteSelection(ResolveMode resolve) const
{
    return getSelection("*", resolve);
}

std::vector<App::SubObjectT> SelectionSingleton::getSelectionT(
    const char* pDocName,
    ResolveMode resolve,
    bool single
) const
{
    auto sels = getSelection(pDocName, resolve, single);
    std::vector<App::SubObjectT> res;
    res.reserve(sels.size());
    for (auto& sel : sels) {
        res.emplace_back(sel.pObject, sel.SubName);
    }
    return res;
}


std::vector<SelectionSingleton::SelObj> SelectionSingleton::getSelection(
    const char* pDocName,
    ResolveMode resolve,
    bool single
) const
{
    std::vector<SelObj> temp;
    if (single) {
        temp.reserve(1);
    }
    SelObj tempSelObj;
    App::Document* pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName, "*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc) {
            return temp;
        }
    }

    std::map<App::DocumentObject*, std::set<std::string>> objMap;

    for (auto& sel : _SelList) {
        if (!sel.pDoc) {
            continue;
        }
        const char* subelement = nullptr;
        auto obj = getObjectOfType(sel, App::DocumentObject::getClassTypeId(), resolve, &subelement);
        if (!obj || (pcDoc && sel.pObject->getDocument() != pcDoc)) {
            continue;
        }

        // In case we are resolving objects, make sure no duplicates
        if (resolve != ResolveMode::NoResolve
            && !objMap[obj].insert(std::string(subelement ? subelement : "")).second) {
            continue;
        }

        if (single && !temp.empty()) {
            temp.clear();
            break;
        }

        tempSelObj.DocName = obj->getDocument()->getName();
        tempSelObj.FeatName = obj->getNameInDocument();
        tempSelObj.SubName = subelement;
        tempSelObj.TypeName = obj->getTypeId().getName();
        tempSelObj.pObject = obj;
        tempSelObj.pResolvedObject = sel.pResolvedObject;
        tempSelObj.pDoc = obj->getDocument();
        tempSelObj.x = sel.x;
        tempSelObj.y = sel.y;
        tempSelObj.z = sel.z;

        temp.push_back(tempSelObj);
    }

    return temp;
}

bool SelectionSingleton::hasSelection(const char* pDocName, ResolveMode resolve) const
{
    App::Document* pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName, "*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc) {
            return false;
        }
    }
    for (auto& sel : _SelList) {
        if (!sel.pDoc) {
            continue;
        }
        auto obj = getObjectOfType(sel, App::DocumentObject::getClassTypeId(), resolve);
        if (obj && (!pcDoc || sel.pObject->getDocument() == pcDoc)) {
            return true;
        }
    }

    return false;
}

bool SelectionSingleton::hasSubSelection(const char* pDocName, bool subElement) const
{
    App::Document* pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName, "*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc) {
            return false;
        }
    }
    for (auto& sel : _SelList) {
        if (pcDoc && pcDoc != sel.pDoc) {
            continue;
        }
        if (sel.SubName.empty()) {
            continue;
        }
        if (subElement && sel.SubName.back() != '.') {
            return true;
        }
        if (sel.pObject != sel.pResolvedObject) {
            return true;
        }
    }

    return false;
}

std::vector<SelectionSingleton::SelObj> SelectionSingleton::getPickedList(const char* pDocName) const
{
    std::vector<SelObj> temp;
    SelObj tempSelObj;

    App::Document* pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName, "*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc) {
            return {};
        }
    }

    for (std::list<_SelObj>::const_iterator It = _PickedList.begin(); It != _PickedList.end(); ++It) {
        if (!pcDoc || It->pDoc == pcDoc) {
            tempSelObj.DocName = It->DocName.c_str();
            tempSelObj.FeatName = It->FeatName.c_str();
            tempSelObj.SubName = It->SubName.c_str();
            tempSelObj.TypeName = It->TypeName.c_str();
            tempSelObj.pObject = It->pObject;
            tempSelObj.pResolvedObject = It->pResolvedObject;
            tempSelObj.pDoc = It->pDoc;
            tempSelObj.x = It->x;
            tempSelObj.y = It->y;
            tempSelObj.z = It->z;
            temp.push_back(tempSelObj);
        }
    }

    return temp;
}

std::vector<Gui::SelectionObject> SelectionSingleton::getSelectionIn(
    App::DocumentObject* container,
    Base::Type typeId,
    bool single
) const
{
    if (!container) {
        return getSelectionEx(nullptr, typeId, ResolveMode::NoResolve, single);
    }

    std::vector<SelectionObject> sels
        = getSelectionEx(nullptr, App::DocumentObject::getClassTypeId(), ResolveMode::NoResolve, single);

    std::vector<SelectionObject> ret;
    std::map<App::DocumentObject*, size_t> SortMap;

    for (auto& sel : sels) {
        auto* rootObj = sel.getObject();
        App::Document* doc = rootObj->getDocument();
        std::vector<std::string> subs = sel.getSubNames();
        bool objPassed = false;

        for (size_t i = 0; i < subs.size(); ++i) {
            auto& sub = subs[i];
            App::DocumentObject* newRootObj = nullptr;
            std::string newSub = "";

            std::vector<std::string> names = Base::Tools::splitSubName(sub);

            if (container == rootObj) {
                objPassed = true;
            }

            if (rootObj->isLink()) {
                // Update doc in case its an external link.
                doc = rootObj->getLinkedObject()->getDocument();
            }

            for (auto& name : names) {
                App::DocumentObject* obj = doc->getObject(name.c_str());
                if (!obj) {  // We reached the element name (for example 'edge1')
                    newSub += name;
                    break;
                }

                if (objPassed) {
                    if (!newRootObj) {
                        // We are the first object after the container is passed.
                        newRootObj = obj;
                    }
                    else {
                        newSub += name + ".";
                    }
                }

                if (obj == container) {
                    objPassed = true;
                }
                if (obj->isLink()) {
                    // Update doc in case its an external link.
                    doc = obj->getLinkedObject()->getDocument();
                }
            }

            if (newRootObj) {
                // Make sure selected object is of correct type
                auto* lastObj = newRootObj->resolve(newSub.c_str());
                if (!lastObj || !lastObj->isDerivedFrom(typeId)) {
                    continue;
                }

                auto it = SortMap.find(newRootObj);
                if (it != SortMap.end()) {
                    // only add sub-element
                    if (newSub != "") {
                        ret[it->second].SubNames.emplace_back(newSub);
                        ret[it->second].SelPoses.emplace_back(sel.SelPoses[i]);
                    }
                }
                else {
                    if (single && !ret.empty()) {
                        ret.clear();
                        break;
                    }
                    // create a new entry
                    ret.emplace_back(newRootObj);
                    if (newSub != "") {
                        ret.back().SubNames.emplace_back(newSub);
                        ret.back().SelPoses.emplace_back(sel.SelPoses[i]);
                    }
                    SortMap.insert(std::make_pair(newRootObj, ret.size() - 1));
                }
            }
        }
    }

    return ret;
}

std::vector<SelectionObject> SelectionSingleton::getSelectionEx(
    const char* pDocName,
    Base::Type typeId,
    ResolveMode resolve,
    bool single
) const
{
    return getObjectList(pDocName, typeId, _SelList, resolve, single);
}

std::vector<SelectionObject> SelectionSingleton::getPickedListEx(
    const char* pDocName,
    Base::Type typeId
) const
{
    return getObjectList(pDocName, typeId, _PickedList, ResolveMode::NoResolve);
}

std::vector<SelectionObject> SelectionSingleton::getObjectList(
    const char* pDocName,
    Base::Type typeId,
    const std::list<_SelObj>& objList,
    ResolveMode resolve,
    bool single
) const
{
    std::vector<SelectionObject> temp;
    if (single) {
        temp.reserve(1);
    }
    std::map<App::DocumentObject*, size_t> SortMap;

    // check the type
    if (typeId.isBad()) {
        return temp;
    }

    App::Document* pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName, "*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc) {
            return temp;
        }
    }

    for (auto& sel : objList) {
        if (!sel.pDoc) {
            continue;
        }
        const char* subelement = nullptr;
        auto obj = getObjectOfType(sel, typeId, resolve, &subelement);
        if (!obj || (pcDoc && sel.pObject->getDocument() != pcDoc)) {
            continue;
        }
        auto it = SortMap.find(obj);
        if (it != SortMap.end()) {
            // only add sub-element
            if (subelement && *subelement) {
                if (resolve != ResolveMode::NoResolve
                    && !temp[it->second]._SubNameSet.insert(subelement).second) {
                    continue;
                }
                temp[it->second].SubNames.emplace_back(subelement);
                temp[it->second].SelPoses.emplace_back(sel.x, sel.y, sel.z);
            }
        }
        else {
            if (single && !temp.empty()) {
                temp.clear();
                break;
            }
            // create a new entry
            temp.emplace_back(obj);
            if (subelement && *subelement) {
                temp.back().SubNames.emplace_back(subelement);
                temp.back().SelPoses.emplace_back(sel.x, sel.y, sel.z);
                if (resolve != ResolveMode::NoResolve) {
                    temp.back()._SubNameSet.insert(subelement);
                }
            }
            SortMap.insert(std::make_pair(obj, temp.size() - 1));
        }
    }

    return temp;
}

bool SelectionSingleton::needPickedList() const
{
    return _needPickedList;
}

SelectionSingleton::SelectionAllowance SelectionSingleton::isSelectionAllowed(const _SelObj& sel)
{
    if (!ActiveGate) {
        return {.allowed = true, .reason = ""};
    }
    const char* subelement = nullptr;
    auto pObject
        = getObjectOfType(sel, App::DocumentObject::getClassTypeId(), gateResolve, &subelement);


    if (!ActiveGate->allow(pObject ? pObject->getDocument() : sel.pDoc, pObject, subelement)) {
        std::string copyNotAllowedReason = ActiveGate->notAllowedReason;
        ActiveGate->notAllowedReason.clear();
        return {.allowed = false, .reason = copyNotAllowedReason};
    }
    return {.allowed = true, .reason = ""};
}

void SelectionSingleton::enablePickedList(bool enable)
{
    if (enable != _needPickedList) {
        _needPickedList = enable;
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }
}

static void notifyDocumentObjectViewProvider(const SelectionChanges& changes)
{
    const auto* doc = App::GetApplication().getDocument(changes.pDocName);
    if (!doc) {
        return;
    }

    const auto* obj = doc->getObject(changes.pObjectName);
    if (!obj) {
        return;
    }

    auto* vp = Application::Instance->getViewProvider(obj);
    if (!vp) {
        return;
    }

    vp->onSelectionChanged(changes);
}

void SelectionSingleton::notify(SelectionChanges&& Chng)
{
    if (Notifying) {
        NotificationQueue.push_back(std::move(Chng));
        return;
    }
    Base::FlagToggler<bool> flag(Notifying);
    NotificationQueue.push_back(std::move(Chng));
    while (!NotificationQueue.empty()) {
        const auto& msg = NotificationQueue.front();
        bool notify = false;
        switch (msg.Type) {
            case SelectionChanges::AddSelection:
                notify = isSelected(msg.pDocName, msg.pObjectName, msg.pSubName, ResolveMode::NoResolve);
                break;
            case SelectionChanges::RmvSelection:
                notify = !isSelected(msg.pDocName, msg.pObjectName, msg.pSubName, ResolveMode::NoResolve);
                break;
            case SelectionChanges::SetPreselect:
                notify = CurrentPreselection.Type == SelectionChanges::SetPreselect
                    && CurrentPreselection.Object == msg.Object;
                break;
            case SelectionChanges::RmvPreselect:
                notify = CurrentPreselection.Type == SelectionChanges::ClrSelection;
                break;
            default:
                notify = true;
        }
        if (notify) {
            // Notify the view provider of the object.
            notifyDocumentObjectViewProvider(msg);

            Notify(msg);
            try {
                signalSelectionChanged(msg);
            }
            catch (const boost::exception&) {
                // reported by code analyzers
                Base::Console().warning("notify: Unexpected boost exception\n");
            }
        }
        NotificationQueue.pop_front();
    }
}

bool SelectionSingleton::hasPickedList() const
{
    return !_PickedList.empty();
}

int SelectionSingleton::getAsPropertyLinkSubList(App::PropertyLinkSubList& prop) const
{
    std::vector<Gui::SelectionObject> sel = this->getSelectionEx();
    std::vector<App::DocumentObject*> objs;
    objs.reserve(sel.size() * 2);
    std::vector<std::string> subs;
    subs.reserve(sel.size() * 2);
    for (auto& selitem : sel) {
        App::DocumentObject* obj = selitem.getObject();
        const std::vector<std::string>& subnames = selitem.getSubNames();

        // whole object is selected
        if (subnames.empty()) {
            objs.push_back(obj);
            subs.emplace_back();
        }
        else {
            for (const auto& subname : subnames) {
                objs.push_back(obj);
                subs.push_back(subname);
            }
        }
    }
    assert(objs.size() == subs.size());
    prop.setValues(objs, subs);
    return objs.size();
}

App::DocumentObject* SelectionSingleton::getObjectOfType(
    const _SelObj& sel,
    Base::Type typeId,
    ResolveMode resolve,
    const char** subelement
)
{
    auto obj = sel.pObject;
    if (!obj || !obj->isAttachedToDocument()) {
        return nullptr;
    }
    const char* subname = sel.SubName.c_str();
    if (resolve != ResolveMode::NoResolve) {
        obj = sel.pResolvedObject;
        if (resolve == ResolveMode::NewStyleElement && !sel.elementName.newName.empty()) {
            subname = sel.elementName.newName.c_str();
        }
        else {
            subname = sel.elementName.oldName.c_str();
        }
    }

    if (!obj) {
        return nullptr;
    }

    if (!obj->isDerivedFrom(typeId)
        && (resolve != ResolveMode::FollowLink || !obj->getLinkedObject(true)->isDerivedFrom(typeId))) {
        return nullptr;
    }

    if (subelement) {
        *subelement = subname;
    }

    return obj;
}

vector<App::DocumentObject*> SelectionSingleton::getObjectsOfType(
    const Base::Type& typeId,
    const char* pDocName,
    ResolveMode resolve
) const
{
    App::Document* pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName, "*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc) {
            return {};
        }
    }

    std::vector<App::DocumentObject*> temp;
    std::set<App::DocumentObject*> objs;

    for (auto& sel : _SelList) {
        if (App::DocumentObject* pObject = getObjectOfType(sel, typeId, resolve)) {
            auto ret = objs.insert(pObject);
            if (ret.second) {
                temp.push_back(pObject);
            }
        }
    }

    return temp;
}

std::vector<App::DocumentObject*> SelectionSingleton::getObjectsOfType(
    const char* typeName,
    const char* pDocName,
    ResolveMode resolve
) const
{
    Base::Type typeId = Base::Type::fromName(typeName);
    if (typeId.isBad()) {
        return {};
    }
    return getObjectsOfType(typeId, pDocName, resolve);
}

unsigned int SelectionSingleton::countObjectsOfType(
    const Base::Type& typeId,
    const char* pDocName,
    ResolveMode resolve
) const
{
    App::Document* pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName, "*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc) {
            return 0;
        }
    }

    return std::count_if(_SelList.begin(), _SelList.end(), [&](auto& sel) {
        return (!pcDoc || pcDoc == sel.pDoc) && getObjectOfType(sel, typeId, resolve);
    });
}

unsigned int SelectionSingleton::countObjectsOfType(
    const char* typeName,
    const char* pDocName,
    ResolveMode resolve
) const
{
    Base::Type typeId = Base::Type::fromName(typeName);
    if (typeId.isBad()) {
        return 0;
    }
    return countObjectsOfType(typeId, pDocName, resolve);
}


void SelectionSingleton::slotSelectionChanged(const SelectionChanges& msg)
{
    if (msg.Type == SelectionChanges::SetPreselectSignal || msg.Type == SelectionChanges::ShowSelection
        || msg.Type == SelectionChanges::HideSelection) {
        return;
    }

    if (!msg.Object.getSubName().empty()) {
        auto pParent = msg.Object.getObject();
        if (!pParent) {
            return;
        }
        App::ElementNamePair elementName;
        auto& newElementName = elementName.newName;
        auto& oldElementName = elementName.oldName;
        auto pObject = App::GeoFeature::resolveElement(pParent, msg.pSubName, elementName);
        if (!pObject) {
            return;
        }
        SelectionChanges msg2(
            msg.Type,
            pObject->getDocument()->getName(),
            pObject->getNameInDocument(),
            !newElementName.empty() ? newElementName.c_str() : oldElementName.c_str(),
            pObject->getTypeId().getName(),
            msg.x,
            msg.y,
            msg.z,
            SelectionChanges::MsgSource::Any,
            msg.hasPickedPoint ? SelectionChanges::PickedPoint::Valid
                               : SelectionChanges::PickedPoint::Invalid
        );

        try {
            msg2.pOriginalMsg = &msg;
            signalSelectionChanged3(msg2);

            msg2.Object.setSubName(oldElementName.c_str());
            msg2.pSubName = msg2.Object.getSubName().c_str();
            signalSelectionChanged2(msg2);
        }
        catch (const boost::exception&) {
            // reported by code analyzers
            Base::Console().warning("slotSelectionChanged: Unexpected boost exception\n");
        }
    }
    else {
        try {
            signalSelectionChanged3(msg);
            signalSelectionChanged2(msg);
        }
        catch (const boost::exception&) {
            // reported by code analyzers
            Base::Console().warning("slotSelectionChanged: Unexpected boost exception\n");
        }
    }
}

bool SelectionSingleton::testSelection(
    App::Document* pDoc,
    App::DocumentObject* pObject,
    const char* pSubName
) const
{
    if (!pObject) {
        return false;
    }

    if (!pDoc) {
        pDoc = pObject->getDocument();
    }
    if (!pDoc) {
        return false;
    }

    if (!ActiveGate) {
        return true;
    }

    const char* objectName = pObject->getNameInDocument();
    if (!objectName) {
        return false;
    }

    _SelObj temp;
    int ret
        = checkSelection(pDoc->getName(), objectName, pSubName, ResolveMode::NoResolve, temp, &_SelList);
    if (ret < 0) {
        return false;
    }

    const char* subelement = nullptr;
    auto gateObject
        = getObjectOfType(temp, App::DocumentObject::getClassTypeId(), gateResolve, &subelement);

    std::string notAllowedReason = ActiveGate->notAllowedReason;
    bool allowed = ActiveGate->allow(
        gateObject ? gateObject->getDocument() : temp.pDoc,
        gateObject,
        subelement
    );
    ActiveGate->notAllowedReason = notAllowedReason;
    return allowed;
}

bool SelectionSingleton::hasSelectionGate(App::Document* /*pDoc*/) const
{
    // if (!pDoc) {
    //     return false;
    // }

    // auto foundContext = docSelectionContext.find(pDoc);
    // return foundContext != docSelectionContext.end() && foundContext->second.gate;
    return ActiveGate != nullptr;
}

int SelectionSingleton::setPreselect(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    float x,
    float y,
    float z,
    SelectionChanges::MsgSource signal,
    SelectionChanges::PickedPoint pickedPoint
)
{
    if (!pDocName || !pObjectName) {
        rmvPreselect();  // Invalid request
        return 0;
    }

    if (!pSubName) {
        pSubName = "";
    }

    if (DocName == pDocName && FeatName == pObjectName && SubName == pSubName) {
        return -1;  // Already preselected
    }

    rmvPreselect();

    if (ActiveGate && signal != SelectionChanges::MsgSource::Internal) {
        App::Document* pDoc = getDocument(pDocName);
        if (!pDoc || !pObjectName) {
            return 0;
        }
        App::ElementNamePair elementName;
        auto pObject = pDoc->getObject(pObjectName);
        if (!pObject) {
            return 0;
        }

        const char* subelement = pSubName;
        if (gateResolve != ResolveMode::NoResolve) {
            auto& newElementName = elementName.newName;
            auto& oldElementName = elementName.oldName;
            pObject = App::GeoFeature::resolveElement(pObject, pSubName, elementName);
            if (!pObject) {
                return 0;
            }
            if (gateResolve > ResolveMode::OldStyleElement) {
                subelement = !newElementName.empty() ? newElementName.c_str()
                                                     : oldElementName.c_str();
            }
            else {
                subelement = oldElementName.c_str();
            }
        }
        if (!ActiveGate->allow(pObject->getDocument(), pObject, subelement)) {
            QString msg;
            if (ActiveGate->notAllowedReason.length() > 0) {
                msg = QObject::tr(ActiveGate->notAllowedReason.c_str());
            }
            else {
                msg = QCoreApplication::translate("SelectionFilter", "Not allowed:");
            }
            msg.append(QStringLiteral(" %1.%2.%3 ")
                           .arg(
                               QString::fromLatin1(pDocName),
                               QString::fromLatin1(pObjectName),
                               QString::fromLatin1(pSubName)
                           ));

            if (getMainWindow()) {
                getMainWindow()->showMessage(msg);
                Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                mdi->setOverrideCursor(QCursor(Qt::ForbiddenCursor));
            }
            return 0;
        }
        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        mdi->restoreOverrideCursor();
    }

    DocName = pDocName;
    FeatName = pObjectName;
    SubName = pSubName;
    hx = x;
    hy = y;
    hz = z;

    // set up the change object
    SelectionChanges Chng(
        signal == SelectionChanges::MsgSource::Internal ? SelectionChanges::SetPreselectSignal
                                                        : SelectionChanges::SetPreselect,
        DocName,
        FeatName,
        SubName,
        std::string(),
        x,
        y,
        z,
        signal,
        pickedPoint
    );

    if (Chng.Type == SelectionChanges::SetPreselect) {
        CurrentPreselection = Chng;
        FC_TRACE("preselect " << DocName << '#' << FeatName << '.' << SubName);
    }
    else {
        FC_TRACE("preselect signal " << DocName << '#' << FeatName << '.' << SubName);
    }

    notify(Chng);

    if (signal == SelectionChanges::MsgSource::Internal && !DocName.empty()) {
        FC_TRACE("preselect " << DocName << '#' << FeatName << '.' << SubName);
        Chng.Type = SelectionChanges::SetPreselect;
        CurrentPreselection = Chng;
        notify(std::move(Chng));
    }

    // It is possible the preselect is removed during notification
    return DocName.empty() ? 0 : 1;
}

namespace Gui
{
std::array<std::pair<double, std::string>, 3> schemaTranslatePoint(
    double x,
    double y,
    double z,
    double precision
)
{
    Base::Quantity mmx(Base::Quantity::MilliMetre);
    mmx.setValue(fabs(x) > precision ? x : 0.0);
    Base::Quantity mmy(Base::Quantity::MilliMetre);
    mmy.setValue(fabs(y) > precision ? y : 0.0);
    Base::Quantity mmz(Base::Quantity::MilliMetre);
    mmz.setValue(fabs(z) > precision ? z : 0.0);

    double xfactor, yfactor, zfactor;
    std::string xunit, yunit, zunit;

    Base::UnitsApi::schemaTranslate(mmx, xfactor, xunit);
    Base::UnitsApi::schemaTranslate(mmy, yfactor, yunit);
    Base::UnitsApi::schemaTranslate(mmz, zfactor, zunit);

    double xuser = fabs(x) > precision ? x / xfactor : 0.0;
    double yuser = fabs(y) > precision ? y / yfactor : 0.0;
    double zuser = fabs(z) > precision ? z / zfactor : 0.0;

    std::array<std::pair<double, std::string>, 3> ret
        = {std::make_pair(xuser, xunit), std::make_pair(yuser, yunit), std::make_pair(zuser, zunit)};
    return ret;
}

QString getPreselectionInfo(
    const char* documentName,
    const char* objectName,
    const char* subElementName,
    float x,
    float y,
    float z,
    double precision
)
{
    auto pts = schemaTranslatePoint(x, y, z, precision);

    int numberDecimals = std::min(6, static_cast<int>(Base::UnitsApi::getDecimals()));

    QString message = QStringLiteral("Preselected: %1.%2.%3 (%4 %5, %6 %7, %8 %9)")
                          .arg(QString::fromUtf8(documentName))
                          .arg(QString::fromUtf8(objectName))
                          .arg(QString::fromUtf8(subElementName))
                          .arg(QString::number(pts[0].first, 'f', numberDecimals))
                          .arg(QString::fromStdString(pts[0].second))
                          .arg(QString::number(pts[1].first, 'f', numberDecimals))
                          .arg(QString::fromStdString(pts[1].second))
                          .arg(QString::number(pts[2].first, 'f', numberDecimals))
                          .arg(QString::fromStdString(pts[2].second));
    return message;
}

void printPreselectionInfo(
    const char* documentName,
    const char* objectName,
    const char* subElementName,
    float x,
    float y,
    float z,
    double precision
)
{
    if (getMainWindow()) {
        QString message
            = getPreselectionInfo(documentName, objectName, subElementName, x, y, z, precision);
        getMainWindow()->showMessage(message);
    }
}
}  // namespace Gui

void SelectionSingleton::setPreselectCoord(float x, float y, float z)
{
    // if nothing is in preselect ignore
    if (CurrentPreselection.Object.getObjectName().empty()) {
        return;
    }

    CurrentPreselection.x = x;
    CurrentPreselection.y = y;
    CurrentPreselection.z = z;

    printPreselectionInfo(
        CurrentPreselection.pDocName,
        CurrentPreselection.pObjectName,
        CurrentPreselection.pSubName,
        x,
        y,
        z,
        0.0
    );
}

void SelectionSingleton::rmvPreselect(bool signal)
{
    if (DocName.empty()) {
        return;
    }

    if (signal) {
        SelectionChanges Chng(SelectionChanges::RmvPreselectSignal, DocName, FeatName, SubName);
        notify(std::move(Chng));
        return;
    }

    SelectionChanges Chng(SelectionChanges::RmvPreselect, DocName, FeatName, SubName);

    // reset the current preselection
    CurrentPreselection = SelectionChanges();

    // Reset preselection helpers
    DocName = "";
    FeatName = "";
    SubName = "";
    hx = 0;
    hy = 0;
    hz = 0;

    if (ActiveGate && getMainWindow()) {
        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        mdi->restoreOverrideCursor();
    }

    FC_TRACE("rmv preselect");

    // notify observing objects
    notify(std::move(Chng));
}

const SelectionChanges& SelectionSingleton::getPreselection() const
{
    return CurrentPreselection;
}

// add a SelectionGate to control what is selectable
void SelectionSingleton::addSelectionGate(Gui::SelectionGate* gate, ResolveMode resolve)
{
    if (ActiveGate) {
        rmvSelectionGate();
    }

    ActiveGate = gate;
    gateResolve = resolve;
}

const Gui::SelectionGate* SelectionSingleton::getSelectionGate(const App::Document* /*doc*/) const
{
    return ActiveGate;
}

// remove the active SelectionGate
void SelectionSingleton::rmvSelectionGate()
{
    if (ActiveGate) {
        delete ActiveGate;
        ActiveGate = nullptr;

        if (Gui::Application::Instance) {
            if (Gui::Document* doc = Gui::Application::Instance->activeDocument()) {
                // if a document is about to be closed it has no MDI view any more
                if (Gui::MDIView* mdi = doc->getActiveView()) {
                    mdi->restoreOverrideCursor();
                }
            }
        }
    }
}

App::Document* SelectionSingleton::getDocument(const char* pDocName) const
{
    if (!Base::Tools::isNullOrEmpty(pDocName)) {
        return App::GetApplication().getDocument(pDocName);
    }
    else {
        return App::GetApplication().getActiveDocument();
    }
}

int SelectionSingleton::disableCommandLog()
{
    if (!logDisabled) {
        logHasSelection = hasSelection();
    }
    return ++logDisabled;
}

int SelectionSingleton::enableCommandLog(bool silent)
{
    --logDisabled;
    if (!logDisabled && !silent) {
        auto manager = Application::Instance->macroManager();
        if (!hasSelection()) {
            if (logHasSelection) {
                manager->addLine(MacroManager::Cmt, "Gui.Selection.clearSelection()");
            }
        }
        else {
            for (auto& sel : _SelList) {
                sel.log();
            }
        }
    }
    return logDisabled;
}

void SelectionSingleton::_SelObj::log(bool remove, bool clearPreselect)
{
    if (logged && !remove) {
        return;
    }
    logged = true;
    std::ostringstream ss;
    ss << "Gui.Selection." << (remove ? "removeSelection" : "addSelection") << "('" << DocName
       << "','" << FeatName << "'";
    if (!SubName.empty()) {
        ss << "," << getSubString();
    }
    if (!remove && (x || y || z || !clearPreselect)) {
        if (SubName.empty()) {
            ss << ",''";
        }
        ss << ',' << x << ',' << y << ',' << z;
        if (!clearPreselect) {
            ss << ",False";
        }
    }
    ss << ')';
    Application::Instance->macroManager()->addLine(MacroManager::Cmt, ss.str().c_str());
}

std::string SelectionSingleton::_SelObj::getSubString() const
{
    if (!SubName.empty()) {
        if (!elementName.oldName.empty() && !elementName.newName.empty()) {
            return "'" + SubName.substr(0, SubName.size() - elementName.newName.size())
                + elementName.oldName + "'";
        }
        return "'" + SubName + "'";
    }
    return {};
}

bool SelectionSingleton::addSelection(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    float x,
    float y,
    float z,
    const std::vector<SelObj>* pickedList,
    bool clearPreselect,
    SelectionChanges::PickedPoint pickedPoint
)
{
    if (pickedList) {
        _PickedList.clear();
        for (const auto& sel : *pickedList) {
            _PickedList.emplace_back();
            auto& s = _PickedList.back();
            s.DocName = sel.DocName;
            s.FeatName = sel.FeatName;
            s.SubName = sel.SubName;
            s.TypeName = sel.TypeName;
            s.pObject = sel.pObject;
            s.pDoc = sel.pDoc;
            s.x = sel.x;
            s.y = sel.y;
            s.z = sel.z;
        }
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    _SelObj temp;
    int ret = checkSelection(pDocName, pObjectName, pSubName, ResolveMode::NoResolve, temp);
    if (ret != 0) {
        return false;
    }

    temp.x = x;
    temp.y = y;
    temp.z = z;


    // check for a Selection Gate

    const auto& selectionAllowance = isSelectionAllowed(temp);
    if (!selectionAllowance.allowed) {
        if (getMainWindow()) {
            QString msg;
            if (selectionAllowance.reason.length() > 0) {
                msg = QObject::tr(selectionAllowance.reason.c_str());
            }
            else {
                msg = QCoreApplication::translate("SelectionFilter", "Selection not allowed by filter");
            }
            getMainWindow()->showMessage(msg);
            Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
            mdi->setOverrideCursor(Qt::ForbiddenCursor);
        }
        QApplication::beep();
        return false;
    }

    if (!logDisabled) {
        temp.log(false, clearPreselect);
    }

    _SelList.push_back(temp);
    _SelStackForward.clear();

    if (clearPreselect) {
        rmvPreselect();
    }

    SelectionChanges Chng(
        SelectionChanges::AddSelection,
        temp.DocName,
        temp.FeatName,
        temp.SubName,
        temp.TypeName,
        x,
        y,
        z,
        SelectionChanges::MsgSource::Any,
        pickedPoint
    );

    FC_LOG(
        "Add Selection " << Chng.pDocName << '#' << Chng.pObjectName << '.' << Chng.pSubName << " ("
                         << x << ", " << y << ", " << z << ')'
    );

    notify(std::move(Chng));

    getMainWindow()->updateActions();

    rmvPreselect(true);

    // There is a possibility that some observer removes or clears selection
    // inside signal handler, hence the check here
    return isSelected(temp.DocName.c_str(), temp.FeatName.c_str(), temp.SubName.c_str());
}

void SelectionSingleton::selStackPush(bool clearForward, bool overwrite)
{
    static int stackSize;
    if (!stackSize) {
        stackSize = App::GetApplication()
                        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                        ->GetInt("SelectionStackSize", 100);
    }
    if (clearForward) {
        _SelStackForward.clear();
    }
    if (_SelList.empty()) {
        return;
    }
    if ((int)_SelStackBack.size() >= stackSize) {
        _SelStackBack.pop_front();
    }
    SelStackItem item;
    for (auto& sel : _SelList) {
        item.emplace(sel.DocName.c_str(), sel.FeatName.c_str(), sel.SubName.c_str());
    }
    if (!_SelStackBack.empty() && _SelStackBack.back() == item) {
        return;
    }
    if (!overwrite || _SelStackBack.empty()) {
        _SelStackBack.emplace_back();
    }
    _SelStackBack.back() = std::move(item);
}

void SelectionSingleton::selStackGoBack(int count)
{
    if ((int)_SelStackBack.size() < count) {
        count = _SelStackBack.size();
    }
    if (count <= 0) {
        return;
    }
    if (!_SelList.empty()) {
        selStackPush(false, true);
        clearCompleteSelection();
    }
    else {
        --count;
    }
    for (int i = 0; i < count; ++i) {
        _SelStackForward.push_front(std::move(_SelStackBack.back()));
        _SelStackBack.pop_back();
    }
    std::deque<SelStackItem> tmpStack;
    _SelStackForward.swap(tmpStack);
    while (!_SelStackBack.empty()) {
        bool found = false;
        for (auto& sobjT : _SelStackBack.back()) {
            if (sobjT.getSubObject()) {
                addSelection(
                    sobjT.getDocumentName().c_str(),
                    sobjT.getObjectName().c_str(),
                    sobjT.getSubName().c_str()
                );
                found = true;
            }
        }
        if (found) {
            break;
        }
        tmpStack.push_front(std::move(_SelStackBack.back()));
        _SelStackBack.pop_back();
    }
    _SelStackForward = std::move(tmpStack);
    getMainWindow()->updateActions();
}

void SelectionSingleton::selStackGoForward(int count)
{
    if ((int)_SelStackForward.size() < count) {
        count = _SelStackForward.size();
    }
    if (count <= 0) {
        return;
    }
    if (!_SelList.empty()) {
        selStackPush(false, true);
        clearCompleteSelection();
    }
    for (int i = 0; i < count; ++i) {
        _SelStackBack.push_back(_SelStackForward.front());
        _SelStackForward.pop_front();
    }
    std::deque<SelStackItem> tmpStack;
    _SelStackForward.swap(tmpStack);
    while (true) {
        bool found = false;
        for (auto& sobjT : _SelStackBack.back()) {
            if (sobjT.getSubObject()) {
                addSelection(
                    sobjT.getDocumentName().c_str(),
                    sobjT.getObjectName().c_str(),
                    sobjT.getSubName().c_str()
                );
                found = true;
            }
        }
        if (found || tmpStack.empty()) {
            break;
        }
        _SelStackBack.push_back(tmpStack.front());
        tmpStack.pop_front();
    }
    _SelStackForward = std::move(tmpStack);
    getMainWindow()->updateActions();
}

std::vector<SelectionObject> SelectionSingleton::selStackGet(
    const char* pDocName,
    ResolveMode resolve,
    int index
) const
{
    const SelStackItem* item = nullptr;
    if (index >= 0) {
        if (index >= (int)_SelStackBack.size()) {
            return {};
        }
        item = &_SelStackBack[_SelStackBack.size() - 1 - index];
    }
    else {
        index = -index - 1;
        if (index >= (int)_SelStackForward.size()) {
            return {};
        }
        item = &_SelStackBack[_SelStackForward.size() - 1 - index];
    }

    std::list<_SelObj> selList;
    for (auto& sobjT : *item) {
        _SelObj sel;
        if (checkSelection(
                sobjT.getDocumentName().c_str(),
                sobjT.getObjectName().c_str(),
                sobjT.getSubName().c_str(),
                ResolveMode::NoResolve,
                sel,
                &selList
            )
            == 0) {
            selList.push_back(sel);
        }
    }

    return getObjectList(pDocName, App::DocumentObject::getClassTypeId(), selList, resolve);
}

bool SelectionSingleton::addSelections(
    const char* pDocName,
    const char* pObjectName,
    const std::vector<std::string>& pSubNames
)
{
    if (!_PickedList.empty()) {
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    std::ostringstream ss;
    bool anyLogged = false;

    if (!logDisabled) {
        ss << "Gui.Selection.addSelection(App.getDocument('" << pDocName << "').getObject('"
           << pObjectName << "'),[";
    }

    bool update = false;
    for (const auto& pSubName : pSubNames) {
        _SelObj temp;
        int ret = checkSelection(pDocName, pObjectName, pSubName.c_str(), ResolveMode::NoResolve, temp);
        if (ret != 0) {
            continue;
        }

        temp.x = 0;
        temp.y = 0;
        temp.z = 0;

        if (!isSelectionAllowed(temp).allowed) {
            continue;
        }

        if (!logDisabled && !temp.SubName.empty()) {
            temp.logged = true;
            if (anyLogged) {
                ss << ",";
            }
            anyLogged = true;

            ss << temp.getSubString();
        }

        _SelList.push_back(temp);
        _SelStackForward.clear();

        SelectionChanges Chng(
            SelectionChanges::AddSelection,
            temp.DocName,
            temp.FeatName,
            temp.SubName,
            temp.TypeName
        );

        FC_LOG("Add Selection " << Chng.pDocName << '#' << Chng.pObjectName << '.' << Chng.pSubName);

        notify(std::move(Chng));
        update = true;
    }

    if (!logDisabled && anyLogged) {
        ss << "])";
        Application::Instance->macroManager()->addLine(MacroManager::Cmt, ss.str().c_str());
    }

    if (update) {
        getMainWindow()->updateActions();
    }
    return true;
}

bool SelectionSingleton::updateSelection(
    bool show,
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName
)
{
    if (!pDocName || !pObjectName) {
        return false;
    }
    if (!pSubName) {
        pSubName = "";
    }

    if (DocName == pDocName && FeatName == pObjectName && SubName == pSubName) {
        if (show) {
            FC_TRACE("preselect signal");
            notify(SelectionChanges(SelectionChanges::SetPreselectSignal, DocName, FeatName, SubName));
        }
        else {
            rmvPreselect();
        }
    }

    auto pDoc = getDocument(pDocName);
    if (!pDoc) {
        return false;
    }
    auto pObject = pDoc->getObject(pObjectName);
    if (!pObject) {
        return false;
    }
    if (!isSelected(pObject, pSubName, ResolveMode::NoResolve)) {
        return false;
    }

    SelectionChanges Chng(
        show ? SelectionChanges::ShowSelection : SelectionChanges::HideSelection,
        pDocName,
        pObjectName,
        pSubName,
        pObject->getTypeId().getName()
    );

    FC_LOG("Update Selection " << Chng.pDocName << '#' << Chng.pObjectName << '.' << Chng.pSubName);

    notify(std::move(Chng));

    return true;
}

bool SelectionSingleton::addSelection(const SelectionObject& obj, bool clearPreselect)
{
    const std::vector<std::string>& subNames = obj.getSubNames();
    const std::vector<Base::Vector3d> points = obj.getPickedPoints();
    if (!subNames.empty() && subNames.size() == points.size()) {
        bool ok = true;
        for (std::size_t i = 0; i < subNames.size(); i++) {
            const std::string& name = subNames[i];
            const Base::Vector3d& pnt = points[i];
            ok &= addSelection(
                obj.getDocName(),
                obj.getFeatName(),
                name.c_str(),
                static_cast<float>(pnt.x),
                static_cast<float>(pnt.y),
                static_cast<float>(pnt.z),
                nullptr,
                clearPreselect
            );
        }
        return ok;
    }
    else if (!subNames.empty()) {
        bool ok = true;
        for (const std::string& name : subNames) {
            ok &= addSelection(obj.getDocName(), obj.getFeatName(), name.c_str());
        }
        return ok;
    }
    else {
        return addSelection(obj.getDocName(), obj.getFeatName());
    }
}


void SelectionSingleton::rmvSelection(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    const std::vector<SelObj>* pickedList
)
{
    if (pickedList) {
        _PickedList.clear();
        for (const auto& sel : *pickedList) {
            _PickedList.emplace_back();
            auto& s = _PickedList.back();
            s.DocName = sel.DocName;
            s.FeatName = sel.FeatName;
            s.SubName = sel.SubName;
            s.TypeName = sel.TypeName;
            s.pObject = sel.pObject;
            s.pDoc = sel.pDoc;
            s.x = sel.x;
            s.y = sel.y;
            s.z = sel.z;
        }
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    if (!pDocName) {
        return;
    }

    _SelObj temp;
    int ret = checkSelection(pDocName, pObjectName, pSubName, ResolveMode::NoResolve, temp);
    if (ret < 0) {
        return;
    }

    std::vector<SelectionChanges> changes;
    for (auto It = _SelList.begin(), ItNext = It; It != _SelList.end(); It = ItNext) {
        ++ItNext;
        if (It->DocName != temp.DocName || It->FeatName != temp.FeatName) {
            continue;
        }
        // if no subname is specified, remove all subobjects of the matching object
        if (!temp.SubName.empty()) {
            // otherwise, match subojects with common prefix, separated by '.'
            if (!boost::starts_with(It->SubName, temp.SubName)
                || (It->SubName.length() != temp.SubName.length()
                    && It->SubName[temp.SubName.length() - 1] != '.')) {
                continue;
            }
        }

        It->log(true);

        changes.emplace_back(
            SelectionChanges::RmvSelection,
            It->DocName,
            It->FeatName,
            It->SubName,
            It->TypeName
        );

        // destroy the _SelObj item
        _SelList.erase(It);
    }

    // NOTE: It can happen that there are nested calls of rmvSelection()
    // so that it's not safe to invoke the notifications inside the loop
    // as this can invalidate the iterators and thus leads to undefined
    // behaviour.
    // So, the notification is done after the loop, see also #0003469
    if (!changes.empty()) {
        for (auto& Chng : changes) {
            FC_LOG(
                "Rmv Selection " << Chng.pDocName << '#' << Chng.pObjectName << '.' << Chng.pSubName
            );
            notify(std::move(Chng));
        }
        getMainWindow()->updateActions();
    }
}

struct SelInfo
{
    std::string DocName;
    std::string FeatName;
    std::string SubName;
    SelInfo(const std::string& docName, const std::string& featName, const std::string& subName)
        : DocName(docName)
        , FeatName(featName)
        , SubName(subName)
    {}
};

void SelectionSingleton::setVisible(VisibleState vis)
{
    std::set<std::pair<App::DocumentObject*, App::DocumentObject*>> filter;
    int visible;
    switch (vis) {
        case VisShow:
            visible = 1;
            break;
        case VisToggle:
            visible = -1;
            break;
        default:
            visible = 0;
    }

    // Copy the selection in case it changes during this function
    std::vector<SelInfo> sels;
    sels.reserve(_SelList.size());
    for (auto& sel : _SelList) {
        if (sel.DocName.empty() || sel.FeatName.empty() || !sel.pObject) {
            continue;
        }
        sels.emplace_back(sel.DocName, sel.FeatName, sel.SubName);
    }

    for (auto& sel : sels) {
        App::Document* doc = App::GetApplication().getDocument(sel.DocName.c_str());
        if (!doc) {
            continue;
        }
        App::DocumentObject* obj = doc->getObject(sel.FeatName.c_str());
        if (!obj) {
            continue;
        }

        // get parent object
        App::DocumentObject* parent = nullptr;
        std::string elementName;
        obj = obj->resolve(sel.SubName.c_str(), &parent, &elementName);
        if (!obj || !obj->isAttachedToDocument() || (parent && !parent->isAttachedToDocument())) {
            continue;
        }
        // try call parent object's setElementVisible
        if (parent) {
            // prevent setting the same object visibility more than once
            if (!filter.insert(std::make_pair(obj, parent)).second) {
                continue;
            }
            int visElement = parent->isElementVisible(elementName.c_str());
            if (visElement >= 0) {
                if (visElement > 0) {
                    visElement = 1;
                }
                if (visible >= 0) {
                    if (visElement == visible) {
                        continue;
                    }
                    visElement = visible;
                }
                else {
                    visElement = !visElement;
                }

                if (!visElement) {
                    updateSelection(
                        false,
                        sel.DocName.c_str(),
                        sel.FeatName.c_str(),
                        sel.SubName.c_str()
                    );
                }
                parent->setElementVisible(elementName.c_str(), visElement ? true : false);
                if (visElement) {
                    updateSelection(true, sel.DocName.c_str(), sel.FeatName.c_str(), sel.SubName.c_str());
                }
                continue;
            }

            // Fall back to direct object visibility setting
        }
        if (!filter.insert(std::make_pair(obj, static_cast<App::DocumentObject*>(nullptr))).second) {
            continue;
        }

        auto vp = Application::Instance->getViewProvider(obj);

        if (vp) {
            if (visible < 0) {
                // Toggle link instead of the original object
                ViewProvider* toggleVp = vp;
                if (parent
                    && parent->hasExtension(App::LinkBaseExtension::getExtensionClassTypeId(), true)) {
                    if (auto* parentVp = Application::Instance->getViewProvider(parent)) {
                        toggleVp = parentVp;
                    }
                }
                toggleVp->toggleVisibility();
                updateSelection(
                    toggleVp->isShow(),
                    sel.DocName.c_str(),
                    sel.FeatName.c_str(),
                    sel.SubName.c_str()
                );
            }
            else {
                if (visible) {
                    vp->show();
                    updateSelection(
                        visible,
                        sel.DocName.c_str(),
                        sel.FeatName.c_str(),
                        sel.SubName.c_str()
                    );
                }
                else {
                    updateSelection(
                        visible,
                        sel.DocName.c_str(),
                        sel.FeatName.c_str(),
                        sel.SubName.c_str()
                    );
                    vp->hide();
                }
            }
        }
    }
}

void SelectionSingleton::setSelection(const char* pDocName, const std::vector<App::DocumentObject*>& sel)
{
    if (!_PickedList.empty()) {
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    bool touched = false;
    for (auto obj : sel) {
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }
        _SelObj temp;
        int ret
            = checkSelection(pDocName, obj->getNameInDocument(), nullptr, ResolveMode::NoResolve, temp);
        if (ret != 0) {
            continue;
        }
        touched = true;
        _SelList.push_back(temp);
    }

    if (touched) {
        _SelStackForward.clear();
        notify(SelectionChanges(SelectionChanges::SetSelection, pDocName));
        getMainWindow()->updateActions();
    }
}

void SelectionSingleton::clearSelection(const char* pDocName, bool clearPreSelect)
{
    // Because the introduction of external editing, it is best to make
    // clearSelection(0) behave as clearCompleteSelection(), which is the same
    // behavior of python Selection.clearSelection(None)
    if (!pDocName || !pDocName[0] || strcmp(pDocName, "*") == 0) {
        clearCompleteSelection(clearPreSelect);
        return;
    }
    if (!_PickedList.empty()) {
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    App::Document* pDoc;
    pDoc = getDocument(pDocName);
    if (pDoc) {
        std::string docName = pDocName;
        if (clearPreSelect && DocName == docName) {
            rmvPreselect();
        }
        bool touched = false;
        for (auto it = _SelList.begin(); it != _SelList.end();) {
            if (it->DocName == docName) {
                touched = true;
                it = _SelList.erase(it);
            }
            else {
                ++it;
            }
        }
        if (!touched) {
            return;
        }

        if (!logDisabled) {
            std::ostringstream ss;
            ss << "Gui.Selection.clearSelection('" << docName << "'";
            if (!clearPreSelect) {
                ss << ", False";
            }
            ss << ')';
            Application::Instance->macroManager()->addLine(MacroManager::Cmt, ss.str().c_str());
        }
        notify(SelectionChanges(SelectionChanges::ClrSelection, docName.c_str()));
        getMainWindow()->updateActions();
    }
}

void SelectionSingleton::clearCompleteSelection(bool clearPreSelect)
{
    if (!_PickedList.empty()) {
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    if (clearPreSelect) {
        rmvPreselect();
    }

    if (_SelList.empty()) {
        return;
    }

    if (!logDisabled) {
        Application::Instance->macroManager()->addLine(
            MacroManager::Cmt,
            clearPreSelect ? "Gui.Selection.clearSelection()" : "Gui.Selection.clearSelection(False)"
        );
    }

    // Send the clear selection notification to all view providers associated with the
    // objects being deselected.

    std::set<ViewProvider*> viewProviders;
    for (_SelObj& sel : _SelList) {
        if (auto vp = Application::Instance->getViewProvider(sel.pObject)) {
            viewProviders.insert(vp);
        }
    }

    for (auto& vp : viewProviders) {
        SelectionChanges Chng(SelectionChanges::ClrSelection);
        vp->onSelectionChanged(Chng);
    }

    _SelList.clear();

    SelectionChanges Chng(SelectionChanges::ClrSelection);

    FC_LOG("Clear selection");

    notify(std::move(Chng));
    getMainWindow()->updateActions();
}

bool SelectionSingleton::isSelected(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    ResolveMode resolve
) const
{
    _SelObj sel;
    return checkSelection(pDocName, pObjectName, pSubName, resolve, sel, &_SelList) > 0;
}

bool SelectionSingleton::isSelected(
    App::DocumentObject* pObject,
    const char* pSubName,
    ResolveMode resolve
) const
{
    if (!pObject || !pObject->isAttachedToDocument() || !pObject->getDocument()) {
        return false;
    }

    _SelObj sel;
    return checkSelection(
               pObject->getDocument()->getName(),
               pObject->getNameInDocument(),
               pSubName,
               resolve,
               sel,
               &_SelList
           )
        > 0;
}

int SelectionSingleton::checkSelection(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    ResolveMode resolve,
    _SelObj& sel,
    const std::list<_SelObj>* selList
) const
{
    sel.pDoc = getDocument(pDocName);
    if (!sel.pDoc) {
        if (!selList) {
            FC_ERR("Cannot find document");
        }
        return -1;
    }

    pDocName = sel.pDoc->getName();
    sel.DocName = pDocName;

    if (pObjectName) {
        sel.pObject = sel.pDoc->getObject(pObjectName);
    }
    else {
        sel.pObject = nullptr;
    }
    if (!sel.pObject) {
        if (!selList) {
            FC_ERR("Object not found");
        }
        return -1;
    }
    if (sel.pObject->testStatus(App::ObjectStatus::Remove)) {
        return -1;
    }
    if (pSubName) {
        sel.SubName = pSubName;
    }
    if (resolve == ResolveMode::NoResolve) {
        TreeWidget::checkTopParent(sel.pObject, sel.SubName);
    }
    pSubName = !sel.SubName.empty() ? sel.SubName.c_str() : nullptr;
    sel.FeatName = sel.pObject->getNameInDocument();
    sel.TypeName = sel.pObject->getTypeId().getName();
    const char* element = nullptr;
    sel.pResolvedObject = App::GeoFeature::resolveElement(
        sel.pObject,
        pSubName,
        sel.elementName,
        false,
        App::GeoFeature::Normal,
        nullptr,
        &element
    );
    if (!sel.pResolvedObject) {
        if (!selList) {
            FC_ERR(
                "Sub-object " << sel.DocName << '#' << sel.FeatName << '.' << sel.SubName << " not found"
            );
        }
        return -1;
    }
    if (sel.pResolvedObject->testStatus(App::ObjectStatus::Remove)) {
        return -1;
    }
    std::string subname;
    std::string prefix;
    if (pSubName && element) {
        prefix = std::string(pSubName, element - pSubName);
        if (!sel.elementName.newName.empty()) {
            // make sure the selected sub name is a new style if available
            subname = prefix + sel.elementName.newName;
            pSubName = subname.c_str();
            sel.SubName = subname;
        }
    }
    if (!selList) {
        selList = &_SelList;
    }

    if (!pSubName) {
        pSubName = "";
    }

    for (auto& s : *selList) {
        if (s.DocName == pDocName && s.FeatName == sel.FeatName) {
            if (s.SubName == pSubName) {
                return 1;
            }
            if (resolve > ResolveMode::OldStyleElement && boost::starts_with(s.SubName, prefix)) {
                return 1;
            }
        }
    }
    if (resolve == ResolveMode::OldStyleElement) {
        for (auto& s : *selList) {
            if (s.pResolvedObject != sel.pResolvedObject) {
                continue;
            }
            if (!pSubName[0]) {
                return 1;
            }
            if (!s.elementName.newName.empty()) {
                if (s.elementName.newName == sel.elementName.newName) {
                    return 1;
                }
            }
            else if (s.SubName == sel.elementName.oldName) {
                return 1;
            }
        }
    }
    return 0;
}

void SelectionSingleton::slotDeletedObject(const App::DocumentObject& Obj)
{
    if (!Obj.isAttachedToDocument()) {
        return;
    }

    // For safety reason, don't bother checking
    rmvPreselect();

    // Remove also from the selection, if selected
    // We don't walk down the hierarchy for each selection, so there may be stray selection
    std::vector<SelectionChanges> changes;
    for (auto it = _SelList.begin(), itNext = it; it != _SelList.end(); it = itNext) {
        ++itNext;
        if (it->pResolvedObject == &Obj || it->pObject == &Obj) {
            changes.emplace_back(
                SelectionChanges::RmvSelection,
                it->DocName,
                it->FeatName,
                it->SubName,
                it->TypeName
            );
            _SelList.erase(it);
        }
    }
    if (!changes.empty()) {
        for (auto& Chng : changes) {
            FC_LOG(
                "Rmv Selection " << Chng.pDocName << '#' << Chng.pObjectName << '.' << Chng.pSubName
            );
            notify(std::move(Chng));
        }
        getMainWindow()->updateActions();
    }

    if (!_PickedList.empty()) {
        bool changed = false;
        for (auto it = _PickedList.begin(), itNext = it; it != _PickedList.end(); it = itNext) {
            ++itNext;
            auto& sel = *it;
            if (sel.DocName == Obj.getDocument()->getName()
                && sel.FeatName == Obj.getNameInDocument()) {
                changed = true;
                _PickedList.erase(it);
            }
        }
        if (changed) {
            notify(SelectionChanges(SelectionChanges::PickedListChanged, Obj.getDocument()->getName()));
        }
    }
}

void SelectionSingleton::setSelectionStyle(SelectionStyle selStyle)
{
    selectionStyle = selStyle;
}

SelectionSingleton::SelectionStyle SelectionSingleton::getSelectionStyle()
{
    return selectionStyle;
}

//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
SelectionSingleton::SelectionSingleton()
    : CurrentPreselection(SelectionChanges::ClrSelection)
{
    // NOLINTBEGIN
    App::GetApplication().signalDeletedObject.connect(
        std::bind(&Gui::SelectionSingleton::slotDeletedObject, this, sp::_1)
    );

    signalSelectionChanged.connect(
        std::bind(&Gui::SelectionSingleton::slotSelectionChanged, this, sp::_1)
    );
    // NOLINTEND
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
SelectionSingleton::~SelectionSingleton() = default;

SelectionSingleton* SelectionSingleton::_pcSingleton = nullptr;

SelectionSingleton& SelectionSingleton::instance()
{
    if (!_pcSingleton) {
        _pcSingleton = new SelectionSingleton;
    }
    return *_pcSingleton;
}

void SelectionSingleton::destruct()
{
    if (_pcSingleton) {
        delete _pcSingleton;
    }
    _pcSingleton = nullptr;
}

bool SelectionSingleton::isClarifySelectionActive()
{
    return clarifySelectionActive;
}

void SelectionSingleton::setClarifySelectionActive(bool active)
{
    clarifySelectionActive = active;
}
