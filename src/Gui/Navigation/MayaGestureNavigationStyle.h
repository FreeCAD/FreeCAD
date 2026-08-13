// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
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

#pragma once

#include <Gui/Navigation/NavigationEventView.h>
#include <Gui/Navigation/NavigationStyle.h>

#include <variant>
#include <vector>

namespace Gui
{

class GuiExport MayaGestureNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MayaGestureNavigationStyle();
    ~MayaGestureNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const event) override;
    int selectionMoveThreshold() const override;
    void zoomByCursor(const SbVec2f& position, const SbVec2f& previousPosition) override;

private:
    enum class State
    {
        Idle,
        AwaitingMove,
        Rotate,
        Pan,
        Zoom,
        Gesture,
    };

    struct AwaitingMoveData
    {
        SbVec2s pressPosition;
        bool moveThresholdBroken = false;
    };

    using StateData = std::variant<std::monostate, AwaitingMoveData>;

    struct EventContext
    {
        const NavigationEventView& event;
        SbVec2f normalizedPosition;
        SbVec2f previousNormalizedPosition;
        float viewportAspect;
        bool editing;
    };

    NavigationEventOutcome dispatchEvent(const EventContext& context);
    NavigationEventOutcome handleIdle(const EventContext& context);
    NavigationEventOutcome handleAwaitingMove(const EventContext& context);
    NavigationEventOutcome handleMotion(const EventContext& context);
    NavigationEventOutcome handleGesture(const EventContext& context);

    void enterAwaitingMove(const EventContext& context);
    void enterMotion(State state);
    void enterGesture(const SoEvent* event);
    void leaveState();

    NavigationEventOutcome handleAwaitingMoveButton(const EventContext& context);
    NavigationEventOutcome handleAwaitingMoveMotion(const EventContext& context);
    NavigationEventOutcome handleAwaitingMoveGesture(const EventContext& context);
    NavigationEventOutcome handleMotionButton(const EventContext& context);
    NavigationEventOutcome handleMotionPointer(const EventContext& context);

    void replayDeferredEvents();
    void clearDeferredEvents();
    bool hasDeferredEvents() const;
    void updateClickState(const NavigationInputState& before, const NavigationInputState& after);

    int mouseMoveThreshold = 0;
    State state = State::Idle;
    StateData stateData;
    std::vector<SoMouseButtonEvent> deferredEvents;
    bool deferredClickIsComplex = false;
};

}  // namespace Gui
