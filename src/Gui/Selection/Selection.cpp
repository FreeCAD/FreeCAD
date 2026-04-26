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
#include <boost/algorithm/string/predicate.hpp>
#include <QApplication>


#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
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
#include "ViewProviderDocumentObject.h"


FC_LOG_LEVEL_INIT("Selection", false, true, true)

using namespace Gui;
using namespace std;
namespace sp = std::placeholders;

//////////////////////////////////////////////////////////////////////////////////////////

SelectionObserver::SelectionObserver(bool attach, ResolveMode resolve)
    : resolve(resolve)
    , blockedSelection(false)
{
    if (auto doc = App::GetApplication().getActiveDocument()) {
        documentScopeName = doc->getName();
    }
    if (attach) {
        attachSelection();
    }
}

SelectionObserver::SelectionObserver(
    const ViewProviderDocumentObject* /*vp*/,
    bool attach,
    ResolveMode resolve
)
    : resolve(resolve)
    , blockedSelection(false)
{
    if (auto doc = App::GetApplication().getActiveDocument()) {
        documentScopeName = doc->getName();
    }
    if (attach) {
        attachSelection();
    }
}


SelectionObserver::~SelectionObserver()
{
    detachSelection();
}

bool SelectionObserver::blockSelection(bool block)
{
    bool ok = blockedSelection;
    blockedSelection = block;
    return ok;
}

bool SelectionObserver::isSelectionBlocked() const
{
    return blockedSelection;
}

bool SelectionObserver::isSelectionAttached() const
{
    return connectSelection.connected();
}

void SelectionObserver::attachSelection()
{
    if (!connectSelection.connected()) {
        bool newStyle = (resolve >= ResolveMode::NewStyleElement);
        bool oldStyle = (resolve == ResolveMode::OldStyleElement);
        auto& signal = newStyle ? Selection().signalSelectionChanged3
            : oldStyle          ? Selection().signalSelectionChanged2
                                : Selection().signalSelectionChanged;
        // NOLINTBEGIN
        connectSelection = signal.connect(
            std::bind(&SelectionObserver::_onSelectionChanged, this, sp::_1)
        );
        // NOLINTEND
    }
}

void SelectionObserver::_onSelectionChanged(const SelectionChanges& msg)
{
    try {
        if (blockedSelection
            || (!documentScopeName.empty() && msg.pDocName && documentScopeName != msg.pDocName)) {
            return;
        }
        onSelectionChanged(msg);
    }
    catch (Base::Exception& e) {
        e.reportException();
        FC_ERR("Unhandled Base::Exception caught in selection observer: ");
    }
    catch (std::exception& e) {
        FC_ERR("Unhandled std::exception caught in selection observer: " << e.what());
    }
    catch (...) {
        FC_ERR("Unhandled unknown exception caught in selection observer");
    }
}

void SelectionObserver::detachSelection()
{
    if (connectSelection.connected()) {
        connectSelection.disconnect();
    }
}

// -------------------------------------------

bool SelectionSingleton::hasPreselection() const
{
    return !CurrentPreselection.Object.getObjectName().empty();
}

unsigned int SelectionSingleton::size(const char* pDocName) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return 0;
    }
    return static_cast<unsigned int>(context.info->selList.size());
}

std::size_t SelectionSingleton::selStackBackSize(const char* pDocName) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return 0;
    }
    return context.info->selStackBack.size();
}
std::size_t SelectionSingleton::selStackForwardSize(const char* pDocName) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return 0;
    }
    return context.info->selStackForward.size();
}

std::vector<SelectionSingleton::SelObj> SelectionSingleton::getCompleteSelection(ResolveMode resolve) const
{
    return getSelection("*", resolve);
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
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return {};
    }

    std::map<App::DocumentObject*, std::set<std::string>> objMap;

    for (auto& sel : context.info->selList) {
        if (!sel.pDoc) {
            continue;
        }
        const char* subelement = nullptr;
        auto obj = getObjectOfType(sel, App::DocumentObject::getClassTypeId(), resolve, &subelement);
        if (!obj) {
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
bool SelectionSingleton::hasSelection(const char* pDocName) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return false;
    }
    return !context.info->selList.empty();
}
bool SelectionSingleton::hasSelection(const char* pDocName, ResolveMode resolve) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return false;
    }
    for (auto& sel : context.info->selList) {
        if (getObjectOfType(sel, App::DocumentObject::getClassTypeId(), resolve)) {
            return true;
        }
    }

    return false;
}

bool SelectionSingleton::hasSubSelection(const char* pDocName, bool subElement) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return false;
    }
    for (auto& sel : context.info->selList) {
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

    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return {};
    }

    for (const auto& picked : context.info->pickedList) {
        tempSelObj.DocName = picked.DocName.c_str();
        tempSelObj.FeatName = picked.FeatName.c_str();
        tempSelObj.SubName = picked.SubName.c_str();
        tempSelObj.TypeName = picked.TypeName.c_str();
        tempSelObj.pObject = picked.pObject;
        tempSelObj.pResolvedObject = picked.pResolvedObject;
        tempSelObj.pDoc = picked.pDoc;
        tempSelObj.x = picked.x;
        tempSelObj.y = picked.y;
        tempSelObj.z = picked.z;
        temp.push_back(tempSelObj);
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
    std::map<App::DocumentObject*, size_t> objectIndices;

    for (auto& sel : sels) {
        auto* rootObj = sel.getObject();
        App::Document* doc = rootObj->getDocument();
        std::vector<std::string> subs = sel.getSubNames();
        bool containerPassed = false;

        for (size_t i = 0; i < subs.size(); ++i) {
            SelectionInResult result;
            if (!selectionInResult(sel, subs[i], container, typeId, doc, containerPassed, result)) {
                continue;
            }
            if (!appendSelectionInResult(ret, objectIndices, result, sel.SelPoses[i], single)) {
                break;
            }
        }
    }

    return ret;
}

bool SelectionSingleton::selectionInResult(
    SelectionObject& sel,
    const std::string& subName,
    App::DocumentObject* container,
    Base::Type typeId,
    App::Document*& doc,
    bool& containerPassed,
    SelectionInResult& result
) const
{
    result = SelectionInResult {};

    auto* rootObj = sel.getObject();
    App::DocumentObject* newRootObj = nullptr;
    std::string newSub;

    std::vector<std::string> names = Base::Tools::splitSubName(subName);

    if (container == rootObj) {
        containerPassed = true;
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

        if (containerPassed) {
            if (!newRootObj) {
                // We are the first object after the container is passed.
                newRootObj = obj;
            }
            else {
                newSub += name + ".";
            }
        }

        if (obj == container) {
            containerPassed = true;
        }
        if (obj->isLink()) {
            // Update doc in case its an external link.
            doc = obj->getLinkedObject()->getDocument();
        }
    }

    if (!newRootObj) {
        return false;
    }

    // Make sure selected object is of correct type
    auto* lastObj = newRootObj->resolve(newSub.c_str());
    if (!lastObj || !lastObj->isDerivedFrom(typeId)) {
        return false;
    }

    result.root = newRootObj;
    result.subName = std::move(newSub);
    return true;
}

bool SelectionSingleton::appendSelectionInResult(
    std::vector<SelectionObject>& selections,
    std::map<App::DocumentObject*, size_t>& objectIndices,
    const SelectionInResult& result,
    const Base::Vector3d& pickedPoint,
    bool single
)
{
    auto it = objectIndices.find(result.root);
    if (it != objectIndices.end()) {
        // only add sub-element
        if (!result.subName.empty()) {
            selections[it->second].SubNames.emplace_back(result.subName);
            selections[it->second].SelPoses.emplace_back(pickedPoint);
        }
        return true;
    }

    if (single && !selections.empty()) {
        selections.clear();
        return false;
    }

    // create a new entry
    selections.emplace_back(result.root);
    if (!result.subName.empty()) {
        selections.back().SubNames.emplace_back(result.subName);
        selections.back().SelPoses.emplace_back(pickedPoint);
    }
    objectIndices.insert(std::make_pair(result.root, selections.size() - 1));
    return true;
}

std::vector<SelectionObject> SelectionSingleton::getSelectionEx(
    const char* pDocName,
    Base::Type typeId,
    ResolveMode resolve,
    bool single
) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return {};
    }
    return getObjectList(pDocName, typeId, context.info->selList, resolve, single);
}

std::vector<SelectionObject> SelectionSingleton::getPickedListEx(
    const char* pDocName,
    Base::Type typeId
) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return {};
    }
    return getObjectList(pDocName, typeId, context.info->pickedList, ResolveMode::NoResolve);
}

std::vector<SelectionObject> SelectionSingleton::getObjectList(
    const char* pDocName,
    Base::Type typeId,
    const std::list<SelectionDescription>& objList,
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

bool SelectionSingleton::needPickedList(const char* pDocName) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return false;
    }
    return context.info->needPickedList;
}

SelectionSingleton::SelectionAllowance SelectionSingleton::isSelectionAllowed(
    const SelectionSingleton::SelectionContext& context,
    const SelectionDescription& sel
)
{
    if (!context.info || !context.info->gate) {
        return {.allowed = true, .reason = ""};
    }
    const char* subelement = nullptr;
    auto pObject = getObjectOfType(
        sel,
        App::DocumentObject::getClassTypeId(),
        context.info->resolveMode,
        &subelement
    );


    if (!context.info->gate->allow(pObject ? pObject->getDocument() : sel.pDoc, pObject, subelement)) {
        std::string copyNotAllowedReason = context.info->gate->notAllowedReason;
        context.info->gate->notAllowedReason.clear();
        return {.allowed = false, .reason = copyNotAllowedReason};
    }
    return {.allowed = true, .reason = ""};
}

void SelectionSingleton::enablePickedList(bool enable, const char* pDocName)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    if (enable != context.info->needPickedList) {
        context.info->needPickedList = enable;
        context.info->pickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged, context.docName.c_str()));
    }
}

void SelectionSingleton::clearPickedList(SelectionContext& context)
{
    if (!context.info || context.info->pickedList.empty()) {
        return;
    }

    context.info->pickedList.clear();
    notify(SelectionChanges(SelectionChanges::PickedListChanged, context.docName.c_str()));
}

void SelectionSingleton::replacePickedList(SelectionContext& context, const std::vector<SelObj>& pickedList)
{
    if (!context.info) {
        return;
    }

    context.info->pickedList.clear();
    for (const auto& sel : pickedList) {
        context.info->pickedList.emplace_back();
        auto& picked = context.info->pickedList.back();
        picked.DocName = sel.DocName;
        picked.FeatName = sel.FeatName;
        picked.SubName = sel.SubName;
        picked.TypeName = sel.TypeName;
        picked.pObject = sel.pObject;
        picked.pDoc = sel.pDoc;
        picked.x = sel.x;
        picked.y = sel.y;
        picked.z = sel.z;
    }

    notify(SelectionChanges(SelectionChanges::PickedListChanged, context.docName.c_str()));
}

void SelectionSingleton::notifySelectionRemovals(std::vector<SelectionChanges>& changes)
{
    if (changes.empty()) {
        return;
    }

    for (auto& Chng : changes) {
        FC_LOG("Rmv Selection " << Chng.pDocName << '#' << Chng.pObjectName << '.' << Chng.pSubName);
        notify(std::move(Chng));
    }
    getMainWindow()->updateActions();
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

bool SelectionSingleton::hasPickedList(const char* pDocName) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return false;
    }
    return !context.info->pickedList.empty();
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
    const SelectionDescription& sel,
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
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return {};
    }

    std::set<App::DocumentObject*> objs;
    for (auto& sel : context.info->selList) {
        if (App::DocumentObject* pObject = getObjectOfType(sel, typeId, resolve)) {
            objs.insert(pObject);
        }
    }

    return std::vector<App::DocumentObject*>(objs.begin(), objs.end());
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
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return 0;
    }

    return std::count_if(context.info->selList.begin(), context.info->selList.end(), [&](auto& sel) {
        return getObjectOfType(sel, typeId, resolve);
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
            msg.z
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

bool SelectionSingleton::preselectionGateAllows(
    const SelectionContext& context,
    App::Document* pDoc,
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    SelectionChanges::MsgSource signal
) const
{
    if (!context.info->gate || signal == SelectionChanges::MsgSource::Internal) {
        return true;
    }

    App::ElementNamePair elementName;
    auto pObject = pDoc->getObject(pObjectName);
    if (!pObject) {
        return false;
    }

    const char* subelement = pSubName;
    if (context.info->resolveMode != ResolveMode::NoResolve) {
        auto& newElementName = elementName.newName;
        auto& oldElementName = elementName.oldName;
        pObject = App::GeoFeature::resolveElement(pObject, pSubName, elementName);
        if (!pObject) {
            return false;
        }
        if (context.info->resolveMode > ResolveMode::OldStyleElement) {
            subelement = !newElementName.empty() ? newElementName.c_str() : oldElementName.c_str();
        }
        else {
            subelement = oldElementName.c_str();
        }
    }
    if (!context.info->gate->allow(pObject->getDocument(), pObject, subelement)) {
        QString msg;
        if (context.info->gate->notAllowedReason.length() > 0) {
            msg = QObject::tr(context.info->gate->notAllowedReason.c_str());
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
        return false;
    }
    Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    mdi->restoreOverrideCursor();
    return true;
}

int SelectionSingleton::setPreselect(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    float x,
    float y,
    float z,
    SelectionChanges::MsgSource signal
)
{
    if (!pDocName || !pObjectName) {
        rmvPreselect();  // Invalid request
        return 0;
    }

    if (!pSubName) {
        pSubName = "";
    }

    if (preselection.matches(pDocName, pObjectName, pSubName)) {
        return -1;  // Already pre-selected
    }

    rmvPreselect();

    App::Document* pDoc = getDocument(pDocName);
    if (!pDoc) {
        return 0;
    }
    auto context = getSelectionContext(pDocName);

    if (!preselectionGateAllows(context, pDoc, pDocName, pObjectName, pSubName, signal)) {
        return 0;
    }

    preselection.set(context.docName, pObjectName, pSubName);

    // set up the change object
    SelectionChanges Chng(
        signal == SelectionChanges::MsgSource::Internal ? SelectionChanges::SetPreselectSignal
                                                        : SelectionChanges::SetPreselect,
        context.docName.c_str(),
        preselection.objectName,
        preselection.subName,
        std::string(),
        x,
        y,
        z,
        signal
    );

    if (Chng.Type == SelectionChanges::SetPreselect) {
        CurrentPreselection = Chng;
        FC_TRACE(
            "preselect " << preselection.docName << '#' << preselection.objectName << '.'
                         << preselection.subName
        );
    }
    else {
        FC_TRACE(
            "preselect signal " << preselection.docName << '#' << preselection.objectName << '.'
                                << preselection.subName
        );
    }

    notify(Chng);

    if (signal == SelectionChanges::MsgSource::Internal && !preselection.empty()) {
        FC_TRACE(
            "preselect " << preselection.docName << '#' << preselection.objectName << '.'
                         << preselection.subName
        );
        Chng.Type = SelectionChanges::SetPreselect;
        CurrentPreselection = Chng;
        notify(std::move(Chng));
    }

    // It is possible the preselect is removed during notification
    return preselection.empty() ? 0 : 1;
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
    if (preselection.empty()) {
        return;
    }

    if (signal) {
        SelectionChanges Chng(
            SelectionChanges::RmvPreselectSignal,
            preselection.docName,
            preselection.objectName,
            preselection.subName
        );
        notify(std::move(Chng));
        return;
    }

    SelectionChanges Chng(
        SelectionChanges::RmvPreselect,
        preselection.docName,
        preselection.objectName,
        preselection.subName
    );

    // reset the current preselection
    CurrentPreselection = SelectionChanges();

    auto context = getSelectionContext(preselection.docName.c_str());
    if (context.info && context.info->gate && getMainWindow()) {
        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        mdi->restoreOverrideCursor();
    }

    preselection.reset();

    FC_TRACE("rmv preselect");

    // notify observing objects
    notify(std::move(Chng));
}

const SelectionChanges& SelectionSingleton::getPreselection() const
{
    return CurrentPreselection;
}

// add a SelectionGate to control what is selectable
void SelectionSingleton::addSelectionGate(Gui::SelectionGate* gate, ResolveMode resolve, const char* pDocName)
{
    App::Document* doc = getDocument(pDocName);
    if (!doc) {
        return;
    }
    rmvSelectionGate(doc);

    auto context = getSelectionContext(doc->getName());
    context.info->resolveMode = resolve;
    context.info->gate = gate;
}

const Gui::SelectionGate* SelectionSingleton::getSelectionGate(const App::Document* doc) const
{
    if (doc == nullptr) {
        return nullptr;
    }
    auto context = getSelectionContext(doc->getName());
    if (!context.info) {
        return nullptr;
    }
    return context.info->gate;
}

// remove the active SelectionGate
void SelectionSingleton::rmvSelectionGate(App::Document* doc)
{
    auto foundContext = docSelectionContext.find(doc);
    if (foundContext != docSelectionContext.end() && foundContext->second.gate) {
        delete foundContext->second.gate;
        foundContext->second.gate = nullptr;

        // if a document is about to be closed it has no MDI view any more
        if (Gui::Document* guiDoc = Gui::Application::Instance->getDocument(doc)) {
            if (Gui::MDIView* mdi = guiDoc->getActiveView()) {
                mdi->restoreOverrideCursor();
            }
        }
    }
}
void SelectionSingleton::rmvSelectionGate(const char* pDocName)
{
    App::Document* doc = getDocument(pDocName);
    if (!doc) {
        return;
    }
    rmvSelectionGate(doc);
}


App::Document* SelectionSingleton::getDocument(const char* pDocName) const
{
    return App::GetApplication().getDocumentOrActive(pDocName);
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
            auto context = getSelectionContext(nullptr);  // get selection context of
                                                          // current active file
            for (auto& sel : context.info->selList) {
                sel.log();
            }
        }
    }
    return logDisabled;
}

void SelectionSingleton::SelectionDescription::log(bool remove, bool clearPreselect)
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

std::string SelectionSingleton::SelectionDescription::getSubString() const
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

bool SelectionSingleton::selectionGateAllows(
    const SelectionContext& context,
    const SelectionDescription& sel
) const
{
    const auto& selectionAllowance = isSelectionAllowed(context, sel);
    if (selectionAllowance.allowed) {
        return true;
    }

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

bool SelectionSingleton::addSelection(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    float x,
    float y,
    float z,
    const std::vector<SelObj>* pickedList,
    bool clearPreselect
)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return false;
    }

    if (pickedList) {
        replacePickedList(context, *pickedList);
    }

    SelectionDescription temp;
    auto checkResult = checkSelection(pDocName, pObjectName, pSubName, ResolveMode::NoResolve, temp);
    if (checkResult != SelectionCheckResult::Available) {
        return false;
    }

    temp.x = x;
    temp.y = y;
    temp.z = z;

    if (!selectionGateAllows(context, temp)) {
        return false;
    }

    if (!logDisabled) {
        temp.log(false, clearPreselect);
    }

    context.info->selList.push_back(temp);
    context.info->selStackForward.clear();

    if (clearPreselect) {
        rmvPreselect();
    }

    SelectionChanges Chng(
        SelectionChanges::AddSelection,
        context.docName.c_str(),
        temp.FeatName,
        temp.SubName,
        temp.TypeName,
        x,
        y,
        z
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

SelectionSingleton::SelStackItem SelectionSingleton::selectionStackItem(
    const SelectionContext& context
) const
{
    SelStackItem item;
    for (const auto& sel : context.info->selList) {
        item.emplace(sel.DocName.c_str(), sel.FeatName.c_str(), sel.SubName.c_str());
    }
    return item;
}

bool SelectionSingleton::restoreSelectionStackItem(const SelStackItem& item)
{
    bool found = false;
    for (const auto& sobjT : item) {
        if (sobjT.getSubObject()) {
            addSelection(
                sobjT.getDocumentName().c_str(),
                sobjT.getObjectName().c_str(),
                sobjT.getSubName().c_str()
            );
            found = true;
        }
    }
    return found;
}

void SelectionSingleton::selStackPush(bool clearForward, bool overwrite, const char* pDocName)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    static int stackSize;
    if (!stackSize) {
        stackSize = App::GetApplication()
                        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                        ->GetInt("SelectionStackSize", 100);
    }
    if (clearForward) {
        context.info->selStackForward.clear();
    }
    if (context.info->selList.empty()) {
        return;
    }
    if ((int)context.info->selStackBack.size() >= stackSize) {
        context.info->selStackBack.pop_front();
    }
    auto item = selectionStackItem(context);
    if (!context.info->selStackBack.empty() && context.info->selStackBack.back() == item) {
        return;
    }
    if (!overwrite || context.info->selStackBack.empty()) {
        context.info->selStackBack.emplace_back();
    }
    context.info->selStackBack.back() = std::move(item);
}

void SelectionSingleton::selStackGoBack(int count, const char* pDocName)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    if ((int)context.info->selStackBack.size() < count) {
        count = context.info->selStackBack.size();
    }
    if (count <= 0) {
        return;
    }
    if (!context.info->selList.empty()) {
        selStackPush(false, true, pDocName);
        clearCompleteSelection(pDocName);
    }
    else {
        --count;
    }
    for (int i = 0; i < count; ++i) {
        context.info->selStackForward.push_front(std::move(context.info->selStackBack.back()));
        context.info->selStackBack.pop_back();
    }
    std::deque<SelStackItem> tmpStack;
    context.info->selStackForward.swap(tmpStack);
    while (!context.info->selStackBack.empty()) {
        if (restoreSelectionStackItem(context.info->selStackBack.back())) {
            break;
        }
        tmpStack.push_front(std::move(context.info->selStackBack.back()));
        context.info->selStackBack.pop_back();
    }
    context.info->selStackForward = std::move(tmpStack);
    getMainWindow()->updateActions();
}

void SelectionSingleton::selStackGoForward(int count, const char* pDocName)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    if ((int)context.info->selStackForward.size() < count) {
        count = context.info->selStackForward.size();
    }
    if (count <= 0) {
        return;
    }
    if (!context.info->selList.empty()) {
        selStackPush(false, true, pDocName);
        clearCompleteSelection(pDocName);
    }
    for (int i = 0; i < count; ++i) {
        context.info->selStackBack.push_back(context.info->selStackForward.front());
        context.info->selStackForward.pop_front();
    }
    std::deque<SelStackItem> tmpStack;
    context.info->selStackForward.swap(tmpStack);
    while (true) {
        if (restoreSelectionStackItem(context.info->selStackBack.back()) || tmpStack.empty()) {
            break;
        }
        context.info->selStackBack.push_back(tmpStack.front());
        tmpStack.pop_front();
    }
    context.info->selStackForward = std::move(tmpStack);
    getMainWindow()->updateActions();
}

std::vector<SelectionObject> SelectionSingleton::selStackGet(
    const char* pDocName,
    ResolveMode resolve,
    int index
) const
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return {};
    }

    const SelStackItem* item = nullptr;
    if (index >= 0) {
        if (index >= (int)context.info->selStackBack.size()) {
            return {};
        }
        item = &context.info->selStackBack[context.info->selStackBack.size() - 1 - index];
    }
    else {
        index = -index - 1;
        if (index >= (int)context.info->selStackForward.size()) {
            return {};
        }
        item = &context.info->selStackBack[context.info->selStackForward.size() - 1 - index];
    }

    std::list<SelectionDescription> selList;
    for (auto& sobjT : *item) {
        SelectionDescription sel;
        if (checkSelection(
                sobjT.getDocumentName().c_str(),
                sobjT.getObjectName().c_str(),
                sobjT.getSubName().c_str(),
                ResolveMode::NoResolve,
                sel,
                &selList
            )
            == SelectionCheckResult::Available) {
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
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return false;
    }

    clearPickedList(context);

    std::ostringstream ss;
    bool anyLogged = false;

    if (!logDisabled) {
        ss << "Gui.Selection.addSelection(App.getDocument('" << pDocName << "').getObject('"
           << pObjectName << "'),[";
    }

    bool update = false;
    for (const auto& pSubName : pSubNames) {
        SelectionDescription temp;
        auto checkResult
            = checkSelection(pDocName, pObjectName, pSubName.c_str(), ResolveMode::NoResolve, temp);
        if (checkResult != SelectionCheckResult::Available) {
            continue;
        }

        temp.x = 0;
        temp.y = 0;
        temp.z = 0;

        if (!isSelectionAllowed(context, temp).allowed) {
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

        context.info->selList.push_back(temp);
        context.info->selStackForward.clear();

        SelectionChanges Chng(
            SelectionChanges::AddSelection,
            context.docName.c_str(),
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

    auto pDoc = getDocument(pDocName);
    if (!pDoc) {
        return false;
    }
    if (preselection.matches(pDocName, pObjectName, pSubName)) {
        if (show) {
            FC_TRACE("preselect signal");
            notify(SelectionChanges(
                SelectionChanges::SetPreselectSignal,
                pDoc->getName(),
                preselection.objectName,
                preselection.subName
            ));
        }
        else {
            rmvPreselect();
        }
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
        pDoc->getName(),
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


std::vector<SelectionChanges> SelectionSingleton::removeSelectionMatches(
    SelectionContext& context,
    const SelectionDescription& sel
)
{
    std::vector<SelectionChanges> changes;
    for (auto It = context.info->selList.begin(), ItNext = It; It != context.info->selList.end();
         It = ItNext) {
        ++ItNext;
        if (It->DocName != sel.DocName || It->FeatName != sel.FeatName) {
            continue;
        }
        // if no subname is specified, remove all subobjects of the matching object
        if (!sel.SubName.empty()) {
            // otherwise, match subojects with common prefix, separated by '.'
            if (!boost::starts_with(It->SubName, sel.SubName)
                || (It->SubName.length() != sel.SubName.length()
                    && It->SubName[sel.SubName.length() - 1] != '.')) {
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

        // destroy the SelectionDescription item
        context.info->selList.erase(It);
    }

    return changes;
}

void SelectionSingleton::rmvSelection(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    const std::vector<SelObj>* pickedList
)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    if (pickedList) {
        replacePickedList(context, *pickedList);
    }

    if (!pDocName) {
        return;
    }

    SelectionDescription temp;
    auto checkResult = checkSelection(pDocName, pObjectName, pSubName, ResolveMode::NoResolve, temp);
    if (checkResult == SelectionCheckResult::Invalid) {
        return;
    }

    auto changes = removeSelectionMatches(context, temp);

    // NOTE: It can happen that there are nested calls of rmvSelection()
    // so that it's not safe to invoke the notifications inside the loop
    // as this can invalidate the iterators and thus leads to undefined
    // behaviour.
    // So, the notification is done after the loop, see also #0003469
    notifySelectionRemovals(changes);
}

int SelectionSingleton::visibilityValue(VisibleState vis)
{
    switch (vis) {
        case VisShow:
            return 1;
        case VisToggle:
            return -1;
        default:
            return 0;
    }
}

bool SelectionSingleton::requestedVisibility(int visible, bool currentVisible)
{
    if (visible >= 0) {
        return visible != 0;
    }
    return !currentVisible;
}

std::vector<SelectionSingleton::VisibilitySelection> SelectionSingleton::visibilitySelectionSnapshot(
    const SelectionContext& context
) const
{
    std::vector<VisibilitySelection> sels;
    sels.reserve(context.info->selList.size());
    for (auto& sel : context.info->selList) {
        if (sel.DocName.empty() || sel.FeatName.empty() || !sel.pObject) {
            continue;
        }
        sels.emplace_back(sel.DocName, sel.FeatName, sel.SubName);
    }
    return sels;
}

bool SelectionSingleton::resolveVisibilityTarget(
    const VisibilitySelection& sel,
    VisibilityTarget& target
) const
{
    target = VisibilityTarget {};

    App::Document* doc = App::GetApplication().getDocument(sel.DocName.c_str());
    if (!doc) {
        return false;
    }
    App::DocumentObject* obj = doc->getObject(sel.FeatName.c_str());
    if (!obj) {
        return false;
    }

    target.object = obj->resolve(sel.SubName.c_str(), &target.parent, &target.elementName);
    return target.object && target.object->isAttachedToDocument()
        && (!target.parent || target.parent->isAttachedToDocument());
}

SelectionSingleton::VisibilityElementResult SelectionSingleton::applyElementVisibility(
    const VisibilitySelection& sel,
    const VisibilityTarget& target,
    int visible,
    VisibilityFilter& filter
)
{
    if (!target.parent) {
        return VisibilityElementResult::FallBack;
    }

    if (!filter.insert(std::make_pair(target.object, target.parent)).second) {
        return VisibilityElementResult::Handled;
    }

    int visElement = target.parent->isElementVisible(target.elementName.c_str());
    if (visElement < 0) {
        return VisibilityElementResult::FallBack;
    }

    if (visElement > 0) {
        visElement = 1;
    }
    if (visible >= 0) {
        if (visElement == visible) {
            return VisibilityElementResult::Handled;
        }
        visElement = visible;
    }
    else {
        visElement = !visElement;
    }

    if (!visElement) {
        updateSelection(false, sel.DocName.c_str(), sel.FeatName.c_str(), sel.SubName.c_str());
    }
    target.parent->setElementVisible(target.elementName.c_str(), visElement ? true : false);
    if (visElement) {
        updateSelection(true, sel.DocName.c_str(), sel.FeatName.c_str(), sel.SubName.c_str());
    }
    return VisibilityElementResult::Handled;
}

void SelectionSingleton::applyObjectVisibility(
    const VisibilitySelection& sel,
    App::DocumentObject* obj,
    int visible,
    VisibilityFilter& filter
)
{
    if (!filter.insert(std::make_pair(obj, static_cast<App::DocumentObject*>(nullptr))).second) {
        return;
    }

    auto vp = Application::Instance->getViewProvider(obj);

    if (vp) {
        bool visObject = requestedVisibility(visible, vp->isShow());

        if (visObject) {
            vp->show();
            updateSelection(visObject, sel.DocName.c_str(), sel.FeatName.c_str(), sel.SubName.c_str());
        }
        else {
            updateSelection(visObject, sel.DocName.c_str(), sel.FeatName.c_str(), sel.SubName.c_str());
            vp->hide();
        }
    }
}

void SelectionSingleton::setVisible(VisibleState vis, const char* pDocName)
{
    VisibilityFilter filter;
    int visible = visibilityValue(vis);

    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    // Copy the selection in case it changes during this function.
    auto sels = visibilitySelectionSnapshot(context);

    for (auto& sel : sels) {
        VisibilityTarget target;
        if (!resolveVisibilityTarget(sel, target)) {
            continue;
        }

        if (applyElementVisibility(sel, target, visible, filter) == VisibilityElementResult::Handled) {
            continue;
        }

        applyObjectVisibility(sel, target.object, visible, filter);
    }
}

void SelectionSingleton::setSelection(const char* pDocName, const std::vector<App::DocumentObject*>& sel)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    clearPickedList(context);

    bool touched = false;
    for (auto obj : sel) {
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }
        SelectionDescription temp;
        auto checkResult
            = checkSelection(pDocName, obj->getNameInDocument(), nullptr, ResolveMode::NoResolve, temp);
        if (checkResult != SelectionCheckResult::Available) {
            continue;
        }
        touched = true;
        context.info->selList.push_back(temp);
    }

    if (touched) {
        context.info->selStackForward.clear();
        notify(SelectionChanges(SelectionChanges::SetSelection, context.docName.c_str()));
        getMainWindow()->updateActions();
    }
}

bool SelectionSingleton::clearDocumentSelectionEntries(SelectionContext& context)
{
    bool touched = false;
    for (auto it = context.info->selList.begin(); it != context.info->selList.end();) {
        if (it->DocName == context.docName.c_str()) {
            touched = true;
            it = context.info->selList.erase(it);
        }
        else {
            ++it;
        }
    }
    return touched;
}

void SelectionSingleton::notifyViewProvidersOfClearSelection(
    const std::list<SelectionDescription>& selections,
    const char* pDocName
) const
{
    std::set<ViewProvider*> viewProviders;
    for (const SelectionDescription& sel : selections) {
        if (auto vp = Application::Instance->getViewProvider(sel.pObject)) {
            viewProviders.insert(vp);
        }
    }

    for (auto& vp : viewProviders) {
        SelectionChanges Chng(SelectionChanges::ClrSelection, pDocName);
        vp->onSelectionChanged(Chng);
    }
}

void SelectionSingleton::logDocumentClearSelection(const std::string& docName, bool clearPreSelect) const
{
    if (logDisabled) {
        return;
    }

    std::ostringstream ss;
    ss << "Gui.Selection.clearSelection('" << docName << "'";
    if (!clearPreSelect) {
        ss << ", False";
    }
    ss << ')';
    Application::Instance->macroManager()->addLine(MacroManager::Cmt, ss.str().c_str());
}

void SelectionSingleton::logCompleteClearSelection(bool clearPreSelect) const
{
    if (logDisabled) {
        return;
    }

    Application::Instance->macroManager()->addLine(
        MacroManager::Cmt,
        clearPreSelect ? "Gui.Selection.clearSelection()" : "Gui.Selection.clearSelection(False)"
    );
}

void SelectionSingleton::clearSelection(const char* pDocName, bool clearPreSelect)
{
    // Because the introduction of external editing, it is best to make
    // clearSelection(0) behave as clearCompleteSelection(), which is the same
    // behavior of python Selection.clearSelection(None)
    if (!pDocName || !pDocName[0] || strcmp(pDocName, "*") == 0) {
        clearCompleteSelection(pDocName, clearPreSelect);
        return;
    }

    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    clearPickedList(context);

    if (clearPreSelect && preselection.docName == context.docName) {
        rmvPreselect();
    }

    if (!clearDocumentSelectionEntries(context)) {
        return;
    }

    logDocumentClearSelection(context.docName, clearPreSelect);

    notify(SelectionChanges(SelectionChanges::ClrSelection, context.docName.c_str()));

    getMainWindow()->updateActions();
}

void SelectionSingleton::clearCompleteSelection(const char* pDocName, bool clearPreSelect)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }

    clearPickedList(context);

    if (clearPreSelect) {
        rmvPreselect();
    }

    if (context.info->selList.empty()) {
        return;
    }

    logCompleteClearSelection(clearPreSelect);

    // Send the clear selection notification to all view providers associated with the
    // objects being deselected.

    notifyViewProvidersOfClearSelection(context.info->selList, pDocName);

    context.info->selList.clear();

    SelectionChanges Chng(SelectionChanges::ClrSelection, context.docName.c_str());

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
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return false;
    }

    SelectionDescription sel;
    return checkSelection(pDocName, pObjectName, pSubName, resolve, sel, &context.info->selList)
        == SelectionCheckResult::Selected;
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

    auto foundContext = docSelectionContext.find(pObject->getDocument());
    if (foundContext == docSelectionContext.end()) {
        return false;
    }

    SelectionDescription sel;
    return checkSelection(
               pObject->getDocument()->getName(),
               pObject->getNameInDocument(),
               pSubName,
               resolve,
               sel,
               &foundContext->second.selList
           )
        == SelectionCheckResult::Selected;
}

SelectionSingleton::SelectionCheckResult SelectionSingleton::checkSelection(
    const char* pDocName,
    const char* pObjectName,
    const char* pSubName,
    ResolveMode resolve,
    SelectionDescription& sel,
    const std::list<SelectionDescription>* selList
) const
{
    const bool reportErrors = !selList;
    std::string subNamePrefix;
    auto checkResult = resolveSelectionDescription(
        pDocName,
        pObjectName,
        pSubName,
        resolve,
        sel,
        subNamePrefix,
        reportErrors
    );
    if (checkResult != SelectionCheckResult::Available) {
        return checkResult;
    }

    selList = selectionListForCheck(sel.DocName.c_str(), selList);
    if (!selList) {
        return SelectionCheckResult::Invalid;
    }

    return findSelectionMatch(sel.DocName.c_str(), pSubName, subNamePrefix, resolve, sel, *selList);
}

SelectionSingleton::SelectionCheckResult SelectionSingleton::resolveSelectionDescription(
    const char* pDocName,
    const char* pObjectName,
    const char*& pSubName,
    ResolveMode resolve,
    SelectionDescription& sel,
    std::string& subNamePrefix,
    bool reportErrors
) const
{
    sel.pDoc = getDocument(pDocName);
    if (!sel.pDoc) {
        if (reportErrors) {
            FC_ERR("Cannot find document");
        }
        return SelectionCheckResult::Invalid;
    }

    pDocName = sel.pDoc->getName();
    sel.DocName = pDocName == nullptr ? std::string() : pDocName;

    if (pObjectName) {
        sel.pObject = sel.pDoc->getObject(pObjectName);
    }
    else {
        sel.pObject = nullptr;
    }
    if (!sel.pObject) {
        if (reportErrors) {
            FC_ERR("Object not found");
        }
        return SelectionCheckResult::Invalid;
    }
    if (sel.pObject->testStatus(App::ObjectStatus::Remove)) {
        return SelectionCheckResult::Invalid;
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
        if (reportErrors) {
            FC_ERR(
                "Sub-object " << sel.DocName << '#' << sel.FeatName << '.' << sel.SubName << " not found"
            );
        }
        return SelectionCheckResult::Invalid;
    }
    if (sel.pResolvedObject->testStatus(App::ObjectStatus::Remove)) {
        return SelectionCheckResult::Invalid;
    }
    if (pSubName && element) {
        subNamePrefix = std::string(pSubName, element - pSubName);
        if (!sel.elementName.newName.empty()) {
            // make sure the selected sub name is a new style if available
            sel.SubName = subNamePrefix + sel.elementName.newName;
            pSubName = sel.SubName.c_str();
        }
    }

    return SelectionCheckResult::Available;
}

const std::list<SelectionSingleton::SelectionDescription>* SelectionSingleton::selectionListForCheck(
    const char* pDocName,
    const std::list<SelectionDescription>* selList
) const
{
    if (selList) {
        return selList;
    }

    auto context = getSelectionContext(pDocName);
    if (context.info) {
        return &context.info->selList;
    }

    return nullptr;
}

SelectionSingleton::SelectionCheckResult SelectionSingleton::findSelectionMatch(
    const char* pDocName,
    const char* pSubName,
    const std::string& subNamePrefix,
    ResolveMode resolve,
    const SelectionDescription& sel,
    const std::list<SelectionDescription>& selList
)
{
    if (!pSubName) {
        pSubName = "";
    }

    for (const auto& s : selList) {
        if (s.DocName == pDocName && s.FeatName == sel.FeatName) {
            if (s.SubName == pSubName) {
                return SelectionCheckResult::Selected;
            }
            if (resolve > ResolveMode::OldStyleElement
                && boost::starts_with(s.SubName, subNamePrefix)) {
                return SelectionCheckResult::Selected;
            }
        }
    }
    if (resolve == ResolveMode::OldStyleElement) {
        for (const auto& s : selList) {
            if (s.pResolvedObject != sel.pResolvedObject) {
                continue;
            }
            if (!pSubName[0]) {
                return SelectionCheckResult::Selected;
            }
            if (!s.elementName.newName.empty()) {
                if (s.elementName.newName == sel.elementName.newName) {
                    return SelectionCheckResult::Selected;
                }
            }
            else if (s.SubName == sel.elementName.oldName) {
                return SelectionCheckResult::Selected;
            }
        }
    }
    return SelectionCheckResult::Available;
}

std::string SelectionSingleton::getSelectedElement(App::DocumentObject* obj, const char* pSubName) const
{
    if (!obj) {
        return {};
    }
    auto context = getSelectionContext(obj->getDocument()->getName());

    for (auto selected : context.info->selList) {
        if (selected.pObject == obj) {
            auto len = selected.SubName.length();
            if (!len) {
                return {};
            }
            if (pSubName
                && strncmp(pSubName, selected.SubName.c_str(), selected.SubName.length()) == 0) {
                if (pSubName[len] == 0 || pSubName[len - 1] == '.') {
                    return selected.SubName;
                }
            }
        }
    }
    return {};
}

std::vector<SelectionChanges> SelectionSingleton::removeDeletedObjectSelections(
    SelectionInfo& info,
    const App::DocumentObject& obj
)
{
    std::vector<SelectionChanges> changes;
    for (auto it = info.selList.begin(), itNext = it; it != info.selList.end(); it = itNext) {
        ++itNext;
        if (it->pResolvedObject == &obj || it->pObject == &obj) {
            changes.emplace_back(
                SelectionChanges::RmvSelection,
                it->DocName,
                it->FeatName,
                it->SubName,
                it->TypeName
            );
            info.selList.erase(it);
        }
    }
    return changes;
}

bool SelectionSingleton::removeDeletedObjectFromPickedList(
    SelectionInfo& info,
    const App::DocumentObject& obj
)
{
    bool changed = false;
    for (auto it = info.pickedList.begin(), itNext = it; it != info.pickedList.end(); it = itNext) {
        ++itNext;
        auto& sel = *it;
        if (sel.DocName == obj.getDocument()->getName() && sel.FeatName == obj.getNameInDocument()) {
            changed = true;
            info.pickedList.erase(it);
        }
    }
    return changed;
}

void SelectionSingleton::slotDeletedObject(const App::DocumentObject& Obj)
{
    if (!Obj.isAttachedToDocument()) {
        return;
    }

    // For safety reason, don't bother checking
    rmvPreselect();

    SelectionInfo& info = docSelectionContext[Obj.getDocument()];

    // Remove also from the selection, if selected
    // We don't walk down the hierarchy for each selection, so there may be stray selection
    auto changes = removeDeletedObjectSelections(info, Obj);
    notifySelectionRemovals(changes);

    if (!info.pickedList.empty() && removeDeletedObjectFromPickedList(info, Obj)) {
        notify(SelectionChanges(SelectionChanges::PickedListChanged, Obj.getDocument()->getName()));
    }
}
void SelectionSingleton::slotClosedDocument(const App::Document& doc)
{
    // const_cast is ok because we just use doc as a key
    docSelectionContext.erase(const_cast<App::Document*>(&doc));
}

void SelectionSingleton::setSelectionStyle(SelectionStyle selStyle, const char* pDocName)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return;
    }
    context.info->selectionStyle = selStyle;
}

SelectionSingleton::SelectionStyle SelectionSingleton::getSelectionStyle(const char* pDocName)
{
    auto context = getSelectionContext(pDocName);
    if (!context.info) {
        return SelectionSingleton::SelectionStyle::NormalSelection;
    }
    return context.info->selectionStyle;
}
SelectionSingleton::SelectionContext SelectionSingleton::getSelectionContext(const char* pDocName)
{
    // Some functions might receive "*" for document selection.
    // This is because there used to be a single selection context
    // for the whole application so "*" meant all documents
    // now that there is a selection context per document
    // we interpret it as "active document" because most (all?)
    // operations on freecad are meant to act on the current active
    // document anyway
    if (pDocName && strcmp(pDocName, "*") == 0) {
        pDocName = nullptr;
    }

    if (App::Document* doc = getDocument(pDocName)) {
        return SelectionContext {.info = &docSelectionContext[doc], .docName = doc->getName()};
    }
    return SelectionContext {.info = nullptr, .docName = std::string()};
}
SelectionSingleton::SelectionConstContext SelectionSingleton::getSelectionContext(
    const char* pDocName
) const
{
    // Some functions might receive "*" for document selection.
    // This is because there used to be a single selection context
    // for the whole application so "*" meant all documents
    // now that there is a selection context per document
    // we interpret it as "active document" because most (all?)
    // operations on freec  ad are meant to act on the current active
    // document anyway
    if (pDocName && strcmp(pDocName, "*") == 0) {
        pDocName = nullptr;
    }

    if (App::Document* doc = getDocument(pDocName)) {
        auto foundContext = docSelectionContext.find(doc);
        if (foundContext != docSelectionContext.end()) {
            return SelectionConstContext {.info = &foundContext->second, .docName = doc->getName()};
        }
    }
    return SelectionConstContext {.info = nullptr, .docName = std::string()};
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
    App::GetApplication().signalDeleteDocument.connect(
        std::bind(&Gui::SelectionSingleton::slotClosedDocument, this, sp::_1)
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
