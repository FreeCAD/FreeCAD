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

#include <Inventor/nodes/SoCamera.h>
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
constexpr auto SHIFT = NavigationInputState::ShiftDown;

using Mode = NavigationStyle::ViewerMode;
constexpr auto Select = Mode::SELECTION;
constexpr auto Pan = Mode::PANNING;
constexpr auto Zoom = Mode::ZOOMING;

constexpr NavigationRule openSCADRules[] {
    // Primary bindings.
    bind(LMB, Select),
    bind(RMB, Pan, ownedBy(RMB)),
    bind(MMB, Zoom, ownedBy(MMB)),
    bind(SHIFT | RMB, Zoom, ownedBy(RMB)),
    bind(SHIFT | MMB, Zoom, ownedBy(MMB)),
};

constexpr NavigationProfile openSCADProfile {
    .rules = openSCADRules,
    .selectionDescription = QT_TR_NOOP("Press left mouse button"),
    .panDescription = QT_TR_NOOP("Press right mouse button and move mouse"),
    .rotateDescription = QT_TR_NOOP("Press left mouse button and move mouse"),
    .zoomDescription = QT_TR_NOOP("Press middle mouse button or SHIFT and right mouse button"),
    .recenterOnMiddleClick = false,
};

}  // namespace

/* TRANSLATOR Gui::OpenSCADNavigationStyle */

TYPESYSTEM_SOURCE(Gui::OpenSCADNavigationStyle, Gui::MappedNavigationStyle)

OpenSCADNavigationStyle::OpenSCADNavigationStyle() = default;

OpenSCADNavigationStyle::~OpenSCADNavigationStyle() = default;

const NavigationProfile& OpenSCADNavigationStyle::profile() const
{
    return openSCADProfile;
}

void OpenSCADNavigationStyle::processStyleButtonEvent(EventContext& context)
{
    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    const bool press = event->getState() == SoButtonEvent::DOWN;

    if (event->getButton() == SoMouseButtonEvent::BUTTON1) {
        lockrecenter = true;
        if (press
            && (context.initialMode == NavigationStyle::PANNING
                || context.initialMode == NavigationStyle::ZOOMING)) {
            context.resolvedMode = context.initialMode;
            context.processed = processClickEvent(event);
        }
        if (!press && context.initialMode == NavigationStyle::ZOOMING) {
            context.resolvedMode = NavigationStyle::IDLE;
            context.processed = true;
        }
        else if (!press && context.initialMode == NavigationStyle::DRAGGING) {
            setViewing(false);
            context.resolvedMode = NavigationStyle::IDLE;
            context.processed = true;
        }
    }
}

bool OpenSCADNavigationStyle::processStylePointerMotionEvent(EventContext& context)
{
    if (viewer->isEditing() || currentmode != NavigationStyle::SELECTION) {
        return false;
    }

    const auto* const event = static_cast<const SoLocation2Event*>(context.event);
    if (button1down && isDraggerUnderCursor(event->getPosition())) {
        context.resolvedMode = NavigationStyle::INTERACT;
    }
    else {
        context.resolvedMode = NavigationStyle::DRAGGING;
    }
    return true;
}

void OpenSCADNavigationStyle::zoomByCursor(const SbVec2f& thispos, const SbVec2f& prevpos)
{
    // OpenSCAD uses vertical mouse position, not horizontal, for zooming.
    float value = (thispos[1] - prevpos[1]) * 10.0F;
    if (invertZoom) {
        value = -value;
    }
    zoom(getCamera(), value);
}
