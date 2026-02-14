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

#ifndef STYLEPARAMETERS_CORNERS_H
#define STYLEPARAMETERS_CORNERS_H

#include <Base/Exception.h>
#include <fmt/format.h>

#include "Value.h"

namespace Gui::StyleParameters
{

/**
 * @brief C++ wrapper providing ergonomic access to a 4-element corner radii tuple
 *        (top_left, top_right, bottom_right, bottom_left).
 *
 * Unlike Insets (which represent box sides), Corners represent the four corners
 * of a rectangle. The CSS border-radius shorthand uses diagonal pairing for 2-value
 * expansion rather than the opposite-side pairing used by margin/padding.
 */
class Corners
{
public:
    explicit Corners(Tuple tuple)
        : tuple_(std::move(tuple))
    {
        if (tuple_.kind != TupleKind::Corners) {
            THROWM(
                Base::TypeError,
                fmt::format(
                    "Expected {} tuple, got {}",
                    tupleKindName(TupleKind::Corners),
                    tupleKindName(tuple_.kind)
                )
            );
        }
    }

    const Numeric& topLeft() const
    {
        return tuple_.get<Numeric>("top_left");
    }
    const Numeric& topRight() const
    {
        return tuple_.get<Numeric>("top_right");
    }
    const Numeric& bottomRight() const
    {
        return tuple_.get<Numeric>("bottom_right");
    }
    const Numeric& bottomLeft() const
    {
        return tuple_.get<Numeric>("bottom_left");
    }

    static constexpr TupleKind kind()
    {
        return TupleKind::Corners;
    }

    const Tuple& tuple() const
    {
        return tuple_;
    }

private:
    Tuple tuple_;
};

}  // namespace Gui::StyleParameters

#endif  // STYLEPARAMETERS_CORNERS_H
