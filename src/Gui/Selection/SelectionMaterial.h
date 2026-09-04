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
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
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

class SoColorPacker;
class SoNode;
class SoState;

namespace Gui::SelectionMaterial
{

enum class VisualRole
{
    Selection,
    Preselection,
};

enum class MaterialMode
{
    // Preserve the historical SoFCSelection::EMISSIVE contract.
    EmissiveOnly,

    // Preserve surface shading by applying the role color to ambient and diffuse,
    // with emissive used only as a controlled brightness contribution.
    Lit,
};

inline SbColor scaledColor(const SbColor& color, float scale)
{
    return SbColor(color[0] * scale, color[1] * scale, color[2] * scale);
}

inline float emissiveScale(VisualRole role)
{
    constexpr float selectionEmissiveScale = 0.20f;
    constexpr float preselectionEmissiveScale = 0.35f;
    return role == VisualRole::Preselection ? preselectionEmissiveScale : selectionEmissiveScale;
}

inline SbColor emissiveColor(VisualRole role, const SbColor& color)
{
    return scaledColor(color, emissiveScale(role));
}

inline void clearColorOverrides(SoState* state, SoNode* node)
{
    SoOverrideElement::setAmbientColorOverride(state, node, false);
    SoOverrideElement::setDiffuseColorOverride(state, node, false);
    SoOverrideElement::setEmissiveColorOverride(state, node, false);
}

// Coin retains the diffuse-color pointer in SoLazyElement until the shape is
// rendered. The caller must keep `color` alive through the GLRender() call that
// consumes this state.
inline void applyMaterial(
    SoState* state,
    SoNode* node,
    VisualRole role,
    MaterialMode mode,
    const SbColor& color,
    SoColorPacker* packer
)
{
    clearColorOverrides(state, node);

    if (mode == MaterialMode::EmissiveOnly) {
        SoLazyElement::setEmissive(state, &color);
        SoOverrideElement::setEmissiveColorOverride(state, node, true);
        return;
    }

    const SbColor emissive = emissiveColor(role, color);

    SoLazyElement::setAmbient(state, &color);
    SoOverrideElement::setAmbientColorOverride(state, node, true);

    SoLazyElement::setDiffuse(state, node, 1, &color, packer);
    SoOverrideElement::setDiffuseColorOverride(state, node, true);

    SoLazyElement::setEmissive(state, &emissive);
    SoOverrideElement::setEmissiveColorOverride(state, node, true);
}

// Coin retains the packed-color pointer in SoLazyElement until the shape is
// rendered. The caller must keep `packedColor` storage alive through the
// GLRender() call that consumes this state.
inline void applyPackedMaterial(
    SoState* state,
    SoNode* node,
    VisualRole role,
    const SbColor& color,
    const std::uint32_t* packedColor,
    bool hasTransparency
)
{
    const SbColor emissive = emissiveColor(role, color);

    clearColorOverrides(state, node);

    SoLazyElement::setAmbient(state, &color);
    SoOverrideElement::setAmbientColorOverride(state, node, true);

    SoLazyElement::setPacked(state, node, 1, packedColor, hasTransparency);
    SoOverrideElement::setDiffuseColorOverride(state, node, true);

    SoLazyElement::setEmissive(state, &emissive);
    SoOverrideElement::setEmissiveColorOverride(state, node, true);
}

}  // namespace Gui::SelectionMaterial
