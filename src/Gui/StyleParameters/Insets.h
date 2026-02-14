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

#ifndef STYLEPARAMETERS_INSETS_H
#define STYLEPARAMETERS_INSETS_H

#include <Base/Exception.h>
#include <fmt/format.h>

#include "Value.h"

namespace Gui::StyleParameters
{

/**
 * @brief C++ wrapper providing ergonomic access to a 4-element insets tuple
 *        (top, right, bottom, left).
 *
 * Owns the Tuple by value so it can be safely returned from functions.
 * Subclasses add kind-specific validation.
 */
class Insets
{
public:
    explicit Insets(Tuple tuple)
        : tuple_(std::move(tuple))
    {}

    const Numeric& top() const
    {
        return tuple_.get<Numeric>("top");
    }
    const Numeric& right() const
    {
        return tuple_.get<Numeric>("right");
    }
    const Numeric& bottom() const
    {
        return tuple_.get<Numeric>("bottom");
    }
    const Numeric& left() const
    {
        return tuple_.get<Numeric>("left");
    }

    Numeric horizontal() const
    {
        return left() + right();
    }
    Numeric vertical() const
    {
        return top() + bottom();
    }

    const Tuple& tuple() const
    {
        return tuple_;
    }

protected:
    Insets(Tuple tuple, TupleKind expected)
        : tuple_(std::move(tuple))
    {
        if (tuple_.kind != expected) {
            THROWM(
                Base::TypeError,
                fmt::format(
                    "Expected {} tuple, got {}",
                    tupleKindName(expected),
                    tupleKindName(tuple_.kind)
                )
            );
        }
    }

private:
    Tuple tuple_;
};

/**
 * @brief Padding insets — wraps a Tuple with kind == TupleKind::Padding.
 */
class Padding: public Insets
{
public:
    explicit Padding(Tuple tuple)
        : Insets(std::move(tuple), TupleKind::Padding)
    {}

    static constexpr TupleKind kind()
    {
        return TupleKind::Padding;
    }
};

/**
 * @brief Margins insets — wraps a Tuple with kind == TupleKind::Margins.
 */
class Margins: public Insets
{
public:
    explicit Margins(Tuple tuple)
        : Insets(std::move(tuple), TupleKind::Margins)
    {}

    static constexpr TupleKind kind()
    {
        return TupleKind::Margins;
    }
};

/**
 * @brief Border thickness insets — wraps a Tuple with kind == TupleKind::BorderThickness.
 */
class BorderThickness: public Insets
{
public:
    explicit BorderThickness(Tuple tuple)
        : Insets(std::move(tuple), TupleKind::BorderThickness)
    {}

    static constexpr TupleKind kind()
    {
        return TupleKind::BorderThickness;
    }
};

}  // namespace Gui::StyleParameters

#endif  // STYLEPARAMETERS_INSETS_H
