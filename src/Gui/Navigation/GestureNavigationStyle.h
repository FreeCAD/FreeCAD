// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2019 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

#pragma once

#include "Navigation/NavigationStyle.h"
#include "Navigation/NavigationEventView.h"

#include <Inventor/events/SoMouseButtonEvent.h>

#include <queue>
#include <variant>

namespace Gui
{


class GestureNavigationStyle: public UserNavigationStyle
{
    using superclass = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    GestureNavigationStyle();
    ~GestureNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;
    ClarifySelectionMode clarifySelectionMode() const override
    {
        return ClarifySelectionMode::Ctrl;
    }

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;
    int selectionMoveThreshold() const override;

public:
    /// calls processSoEvent of NavigationStyle.
    SbBool processSoEvent_bypass(const SoEvent* const ev);

protected:
    enum class State
    {
        Idle,
        AwaitingRelease,
        AwaitingMove,
        Rotate,
        Pan,
        StickyPan,
        Tilt,
        Gesture,
        Interact,
    };

    struct AwaitingMoveData
    {
        SbVec2s pressPosition;
        SbTime pressedAt;
        int holdTimeout = 0;
    };

    struct MotionData
    {
        SbVec2s previousPosition;
        float viewportAspect = 1.0F;
        bool enableTilt = false;
    };

    using StateData = std::variant<std::monostate, AwaitingMoveData, MotionData>;

    NavigationEventOutcome dispatchEvent(const SoEvent* event);
    NavigationEventOutcome handleIdle(const NavigationEventView& event);
    NavigationEventOutcome handleAwaitingRelease(const NavigationEventView& event);
    NavigationEventOutcome handleAwaitingMove(const NavigationEventView& event);
    NavigationEventOutcome handleRotate(const NavigationEventView& event);
    NavigationEventOutcome handlePan(const NavigationEventView& event);
    NavigationEventOutcome handleStickyPan(const NavigationEventView& event);
    NavigationEventOutcome handleTilt(const NavigationEventView& event);
    NavigationEventOutcome handleGesture(const NavigationEventView& event);
    NavigationEventOutcome handleInteract(const NavigationEventView& event);

    static const char* stateName(State state);
    void transitionTo(State state, const SoEvent* event);
    void leaveState();
    void enterPan(const SoEvent* event);
    bool updatePan(const NavigationEventView& event);

    class EventQueue: public std::queue<SoMouseButtonEvent>
    {
    public:
        EventQueue(GestureNavigationStyle& ns)
            : ns(ns)
        {}

        void post(const SoMouseButtonEvent& event);
        void discardAll();
        void forwardAll();

    public:
        GestureNavigationStyle& ns;
    };


protected:  // members variables
    State state = State::Idle;
    StateData stateData;
    EventQueue postponedEvents;

    // settings:
    /// distance in px to treat as a definite drag (noise gate)
    int mouseMoveThreshold = 5;
    /// Used by roll gesture detection logic while awaiting a gesture decision or release.
    int rollDir = 0;
    bool logging = false;

public:
    bool is2DViewing() const;

public:  // gesture reactions
    /// Roll gesture is like: press LMB, press RMB, release LMB, release RMB.
    ///  This function is called by the explicit gesture state machine when it detects a roll gesture.
    void onRollGesture(int direction);
};

}  // namespace Gui
