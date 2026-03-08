// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <Base/Bitmask.h>

namespace Gui
{

/**
 * @brief Widget components that participate in style token resolution.
 *
 * Add new components before COUNT. The string representation of each component
 * (used in token names) is defined in FreeCADStyle.cpp::componentString().
 */
enum class StyleComponent : uint8_t
{
    PushButton,
    ToolButton,
    LineEdit,  // QLineEdit + QAbstractSpinBox edit frame
    TextEdit,  // QPlainTextEdit, QTextEdit and derivatives
    Select,    // QComboBox (non-editable), inherits Button styles
    ComboBox,  // QComboBox (editable), inherits LineEdit styles
    // Add new components before COUNT
    COUNT
};

/**
 * @brief Visual type variant for button-like components.
 *
 * Derived from Qt widget properties:
 *   - Primary : QPushButton::isDefault() or QStyleOptionButton::DefaultButton feature
 *   - Link    : QPushButton::isFlat(), QToolButton::autoRaise(), or property("flat") == true
 *   - Default : everything else
 *
 * Add new types before COUNT.
 */
enum class ButtonType : uint8_t
{
    Default,
    Primary,
    Link,
    // Add new button types before COUNT
    COUNT
};

/**
 * @brief Size variant, derived from the "controlSize" widget property.
 *
 * Add new sizes before COUNT.
 */
enum class ControlSize : uint8_t
{
    Default,
    Small,
    Big,
    // Add new sizes before COUNT
    COUNT
};

/**
 * @brief Interaction state bitmask — multiple flags may be active simultaneously.
 *
 * Token resolution expands active flags into a fallback prefix list in priority
 * order (highest priority first): Pressed > Hovered > Checked > Focused.
 */
enum class StyleState : uint8_t
{
    Normal = 0,
    Focused = 1 << 0,
    Checked = 1 << 1,
    Hovered = 1 << 2,
    Pressed = 1 << 3,
};

/**
 * @brief Style properties that can be resolved from tokens.
 *
 * Each value corresponds to a token suffix (e.g. Padding → "Padding").
 * Add new properties before COUNT.
 */
enum class StyleProperty : uint8_t
{
    Width,
    MinWidth,
    MaxWidth,
    Height,
    MinHeight,
    MaxHeight,
    BorderThickness,
    BorderRadius,
    BorderColor,
    Padding,
    Margin,
    IconSize,
    IconSpacing,
    FontSize,
    FontWeight,
    Background,
    TextColor,
    Overlay,
    OverlayOpacity,
    InnerShadow,
    // Add new properties before COUNT
    COUNT
};

/**
 * @brief Registry of variant dimensions used in token names.
 *
 * Each slot corresponds to one enum dimension (ButtonType, ControlSize, …).
 * Adding a new variant dimension requires:
 *   1. Adding a slot entry before COUNT here.
 *   2. Defining the values enum with Default = 0 and COUNT as the last value.
 *   3. Adding a string table and entry in FreeCADStyle.cpp::variantSlotNames.
 *   4. Setting the slot in FreeCADStyle.cpp::contextOf().
 */
enum class VariantSlot : uint8_t
{
    ButtonType,
    ControlSize,
    // Add new variant dimensions before COUNT
    COUNT
};

/**
 * @brief Holds one uint8_t value per VariantSlot, defaulting to 0 (the Default
 *        value of each dimension's enum).
 *
 * The array size is determined at compile time by VariantSlot::COUNT, so adding
 * a new slot automatically expands the array — no manual size management needed.
 */
struct VariantKey
{
    std::array<uint8_t, size_t(VariantSlot::COUNT)> slots = {};

    template<typename EnumT>
    void set(VariantSlot slot, EnumT value)
    {
        slots.at(size_t(slot)) = uint8_t(value);
    }

    uint8_t get(VariantSlot slot) const
    {
        return slots.at(size_t(slot));
    }

    bool operator==(const VariantKey&) const = default;
};

/**
 * @brief Fully describes the styling context for a widget in a given state.
 *
 * Built once per draw call via FreeCADStyle::contextOf(), then passed to
 * resolve() and resolveBoxBackground() for cached token lookup.
 */
struct StyleContext
{
    StyleComponent component = StyleComponent::PushButton;
    VariantKey variant = {};
    Base::Flags<StyleState> state;

    bool operator==(const StyleContext&) const = default;
};

}  // namespace Gui

ENABLE_BITMASK_OPERATORS(Gui::StyleState)
