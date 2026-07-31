// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>

#include "NavigationInputState.h"
#include "SoTouchEvents.h"

namespace Gui
{

/** Read-only view of a Coin event and the normalized navigation input state. */
class NavigationEventView
{
public:
    NavigationEventView(const SoEvent* event, const NavigationInputState input)
        : rawEvent(event)
        , inputState(input)
    {}

    const SoEvent* event() const
    {
        return rawEvent;
    }

    const NavigationInputState& input() const
    {
        return inputState;
    }

    unsigned int chord() const
    {
        return inputState.chord();
    }

    unsigned int buttons() const
    {
        return chord() & NavigationInputState::ButtonMask;
    }

    bool isMouseButton() const
    {
        return rawEvent->isOfType(SoMouseButtonEvent::getClassTypeId());
    }

    bool isKeyboard() const
    {
        return rawEvent->isOfType(SoKeyboardEvent::getClassTypeId());
    }

    bool isPointerMotion() const
    {
        return rawEvent->isOfType(SoLocation2Event::getClassTypeId());
    }

    bool isGesture() const
    {
        return rawEvent->isOfType(SoGestureEvent::getClassTypeId());
    }

    bool isGestureActive() const
    {
        if (!isGesture()) {
            return false;
        }

        const auto state = gesture()->state;
        return state == SoGestureEvent::SbGSStart || state == SoGestureEvent::SbGSUpdate;
    }

    bool isPress(const SoMouseButtonEvent::Button button) const
    {
        const auto* event = mouseButton();
        return event && event->getButton() == button && event->getState() == SoButtonEvent::DOWN;
    }

    bool isRelease(const SoMouseButtonEvent::Button button) const
    {
        const auto* event = mouseButton();
        return event && event->getButton() == button && event->getState() == SoButtonEvent::UP;
    }

    bool isKeyPress(const SoKeyboardEvent::Key key) const
    {
        const auto* event = keyboard();
        return event && event->getKey() == key && event->getState() == SoButtonEvent::DOWN;
    }

    bool isKeyRelease(const SoKeyboardEvent::Key key) const
    {
        const auto* event = keyboard();
        return event && event->getKey() == key && event->getState() == SoButtonEvent::UP;
    }

    const SoMouseButtonEvent* mouseButton() const
    {
        return isMouseButton() ? static_cast<const SoMouseButtonEvent*>(rawEvent) : nullptr;
    }

    const SoKeyboardEvent* keyboard() const
    {
        return isKeyboard() ? static_cast<const SoKeyboardEvent*>(rawEvent) : nullptr;
    }

    const SoLocation2Event* pointerMotion() const
    {
        return isPointerMotion() ? static_cast<const SoLocation2Event*>(rawEvent) : nullptr;
    }

    const SoGestureEvent* gesture() const
    {
        return isGesture() ? static_cast<const SoGestureEvent*>(rawEvent) : nullptr;
    }

private:
    const SoEvent* rawEvent;
    NavigationInputState inputState;
};

}  // namespace Gui
