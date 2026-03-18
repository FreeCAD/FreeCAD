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
#include <concepts>

#include <Base/Exception.h>
#include <fmt/format.h>

#include "Value.h"

namespace Gui::StyleParameters
{

/**
 * @brief Generic 4-side (top, right, bottom, left) CSS box-model wrapper.
 *
 * Owns a 4-element Tuple and provides typed accessors for each side.
 * @p expand() handles CSS 1–4 arg shorthand and named group arguments
 * (vertical, horizontal, top, right, bottom, left). Subclasses add
 * kind-specific validation via the protected constructor.
 */
template<typename T>
class Edges
{
public:
    explicit Edges(Tuple tuple)
        : tuple_(std::move(tuple))
    {}

    explicit Edges(const Value& value)
        : tuple_([&value]() -> Tuple {
            if (value.holds<Tuple>()) {
                const auto& tuple = value.get<Tuple>();
                return tuple.kind == TupleKind::Generic ? expand(tuple) : tuple;
            }
            return expand(asTuple(value));
        }())
    {}

    const T& top() const
    {
        return tuple_.get<T>("top");
    }
    const T& right() const
    {
        return tuple_.get<T>("right");
    }
    const T& bottom() const
    {
        return tuple_.get<T>("bottom");
    }
    const T& left() const
    {
        return tuple_.get<T>("left");
    }

    Numeric horizontal() const
        requires std::same_as<T, Numeric>
    {
        return left() + right();
    }

    Numeric vertical() const
        requires std::same_as<T, Numeric>
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

        // Single tuple argument: always expand to normalize shape.
        if (positional.size() == 1 && positional[0]->holds<Tuple>()) {
            return expand(positional[0]->get<Tuple>());
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
                    THROWM(Base::ExpressionError, "Edges accept 1-4 positional arguments");
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

        // clang-format off
        static const Value zero = []() -> Value {
            if constexpr (std::is_same_v<T, Numeric>) { return Numeric{0.0, ""}; }
            else                                       { return Base::Color{0.0f, 0.0f, 0.0f, 0.0f}; }
        }();
        // clang-format on

        return Tuple({
            Tuple::Element::named("top",    top    ? *top    : zero),
            Tuple::Element::named("right",  right  ? *right  : zero),
            Tuple::Element::named("bottom", bottom ? *bottom : zero),
            Tuple::Element::named("left",   left   ? *left   : zero),
        });
    }

protected:
    explicit Edges(Tuple tuple, TupleKind expected)
        : tuple_(expand(tuple))
    {
        tuple_.kind = expected;
    }

private:
    Tuple tuple_;
};

}  // namespace Gui::StyleParameters
