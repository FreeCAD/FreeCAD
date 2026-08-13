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

#include <optional>
#include <span>

#include <Gui/Navigation/NavigationInputState.h>
#include <Gui/Navigation/NavigationStyle.h>

namespace Gui
{

enum class OwnerMatch
{
    AllHeld,
    AnyHeld,
};

enum class EditingSelectionPolicy
{
    Preserve,
    CancelOnLeftRightChord,
};

struct GuiExport GestureOwnership
{
    unsigned int buttons = 0U;
    OwnerMatch ownerMatch = OwnerMatch::AllHeld;
};

constexpr bool gestureContinues(const GestureOwnership& ownership, const unsigned int input)
{
    return ownership.ownerMatch == OwnerMatch::AllHeld
        ? (input & ownership.buttons) == ownership.buttons
        : (input & ownership.buttons) != 0U;
}

struct GuiExport NavigationRule
{
    // A missing source mode makes this a global chord rule.
    std::optional<NavigationStyle::ViewerMode> fromMode;
    unsigned int chord;
    NavigationStyle::ViewerMode toMode;
    GestureOwnership ownership {};
};

constexpr GestureOwnership ownedBy(const unsigned int buttons)
{
    return {.buttons = buttons};
}

constexpr GestureOwnership ownedByAny(const unsigned int buttons)
{
    return {.buttons = buttons, .ownerMatch = OwnerMatch::AnyHeld};
}

constexpr NavigationRule bind(
    const unsigned int chord,
    const NavigationStyle::ViewerMode mode,
    const GestureOwnership ownership = {}
)
{
    return {std::nullopt, chord, mode, ownership};
}

constexpr NavigationRule transition(
    const NavigationStyle::ViewerMode fromMode,
    const unsigned int chord,
    const NavigationStyle::ViewerMode toMode,
    const GestureOwnership ownership = {}
)
{
    return {fromMode, chord, toMode, ownership};
}

struct GuiExport NavigationProfile
{
    std::span<const NavigationRule> rules;
    const char* selectionDescription = nullptr;
    const char* panDescription = nullptr;
    const char* rotateDescription = nullptr;
    const char* zoomDescription = nullptr;
    bool forceRotationOnAddedButton = true;
    bool lockPrimaryAfterMultiButton = true;
    bool preserveModeOnUnmappedInput = false;
    bool recenterOnMiddleClick = true;
    EditingSelectionPolicy editingSelectionPolicy = EditingSelectionPolicy::Preserve;
};

}  // namespace Gui
