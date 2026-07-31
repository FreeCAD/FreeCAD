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

#pragma once

#include <Gui/Navigation/NavigationEventView.h>
#include <Gui/Navigation/NavigationStyle.h>

#include <variant>

// NOLINTBEGIN(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
namespace Gui
{

class GuiExport SiemensNXNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    SiemensNXNavigationStyle();
    ~SiemensNXNavigationStyle() override;
    const char* mouseButtons(ViewerMode mode) override;
    std::string userFriendlyName() const override;

protected:
    SbBool processSoEvent(const SoEvent* const event) override;
    SbBool processKeyboardEvent(const SoKeyboardEvent* const event) override;

private:
    enum class State
    {
        Idle,
        AwaitingRelease,
        AwaitingMove,
        Rotate,
        Pan,
        Zoom,
    };

    struct AwaitingMoveData
    {
        SbTime pressedAt;
    };

    struct PanZoomData
    {
        SbVec2s previousPosition;
        float viewportAspect = 1.0F;
    };

    using StateData = std::variant<std::monostate, AwaitingMoveData, PanZoomData>;

    bool dispatchEvent(const NavigationEventView& event);
    bool handleIdle(const NavigationEventView& event);
    bool handleAwaitingRelease(const NavigationEventView& event);
    bool handleAwaitingMove(const NavigationEventView& event);
    bool handleRotate(const NavigationEventView& event);
    bool handlePan(const NavigationEventView& event);
    bool handleZoom(const NavigationEventView& event);

    void transitionTo(State next, const SoEvent* event);
    void enterAwaitingMove(const SoEvent* event);
    void enterRotate(const SoEvent* event);
    void enterPan(const SoEvent* event);
    void enterZoom(const SoEvent* event);

    State state = State::Idle;
    StateData stateData;
};

}  // namespace Gui
// NOLINTEND(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
