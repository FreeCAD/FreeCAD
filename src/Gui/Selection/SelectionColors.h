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
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoOverrideElement.h>

#include "Window.h"

class SoColorPacker;
class SoNode;
class SoState;

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

enum class VisualRole
{
    Selection,
    Preselection,
};

inline SbColor scaledColor(const SbColor& color, float scale)
{
    return SbColor(color[0] * scale, color[1] * scale, color[2] * scale);
}

inline SbColor emissiveColor(VisualRole role, const SbColor& color)
{
    // Emissive is only a small boost so selection colors stay visible without
    // flattening shaded faces. Selection and preselection intentionally share
    // the scale for now; keep the role explicit so future tuning stays local.
    constexpr float selectionEmissiveScale = 0.35f;
    constexpr float preselectionEmissiveScale = 0.35f;
    const float scale = role == VisualRole::Preselection ? preselectionEmissiveScale
                                                         : selectionEmissiveScale;
    return scaledColor(color, scale);
}

inline void clearColorOverrides(SoState* state, SoNode* node)
{
    SoOverrideElement::setAmbientColorOverride(state, node, false);
    SoOverrideElement::setDiffuseColorOverride(state, node, false);
    SoOverrideElement::setEmissiveColorOverride(state, node, false);
}

inline void applyMaterial(
    SoState* state,
    SoNode* node,
    VisualRole role,
    const SbColor& color,
    SoColorPacker* packer
)
{
    const SbColor emissive = emissiveColor(role, color);

    clearColorOverrides(state, node);

    SoLazyElement::setAmbient(state, &color);
    SoOverrideElement::setAmbientColorOverride(state, node, true);

    SoLazyElement::setDiffuse(state, node, 1, &color, packer);
    SoOverrideElement::setDiffuseColorOverride(state, node, true);

    SoLazyElement::setEmissive(state, &emissive);
    SoOverrideElement::setEmissiveColorOverride(state, node, true);
}

inline void applyPackedMaterial(
    SoState* state,
    SoNode* node,
    VisualRole role,
    const SbColor& color,
    std::uint32_t packedColor,
    bool hasTransparency
)
{
    const SbColor emissive = emissiveColor(role, color);

    clearColorOverrides(state, node);

    SoLazyElement::setAmbient(state, &color);
    SoOverrideElement::setAmbientColorOverride(state, node, true);

    SoLazyElement::setPacked(state, node, 1, &packedColor, hasTransparency);
    SoOverrideElement::setDiffuseColorOverride(state, node, true);

    SoLazyElement::setEmissive(state, &emissive);
    SoOverrideElement::setEmissiveColorOverride(state, node, true);
}
}  // namespace Gui::SelectionColors
