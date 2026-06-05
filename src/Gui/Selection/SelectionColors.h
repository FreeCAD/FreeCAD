// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
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

#include <cstdint>

#include <Inventor/SbColor.h>

#include "Window.h"

namespace Gui::SelectionColors
{
inline SbColor highlightFallbackColor()
{
    // Keep this aligned with the generic Selection preferences page defaults.
    return SbColor(225.0f / 255.0f, 225.0f / 255.0f, 20.0f / 255.0f);
}

inline SbColor selectionFallbackColor()
{
    // Keep this aligned with the generic Selection preferences page defaults.
    return SbColor(28.0f / 255.0f, 173.0f / 255.0f, 28.0f / 255.0f);
}

inline SbColor viewPreferenceColor(const char* key, const SbColor& fallback)
{
    float transparency = 0.0f;
    SbColor color = fallback;
    auto hGrp = WindowParameter::getDefaultParameter()->GetGroup("View");
    auto packed = static_cast<unsigned long>(color.getPackedValue());
    packed = hGrp->GetUnsigned(key, packed);
    color.setPackedValue(static_cast<std::uint32_t>(packed), transparency);
    return color;
}

inline SbColor defaultHighlightColor()
{
    return viewPreferenceColor("HighlightColor", highlightFallbackColor());
}

inline SbColor defaultSelectionColor()
{
    return viewPreferenceColor("SelectionColor", selectionFallbackColor());
}
}  // namespace Gui::SelectionColors
