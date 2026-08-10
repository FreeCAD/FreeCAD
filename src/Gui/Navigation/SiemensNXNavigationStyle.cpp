// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <QApplication>

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoMotion3Event.h>

#include "Camera.h"
#include "SiemensNXNavigationStyle.h"
#include "View3DInventorViewer.h"

// NOLINTBEGIN(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
using namespace Gui;
using NS = SiemensNXNavigationStyle;

namespace
{

bool noMouseButtons(const NavigationInputState& input)
{
    return (input.chord() & NavigationInputState::ButtonMask) == 0U;
}

}  // namespace

/* TRANSLATOR Gui::SiemensNXNavigationStyle */

TYPESYSTEM_SOURCE(Gui::SiemensNXNavigationStyle, Gui::UserNavigationStyle)

SiemensNXNavigationStyle::SiemensNXNavigationStyle() = default;

SiemensNXNavigationStyle::~SiemensNXNavigationStyle() = default;

SbBool SiemensNXNavigationStyle::processSoEvent(const SoEvent* const event)
{
    if (isSeekMode()) {
        return inherited::processSoEvent(event);
    }

    if (!isAnimating() && isViewing()) {
        setViewing(false);
    }

    syncModifierKeys(event);

    if (!viewer->isEditing() && handleEventInForeground(event)) {
        return true;
    }

    if (event->isOfType(SoMotion3Event::getClassTypeId())) {
        processMotionEvent(static_cast<const SoMotion3Event*>(event));
        return true;
    }

    bool processed = false;
    if (event->isOfType(SoKeyboardEvent::getClassTypeId())) {
        processed = processKeyboardEvent(static_cast<const SoKeyboardEvent*>(event));
    }

    updateInputState(event);

    const NavigationEventView navigationEvent(event, currentInputState());
    if (!processed) {
        processed = dispatchEvent(navigationEvent);
    }

    if (!processed) {
        return inherited::processSoEvent(event);
    }

    return processed;
}

bool NS::dispatchEvent(const NavigationEventView& event)
{
    switch (state) {
        case State::Idle:
            return handleIdle(event);
        case State::AwaitingRelease:
            return handleAwaitingRelease(event);
        case State::AwaitingMove:
            return handleAwaitingMove(event);
        case State::Rotate:
            return handleRotate(event);
        case State::Pan:
            return handlePan(event);
        case State::Zoom:
            return handleZoom(event);
    }

    return {};
}

bool NS::handleIdle(const NavigationEventView& event)
{
    switch (getViewingMode()) {
        case NavigationStyle::SEEK_WAIT_MODE:
            if (event.isPress(SoMouseButtonEvent::BUTTON1)) {
                seekToPoint(event.event()->getPosition());
                setViewingMode(NavigationStyle::SEEK_MODE);
                transitionTo(State::AwaitingRelease, event.event());
                return true;
            }
            break;

        case NavigationStyle::SPINNING:
        case NavigationStyle::SEEK_MODE:
            if (event.isMouseButton()) {
                transitionTo(State::AwaitingRelease, event.event());
                return true;
            }
            if (event.isKeyboard()) {
                setViewingMode(NavigationStyle::IDLE);
            }
            break;

        case NavigationStyle::BOXZOOM:
            return false;

        default:
            break;
    }

    if (event.isRelease(SoMouseButtonEvent::BUTTON2) && noMouseButtons(event.input())
        && !viewer->isEditing() && isPopupMenuEnabled()) {
        openPopupMenu(event.event()->getPosition());
    }

    if (event.isPress(SoMouseButtonEvent::BUTTON3)) {
        if (event.input().shift) {
            transitionTo(State::Pan, event.event());
            return true;
        }

        if (event.input().middle) {
            transitionTo(State::AwaitingMove, event.event());
            return true;
        }
    }

    return false;
}

bool NS::handleAwaitingRelease(const NavigationEventView& event)
{
    if (event.isMouseButton() && noMouseButtons(event.input())) {
        transitionTo(State::Idle, event.event());
    }
    return false;
}

bool NS::handleAwaitingMove(const NavigationEventView& event)
{
    bool processed = event.isMouseButton() || event.isPointerMotion();

    if (event.isPointerMotion()) {
        transitionTo(State::Rotate, event.event());
        return processed;
    }

    if (event.isPress(SoMouseButtonEvent::BUTTON2) && event.input().middle) {
        transitionTo(State::Pan, event.event());
        return processed;
    }

    if (event.isKeyPress(SoKeyboardEvent::LEFT_SHIFT)) {
        transitionTo(State::Pan, event.event());
        return true;
    }

    if (event.isPress(SoMouseButtonEvent::BUTTON1) && event.input().middle) {
        transitionTo(State::Zoom, event.event());
        return processed;
    }

    if (event.isKeyPress(SoKeyboardEvent::LEFT_CONTROL)) {
        transitionTo(State::Zoom, event.event());
        return true;
    }

    if (event.isRelease(SoMouseButtonEvent::BUTTON3) && noMouseButtons(event.input())) {
        const auto& data = std::get<AwaitingMoveData>(stateData);
        const SbTime elapsed = event.event()->getTime() - data.pressedAt;
        const double doubleClickInterval = QApplication::doubleClickInterval() / 1000.0;
        if (elapsed.getValue() < doubleClickInterval) {
            processed = true;
            lookAtPoint(event.event()->getPosition());
        }
        transitionTo(State::Idle, event.event());
        return processed;
    }

    return processed;
}

bool NS::handleRotate(const NavigationEventView& event)
{
    bool processed = false;
    if (event.isPointerMotion()) {
        addToLog(event.event()->getPosition(), event.event()->getTime());
        spin(normalizePixelPos(event.event()->getPosition()));
        moveCursorPosition();
        processed = true;
    }

    if (event.isPress(SoMouseButtonEvent::BUTTON2) && event.input().middle) {
        transitionTo(State::Pan, event.event());
        return true;
    }

    if (event.isKeyPress(SoKeyboardEvent::LEFT_SHIFT)) {
        transitionTo(State::Pan, event.event());
        return true;
    }

    if (event.isPress(SoMouseButtonEvent::BUTTON1) && event.input().middle) {
        transitionTo(State::Zoom, event.event());
        return true;
    }

    if (event.isKeyPress(SoKeyboardEvent::LEFT_CONTROL)) {
        transitionTo(State::Zoom, event.event());
        return true;
    }

    if (event.isRelease(SoMouseButtonEvent::BUTTON3) && noMouseButtons(event.input())) {
        transitionTo(State::Idle, event.event());
        return true;
    }

    return processed;
}

bool NS::handlePan(const NavigationEventView& event)
{
    auto& data = std::get<PanZoomData>(stateData);

    bool processed = false;
    if (event.isPointerMotion()) {
        const SbVec2s position = event.event()->getPosition();
        panCamera(
            viewer->getSoRenderManager()->getCamera(),
            data.viewportAspect,
            panningplane,
            normalizePixelPos(position),
            normalizePixelPos(data.previousPosition)
        );
        data.previousPosition = position;
        processed = true;
    }

    if (event.isRelease(SoMouseButtonEvent::BUTTON2) && event.input().middle) {
        transitionTo(State::Rotate, event.event());
        return true;
    }

    if (event.isKeyRelease(SoKeyboardEvent::LEFT_SHIFT) && event.input().middle) {
        transitionTo(State::Rotate, event.event());
        return true;
    }

    if (event.isRelease(SoMouseButtonEvent::BUTTON3)) {
        transitionTo(State::Idle, event.event());
        return true;
    }

    return processed;
}

bool NS::handleZoom(const NavigationEventView& event)
{
    bool processed = false;
    if (event.isPointerMotion()) {
        auto& data = std::get<PanZoomData>(stateData);
        const SbVec2s position = event.event()->getPosition();
        zoomByCursor(normalizePixelPos(position), normalizePixelPos(data.previousPosition));
        data.previousPosition = position;
        processed = true;
    }

    if (event.isRelease(SoMouseButtonEvent::BUTTON1) && event.input().middle) {
        transitionTo(State::Rotate, event.event());
        return true;
    }

    if (event.isKeyRelease(SoKeyboardEvent::LEFT_CONTROL) && event.input().middle) {
        transitionTo(State::Rotate, event.event());
        return true;
    }

    if (event.isRelease(SoMouseButtonEvent::BUTTON3)) {
        transitionTo(State::Idle, event.event());
        return true;
    }

    return processed;
}

void NS::transitionTo(const State next, const SoEvent* const event)
{
    state = next;
    stateData = std::monostate {};

    switch (next) {
        case State::Idle:
            setViewingMode(NavigationStyle::IDLE);
            break;
        case State::AwaitingRelease:
            break;
        case State::AwaitingMove:
            enterAwaitingMove(event);
            break;
        case State::Rotate:
            enterRotate(event);
            break;
        case State::Pan:
            enterPan(event);
            break;
        case State::Zoom:
            enterZoom(event);
            break;
    }
}

void NS::enterAwaitingMove(const SoEvent* const event)
{
    setViewingMode(NavigationStyle::DRAGGING);
    stateData = AwaitingMoveData {.pressedAt = event->getTime()};
}

void NS::enterRotate(const SoEvent* const event)
{
    saveCursorPosition(event);
    setViewingMode(NavigationStyle::DRAGGING);
}

void NS::enterPan(const SoEvent* const event)
{
    setViewingMode(NavigationStyle::PANNING);
    const float viewportAspect
        = viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
    centerTime = event->getTime();
    setupPanningPlane(getCamera());
    stateData
        = PanZoomData {.previousPosition = event->getPosition(), .viewportAspect = viewportAspect};
}

void NS::enterZoom(const SoEvent* const event)
{
    setViewingMode(NavigationStyle::ZOOMING);
    stateData = PanZoomData {.previousPosition = event->getPosition()};
}

const char* NS::mouseButtons(const ViewerMode mode)
{
    switch (mode) {
        case NavigationStyle::SELECTION:
            return QT_TR_NOOP("Press left mouse button");
        case NavigationStyle::PANNING:
            return QT_TR_NOOP("Press middle+right click");
        case NavigationStyle::DRAGGING:
            return QT_TR_NOOP("Press middle mouse button");
        case NavigationStyle::ZOOMING:
            return QT_TR_NOOP("Scroll mouse wheel");
        default:
            return "No description";
    }
}

std::string NS::userFriendlyName() const
{
    return {"Siemens NX"};
}

SbBool NS::processKeyboardEvent(const SoKeyboardEvent* const event)
{
    // See https://forum.freecad.org/viewtopic.php?t=96459
    // Isometric view: Home key button
    // Trimetric view: End key button
    // Fit all: CTRL+F
    // Normal view: F8
    switch (event->getKey()) {
        case SoKeyboardEvent::F:
            if (event->wasCtrlDown()) {
                viewer->viewAll();
                return true;
            }
            break;
        case SoKeyboardEvent::HOME:
            viewer->setCameraOrientation(Camera::rotation(Camera::Isometric));
            return true;
        case SoKeyboardEvent::END:
            viewer->setCameraOrientation(Camera::rotation(Camera::Trimetric));
            return true;
        case SoKeyboardEvent::F8:
            viewer->setCameraOrientation(Camera::rotation(Camera::Top));
            return true;
        default:
            break;
    }

    return inherited::processKeyboardEvent(event);
}

// NOLINTEND(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
