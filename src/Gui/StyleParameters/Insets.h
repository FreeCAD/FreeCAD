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

#include "Edges.h"

namespace Gui::StyleParameters
{

/// @brief 4-side numeric insets (top, right, bottom, left).
using Insets = Edges<Numeric>;

/**
 * @brief Padding insets — wraps a Tuple with kind == TupleKind::Padding.
 */
class Padding: public Edges<Numeric>
{
public:
    explicit Padding(Tuple tuple)
        : Edges<Numeric>(std::move(tuple), TupleKind::Padding)
    {}

    explicit Padding(const Value& value)
        : Edges<Numeric>(asTuple(value), TupleKind::Padding)
    {}

    static constexpr TupleKind kind()
    {
        return TupleKind::Padding;
    }
};

/**
 * @brief Margins insets — wraps a Tuple with kind == TupleKind::Margins.
 */
class Margins: public Edges<Numeric>
{
public:
    explicit Margins(Tuple tuple)
        : Edges<Numeric>(std::move(tuple), TupleKind::Margins)
    {}

    explicit Margins(const Value& value)
        : Edges<Numeric>(asTuple(value), TupleKind::Margins)
    {}

    static constexpr TupleKind kind()
    {
        return TupleKind::Margins;
    }
};

/**
 * @brief Border thickness insets — wraps a Tuple with kind == TupleKind::BorderThickness.
 */
class BorderThickness: public Edges<Numeric>
{
public:
    explicit BorderThickness(Tuple tuple)
        : Edges<Numeric>(std::move(tuple), TupleKind::BorderThickness)
    {}

    explicit BorderThickness(const Value& value)
        : Edges<Numeric>(asTuple(value), TupleKind::BorderThickness)
    {}

    static constexpr TupleKind kind()
    {
        return TupleKind::BorderThickness;
    }
};

/**
 * @brief Per-side border colors — wraps a Tuple with kind == TupleKind::BorderColors.
 *
 * Accepts the same CSS shorthand as numeric insets: a bare color value expands to
 * all four sides, two values set vertical and horizontal, and so on. The
 * border_colors() YAML function produces a typed tuple directly.
 */
class BorderColors: public Edges<Base::Color>
{
public:
    explicit BorderColors(Tuple tuple)
        : Edges<Base::Color>(std::move(tuple), TupleKind::BorderColors)
    {}

    explicit BorderColors(const Value& value)
        : Edges<Base::Color>(asTuple(value), TupleKind::BorderColors)
    {}

    static constexpr TupleKind kind()
    {
        return TupleKind::BorderColors;
    }
};

}  // namespace Gui::StyleParameters
