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
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

namespace Gui
{

/** A compact representation of the buttons and modifiers held for a navigation event. */
struct NavigationInputState
{
    enum InputFlag : unsigned int
    {
        // Keep these values stable because they are used in persisted navigation chords.
        LeftDown = 0x00000100,
        RightDown = 0x00000001,
        MiddleDown = 0x00000010,
        CtrlDown = 0x00100000,
        ShiftDown = 0x01000000,
        AltDown = 0x00010000,
        ButtonMask = LeftDown | MiddleDown | RightDown,
        ModifierMask = CtrlDown | ShiftDown | AltDown,
    };

    bool left = false;
    bool middle = false;
    bool right = false;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    [[nodiscard]] constexpr unsigned int chord() const noexcept
    {
        return (left ? LeftDown : 0U) | (middle ? MiddleDown : 0U) | (right ? RightDown : 0U)
            | (ctrl ? CtrlDown : 0U) | (shift ? ShiftDown : 0U) | (alt ? AltDown : 0U);
    }
};

}  // namespace Gui
