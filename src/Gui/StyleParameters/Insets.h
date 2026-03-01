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

#include <array>

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

    explicit Insets(const Value& value)
        : tuple_([&value]() -> Tuple {
            if (value.holds<Tuple>()) {
                const auto& tuple = value.get<Tuple>();
                return tuple.kind == TupleKind::Generic ? expand(tuple) : tuple;
            }
            return expand(asGenericTuple(value));
        }())
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

    /**
     * @brief Expands a tuple using CSS box-model shorthand rules.
     *
     * Handles positional shorthand (1-4 args), group names (vertical, horizontal),
     * and explicit side overrides (top, right, bottom, left). Returns a normalized
     * 4-element tuple with Generic kind — caller sets the target kind.
     */
    static Tuple expand(const Tuple& args)
    {
        std::vector<const Value*> positional;
        for (const auto& elem : args.elements) {
            if (!elem.name) {
                positional.push_back(elem.value.get());
            }
        }

        // Single tuple argument: pass through for re-tagging (e.g., padding(@existingTuple))
        if (positional.size() == 1 && positional[0]->holds<Tuple>()) {
            return positional[0]->get<Tuple>();
        }

        // CSS box-model shorthand: top, right, bottom, left
        auto [top, right, bottom, left] = [&]() -> std::array<const Value*, 4> {
            switch (positional.size()) {
                case 0:
                    return {nullptr, nullptr, nullptr, nullptr};
                case 1:
                    return {positional[0], positional[0], positional[0], positional[0]};
                case 2:
                    return {positional[0], positional[1], positional[0], positional[1]};
                case 3:  // NOLINT(*-magic-numbers)
                    return {positional[0], positional[1], positional[2], positional[1]};
                case 4:  // NOLINT(*-magic-numbers)
                    return {positional[0], positional[1], positional[2], positional[3]};
                default:
                    THROWM(Base::ExpressionError, "Insets accept 1-4 positional arguments");
            }
        }();

        // Group names override positional
        if (const Value* vertical = args.find("vertical")) {
            top = bottom = vertical;
        }
        if (const Value* horizontal = args.find("horizontal")) {
            right = left = horizontal;
        }

        // Explicit side names override everything
        if (const Value* found = args.find("top")) {
            top = found;
        }
        if (const Value* found = args.find("right")) {
            right = found;
        }
        if (const Value* found = args.find("bottom")) {
            bottom = found;
        }
        if (const Value* found = args.find("left")) {
            left = found;
        }

        if (!top || !right || !bottom || !left) {
            THROWM(Base::ExpressionError, "Insets require all four sides to be specified");
        }

        return Tuple({
            Tuple::Element::named("top", *top),
            Tuple::Element::named("right", *right),
            Tuple::Element::named("bottom", *bottom),
            Tuple::Element::named("left", *left),
        });
    }

protected:
    Insets(Tuple tuple, TupleKind expected)
        : tuple_(std::move(tuple))
    {
        if (tuple_.kind == TupleKind::Generic) {
            tuple_ = expand(tuple_);
            tuple_.kind = expected;
        }
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

    explicit Padding(const Value& value)
        : Insets(asGenericTuple(value), TupleKind::Padding)
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

    explicit Margins(const Value& value)
        : Insets(asGenericTuple(value), TupleKind::Margins)
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

    explicit BorderThickness(const Value& value)
        : Insets(asGenericTuple(value), TupleKind::BorderThickness)
    {}

    static constexpr TupleKind kind()
    {
        return TupleKind::BorderThickness;
    }
};

}  // namespace Gui::StyleParameters

#endif  // STYLEPARAMETERS_INSETS_H
