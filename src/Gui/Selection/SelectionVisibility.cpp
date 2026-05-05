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

#include <utility>
#include <vector>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include "Application.h"
#include "Selection.h"
#include "ViewProvider.h"

using namespace Gui;

namespace
{

int visibilityValue(SelectionSingleton::VisibleState vis)
{
    switch (vis) {
        case SelectionSingleton::VisShow:
            return 1;
        case SelectionSingleton::VisToggle:
            return -1;
        default:
            return 0;
    }
}

bool requestedVisibility(int visible, bool currentVisible)
{
    if (visible >= 0) {
        return visible != 0;
    }
    return !currentVisible;
}

}  // namespace

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
    VisibilityOperation& operation
)
{
    if (!target.parent) {
        return VisibilityElementResult::Fallback;
    }

    if (!operation.filter.insert(std::make_pair(target.object, target.parent)).second) {
        return VisibilityElementResult::Handled;
    }

    int visElement = target.parent->isElementVisible(target.elementName.c_str());
    if (visElement < 0) {
        return VisibilityElementResult::Fallback;
    }

    if (visElement > 0) {
        visElement = 1;
    }
    if (operation.visible >= 0) {
        if (visElement == operation.visible) {
            return VisibilityElementResult::Handled;
        }
        visElement = operation.visible;
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
    VisibilityOperation& operation
)
{
    if (
        !operation.filter.insert(std::make_pair(obj, static_cast<App::DocumentObject*>(nullptr))).second
    ) {
        return;
    }

    auto vp = Application::Instance->getViewProvider(obj);

    if (vp) {
        bool visObject = requestedVisibility(operation.visible, vp->isShow());

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
    VisibilityOperation operation {visibilityValue(vis)};

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

        if (applyElementVisibility(sel, target, operation) == VisibilityElementResult::Handled) {
            continue;
        }

        applyObjectVisibility(sel, target.object, operation);
    }
}
