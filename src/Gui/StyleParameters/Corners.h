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

#include <array>

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
        if (tuple_.kind == TupleKind::Generic) {
            tuple_ = expand(tuple_);
            tuple_.kind = TupleKind::Corners;
        }
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

    /**
     * @brief Expands a tuple using CSS border-radius shorthand rules.
     *
     * Uses diagonal pairing (2 values = top-left/bottom-right paired),
     * and explicit corner name overrides. Returns a normalized 4-element
     * tuple with Generic kind — caller sets the target kind.
     */
    static Tuple expand(const Tuple& args)
    {
        std::vector<const Value*> positional;
        for (const auto& elem : args.elements) {
            if (!elem.name) {
                positional.push_back(elem.value.get());
            }
        }

        // Single tuple argument: pass through for re-tagging (e.g., border_radius(@existingTuple))
        if (positional.size() == 1 && positional[0]->holds<Tuple>()) {
            return positional[0]->get<Tuple>();
        }

        // CSS border-radius shorthand: diagonal pairing
        auto [topLeft, topRight, bottomRight, bottomLeft] = [&]() -> std::array<const Value*, 4> {
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
                    THROWM(Base::ExpressionError, "Corners accept 1-4 positional arguments");
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

        if (!topLeft || !topRight || !bottomRight || !bottomLeft) {
            THROWM(Base::ExpressionError, "Corners require all four corners to be specified");
        }

        auto makeElement = [](const char* name, const Value& val) {
            return Tuple::Element {
                .name = std::string(name),
                .value = std::make_shared<const Value>(val)
            };
        };

        Tuple result;
        result.elements.push_back(makeElement("top_left", *topLeft));
        result.elements.push_back(makeElement("top_right", *topRight));
        result.elements.push_back(makeElement("bottom_right", *bottomRight));
        result.elements.push_back(makeElement("bottom_left", *bottomLeft));
        return result;
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
