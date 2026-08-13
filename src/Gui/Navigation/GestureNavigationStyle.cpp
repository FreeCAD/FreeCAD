// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2019 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software: you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Library General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

/*
 * This style deliberately keeps its temporal interaction protocol local.
 * LMB serves dual purpose: it selects objects as well as spinning the view.
 * The initial button event is postponed until movement or release identifies
 * the user's intent.
 *
 * Touchscreen input also produces synthetic mouse events around gesture
 * events. The explicit state machine below preserves the existing safeguards
 * for postponed input, tap-and-hold, roll gestures, and missing releases.
 */

#include <Inventor/SoFullPath.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/draggers/SoDragger.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMotion3Event.h>

#include <QApplication>
#include <QTapAndHoldGesture>

#include <FCConfig.h>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "Application.h"
#include "GestureNavigationStyle.h"
#include "NavigationInputState.h"
#include "SoTouchEvents.h"
#include "View3DInventorViewer.h"

#include <cmath>
#include <sstream>
#include <string>

namespace Gui
{

using InputFlag = NavigationInputState::InputFlag;
constexpr unsigned int LeftDown = InputFlag::LeftDown;
constexpr unsigned int RightDown = InputFlag::RightDown;
constexpr unsigned int MiddleDown = InputFlag::MiddleDown;
constexpr unsigned int CtrlDown = InputFlag::CtrlDown;
constexpr unsigned int ShiftDown = InputFlag::ShiftDown;

void logNavigationEvent(const NavigationEventView& event)
{
    if (event.isPress(SoMouseButtonEvent::BUTTON1)) {
        Base::Console().log("button1 press ");
    }
    if (event.isPress(SoMouseButtonEvent::BUTTON2)) {
        Base::Console().log("button2 press ");
    }
    if (event.isPress(SoMouseButtonEvent::BUTTON3)) {
        Base::Console().log("button3 press ");
    }
    if (event.isRelease(SoMouseButtonEvent::BUTTON1)) {
        Base::Console().log("button1 release ");
    }
    if (event.isRelease(SoMouseButtonEvent::BUTTON2)) {
        Base::Console().log("button2 release ");
    }
    if (event.isRelease(SoMouseButtonEvent::BUTTON3)) {
        Base::Console().log("button3 release ");
    }
    if (event.isMouseButton()) {
        Base::Console().log("%x", event.chord());
    }
    if (event.isGesture()) {
        Base::Console().log("Gesture ");
        switch (event.gesture()->state) {
            case SoGestureEvent::SbGSStart:
                Base::Console().log("start ");
                break;
            case SoGestureEvent::SbGSEnd:
                Base::Console().log("end ");
                break;
            case SoGestureEvent::SbGSUpdate:
                Base::Console().log("data ");
                break;
            default:
                Base::Console().log("??? ");
        }
        Base::Console().log(event.event()->getTypeId().getName().getString());
    }
    if (event.isMouseButton() || event.isGesture()) {
        Base::Console()
            .log("(%i,%i)\n", event.event()->getPosition()[0], event.event()->getPosition()[1]);
    }
}

const char* GestureNavigationStyle::stateName(const State state)
{
    switch (state) {
        case State::Idle:
            return "Idle";
        case State::AwaitingRelease:
            return "AwaitingRelease";
        case State::AwaitingMove:
            return "AwaitingMove";
        case State::Rotate:
            return "Rotate";
        case State::Pan:
            return "Pan";
        case State::StickyPan:
            return "StickyPan";
        case State::Tilt:
            return "Tilt";
        case State::Gesture:
            return "Gesture";
        case State::Interact:
            return "Interact";
    }
    return "Unknown";
}

void GestureNavigationStyle::transitionTo(const State nextState, const SoEvent* event)
{
    leaveState();
    state = nextState;
    stateData.emplace<std::monostate>();

    if (logging) {
        Base::Console().log(" -> %s\n", stateName(nextState));
    }

    switch (nextState) {
        case State::Idle:
            setViewingMode(NavigationStyle::IDLE);
            break;

        case State::AwaitingRelease:
            break;

        case State::AwaitingMove: {
            auto& data = stateData.emplace<AwaitingMoveData>();
            data.pressPosition = event->getPosition();
            data.pressedAt = event->getTime();
            setViewingMode(NavigationStyle::IDLE);

            auto viewPreferences = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/View"
            );
            mouseMoveThreshold = viewPreferences->GetInt("GestureMoveThreshold", mouseMoveThreshold);
            data.holdTimeout = int(double(QTapAndHoldGesture::timeout()) * 0.9);
            data.holdTimeout = viewPreferences->GetInt("GestureTapHoldTimeout", data.holdTimeout);
            if (data.holdTimeout == 0) {
                data.holdTimeout = 650;
            }
            QTapAndHoldGesture::setTimeout(int(double(data.holdTimeout) / 0.9));
            break;
        }

        case State::Rotate: {
            auto& data = stateData.emplace<MotionData>();
            saveCursorPosition(event);
            setViewingMode(NavigationStyle::DRAGGING);
            data.previousPosition = event->getPosition();
            break;
        }

        case State::Pan:
        case State::StickyPan:
            enterPan(event);
            break;

        case State::Tilt: {
            auto& data = stateData.emplace<MotionData>();
            setRotationCenter(viewer->getFocalPoint());
            setViewingMode(NavigationStyle::DRAGGING);
            data.previousPosition = event->getPosition();
            setupPanningPlane(viewer->getSoRenderManager()->getCamera());
            break;
        }

        case State::Gesture: {
            auto& data = stateData.emplace<MotionData>();
            setViewingMode(NavigationStyle::PANNING);
            data.previousPosition = event->getPosition();
            data.viewportAspect
                = viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
            data.enableTilt = !(App::GetApplication()
                                    .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                                    ->GetBool("DisableTouchTilt", true));
            setupPanningPlane(viewer->getSoRenderManager()->getCamera());
            break;
        }

        case State::Interact:
            setViewingMode(NavigationStyle::INTERACT);
            break;
    }
}

void GestureNavigationStyle::enterPan(const SoEvent* const event)
{
    auto& data = stateData.emplace<MotionData>();
    setViewingMode(NavigationStyle::PANNING);
    data.previousPosition = event->getPosition();
    data.viewportAspect = viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
    setupPanningPlane(viewer->getSoRenderManager()->getCamera());
}

bool GestureNavigationStyle::updatePan(const NavigationEventView& event)
{
    if (!event.isPointerMotion()) {
        return false;
    }

    auto& data = std::get<MotionData>(stateData);
    const SbVec2s position = event.event()->getPosition();
    panCamera(
        viewer->getSoRenderManager()->getCamera(),
        data.viewportAspect,
        panningplane,
        normalizePixelPos(position),
        normalizePixelPos(data.previousPosition)
    );
    data.previousPosition = position;
    return true;
}

void GestureNavigationStyle::leaveState()
{
    switch (state) {
        case State::AwaitingMove:
            postponedEvents.discardAll();
            break;
        case State::StickyPan:
            // Qt may omit the release after a tap-and-hold drag.
            button2down = false;
            break;
        case State::Gesture:
            // Qt may omit releases while a touchscreen gesture is active.
            button1down = false;
            button2down = false;
            break;
        default:
            break;
    }
}

NavigationEventOutcome GestureNavigationStyle::handleIdle(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;
    const SbVec2f position = normalizePixelPos(event.event()->getPosition());

    switch (getViewingMode()) {
        case NavigationStyle::SEEK_WAIT_MODE:
            if (event.isPress(SoMouseButtonEvent::BUTTON1)) {
                seekToPoint(event.event()->getPosition());
                setViewingMode(NavigationStyle::SEEK_MODE);
                outcome.processed = true;
                transitionTo(State::AwaitingRelease, event.event());
                return outcome;
            }
            [[fallthrough]];
        case NavigationStyle::SPINNING:
        case NavigationStyle::SEEK_MODE:
            if (event.isMouseButton()) {
                outcome.processed = true;
                transitionTo(State::AwaitingRelease, event.event());
                return outcome;
            }
            if (event.isGesture() || event.isKeyboard()) {
                setViewingMode(NavigationStyle::IDLE);
            }
            break;
        case NavigationStyle::BOXZOOM:
            return outcome;
        default:
            break;
    }

    if (event.isPress(SoMouseButtonEvent::BUTTON1) && event.buttons() == LeftDown
        && isDraggerUnderCursor(event.event()->getPosition())) {
        transitionTo(State::Interact, event.event());
        return outcome;
    }

    if ((event.isPress(SoMouseButtonEvent::BUTTON1) && event.buttons() == LeftDown)
        || (event.isPress(SoMouseButtonEvent::BUTTON2) && event.buttons() == RightDown)) {
        postponedEvents.post(*event.mouseButton());
        outcome.processed = true;
        transitionTo(State::AwaitingMove, event.event());
        return outcome;
    }

    if (event.isPress(SoMouseButtonEvent::BUTTON3) && event.buttons() == MiddleDown) {
        outcome.processed = true;
        setupPanningPlane(viewer->getCamera());
        lookAtPoint(event.event()->getPosition());
        transitionTo(State::AwaitingRelease, event.event());
        return outcome;
    }

    if (event.isGestureActive()) {
        outcome.processed = true;
        transitionTo(State::Gesture, event.event());
        return outcome;
    }

    if (event.isKeyboard()) {
        outcome.processed = true;
        const auto* keyboardEvent = event.keyboard();
        const bool press = keyboardEvent->getState() == SoKeyboardEvent::DOWN;
        switch (keyboardEvent->getKey()) {
            case SoKeyboardEvent::H:
                if (!viewer->isEditing() && !press) {
                    setupPanningPlane(viewer->getCamera());
                    lookAtPoint(keyboardEvent->getPosition());
                }
                break;
            case SoKeyboardEvent::PAGE_UP:
                if (!press) {
                    doZoom(viewer->getSoRenderManager()->getCamera(), getDelta(), position);
                }
                break;
            case SoKeyboardEvent::PAGE_DOWN:
                if (!press) {
                    doZoom(viewer->getSoRenderManager()->getCamera(), -getDelta(), position);
                }
                break;
            default:
                outcome.processed = false;
                break;
        }
    }

    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::handleAwaitingMove(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;
    auto& data = std::get<AwaitingMoveData>(stateData);
    const bool longClick = (event.event()->getTime() - data.pressedAt).getValue() * 1000.0
        >= data.holdTimeout;

    outcome.processed = event.isMouseButton() || event.isPointerMotion();

    if (event.isRelease(SoMouseButtonEvent::BUTTON2) && event.buttons() == 0 && !viewer->isEditing()
        && isPopupMenuEnabled()) {
        openPopupMenu(event.event()->getPosition());
        transitionTo(State::Idle, event.event());
        return outcome;
    }

    if (event.buttons() == (LeftDown | RightDown)) {
        if (event.isPress(SoMouseButtonEvent::BUTTON1)) {
            rollDir = -1;
        }
        if (event.isPress(SoMouseButtonEvent::BUTTON2)) {
            rollDir = +1;
        }
    }
    if ((event.isRelease(SoMouseButtonEvent::BUTTON1) && event.buttons() == RightDown)
        || (event.isRelease(SoMouseButtonEvent::BUTTON2) && event.buttons() == LeftDown)) {
        onRollGesture(rollDir);
        transitionTo(State::AwaitingRelease, event.event());
        return outcome;
    }

    if (event.isMouseButton() && event.buttons() == 0) {
        if (longClick) {
            openPopupMenu(event.event()->getPosition());
            transitionTo(State::Idle, event.event());
            return outcome;
        }

        setViewingMode(NavigationStyle::SELECTION);
        postponedEvents.forwardAll();
        outcome.processed = processSoEvent_bypass(event.event());
        outcome.propagated = true;
        transitionTo(State::Idle, event.event());
        return outcome;
    }

    if (event.isPress(SoMouseButtonEvent::BUTTON3)) {
        postponedEvents.forwardAll();
        outcome.processed = processSoEvent_bypass(event.event());
        outcome.propagated = true;
        transitionTo(State::Idle, event.event());
        return outcome;
    }

    if (event.isMouseButton()) {
        postponedEvents.post(*event.mouseButton());
    }

    if (event.isPointerMotion()) {
        const auto movement = event.event()->getPosition() - data.pressPosition;
        if (SbVec2f(movement).length() > mouseMoveThreshold) {
            if (event.buttons() == LeftDown && (event.chord() & ShiftDown)) {
                if (
                    tryStartBoxSelection(data.pressPosition, event.pointerMotion(), event.chord() & CtrlDown)
                ) {
                    outcome.processed = true;
                    transitionTo(State::Idle, event.event());
                    return outcome;
                }
            }

            switch (event.buttons()) {
                case LeftDown: {
                    if (!longClick) {
                        const bool alt = event.chord() & NavigationInputState::AltDown;
                        const bool allowSpin = alt == is2DViewing();
                        if (allowSpin) {
                            transitionTo(State::Rotate, event.event());
                        }
                        else {
                            postponedEvents.forwardAll();
                            outcome.processed = processSoEvent_bypass(event.event());
                            outcome.propagated = true;
                            transitionTo(State::Idle, event.event());
                        }
                    }
                    else {
                        transitionTo(State::StickyPan, event.event());
                    }
                    return outcome;
                }
                case RightDown:
                    transitionTo(State::Pan, event.event());
                    return outcome;
                case LeftDown | RightDown:
                    transitionTo(State::Tilt, event.event());
                    return outcome;
                default:
                    postponedEvents.forwardAll();
                    outcome.processed = processSoEvent_bypass(event.event());
                    outcome.propagated = true;
                    transitionTo(State::Idle, event.event());
                    return outcome;
            }
        }
    }

    if (event.isGestureActive()) {
        outcome.processed = true;
        transitionTo(State::Gesture, event.event());
    }
    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::handleRotate(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;
    auto& data = std::get<MotionData>(stateData);

    if (event.isMouseButton()) {
        outcome.processed = true;
        if (event.buttons() == (LeftDown | RightDown)) {
            transitionTo(State::Tilt, event.event());
        }
        else if (event.buttons() == 0) {
            transitionTo(State::Idle, event.event());
        }
        return outcome;
    }
    if (event.isPointerMotion()) {
        outcome.processed = true;
        const SbVec2s position = event.event()->getPosition();
        spin_simplified(normalizePixelPos(position), normalizePixelPos(data.previousPosition));
        data.previousPosition = position;
    }
    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::handlePan(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;

    if (event.isMouseButton()) {
        outcome.processed = true;
        if (event.buttons() == (LeftDown | RightDown)) {
            transitionTo(State::Tilt, event.event());
        }
        else if (event.buttons() == 0) {
            transitionTo(State::Idle, event.event());
        }
        return outcome;
    }
    outcome.processed = updatePan(event);
    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::handleStickyPan(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;

    if (event.isMouseButton()) {
        outcome.processed = true;
        if (event.isRelease(SoMouseButtonEvent::BUTTON1)) {
            transitionTo(State::Idle, event.event());
        }
        return outcome;
    }
    outcome.processed = updatePan(event);
    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::handleTilt(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;
    auto& data = std::get<MotionData>(stateData);

    if (event.isMouseButton()) {
        outcome.processed = true;
        if (event.buttons() == RightDown) {
            transitionTo(State::Pan, event.event());
        }
        else if (event.buttons() == LeftDown) {
            transitionTo(State::Rotate, event.event());
        }
        else if (event.buttons() == 0) {
            transitionTo(State::Idle, event.event());
        }
        return outcome;
    }
    if (event.isPointerMotion()) {
        outcome.processed = true;
        const SbVec2s position = event.event()->getPosition();
        const float deltaX
            = (normalizePixelPos(position) - normalizePixelPos(data.previousPosition))[0];
        doRotate(viewer->getSoRenderManager()->getCamera(), deltaX * (-2), SbVec2f(0.5, 0.5));
        data.previousPosition = position;
    }
    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::handleGesture(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;

    if (event.isMouseButton()) {
        outcome.processed = true;
        if (event.buttons() == 0) {
            Base::Console().warning("leaving gesture state by mouse-click (fail-safe)\n");
            transitionTo(State::Idle, event.event());
        }
    }
    if (event.isPointerMotion()) {
        outcome.processed = true;
    }
    if (event.isGesture()) {
        outcome.processed = true;
        const auto* gesture = event.gesture();
        if (gesture->state == SoGestureEvent::SbGSEnd
            || gesture->state == SoGestureEvent::SbGsCanceled) {
            transitionTo(State::Idle, event.event());
        }
        else if (event.event()->isOfType(SoGesturePanEvent::getClassTypeId())) {
            const auto& panGesture = static_cast<const SoGesturePanEvent&>(*event.event());
            const auto& data = std::get<MotionData>(stateData);
            panCamera(
                viewer->getSoRenderManager()->getCamera(),
                data.viewportAspect,
                panningplane,
                normalizePixelPos(panGesture.deltaOffset),
                SbVec2f(0, 0)
            );
        }
        else if (event.event()->isOfType(SoGesturePinchEvent::getClassTypeId())) {
            const auto& pinch = static_cast<const SoGesturePinchEvent&>(*event.event());
            const auto& data = std::get<MotionData>(stateData);
            panCamera(
                viewer->getSoRenderManager()->getCamera(),
                data.viewportAspect,
                panningplane,
                normalizePixelPos(pinch.deltaCenter.getValue()),
                SbVec2f(0, 0)
            );
            doZoom(
                viewer->getSoRenderManager()->getCamera(),
                -logf(float(pinch.deltaZoom)),
                normalizePixelPos(pinch.curCenter)
            );
            if (pinch.deltaAngle != 0.0 && data.enableTilt) {
                doRotate(
                    viewer->getSoRenderManager()->getCamera(),
                    float(pinch.deltaAngle),
                    normalizePixelPos(pinch.curCenter)
                );
            }
        }
        else {
            outcome.processed = false;
        }
    }
    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::handleAwaitingRelease(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;

    if (event.isMouseButton()) {
        outcome.processed = true;
        if (event.buttons() == 0) {
            transitionTo(State::Idle, event.event());
            return outcome;
        }
    }

    if (event.buttons() == (LeftDown | RightDown)) {
        if (event.isPress(SoMouseButtonEvent::BUTTON1)) {
            rollDir = -1;
        }
        if (event.isPress(SoMouseButtonEvent::BUTTON2)) {
            rollDir = +1;
        }
    }
    if ((event.isRelease(SoMouseButtonEvent::BUTTON1) && event.buttons() == RightDown)
        || (event.isRelease(SoMouseButtonEvent::BUTTON2) && event.buttons() == LeftDown)) {
        onRollGesture(rollDir);
    }

    if (event.isPointerMotion()) {
        outcome.processed = true;
    }
    if (event.isGestureActive()) {
        outcome.processed = true;
        transitionTo(State::Gesture, event.event());
    }
    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::handleInteract(const NavigationEventView& event)
{
    NavigationEventOutcome outcome;
    if (event.isMouseButton()) {
        if (event.buttons() == 0) {
            transitionTo(State::Idle, event.event());
        }
    }
    return outcome;
}

NavigationEventOutcome GestureNavigationStyle::dispatchEvent(const SoEvent* event)
{
    const NavigationEventView navigationEvent(event, currentInputState());
    if (logging) {
        logNavigationEvent(navigationEvent);
    }

    switch (state) {
        case State::Idle:
            return handleIdle(navigationEvent);
        case State::AwaitingRelease:
            return handleAwaitingRelease(navigationEvent);
        case State::AwaitingMove:
            return handleAwaitingMove(navigationEvent);
        case State::Rotate:
            return handleRotate(navigationEvent);
        case State::Pan:
            return handlePan(navigationEvent);
        case State::StickyPan:
            return handleStickyPan(navigationEvent);
        case State::Tilt:
            return handleTilt(navigationEvent);
        case State::Gesture:
            return handleGesture(navigationEvent);
        case State::Interact:
            return handleInteract(navigationEvent);
    }
    return {};
}

/* TRANSLATOR Gui::GestureNavigationStyle */

TYPESYSTEM_SOURCE(Gui::GestureNavigationStyle, Gui::UserNavigationStyle)

GestureNavigationStyle::GestureNavigationStyle()
    : postponedEvents(*this)
{
    logging = App::GetApplication()
                  .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                  ->GetBool("NavigationDebug");
    mouseMoveThreshold = QApplication::startDragDistance();
    setViewingMode(NavigationStyle::IDLE);
}

GestureNavigationStyle::~GestureNavigationStyle() = default;

int GestureNavigationStyle::selectionMoveThreshold() const
{
    return mouseMoveThreshold;
}

const char* GestureNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
        case NavigationStyle::SELECTION:
            return QT_TR_NOOP("Tap OR click left mouse button.");
        case NavigationStyle::PANNING:
            return QT_TR_NOOP("Drag screen with two fingers OR press right mouse button.");
        case NavigationStyle::DRAGGING:
            return QT_TR_NOOP(
                "Drag screen with one finger OR press left mouse button. In Sketcher and other "
                "edit modes, hold Alt in addition."
            );
        case NavigationStyle::ZOOMING:
            return QT_TR_NOOP(
                "Pinch (place two fingers on the screen and drag them apart from or towards each "
                "other) OR scroll mouse wheel OR PgUp/PgDown on keyboard."
            );
        default:
            return "No description";
    }
}

SbBool GestureNavigationStyle::processSoEvent(const SoEvent* const ev)
{
    if (isSeekMode()) {
        return superclass::processSoEvent(ev);
    }
    if (!isSeekMode() && !isAnimating() && isViewing()) {
        setViewing(false);
    }

    if (ev->isOfType(SoMotion3Event::getClassTypeId())) {
        processMotionEvent(static_cast<const SoMotion3Event*>(ev));
        return true;
    }

    if (!viewer->isEditing() && handleEventInForeground(ev)) {
        return true;
    }

    const auto isRelease = [ev](const SoMouseButtonEvent::Button button) {
        return ev->isOfType(SoMouseButtonEvent::getClassTypeId())
            && static_cast<const SoMouseButtonEvent*>(ev)->getButton() == button
            && static_cast<const SoMouseButtonEvent*>(ev)->getState() == SoButtonEvent::UP;
    };
    if ((isRelease(SoMouseButtonEvent::BUTTON1) && !button1down)
        || (isRelease(SoMouseButtonEvent::BUTTON2) && !button2down)
        || (isRelease(SoMouseButtonEvent::BUTTON3) && !button3down)) {
        // Qt can emit synthetic releases without a corresponding press during gestures.
        return true;
    }

    updateInputState(ev);

#ifdef FC_OS_MACOSX
    if (ev->isOfType(SoGestureEvent::getClassTypeId())) {
        return superclass::processSoEvent(ev);
    }
#endif

    const NavigationEventOutcome outcome = dispatchEvent(ev);
    if (!outcome.propagated && !outcome.processed) {
        return superclass::processSoEvent(ev);
    }
    return outcome.processed;
}

SbBool GestureNavigationStyle::processSoEvent_bypass(const SoEvent* const ev)
{
    return superclass::processSoEvent(ev);
}

bool GestureNavigationStyle::is2DViewing() const
{
    return viewer->isEditing();
}

void GestureNavigationStyle::onRollGesture(const int direction)
{
    std::string command;
    if (direction == +1) {
        if (logging) {
            Base::Console().log("Roll forward gesture\n");
        }
        command = App::GetApplication()
                      .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                      ->GetASCII("GestureRollFwdCommand");
    }
    else if (direction == -1) {
        if (logging) {
            Base::Console().log("Roll backward gesture\n");
        }
        command = App::GetApplication()
                      .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                      ->GetASCII("GestureRollBackCommand");
    }
    if (command.empty()) {
        return;
    }

    std::stringstream code;
    code << "Gui.runCommand(\"" << command << "\")";
    try {
        Base::Interpreter().runString(code.str().c_str());
    }
    catch (Base::PyException& exception) {
        exception.reportException();
    }
    catch (...) {
        Base::Console().error(
            "GestureNavigationStyle::onRollGesture: unknown C++ exception when invoking command "
            "%s\n",
            command.c_str()
        );
    }
}

void GestureNavigationStyle::EventQueue::post(const SoMouseButtonEvent& event)
{
    push(event);
    if (ns.logging) {
        Base::Console().log("postponed mouse button event\n");
    }
}

void GestureNavigationStyle::EventQueue::discardAll()
{
    while (!empty()) {
        pop();
    }
}

void GestureNavigationStyle::EventQueue::forwardAll()
{
    while (!empty()) {
        auto event = front();
        ns.processSoEvent_bypass(&event);
        pop();
    }
}

}  // namespace Gui
