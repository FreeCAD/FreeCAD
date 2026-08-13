// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Victor Titov (DeepSOIC) <vv.titov@gmail.com>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   License, or (at your option) any later version.                          *
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

#include "MayaGestureNavigationStyle.h"

#include <QApplication>

#include <cmath>

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMotion3Event.h>

#include "Camera.h"
#include "View3DInventorViewer.h"

using namespace Gui;

/* TRANSLATOR Gui::MayaGestureNavigationStyle */

TYPESYSTEM_SOURCE(Gui::MayaGestureNavigationStyle, Gui::UserNavigationStyle)

MayaGestureNavigationStyle::MayaGestureNavigationStyle()
    : mouseMoveThreshold(QApplication::startDragDistance())
{}

MayaGestureNavigationStyle::~MayaGestureNavigationStyle() = default;

int MayaGestureNavigationStyle::selectionMoveThreshold() const
{
    return mouseMoveThreshold;
}

const char* MayaGestureNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
        case NavigationStyle::SELECTION:
            return QT_TR_NOOP("Tap OR click left mouse button.");
        case NavigationStyle::PANNING:
            return QT_TR_NOOP("Drag screen with two fingers OR press Alt + middle mouse button.");
        case NavigationStyle::DRAGGING:
            return QT_TR_NOOP(
                "Drag screen with one finger OR press Alt + left mouse button. In Sketcher and "
                "other edit modes, hold Alt in addition."
            );
        case NavigationStyle::ZOOMING:
            return QT_TR_NOOP(
                "Pinch (place two fingers on the screen and drag them apart from or towards each "
                "other) OR scroll mouse wheel OR press Alt + right mouse button OR PgUp/PgDown on "
                "keyboard."
            );
        default:
            return "No description";
    }
}

void MayaGestureNavigationStyle::zoomByCursor(const SbVec2f& position, const SbVec2f& previousPosition)
{
    const float dx = position[0] - previousPosition[0];
    const float dy = position[1] - previousPosition[1];
    float value = (dx - dy) * 10.0F;

    if (invertZoom) {
        value = -value;
    }
    zoom(viewer->getSoRenderManager()->getCamera(), value);
}

void MayaGestureNavigationStyle::updateClickState(
    const NavigationInputState& before,
    const NavigationInputState& after
)
{
    if (deferredEvents.empty()) {
        return;
    }

    const unsigned int beforeButtons = before.chord() & NavigationInputState::ButtonMask;
    const unsigned int afterButtons = after.chord() & NavigationInputState::ButtonMask;
    const auto buttonCount = [](unsigned int buttons) {
        return ((buttons & NavigationInputState::LeftDown) != 0U)
            + ((buttons & NavigationInputState::MiddleDown) != 0U)
            + ((buttons & NavigationInputState::RightDown) != 0U);
    };

    if (buttonCount(beforeButtons) >= 2) {
        deferredClickIsComplex = true;
    }
    if (buttonCount(afterButtons) >= 2) {
        deferredClickIsComplex = true;
    }
}

void MayaGestureNavigationStyle::enterAwaitingMove(const EventContext& context)
{
    AwaitingMoveData data;
    data.pressPosition = context.event.event()->getPosition();
    stateData = std::move(data);
    state = State::AwaitingMove;
    if (deferredEvents.empty()) {
        deferredClickIsComplex = false;
    }
    deferredEvents.push_back(*context.event.mouseButton());
    setupPanningPlane(viewer->getSoRenderManager()->getCamera());
}

void MayaGestureNavigationStyle::enterMotion(State next)
{
    state = next;

    switch (next) {
        case State::Rotate:
            setViewingMode(NavigationStyle::DRAGGING);
            break;
        case State::Pan:
            setViewingMode(NavigationStyle::PANNING);
            break;
        case State::Zoom:
            setViewingMode(NavigationStyle::ZOOMING);
            break;
        default:
            break;
    }
}

void MayaGestureNavigationStyle::enterGesture(const SoEvent* event)
{
    state = State::Gesture;
    if (event->isOfType(SoGesturePanEvent::getClassTypeId())) {
        setViewingMode(NavigationStyle::PANNING);
    }
    else {
        setViewingMode(NavigationStyle::DRAGGING);
        saveCursorPosition(event);
    }
}

void MayaGestureNavigationStyle::leaveState()
{
    state = State::Idle;
    stateData = std::monostate {};
}

void MayaGestureNavigationStyle::replayDeferredEvents()
{
    for (const SoMouseButtonEvent& event : deferredEvents) {
        inherited::processSoEvent(&event);
    }
}

void MayaGestureNavigationStyle::clearDeferredEvents()
{
    deferredEvents.clear();
    deferredClickIsComplex = false;
}

bool MayaGestureNavigationStyle::hasDeferredEvents() const
{
    return !deferredEvents.empty();
}

NavigationEventOutcome MayaGestureNavigationStyle::handleIdle(const EventContext& context)
{
    const NavigationEventView& event = context.event;

    if (event.isKeyboard()) {
        const auto* keyboard = event.keyboard();
        switch (keyboard->getKey()) {
            case SoKeyboardEvent::S:
            case SoKeyboardEvent::HOME:
            case SoKeyboardEvent::LEFT_ARROW:
            case SoKeyboardEvent::UP_ARROW:
            case SoKeyboardEvent::RIGHT_ARROW:
            case SoKeyboardEvent::DOWN_ARROW:
                return {
                    .processed = static_cast<bool>(inherited::processSoEvent(event.event())),
                    .propagated = true,
                };
            case SoKeyboardEvent::PAGE_UP:
                if (keyboard->getState() == SoButtonEvent::DOWN) {
                    doZoom(
                        viewer->getSoRenderManager()->getCamera(),
                        getDelta(),
                        context.normalizedPosition
                    );
                }
                return {.processed = true};
            case SoKeyboardEvent::PAGE_DOWN:
                if (keyboard->getState() == SoButtonEvent::DOWN) {
                    doZoom(
                        viewer->getSoRenderManager()->getCamera(),
                        -getDelta(),
                        context.normalizedPosition
                    );
                }
                return {.processed = true};
            default:
                break;
        }
    }

    if (event.isMouseButton()) {
        return handleAwaitingMoveButton(context);
    }

    if (event.isGesture()) {
        return handleAwaitingMoveGesture(context);
    }

    return {};
}

NavigationEventOutcome MayaGestureNavigationStyle::handleAwaitingMoveButton(const EventContext& context)
{
    const NavigationEventView& event = context.event;
    const auto* mouse = event.mouseButton();
    const bool press = mouse->getState() == SoButtonEvent::DOWN;
    const auto button = mouse->getButton();

    if (button == SoMouseButtonEvent::BUTTON1 || button == SoMouseButtonEvent::BUTTON2) {
        if (press) {
            if (context.editing && button == SoMouseButtonEvent::BUTTON1
                && !context.event.input().alt) {
                clearDeferredEvents();
                leaveState();
                setViewingMode(NavigationStyle::SELECTION);
                return {};
            }

            if (state == State::Idle) {
                enterAwaitingMove(context);
            }
            else {
                deferredEvents.push_back(*mouse);
            }
            return {.processed = true};
        }

        if (!press && button == SoMouseButtonEvent::BUTTON1 && context.editing) {
            clearDeferredEvents();
            leaveState();
            setViewingMode(NavigationStyle::IDLE);
            const bool processed = inherited::processSoEvent(event.event());
            return {.processed = processed, .propagated = true};
        }

        if (button == SoMouseButtonEvent::BUTTON2 && !deferredClickIsComplex && !context.editing
            && isPopupMenuEnabled()) {
            clearDeferredEvents();
            leaveState();
            openPopupMenu(mouse->getPosition());
            return {.processed = true};
        }

        replayDeferredEvents();
        clearDeferredEvents();
        leaveState();
        const bool processed = inherited::processSoEvent(event.event());
        return {.processed = processed, .propagated = true};
    }

    if (button == SoMouseButtonEvent::BUTTON3) {
        if (press && context.event.input().alt) {
            setupPanningPlane(viewer->getCamera());
            enterMotion(State::Pan);
        }
        else if (press) {
            setupPanningPlane(viewer->getCamera());
            lookAtPoint(mouse->getPosition());
        }
        return {.processed = true};
    }

    return {};
}

NavigationEventOutcome MayaGestureNavigationStyle::handleAwaitingMoveMotion(const EventContext& context)
{
    auto& data = std::get<AwaitingMoveData>(stateData);
    const SbVec2s position = context.event.event()->getPosition();
    data.moveThresholdBroken |= SbVec2f(position - data.pressPosition).length() >= mouseMoveThreshold;

    if (data.moveThresholdBroken && hasDeferredEvents()) {
        const auto& input = context.event.input();
        const bool startedBoxSelection = input.left && input.shift && !input.right && !input.middle
            && tryStartBoxSelection(data.pressPosition,
                                    static_cast<const SoLocation2Event*>(context.event.event()),
                                    input.ctrl);
        if (startedBoxSelection) {
            clearDeferredEvents();
            leaveState();
            return {.processed = true};
        }

        const bool suppressLmbDrag = context.editing && !input.alt;
        if ((input.left && !suppressLmbDrag && input.alt) || (input.right && input.alt)) {
            clearDeferredEvents();
            saveCursorPosition(context.event.event());
            enterMotion(input.left ? State::Rotate : State::Zoom);
            return {.processed = true};
        }

        replayDeferredEvents();
        clearDeferredEvents();
        leaveState();
        const bool processed = inherited::processSoEvent(context.event.event());
        return {.processed = processed, .propagated = true};
    }

    return hasDeferredEvents() ? NavigationEventOutcome {.processed = true}
                               : NavigationEventOutcome {};
}

NavigationEventOutcome MayaGestureNavigationStyle::handleAwaitingMoveGesture(const EventContext& context)
{
    const NavigationEventView& event = context.event;
    if (context.event.input().right) {
        return {};
    }

    const auto* gesture = event.gesture();
    if (!gesture
        || (gesture->state != SoGestureEvent::SbGSStart
            && gesture->state != SoGestureEvent::SbGSUpdate)) {
        return {};
    }

    if (event.event()->isOfType(SoGesturePanEvent::getClassTypeId())) {
        setupPanningPlane(viewer->getSoRenderManager()->getCamera());
        enterGesture(event.event());
        return {.processed = true};
    }
    if (event.event()->isOfType(SoGesturePinchEvent::getClassTypeId())) {
        setupPanningPlane(viewer->getSoRenderManager()->getCamera());
        enterGesture(event.event());
        return {.processed = true};
    }

    return {};
}

NavigationEventOutcome MayaGestureNavigationStyle::handleAwaitingMove(const EventContext& context)
{
    if (context.event.isMouseButton()) {
        return handleAwaitingMoveButton(context);
    }
    if (context.event.isPointerMotion()) {
        return handleAwaitingMoveMotion(context);
    }
    if (context.event.isGesture()) {
        return handleAwaitingMoveGesture(context);
    }
    return {};
}

NavigationEventOutcome MayaGestureNavigationStyle::handleMotionButton(const EventContext& context)
{
    const auto* mouse = context.event.mouseButton();
    const NavigationInputState& input = context.event.input();
    if (!mouse) {
        return {};
    }

    if (mouse->getButton() != SoMouseButtonEvent::BUTTON1
        && mouse->getButton() != SoMouseButtonEvent::BUTTON2
        && mouse->getButton() != SoMouseButtonEvent::BUTTON3) {
        return {};
    }

    if (input.left || input.right) {
        if (input.left && input.right) {
            setRotationCenter(viewer->getFocalPoint());
            enterMotion(State::Rotate);
        }
        else if (input.left) {
            saveCursorPosition(context.event.event());
            enterMotion(State::Rotate);
        }
        else {
            saveCursorPosition(context.event.event());
            enterMotion(State::Pan);
        }
    }
    else {
        leaveState();
        setViewingMode(NavigationStyle::IDLE);
    }
    return {.processed = true};
}

NavigationEventOutcome MayaGestureNavigationStyle::handleMotionPointer(const EventContext& context)
{
    if (state == State::Zoom) {
        zoomByCursor(context.normalizedPosition, context.previousNormalizedPosition);
    }
    else if (state == State::Pan) {
        panCamera(
            viewer->getSoRenderManager()->getCamera(),
            context.viewportAspect,
            panningplane,
            context.normalizedPosition,
            context.previousNormalizedPosition
        );
    }
    else if (state == State::Rotate) {
        if (context.event.input().left && context.event.input().right) {
            NavigationStyle::doRotate(
                viewer->getSoRenderManager()->getCamera(),
                (context.normalizedPosition - context.previousNormalizedPosition)[0] * (-2),
                SbVec2f(0.5, 0.5)
            );
        }
        else {
            spin_simplified(context.normalizedPosition, context.previousNormalizedPosition);
        }
    }
    else {
        return {};
    }
    return {.processed = true};
}

NavigationEventOutcome MayaGestureNavigationStyle::handleMotion(const EventContext& context)
{
    if (context.event.isMouseButton()) {
        return handleMotionButton(context);
    }
    if (context.event.isPointerMotion()) {
        return handleMotionPointer(context);
    }
    if (context.event.isGesture()) {
        return handleGesture(context);
    }
    return {};
}

NavigationEventOutcome MayaGestureNavigationStyle::handleGesture(const EventContext& context)
{
    if (context.event.isMouseButton()) {
        clearDeferredEvents();
        const NavigationInputState& input = context.event.input();
        if (input.left) {
            enterMotion(State::Rotate);
        }
        else if (input.right) {
            enterMotion(State::Pan);
        }
        else {
            leaveState();
            setViewingMode(NavigationStyle::IDLE);
        }
        return {.processed = true};
    }

    if (!context.event.isGesture()) {
        return {};
    }

    const auto* gesture = context.event.gesture();
    if (gesture->state == SoGestureEvent::SbGSEnd || gesture->state == SoGestureEvent::SbGsCanceled) {
        leaveState();
        setViewingMode(NavigationStyle::SELECTION);
        return {.processed = true};
    }

    if (gesture->state != SoGestureEvent::SbGSUpdate) {
        return {.processed = true};
    }

    if (context.event.event()->isOfType(SoGesturePinchEvent::getClassTypeId())) {
        const auto* pinch = static_cast<const SoGesturePinchEvent*>(context.event.event());
        if (zoomAtCursor) {
            panCamera(
                viewer->getSoRenderManager()->getCamera(),
                context.viewportAspect,
                panningplane,
                normalizePixelPos(pinch->deltaCenter.getValue()),
                SbVec2f(0, 0)
            );
        }
        NavigationStyle::doZoom(
            viewer->getSoRenderManager()->getCamera(),
            static_cast<float>(-std::log(pinch->deltaZoom)),
            normalizePixelPos(pinch->curCenter)
        );
        if (pinch->deltaAngle != 0) {
            NavigationStyle::doRotate(
                viewer->getSoRenderManager()->getCamera(),
                pinch->deltaAngle,
                normalizePixelPos(pinch->curCenter)
            );
        }
    }
    else if (context.event.event()->isOfType(SoGesturePanEvent::getClassTypeId())) {
        const auto* pan = static_cast<const SoGesturePanEvent*>(context.event.event());
        panCamera(
            viewer->getSoRenderManager()->getCamera(),
            context.viewportAspect,
            panningplane,
            normalizePixelPos(pan->deltaOffset),
            SbVec2f(0, 0)
        );
    }
    return {.processed = true};
}

NavigationEventOutcome MayaGestureNavigationStyle::dispatchEvent(const EventContext& context)
{
    switch (state) {
        case State::Idle:
            return handleIdle(context);
        case State::AwaitingMove:
            return handleAwaitingMove(context);
        case State::Rotate:
        case State::Pan:
        case State::Zoom:
            return handleMotion(context);
        case State::Gesture:
            return handleGesture(context);
    }
    return {};
}

SbBool MayaGestureNavigationStyle::processSoEvent(const SoEvent* const event)
{
    if (isSeekMode()) {
        return inherited::processSoEvent(event);
    }

    if (!isAnimating() && isViewing()) {
        setViewing(false);
    }

    const bool isButton = event->isOfType(SoMouseButtonEvent::getClassTypeId());
    const bool isPointerMotion = event->isOfType(SoLocation2Event::getClassTypeId());
    const SbVec2f previousNormalizedPosition = lastmouseposition;
    const SbVec2s position = event->getPosition();
    const SbVec2f normalizedPosition = normalizePixelPos(position);
    if (isButton || isPointerMotion) {
        lastmouseposition = normalizedPosition;
    }

    const NavigationInputState before = currentInputState();
    updateInputState(event);
    const NavigationInputState after = currentInputState();
    updateClickState(before, after);
    const NavigationEventView navigationEvent(event, after);
    EventContext context {
        navigationEvent,
        normalizedPosition,
        previousNormalizedPosition,
        viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio(),
        viewer->isEditing(),
    };

    if (!context.editing && handleEventInForeground(event)) {
        return true;
    }

    if (navigationEvent.isKeyboard() && navigationEvent.keyboard()->getKey() == SoKeyboardEvent::H
        && !context.editing) {
        setupPanningPlane(viewer->getCamera());
        if (navigationEvent.keyboard()->getState() == SoButtonEvent::UP) {
            lookAtPoint(position);
        }
        return true;
    }

    if (event->isOfType(SoMotion3Event::getClassTypeId())) {
        processMotionEvent(static_cast<const SoMotion3Event*>(event));
        return true;
    }

    if (currentmode == NavigationStyle::SEEK_WAIT_MODE
        && navigationEvent.isPress(SoMouseButtonEvent::BUTTON1)) {
        seekToPoint(position);
        setViewingMode(NavigationStyle::SEEK_MODE);
        return true;
    }

    if (currentmode == NavigationStyle::SPINNING || currentmode == NavigationStyle::SEEK_MODE) {
        if (isButton || navigationEvent.isGesture() || navigationEvent.isKeyboard()
            || event->isOfType(SoMotion3Event::getClassTypeId())) {
            leaveState();
            setViewingMode(NavigationStyle::SELECTION);
            return inherited::processSoEvent(event);
        }
    }

    NavigationEventOutcome outcome = dispatchEvent(context);
    if (!outcome.processed && !outcome.propagated) {
        outcome.processed = inherited::processSoEvent(event);
    }
    return outcome.processed;
}
