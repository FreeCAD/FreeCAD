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

#include <Gui/Navigation/NavigationStateChart.h>

// NOLINTBEGIN(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
namespace Gui
{

class GuiExport SiemensNXNavigationStyle: public NavigationStateChart
{
    using inherited = NavigationStateChart;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    SiemensNXNavigationStyle();
    ~SiemensNXNavigationStyle() override;
    const char* mouseButtons(ViewerMode mode) override;
    std::string userFriendlyName() const override;

protected:
    SbBool processKeyboardEvent(const SoKeyboardEvent* const event) override;

private:
    struct NaviMachine;
    struct IdleState;
    struct AwaitingReleaseState;
    struct AwaitingMoveState;
    struct InteractState;
    struct RotateState;
    struct PanState;
    struct ZoomState;
    struct SelectionState;
};

}  // namespace Gui
// NOLINTEND(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
