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

#include <exception>
#include <functional>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "Selection.h"
#include "SelectionObserver.h"

FC_LOG_LEVEL_INIT("Selection", false, true, true)

using namespace Gui;
namespace sp = std::placeholders;

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

SelectionObserver::SelectionObserver(const ViewProviderDocumentObject*, bool attach, ResolveMode resolve)
    : resolve(resolve)
    , blockedSelection(false)
{
    // FIXME: Scope this constructor to the passed view provider's document.
    // It currently uses the active document instead, so callers opened on a
    // non-active document can observe the wrong selection stream.
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
