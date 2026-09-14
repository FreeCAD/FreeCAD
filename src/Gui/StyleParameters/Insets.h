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

/// @brief Padding insets — wraps a Tuple with kind == TupleKind::Padding.
class Padding: public TypedEdges<Padding, Numeric, TupleKind::Padding>
{
public:
    using TypedEdges::TypedEdges;
};

/// @brief Margins insets — wraps a Tuple with kind == TupleKind::Margins.
class Margins: public TypedEdges<Margins, Numeric, TupleKind::Margins>
{
public:
    using TypedEdges::TypedEdges;
};

/// @brief Border thickness insets — wraps a Tuple with kind == TupleKind::BorderThickness.
class BorderThickness: public TypedEdges<BorderThickness, Numeric, TupleKind::BorderThickness>
{
public:
    using TypedEdges::TypedEdges;
};

/**
 * @brief Per-side border colors — wraps a Tuple with kind == TupleKind::BorderColors.
 *
 * Accepts the same CSS shorthand as numeric insets: a bare color expands to all four
 * sides, two values set vertical and horizontal, and so on.
 */
class BorderColors: public TypedEdges<BorderColors, Base::Color, TupleKind::BorderColors>
{
public:
    using TypedEdges::TypedEdges;
};

}  // namespace Gui::StyleParameters
