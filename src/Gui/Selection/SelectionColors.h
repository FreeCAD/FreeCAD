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

#include <algorithm>
#include <cmath>
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

inline SbColor backgroundFallbackColor()
{
    // Keep this aligned with the generic View preferences page defaults.
    return SbColor(234.0f / 255.0f, 229.0f / 255.0f, 220.0f / 255.0f);
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

inline SbColor defaultBackgroundColor()
{
    return viewPreferenceColor("BackgroundColor", backgroundFallbackColor());
}

inline float linearizeSrgb(float value)
{
    value = std::clamp(value, 0.0f, 1.0f);
    if (value <= 0.04045f) {
        return value / 12.92f;
    }
    return std::pow((value + 0.055f) / 1.055f, 2.4f);
}

inline float relativeLuminance(const SbColor& color)
{
    return 0.2126f * linearizeSrgb(color[0]) + 0.7152f * linearizeSrgb(color[1])
        + 0.0722f * linearizeSrgb(color[2]);
}

inline float contrastRatio(const SbColor& first, const SbColor& second)
{
    const float firstLuminance = relativeLuminance(first);
    const float secondLuminance = relativeLuminance(second);
    const float lighter = std::max(firstLuminance, secondLuminance);
    const float darker = std::min(firstLuminance, secondLuminance);
    return (lighter + 0.05f) / (darker + 0.05f);
}

inline SbColor contrastOutlineColor(const SbColor& first, const SbColor& second)
{
    const SbColor black(0.0f, 0.0f, 0.0f);
    const SbColor white(1.0f, 1.0f, 1.0f);

    const float blackScore = std::min(contrastRatio(black, first), contrastRatio(black, second));
    const float whiteScore = std::min(contrastRatio(white, first), contrastRatio(white, second));
    return blackScore >= whiteScore ? black : white;
}
}  // namespace Gui::SelectionColors
