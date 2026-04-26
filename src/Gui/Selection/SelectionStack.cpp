// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Juergen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <deque>
#include <list>
#include <utility>
#include <vector>

#include <App/Application.h>
#include <App/DocumentObject.h>

#include "MainWindow.h"
#include "Selection.h"

using namespace Gui;

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
        // FIXME: Negative indices are documented as forward-stack lookups, so
        // this should index selStackForward here. Add coverage for the
        // negative-index path before changing the lookup.
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
