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

#include "NavigationResolver.h"

namespace Gui
{

namespace
{

const NavigationRule* findNavigationRule(
    const NavigationProfile& profile,
    const NavigationStyle::ViewerMode currentMode,
    const unsigned int input
)
{
    for (const NavigationRule& rule : profile.rules) {
        if (rule.fromMode && *rule.fromMode == currentMode && rule.chord == input) {
            return &rule;
        }
    }

    for (const NavigationRule& rule : profile.rules) {
        if (!rule.fromMode && rule.chord == input) {
            return &rule;
        }
    }

    return nullptr;
}

}  // namespace

ResolutionResult resolveNavigation(const NavigationProfile& profile, const ResolutionInput& input)
{
    ResolutionResult result {
        .mode = input.requestedMode.value_or(input.currentMode),
        .activeGesture = input.activeGesture,
    };

    if (input.chord != 0U
        && (input.currentMode == NavigationStyle::INTERACT
            || input.requestedMode == NavigationStyle::SEEK_MODE)) {
        return result;
    }

    if (input.chord == 0U) {
        result.mode = input.currentMode == NavigationStyle::SPINNING ? NavigationStyle::SPINNING
                                                                     : NavigationStyle::IDLE;
        result.activeGesture.reset();
        return result;
    }

    const NavigationRule* rule = findNavigationRule(profile, input.currentMode, input.chord);

    if (rule != nullptr && rule->fromMode) {
        result.mode = rule->toMode;
    }
    else if (input.requestedMode) {
        result.mode = *input.requestedMode;
    }
    else if (rule != nullptr) {
        result.mode = rule->toMode;
    }
    else {
        if (input.activeGesture && gestureContinues(*input.activeGesture, input.chord)) {
            result.mode = input.currentMode;
        }
        else if (profile.preserveModeOnUnmappedInput) {
            result.mode = input.currentMode;
        }
        else if (
            input.currentMode == NavigationStyle::PANNING
            || input.currentMode == NavigationStyle::DRAGGING
            || input.currentMode == NavigationStyle::ZOOMING
        ) {
            result.mode = NavigationStyle::IDLE;
        }
        else {
            result.mode = input.currentMode;
        }
    }

    if (rule != nullptr && rule->ownership.buttons != 0U) {
        result.activeGesture = rule->ownership;
    }
    else if (rule != nullptr) {
        result.activeGesture.reset();
    }
    else if (!result.activeGesture || !gestureContinues(*result.activeGesture, input.chord)) {
        result.activeGesture.reset();
    }

    return result;
}

}  // namespace Gui
