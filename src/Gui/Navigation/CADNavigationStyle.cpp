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

#include <QApplication>

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
constexpr auto SHIFT = NavigationInputState::ShiftDown;

using Mode = NavigationStyle::ViewerMode;
constexpr auto Select = Mode::SELECTION;
constexpr auto Pan = Mode::PANNING;
constexpr auto Rotate = Mode::DRAGGING;
constexpr auto Zoom = Mode::ZOOMING;

constexpr NavigationRule cadRules[] {
    // Primary bindings.
    bind(LMB, Select),
    bind(CTRL | LMB, Select),
    bind(MMB, Pan, ownedBy(MMB)),
    bind(CTRL | RMB, Pan, ownedBy(RMB)),
    bind(SHIFT | RMB, Rotate, ownedBy(RMB)),
    bind(CTRL | SHIFT | RMB, Zoom, ownedBy(RMB)),

    // Multi-button transitions into rotation.
    transition(Pan, LMB | MMB, Rotate, ownedByAny(LMB | MMB)),
    transition(Pan, RMB | MMB, Rotate, ownedByAny(RMB | MMB)),
    transition(Pan, CTRL | LMB | RMB, Rotate, ownedByAny(LMB | RMB)),
    transition(Zoom, LMB | MMB, Rotate, ownedByAny(LMB | MMB)),
    transition(Zoom, RMB | MMB, Rotate, ownedByAny(RMB | MMB)),
    transition(Zoom, CTRL | SHIFT | LMB | RMB, Rotate, ownedByAny(LMB | RMB)),

    // Release-order and mode-continuation rules.
    transition(Zoom, LMB, Zoom),
    transition(Zoom, CTRL | LMB, Zoom),
    transition(Zoom, MMB, Zoom, ownedBy(MMB)),
    transition(Rotate, MMB, Pan, ownedBy(MMB)),
};

constexpr NavigationProfile cadProfile {
    cadRules,
    QT_TR_NOOP("Press left mouse button"),
    QT_TR_NOOP("Press middle or ctrl+right mouse button"),
    QT_TR_NOOP("Press middle+left, middle+right or shift+right mouse button"),
    QT_TR_NOOP(
        "Scroll mouse wheel or keep middle button depressed\n"
        "while doing a left or right click and move the mouse up or down"
    ),
};

}  // namespace

/* TRANSLATOR Gui::CADNavigationStyle */

TYPESYSTEM_SOURCE(Gui::CADNavigationStyle, Gui::MappedNavigationStyle)

CADNavigationStyle::CADNavigationStyle() = default;

CADNavigationStyle::~CADNavigationStyle() = default;

const NavigationProfile& CADNavigationStyle::profile() const
{
    return cadProfile;
}

void CADNavigationStyle::processStyleButtonEvent(EventContext& context)
{
    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    if (event->getButton() == SoMouseButtonEvent::BUTTON2 && event->getState() == SoButtonEvent::UP
        && context.initialMode == NavigationStyle::DRAGGING && !button3down) {
        context.processed = true;
    }
}

void CADNavigationStyle::adjustResolvedMode(EventContext& context)
{
    if (context.initialMode == NavigationStyle::SPINNING) {
        context.resolvedMode = NavigationStyle::SPINNING;
        return;
    }

    if (!context.event->isOfType(SoMouseButtonEvent::getClassTypeId())
        || context.initialMode != NavigationStyle::DRAGGING
        || context.resolvedMode != NavigationStyle::PANNING || context.chord != MiddleDown) {
        return;
    }

    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    if (event->getState() != SoButtonEvent::UP) {
        return;
    }

    context.processed = true;
    const SbTime elapsed = event->getTime() - centerTime;
    const float doubleClickInterval = static_cast<float>(QApplication::doubleClickInterval())
        / 1000.0F;
    if (elapsed.getValue() < doubleClickInterval) {
        context.resolvedMode = NavigationStyle::ZOOMING;
    }
    else if (doSpin()) {
        context.resolvedMode = NavigationStyle::SPINNING;
    }
}
