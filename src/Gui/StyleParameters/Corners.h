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
#include <optional>

#include "Diagnostics.h"
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
    /// Builds corners from a value; a value that is not valid corners yields zero corners.
    explicit Corners(const Value& value)
        : tuple_(validated(value).value_or(zeros()))
    {}

    /// Builds corners from a raw argument tuple; an invalid tuple yields zero corners.
    explicit Corners(Tuple tuple)
        : Corners(Value {std::move(tuple)})
    {}

    /// Builds corners from a value, or nothing when the value is not valid corners.
    static std::optional<Corners> tryFrom(const Value& value)
    {
        if (auto tuple = validated(value)) {
            return Corners(std::move(*tuple));
        }
        return std::nullopt;
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

    /**
     * @brief Expands a tuple using CSS border-radius shorthand rules.
     *
     * Uses diagonal pairing (2 values = top-left/bottom-right paired) and explicit corner
     * name overrides. Returns nothing when there are more than four positional arguments.
     */
    static std::optional<Tuple> expand(const Tuple& args)
    {
        std::vector<const Value*> positional;
        for (const auto& element : args.elements) {
            if (!element.name) {
                positional.push_back(element.value.get());
            }
        }

        // Single tuple argument: always expand to normalize shape.
        if (positional.size() == 1 && positional[0]->holds<Tuple>()) {
            return expand(positional[0]->get<Tuple>());
        }

        if (positional.size() > 4) {  // NOLINT(*-magic-numbers)
            Diagnostics::report("Corners accept 1-4 positional arguments, got {}", positional.size());
            return std::nullopt;
        }

        // CSS border-radius shorthand: diagonal pairing
        auto [topLeft, topRight, bottomRight, bottomLeft] = [&]() -> std::array<const Value*, 4> {
            switch (positional.size()) {
                case 1:
                    return {positional[0], positional[0], positional[0], positional[0]};
                case 2:
                    return {positional[0], positional[1], positional[0], positional[1]};
                case 3:  // NOLINT(*-magic-numbers)
                    return {positional[0], positional[1], positional[2], positional[1]};
                case 4:  // NOLINT(*-magic-numbers)
                    return {positional[0], positional[1], positional[2], positional[3]};
                default:
                    return {nullptr, nullptr, nullptr, nullptr};
            }
        }();

        // Explicit corner names override positional
        if (const Value* found = args.find("top_left")) {
            topLeft = found;
        }
        if (const Value* found = args.find("top_right")) {
            topRight = found;
        }
        if (const Value* found = args.find("bottom_right")) {
            bottomRight = found;
        }
        if (const Value* found = args.find("bottom_left")) {
            bottomLeft = found;
        }

        static const Value zero {styleDefault<Numeric>()};

        // clang-format off
        return Tuple({
            Element::named("top_left",     topLeft     ? *topLeft     : zero),
            Element::named("top_right",    topRight    ? *topRight    : zero),
            Element::named("bottom_right", bottomRight ? *bottomRight : zero),
            Element::named("bottom_left",  bottomLeft  ? *bottomLeft  : zero),
        });
        // clang-format on
    }

    const Tuple& tuple() const
    {
        return tuple_;
    }

private:
    /**
     * @brief Normalizes and fully validates a value as corners.
     *
     * Accepts a Generic tuple or an existing Corners tuple only — unlike the edge kinds,
     * Corners has no sibling kinds it coerces with, so anything else (Padding, a gradient,
     * ...) is rejected outright. Otherwise such a tuple would silently backfill as all-zero
     * corners with no diagnostic, since expand() fills missing corner names with zeros
     * regardless of what the tuple actually was. What remains is checking that every corner
     * resolves to a Numeric, which separately rejects same-shaped-but-mistyped input.
     */
    static std::optional<Tuple> validated(const Value& value)
    {
        const Tuple source = asTuple(value);

        if (source.kind != TupleKind::Generic && source.kind != TupleKind::Corners) {
            Diagnostics::report(
                "Expected {} tuple, got {}",
                tupleKindName(TupleKind::Corners),
                tupleKindName(source.kind)
            );
            return std::nullopt;
        }

        auto expanded = expand(source);
        if (!expanded) {
            return std::nullopt;
        }

        for (const char* corner : {"top_left", "top_right", "bottom_right", "bottom_left"}) {
            if (!expanded->tryGetOrReport<Numeric>(corner)) {
                return std::nullopt;
            }
        }

        expanded->kind = TupleKind::Corners;
        return expanded;
    }

    /// All-zero corners, used when a value cannot be interpreted as corners.
    static Tuple zeros()
    {
        Tuple result = *expand(Tuple {});
        result.kind = TupleKind::Corners;
        return result;
    }

    Tuple tuple_;
};

}  // namespace Gui::StyleParameters
