// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Juergen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
// SPDX-FileNotice: Part of the FreeCAD project.

#include <exception>
#include <functional>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "Selection.h"
#include "SelectionObserver.h"
#include "ViewProviderDocumentObject.h"

FC_LOG_LEVEL_INIT("Selection", false, true, true)

using namespace Gui;
namespace sp = std::placeholders;

SelectionObserver::SelectionObserver(bool attach, ResolveMode resolve)
    : resolve(resolve)
    , blockedSelection(false)
{
    if (attach) {
        attachSelection();
    }
}

SelectionObserver::SelectionObserver(
    const ViewProviderDocumentObject* vp,
    bool attach,
    ResolveMode resolve
)
    : resolve(resolve)
    , blockedSelection(false)
{
    if (vp && vp->getObject() && vp->getObject()->getDocument()) {
        filterDocName = vp->getObject()->getDocument()->getName();
        filterObjName = vp->getObject()->getNameInDocument();
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
        if (!filterDocName.empty()) {
            Selection().addSelectionGate(
                new SelectionGateFilterExternal(filterDocName.c_str(), filterObjName.c_str())
            );
        }
    }
}

void SelectionObserver::_onSelectionChanged(const SelectionChanges& msg)
{
    try {
        if (blockedSelection) {
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
        if (!filterDocName.empty()) {
            Selection().rmvSelectionGate();
        }
    }
}
