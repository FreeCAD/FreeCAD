// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Juergen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
// SPDX-FileNotice: Part of the FreeCAD project.

#include <deque>
#include <list>
#include <utility>
#include <vector>

#include <App/Application.h>
#include <App/DocumentObject.h>

#include "MainWindow.h"
#include "Selection.h"

using namespace Gui;

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
            == SelectionCheckResult::Available) {
            selList.push_back(sel);
        }
    }

    return getObjectList(pDocName, App::DocumentObject::getClassTypeId(), selList, resolve);
}
