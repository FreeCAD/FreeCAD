// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Library General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>

#include "Inventor/SoMouseWheelEvent.h"
#include "Navigation/MappedNavigationStyle.h"
#include "View3DInventorViewer.h"

using namespace Gui;

namespace
{

constexpr auto LMB = NavigationInputState::LeftDown;
constexpr auto MMB = NavigationInputState::MiddleDown;
constexpr auto RMB = NavigationInputState::RightDown;
constexpr auto CTRL = NavigationInputState::CtrlDown;
constexpr auto SHIFT = NavigationInputState::ShiftDown;

using Mode = NavigationStyle::ViewerMode;
constexpr auto Select = Mode::SELECTION;
constexpr auto Pan = Mode::PANNING;
constexpr auto Rotate = Mode::DRAGGING;
constexpr auto Zoom = Mode::ZOOMING;

constexpr NavigationRule inventorRules[] {
    // Primary bindings.
    bind(LMB, Rotate, ownedBy(LMB)),
    bind(MMB, Pan, ownedBy(MMB)),
    bind(CTRL | SHIFT, Pan),
    bind(CTRL | SHIFT | LMB, Pan),
    bind(CTRL, Select),
    bind(CTRL | LMB, Select),
    bind(SHIFT, Select),
    bind(SHIFT | LMB, Select),
    bind(LMB | MMB, Zoom, ownedBy(LMB | MMB)),
    bind(CTRL | MMB, Zoom, ownedBy(MMB)),
    bind(CTRL | SHIFT | RMB, Zoom, ownedBy(RMB)),

    // Selection continuation while the primary button remains held.
    transition(Select, LMB, Select),
};

constexpr NavigationProfile inventorProfile {
    .rules = inventorRules,
    .selectionDescription = QT_TR_NOOP("Press Ctrl and left mouse button"),
    .panDescription = QT_TR_NOOP("Press middle mouse button"),
    .rotateDescription = QT_TR_NOOP("Press left mouse button"),
    .zoomDescription = QT_TR_NOOP("Scroll mouse wheel"),
    .forceRotationOnAddedButton = false,
    .lockPrimaryAfterMultiButton = false,
};

}  // namespace

// ----------------------------------------------------------------------------------

/* TRANSLATOR Gui::InventorNavigationStyle */

TYPESYSTEM_SOURCE(Gui::InventorNavigationStyle, Gui::MappedNavigationStyle)

InventorNavigationStyle::InventorNavigationStyle() = default;

InventorNavigationStyle::~InventorNavigationStyle() = default;

std::string InventorNavigationStyle::userFriendlyName() const
{
    // do not mark this for translation
    return "OpenInventor";
}

const NavigationProfile& InventorNavigationStyle::profile() const
{
    return inventorProfile;
}

void InventorNavigationStyle::processStyleButtonEvent(EventContext& context)
{
    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    if (event->getButton() != SoMouseButtonEvent::BUTTON1) {
        return;
    }

    const bool press = event->getState() == SoButtonEvent::DOWN;
    if (press && event->wasShiftDown() && context.initialMode != NavigationStyle::SELECTION) {
        centerTime = event->getTime();
        setupPanningPlane(getCamera());
        lockrecenter = false;
    }
    else if (!press && event->wasShiftDown() && context.initialMode != NavigationStyle::SELECTION) {
        const SbTime elapsed = event->getTime() - centerTime;
        const float doubleClickInterval = static_cast<float>(QApplication::doubleClickInterval())
            / 1000.0F;
        if (elapsed.getValue() < doubleClickInterval && !lockrecenter) {
            lookAtPoint(context.position);
            context.processed = true;
        }
    }
    else if (press && context.initialMode == NavigationStyle::IDLE) {
        setViewing(true);
        context.processed = true;
        lockrecenter = true;
    }
    else if (!press && context.initialMode == NavigationStyle::DRAGGING) {
        setViewing(false);
        context.processed = true;
        lockrecenter = true;
    }
}

void InventorNavigationStyle::adjustResolvedMode(EventContext& context)
{
    if (context.chord == 0U && context.initialMode == NavigationStyle::DRAGGING && doSpin()) {
        context.resolvedMode = NavigationStyle::SPINNING;
    }

    if (!context.event->isOfType(SoMouseWheelEvent::getClassTypeId()) && !viewer->isEditing()
        && context.initialMode != NavigationStyle::SELECTION
        && context.resolvedMode != NavigationStyle::SELECTION) {
        context.processed = true;
    }
}

bool InventorNavigationStyle::shouldPropagate(const EventContext& context) const
{
    if (context.event->isOfType(SoMouseWheelEvent::getClassTypeId())) {
        return true;
    }

    return context.initialMode == NavigationStyle::SELECTION
        || context.resolvedMode == NavigationStyle::SELECTION || viewer->isEditing();
}
