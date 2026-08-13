// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2015 Kirill Gavrilov <kirill.gavrilov@opencascade.com>  *
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
constexpr auto CTRL = NavigationInputState::CtrlDown;

using Mode = NavigationStyle::ViewerMode;
constexpr auto Select = Mode::SELECTION;
constexpr auto Pan = Mode::PANNING;
constexpr auto Rotate = Mode::DRAGGING;
constexpr auto Zoom = Mode::ZOOMING;

constexpr NavigationRule openCascadeRules[] {
    // Primary bindings.
    bind(LMB, Select),
    bind(CTRL | LMB, Select),
    bind(MMB, Pan, ownedBy(MMB)),
    bind(CTRL | MMB, Pan, ownedBy(MMB)),
    bind(CTRL | RMB, Rotate, ownedBy(RMB)),
    bind(RMB, Mode::IDLE),

    // Zoom continuation while the primary button remains held.
    transition(Zoom, LMB, Zoom),
    transition(Zoom, CTRL | LMB, Zoom),
};

constexpr NavigationProfile openCascadeProfile {
    openCascadeRules,
    QT_TR_NOOP("Press left mouse button"),
    QT_TR_NOOP("Press Ctrl and middle mouse button"),
    QT_TR_NOOP("Press Ctrl and right mouse button"),
    QT_TR_NOOP("Press Ctrl and left mouse button"),
};

}  // namespace

/* TRANSLATOR Gui::OpenCascadeNavigationStyle */

TYPESYSTEM_SOURCE(Gui::OpenCascadeNavigationStyle, Gui::MappedNavigationStyle)

OpenCascadeNavigationStyle::OpenCascadeNavigationStyle() = default;

OpenCascadeNavigationStyle::~OpenCascadeNavigationStyle() = default;

const NavigationProfile& OpenCascadeNavigationStyle::profile() const
{
    return openCascadeProfile;
}

void OpenCascadeNavigationStyle::processStyleButtonEvent(EventContext& context)
{
    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    const bool press = event->getState() == SoButtonEvent::DOWN;

    if (press && event->getButton() == SoMouseButtonEvent::BUTTON2 && context.chord == RightDown) {
        context.resolvedMode = NavigationStyle::IDLE;
    }

    if (!press) {
        switch (event->getButton()) {
            case SoMouseButtonEvent::BUTTON1:
                if (context.initialMode == NavigationStyle::ZOOMING) {
                    context.resolvedMode = NavigationStyle::IDLE;
                    context.processed = true;
                }
                else if (context.initialMode == NavigationStyle::DRAGGING) {
                    setViewing(false);
                    context.resolvedMode = NavigationStyle::IDLE;
                    context.processed = true;
                }
                break;
            case SoMouseButtonEvent::BUTTON2:
                if (context.initialMode == NavigationStyle::DRAGGING) {
                    context.resolvedMode = NavigationStyle::IDLE;
                    context.processed = true;
                }
                break;
            case SoMouseButtonEvent::BUTTON3:
                if (context.initialMode == NavigationStyle::PANNING) {
                    context.resolvedMode = NavigationStyle::IDLE;
                    context.processed = true;
                }
                break;
            default:
                break;
        }
    }

    if (button1down && (button2down || button3down || ctrldown)) {
        clearSelectionStartPosition();
        context.processed = true;
    }
}

bool OpenCascadeNavigationStyle::processStylePointerMotionEvent(EventContext& context)
{
    if (currentmode != NavigationStyle::SELECTION || (context.chord & CtrlDown) == 0U) {
        return false;
    }

    if (context.chord == (CtrlDown | LeftDown)) {
        context.resolvedMode = NavigationStyle::ZOOMING;
        context.processed = true;
    }

    return true;
}

void OpenCascadeNavigationStyle::zoomByCursor(const SbVec2f& thispos, const SbVec2f& prevpos)
{
    float value = (thispos[0] - prevpos[0]) * 10.0F;
    if (invertZoom) {
        value = -value;
    }
    zoom(viewer->getSoRenderManager()->getCamera(), value);
}
