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

#ifndef STYLEPARAMETERS_INNERSHADOW_H
#define STYLEPARAMETERS_INNERSHADOW_H

#include <Base/Exception.h>
#include <fmt/format.h>

#include "Value.h"

namespace Gui::StyleParameters
{

/**
 * @brief Wrapper for InnerShadow tuples.
 *
 * Represents an inward shadow effect with horizontal/vertical offsets, blur radius,
 * and color. Created via the inner_shadow() parser function.
 *
 * Parameters:
 *   - x:     horizontal offset in pixels (default 0)
 *   - y:     vertical offset in pixels   (default 0)
 *   - blur:  blur radius in pixels       (default 0)
 *   - color: shadow color                (required)
 */
class InnerShadow
{
public:
    /**
     * @brief Constructs from a raw args Tuple (as received from the parser).
     *
     * If the tuple already has kind InnerShadow, it is used as-is.
     * Otherwise it is treated as positional/named arguments and expanded.
     */
    explicit InnerShadow(const Tuple& args)
        : tuple_(args.kind == TupleKind::InnerShadow ? args : expand(args))
    {
        if (tuple_.kind != TupleKind::InnerShadow) {
            THROWM(
                Base::TypeError,
                fmt::format("Expected inner_shadow tuple, got {}", tupleKindName(tuple_.kind))
            );
        }
    }

    /**
     * @brief Constructs from a Value (convenience overload for resolveBoxBackground).
     */
    explicit InnerShadow(const Value& value)
        : InnerShadow(asGenericTuple(value))
    {}

    /// Horizontal offset in pixels.
    double x() const
    {
        return tuple_.get<Numeric>("x").value;
    }

    /// Vertical offset in pixels.
    double y() const
    {
        return tuple_.get<Numeric>("y").value;
    }

    /// Blur radius in pixels.
    double blur() const
    {
        return tuple_.get<Numeric>("blur").value;
    }

    /// Shadow color.
    Base::Color color() const
    {
        return tuple_.get<Base::Color>("color");
    }

    const Tuple& tuple() const
    {
        return tuple_;
    }

    static constexpr TupleKind kind()
    {
        return TupleKind::InnerShadow;
    }

private:
    Tuple tuple_;

    static Tuple expand(const Tuple& args)
    {
        Tuple resolved = ArgumentParser {
            {.name = "x", .defaultValue = Numeric {.value = 0.0, .unit = "px"}},
            {.name = "y", .defaultValue = Numeric {.value = 0.0, .unit = "px"}},
            {.name = "blur", .defaultValue = Numeric {.value = 0.0, .unit = "px"}},
            {.name = "color"},
        }.resolve(args);

        resolved.kind = TupleKind::InnerShadow;
        return resolved;
    }
};

}  // namespace Gui::StyleParameters

#endif  // STYLEPARAMETERS_INNERSHADOW_H
