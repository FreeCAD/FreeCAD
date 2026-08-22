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
#include <optional>

#include <fmt/format.h>

#include "Diagnostics.h"
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
    /// Builds edges from any value; a value that is not valid edges yields zero edges.
    explicit Edges(const Value& value)
        : tuple_(validated(value, std::nullopt).value_or(zeros(std::nullopt)))
    {}

    /// Builds edges from a value, or nothing when the value is not valid edges.
    static std::optional<Edges> tryFrom(const Value& value)
    {
        if (auto tuple = validated(value, std::nullopt)) {
            return Edges(std::move(*tuple));
        }
        return std::nullopt;
    }

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
     * and explicit side overrides. Returns nothing when there are more than four
     * positional arguments.
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
            Diagnostics::report("Edges accept 1-4 positional arguments, got {}", positional.size());
            return std::nullopt;
        }

        // CSS box-model shorthand: top, right, bottom, left
        auto [top, right, bottom, left] = [&]() -> std::array<const Value*, 4> {
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

        static const Value zero {styleDefault<T>()};

        return Tuple({
            Element::named("top", top ? *top : zero),
            Element::named("right", right ? *right : zero),
            Element::named("bottom", bottom ? *bottom : zero),
            Element::named("left", left ? *left : zero),
        });
    }

protected:
    /**
     * @brief Normalizes and fully validates a value as edges of the given kind.
     *
     * Accepts a Generic tuple or any of the four edge kinds (Padding, Margins,
     * BorderThickness, BorderColors) — they share the four-sided box shape and coerce
     * between each other, e.g. a Margins token reads correctly as Padding. Anything else
     * (Corners, a gradient, ...) is rejected outright, since such a tuple would otherwise
     * silently backfill as all-zero edges with no side actually resolved from it. What
     * remains is checking that every side resolves to a T, which is what separately
     * rejects same-shaped-but-mistyped input such as border_colors(...) passed where
     * padding(...) is expected. Passing no expected kind leaves the kind untouched;
     * otherwise the result is retagged with it.
     */
    static std::optional<Tuple> validated(const Value& value, std::optional<TupleKind> expectedKind)
    {
        const Tuple source = asTuple(value);

        if (source.kind != TupleKind::Generic && !isEdgeKind(source.kind)) {
            Diagnostics::report(
                "Expected {} tuple, got {}",
                expectedKind ? tupleKindName(*expectedKind) : "edges",
                tupleKindName(source.kind)
            );
            return std::nullopt;
        }

        auto expanded = expand(source);
        if (!expanded) {
            return std::nullopt;
        }

        for (const char* side : {"top", "right", "bottom", "left"}) {
            if (!expanded->tryGetOrReport<T>(side)) {
                return std::nullopt;
            }
        }

        if (expectedKind) {
            expanded->kind = *expectedKind;
        }

        return expanded;
    }

    /// All-zero edges, used when a value cannot be interpreted as edges.
    static Tuple zeros(std::optional<TupleKind> kind)
    {
        Tuple result = *expand(Tuple {});
        if (kind) {
            result.kind = *kind;
        }
        return result;
    }

    /// Expands and fully validates a raw argument tuple, tagging it with the given kind.
    explicit Edges(Tuple tuple, TupleKind expected)
        : tuple_(validated(Value {std::move(tuple)}, expected).value_or(zeros(expected)))
    {}

    /**
     * @brief Wraps a tuple that validated() has already accepted.
     *
     * Not public: holding an Edges must imply its tuple is well formed, so the only way in
     * from outside is through a validating entry point — tryFrom, or the Value constructor.
     */
    explicit Edges(Tuple tuple)
        : tuple_(std::move(tuple))
    {}

private:
    Tuple tuple_;
};

/**
 * @brief Adds kind identity to an Edges specialization.
 *
 * A concrete edge type states only which TupleKind it is; construction, validation and
 * the tryFrom factory come from here.
 */
template<typename Derived, typename T, TupleKind Kind>
class TypedEdges: public Edges<T>
{
public:
    /// Wraps a raw argument tuple, expanding shorthand and tagging it with this kind.
    explicit TypedEdges(Tuple tuple)
        : Edges<T>(std::move(tuple), Kind)
    {
        assertUsedAsCrtpBase();
    }

    /// Total: a value that is not valid edges of this kind yields zero edges.
    explicit TypedEdges(const Value& value)
        : Edges<T>(Edges<T>::validated(value, Kind).value_or(Edges<T>::zeros(Kind)))
    {
        assertUsedAsCrtpBase();
    }

    /// Builds this kind from a value, or nothing when the value is not that shape.
    static std::optional<Derived> tryFrom(const Value& value)
    {
        if (auto tuple = Edges<T>::validated(value, Kind)) {
            return Derived(std::move(*tuple));
        }
        return std::nullopt;
    }

    static constexpr TupleKind kind()
    {
        return Kind;
    }

private:
    /**
     * @brief Rejects instantiating this template other than as Derived's own base.
     *
     * Without it, TypedEdges<Padding, Numeric, TupleKind::Corners> is publicly constructible
     * and validates a tuple as edges before tagging it Corners — a mislabelled tuple no
     * accessor would catch. Checked from the constructor bodies, where Derived is complete.
     */
    static constexpr void assertUsedAsCrtpBase()
    {
        static_assert(
            std::is_base_of_v<TypedEdges, Derived>,
            "TypedEdges may only be instantiated as the base of the kind it names"
        );
    }
};

}  // namespace Gui::StyleParameters
