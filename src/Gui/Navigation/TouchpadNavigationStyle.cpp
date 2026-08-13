// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
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

#include "Navigation/MappedNavigationStyle.h"
#include "View3DInventorViewer.h"

#include <Inventor/events/SoMouseButtonEvent.h>

using namespace Gui;

// ----------------------------------------------------------------------------------

namespace
{

constexpr auto LMB = NavigationInputState::LeftDown;
constexpr auto CTRL = NavigationInputState::CtrlDown;
constexpr auto SHIFT = NavigationInputState::ShiftDown;
constexpr auto ALT = NavigationInputState::AltDown;

using Mode = NavigationStyle::ViewerMode;
constexpr auto Idle = Mode::IDLE;
constexpr auto Select = Mode::SELECTION;
constexpr auto Pan = Mode::PANNING;
constexpr auto Rotate = Mode::DRAGGING;
constexpr auto Zoom = Mode::ZOOMING;

constexpr NavigationRule touchpadRules[] {
    // Modifier-only bindings.
    bind(CTRL, Idle),
    bind(SHIFT, Pan),
    bind(ALT, Rotate),
    bind(CTRL | SHIFT, Zoom),

    // Primary-button bindings.
    bind(LMB, Select),
    bind(CTRL | LMB, Select),
    bind(ALT | LMB, Rotate),
    bind(CTRL | SHIFT | LMB, Zoom),

    // Spin cancellation transitions.
    transition(Mode::SPINNING, LMB, Idle),
    transition(Mode::SPINNING, CTRL | LMB, Idle),
};

}  // namespace

/* TRANSLATOR Gui::TouchpadNavigationStyle */

TYPESYSTEM_SOURCE(Gui::TouchpadNavigationStyle, Gui::MappedNavigationStyle)

TouchpadNavigationStyle::TouchpadNavigationStyle() = default;

TouchpadNavigationStyle::~TouchpadNavigationStyle() = default;

const NavigationProfile& TouchpadNavigationStyle::profile() const
{
    static constexpr NavigationProfile touchpadProfile {
        .rules = touchpadRules,
        .selectionDescription = QT_TR_NOOP("Press left mouse button"),
        .panDescription = QT_TR_NOOP("Press Shift button"),
        .rotateDescription = QT_TR_NOOP("Press Alt button"),
        .zoomDescription = QT_TR_NOOP("Press Ctrl and Shift buttons"),
        .lockPrimaryAfterMultiButton = false,
        .preserveModeOnUnmappedInput = true,
    };
    return touchpadProfile;
}

bool TouchpadNavigationStyle::shouldForceRotationWhenButtonAdded(const EventContext& context) const
{
    if (!context.event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        return true;
    }

    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    if (event->getButton() != SoMouseButtonEvent::BUTTON1
        || event->getState() != SoButtonEvent::DOWN) {
        return true;
    }

    const unsigned int modifiers = context.chord & NavigationInputState::ModifierMask;
    return modifiers != 0U && modifiers != CtrlDown && modifiers != (CtrlDown | ShiftDown);
}

bool TouchpadNavigationStyle::shouldProcessMouseButtonEvent(const SoEvent* event) const
{
    if (!event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        return true;
    }

    return static_cast<const SoMouseButtonEvent*>(event)->getButton() != SoMouseButtonEvent::BUTTON3;
}

void TouchpadNavigationStyle::processStyleButtonEvent(EventContext& context)
{
    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    if (event->getButton() != SoMouseButtonEvent::BUTTON1) {
        return;
    }

    const bool leftPress = event->getState() == SoButtonEvent::DOWN;
    if (leftPress && (context.chord & AltDown) != 0U) {
        context.processed = true;
    }
    else if (!leftPress && context.initialMode == NavigationStyle::DRAGGING) {
        context.processed = false;
    }
}

bool TouchpadNavigationStyle::processStylePointerMotionEvent(EventContext& context)
{
    if (!context.event->isOfType(SoLocation2Event::getClassTypeId())
        || currentmode != NavigationStyle::PANNING) {
        return false;
    }

    if (!blockPan) {
        const SbViewportRegion& viewport = viewer->getSoRenderManager()->getViewportRegion();
        panCamera(
            viewer->getSoRenderManager()->getCamera(),
            viewport.getViewportAspectRatio(),
            panningplane,
            context.normalizedPosition,
            context.previousNormalizedPosition
        );
    }
    blockPan = false;
    context.processed = true;
    return true;
}

void TouchpadNavigationStyle::adjustResolvedMode(EventContext& context)
{
    if (context.chord == ShiftDown && context.initialMode != NavigationStyle::PANNING) {
        blockPan = true;
    }

    if (context.initialMode == NavigationStyle::ZOOMING
        && (context.chord == (CtrlDown | ShiftDown)
            || context.chord == (CtrlDown | ShiftDown | LeftDown))) {
        context.processed = true;
    }
}
