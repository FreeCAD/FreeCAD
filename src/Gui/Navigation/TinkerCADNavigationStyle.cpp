// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "Navigation/MappedNavigationStyle.h"
#include "View3DInventorViewer.h"

#include <QCoreApplication>

using namespace Gui;

namespace
{

constexpr auto LMB = NavigationInputState::LeftDown;
constexpr auto MMB = NavigationInputState::MiddleDown;
constexpr auto RMB = NavigationInputState::RightDown;
constexpr auto CTRL = NavigationInputState::CtrlDown;

using Mode = NavigationStyle::ViewerMode;
constexpr auto Select = Mode::SELECTION;
constexpr auto Pan = Mode::PANNING;
constexpr auto Rotate = Mode::DRAGGING;

constexpr NavigationRule tinkerCADRules[] {
    // Primary bindings.
    bind(LMB, Select),
    bind(CTRL | LMB, Select),
    bind(MMB, Pan, ownedBy(MMB)),
    bind(RMB, Rotate, ownedBy(RMB)),
};

constexpr NavigationProfile tinkerCADProfile {
    .rules = tinkerCADRules,
    .selectionDescription = QT_TR_NOOP("Press left mouse button"),
    .panDescription = QT_TR_NOOP("Press middle mouse button"),
    .rotateDescription = QT_TR_NOOP("Press right mouse button"),
    .zoomDescription = QT_TR_NOOP("Scroll mouse wheel"),
    .forceRotationOnAddedButton = false,
    .recenterOnMiddleClick = false,
};

}  // namespace

/* TRANSLATOR Gui::TinkerCADNavigationStyle */

TYPESYSTEM_SOURCE(Gui::TinkerCADNavigationStyle, Gui::MappedNavigationStyle)

TinkerCADNavigationStyle::TinkerCADNavigationStyle() = default;

TinkerCADNavigationStyle::~TinkerCADNavigationStyle() = default;

const NavigationProfile& TinkerCADNavigationStyle::profile() const
{
    return tinkerCADProfile;
}

void TinkerCADNavigationStyle::processStyleButtonEvent(EventContext& context)
{
    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    const bool press = event->getState() == SoButtonEvent::DOWN;

    if (press
        && (event->getButton() == SoMouseButtonEvent::BUTTON1
            || event->getButton() == SoMouseButtonEvent::BUTTON2)
        && (context.initialMode == NavigationStyle::PANNING
            || context.initialMode == NavigationStyle::ZOOMING)) {
        // TinkerCAD keeps the current pan/zoom mode when another button is added.
        context.resolvedMode = context.initialMode;
        if (event->getButton() == SoMouseButtonEvent::BUTTON1) {
            context.processed = processClickEvent(event);
        }
        else {
            context.processed = false;
        }
        return;
    }

    if (event->getButton() == SoMouseButtonEvent::BUTTON3 && !press
        && context.initialMode == NavigationStyle::PANNING) {
        context.processed = true;
        return;
    }

    if (event->getButton() != SoMouseButtonEvent::BUTTON2) {
        return;
    }

    if (press) {
        if (context.initialMode == NavigationStyle::IDLE) {
            context.processed = true;
        }
        return;
    }

    if (viewer->isEditing()) {
        return;
    }

    // TinkerCAD keeps the context menu on a simple right click, while a right-button
    // drag suppresses it after the shared gesture flags have been set.
    if (context.initialMode != NavigationStyle::PANNING
        && context.initialMode != NavigationStyle::ZOOMING && !hasDragged && !hasPanned
        && !hasZoomed && isPopupMenuEnabled()) {
        openPopupMenu(event->getPosition());
    }
    context.processed = true;
}
