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

#include "NavigationProfile.h"

namespace Gui
{

struct ResolutionInput
{
    NavigationStyle::ViewerMode currentMode;
    unsigned int chord;
    std::optional<NavigationStyle::ViewerMode> requestedMode;
    std::optional<GestureOwnership> activeGesture;
};

struct ResolutionResult
{
    NavigationStyle::ViewerMode mode;
    std::optional<GestureOwnership> activeGesture;
};

GuiExport ResolutionResult
resolveNavigation(const NavigationProfile& profile, const ResolutionInput& input);

}  // namespace Gui
