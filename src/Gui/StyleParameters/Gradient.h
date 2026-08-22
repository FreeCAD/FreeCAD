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

#include <algorithm>
#include <functional>
#include <optional>
#include <ranges>
#include <vector>

#include "Diagnostics.h"
#include "Value.h"

namespace Gui::StyleParameters
{

/**
 * @brief Represents a single color stop in a gradient.
 */
struct ColorStop
{
    Numeric position;
    Base::Color color;
};

/**
 * @brief Base class for gradient tuple wrappers.
 *
 * Gradients are stored as typed Tuples with named geometry elements
 * (e.g. x1, y1, x2, y2 for linear) and a named "stops" element
 * containing a Tuple of (position, color) sub-tuples.
 *
 * Gradients must be created via parser functions (linear_gradient,
 * radial_gradient) — Generic tuples do NOT auto-expand to gradients
 * unless explicitly constructed via the wrapper class.
 */
class Gradient
{
public:
    /**
     * @brief Returns the stops sub-tuple.
     */
    const Tuple& stops() const
    {
        return tuple_.get<Tuple>("stops");
    }

    /**
     * @brief Extracts structured color stops.
     *
     * tuple_ is only ever populated through validated construction (the Tuple/Value
     * constructors and tryFrom on TypedGradient), so every stop is already known to be a
     * well-formed (position, color) pair.
     */
    std::vector<ColorStop> colorStops() const
    {
        std::vector<ColorStop> result;

        for (const auto& stop : stops().elements) {
            const Tuple& entry = stop.value->get<Tuple>();
            result.push_back(
                {.position = entry.at(0).get<Numeric>(), .color = entry.at(1).get<Base::Color>()}
            );
        }

        return result;
    }

    const Tuple& tuple() const
    {
        return tuple_;
    }

    /**
     * @brief Applies a color transform to every stop, keeping geometry and positions unchanged.
     *
     * Operates on the already-validated tuple_, so every stop is known to be a well-formed
     * (position, color) pair.
     */
    Tuple mapStopColors(const std::function<Base::Color(const Base::Color&)>& transform) const
    {
        auto makeStopTuple = [](Numeric position, Base::Color color) {
            return Tuple({
                Element::unnamed(position),
                Element::unnamed(color),
            });
        };

        auto transformStop = [&](const Tuple::Element& stop) -> Tuple::Element {
            const Tuple& entry = stop.value->get<Tuple>();
            return Element::unnamed(
                makeStopTuple(entry.at(0).get<Numeric>(), transform(entry.at(1).get<Base::Color>()))
            );
        };

        auto transformElement = [&](const Tuple::Element& element) -> Tuple::Element {
            if (element.name != "stops") {
                return element;
            }

            const auto& stopsTuple = element.value->get<Tuple>();
            Tuple transformedStops;
            std::ranges::transform(
                stopsTuple.elements,
                std::back_inserter(transformedStops.elements),
                transformStop
            );
            return Element::named("stops", std::move(transformedStops));
        };

        Tuple result;
        result.kind = tuple_.kind;
        std::ranges::transform(tuple_.elements, std::back_inserter(result.elements), transformElement);
        return result;
    }

protected:
    explicit Gradient(Tuple tuple)
        : tuple_(std::move(tuple))
    {}

    /// True when the tuple has a stops element holding well-formed (position, color) entries.
    static bool isWellFormed(const Tuple& tuple)
    {
        const Tuple* stops = tuple.tryGet<Tuple>("stops");
        if (!stops || stops->size() < 2) {
            return false;
        }

        for (const auto& stop : stops->elements) {
            const Tuple* entry = stop.value->tryGet<Tuple>();
            if (!entry || entry->size() != 2 || !entry->at(0).tryGet<Numeric>()
                || !entry->at(1).tryGet<Base::Color>()) {
                return false;
            }
        }

        for (const auto& element : tuple.elements) {
            if (element.name && *element.name != "stops" && !element.value->tryGet<Numeric>()) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Extracts a numeric value from a found element, using the default when absent
     *        or reporting and defaulting when present but not Numeric.
     */
    static double numericOrDefault(const Value* found, const char* paramName, double defaultValue)
    {
        if (!found) {
            return defaultValue;
        }

        if (const Numeric* numeric = found->tryGet<Numeric>()) {
            return numeric->value;
        }

        Diagnostics::report(
            "Gradient geometry parameter '{}' must be a number, got {}",
            paramName,
            found->toString()
        );
        return defaultValue;
    }

    /**
     * @brief Creates a geometry element with a default value.
     *
     * Looks up a named element in the args tuple and uses the provided
     * default if not found.
     */
    static Tuple::Element makeGeometryElement(const char* name, double defaultValue, const Tuple& args)
    {
        double value = numericOrDefault(args.find(name), name, defaultValue);
        return Element::named(name, Numeric {.value = value, .unit = ""});
    }

    /**
     * @brief Builds the "stops" element from positional arguments.
     *
     * Processes positional args from the function call:
     * - Bare Color values become auto-positioned stops
     * - Tuple(Numeric, Color) values become explicit stops
     *
     * Auto-positioning distributes bare colors evenly from 0 to 1
     * based on their index in the total stop list.
     * Stops are sorted by position after processing.
     *
     * Returns nothing (and reports) when there are fewer than 2 stops or a stop is malformed.
     */
    static std::optional<Tuple::Element> buildStopsElement(const Tuple& args)
    {
        struct RawStop
        {
            std::optional<double> position;
            Base::Color color;
        };

        std::vector<RawStop> rawStops;

        for (const auto& element : args.elements) {
            if (element.name) {
                continue;  // named args are geometry, skip
            }

            const Value& value = *element.value;

            if (value.holds<Base::Color>()) {
                rawStops.push_back({
                    .position = std::nullopt,
                    .color = value.get<Base::Color>(),
                });
            }
            else if (value.holds<Tuple>()) {
                const auto& stopTuple = value.get<Tuple>();
                if (stopTuple.size() != 2 || !stopTuple.at(0).holds<Numeric>()
                    || !stopTuple.at(1).holds<Base::Color>()) {
                    Diagnostics::report("Gradient stop tuple must be (position, color)");
                    return std::nullopt;
                }
                rawStops.push_back({
                    .position = stopTuple.at(0).get<Numeric>().value,
                    .color = stopTuple.at(1).get<Base::Color>(),
                });
            }
            else {
                Diagnostics::report(
                    "Gradient arguments must be colors or (position, color) tuples, got {}",
                    value.toString()
                );
                return std::nullopt;
            }
        }

        if (rawStops.size() < 2) {
            Diagnostics::report("Gradient requires at least 2 color stops");
            return std::nullopt;
        }

        // Auto-distribute positions for bare colors
        for (size_t index = 0; index < rawStops.size(); ++index) {
            if (!rawStops[index].position) {
                rawStops[index].position = static_cast<double>(index)
                    / static_cast<double>(rawStops.size() - 1);
            }
        }

        // Sort stops by position
        std::ranges::sort(rawStops, [](const RawStop& left, const RawStop& right) {
            return *left.position < *right.position;
        });

        // Build stops tuple
        Tuple stopsTuple;
        for (const auto& stop : rawStops) {
            Tuple stopEntry({
                Element::unnamed(Numeric {.value = *stop.position, .unit = ""}),
                Element::unnamed(stop.color),
            });
            stopsTuple.elements.push_back(Element::unnamed(std::move(stopEntry)));
        }

        return Element::named("stops", std::move(stopsTuple));
    }

    /// Two fully transparent stops, used when a value cannot be interpreted as a gradient.
    static Tuple::Element transparentStops()
    {
        const auto stop = [](double position) {
            return Element::unnamed(Tuple({
                Element::unnamed(Numeric {.value = position, .unit = ""}),
                Element::unnamed(Base::Color(0.0F, 0.0F, 0.0F, 0.0F)),
            }));
        };

        return Element::named("stops", Tuple({stop(0.0), stop(1.0)}));
    }

    Tuple tuple_;
};

/**
 * @brief Adds kind identity to a Gradient specialization.
 *
 * A concrete gradient type states only its kind and supplies its own expand() (Generic-args
 * shorthand expansion) and zeros() (transparent fallback tagged with its kind) — those
 * genuinely differ per gradient (different geometry fields, different defaults). Validation,
 * the tryFrom factory, and both constructors are shape-identical across kinds and come from
 * here, mirroring TypedEdges in Edges.h.
 */
template<typename Derived, TupleKind Kind>
class TypedGradient: public Gradient
{
public:
    /// Total: a value that is not a valid Kind gradient yields Derived::zeros().
    explicit TypedGradient(const Value& value)
        : Gradient(validated(value).value_or(Derived::zeros()))
    {}

    /// Total: wraps a raw tuple with the same validation as the Value constructor, so this
    /// can never produce an ill-formed gradient either.
    explicit TypedGradient(Tuple tuple)
        : TypedGradient(Value {std::move(tuple)})
    {}

    /// Builds this kind from a value, or nothing when the value is not that shape.
    static std::optional<Derived> tryFrom(const Value& value)
    {
        if (auto tuple = validated(value)) {
            return Derived(std::move(*tuple));
        }
        return std::nullopt;
    }

    static constexpr TupleKind kind()
    {
        return Kind;
    }

protected:
    /**
     * @brief Normalizes and fully validates a value as a Kind gradient.
     *
     * Accepts a Generic tuple (expanded via Derived::expand) or an already-tagged Kind tuple
     * whose stops and geometry check out. Anything else is rejected outright.
     */
    static std::optional<Tuple> validated(const Value& value)
    {
        const Tuple source = asTuple(value);

        if (source.kind == Kind) {
            if (!isWellFormed(source)) {
                Diagnostics::report("Malformed {} tuple {}", tupleKindName(Kind), Value {source});
                return std::nullopt;
            }
            return source;
        }

        if (source.kind != TupleKind::Generic) {
            Diagnostics::report(
                "Expected {} tuple, got {}",
                tupleKindName(Kind),
                tupleKindName(source.kind)
            );
            return std::nullopt;
        }

        return Derived::expand(source);
    }
};

/**
 * @brief Wrapper for LinearGradient tuples.
 *
 * Geometry: x1, y1, x2, y2 (defaults: 0, 0, 0, 1 = top to bottom).
 * Plus a "stops" element containing color stop sub-tuples.
 */
class LinearGradient: public TypedGradient<LinearGradient, TupleKind::LinearGradient>
{
public:
    using TypedGradient::TypedGradient;

    double x1() const
    {
        return tuple_.get<Numeric>("x1").value;
    }
    double y1() const
    {
        return tuple_.get<Numeric>("y1").value;
    }
    double x2() const
    {
        return tuple_.get<Numeric>("x2").value;
    }
    double y2() const
    {
        return tuple_.get<Numeric>("y2").value;
    }

private:
    friend class TypedGradient<LinearGradient, TupleKind::LinearGradient>;

    /// A transparent top-to-bottom gradient, used when a value cannot be interpreted as a
    /// linear gradient.
    static Tuple zeros()
    {
        return Tuple(
            {
                Element::named("x1", Numeric {.value = 0.0, .unit = ""}),
                Element::named("y1", Numeric {.value = 0.0, .unit = ""}),
                Element::named("x2", Numeric {.value = 0.0, .unit = ""}),
                Element::named("y2", Numeric {.value = 1.0, .unit = ""}),
                transparentStops(),
            },
            TupleKind::LinearGradient
        );
    }

    static std::optional<Tuple> expand(const Tuple& args)
    {
        auto stops = buildStopsElement(args);
        if (!stops) {
            return std::nullopt;
        }

        return Tuple(
            {
                makeGeometryElement("x1", 0.0, args),
                makeGeometryElement("y1", 0.0, args),
                makeGeometryElement("x2", 0.0, args),
                makeGeometryElement("y2", 1.0, args),
                std::move(*stops),
            },
            TupleKind::LinearGradient
        );
    }
};

/**
 * @brief Wrapper for RadialGradient tuples.
 *
 * Geometry: cx, cy, radius, fx, fy (defaults: 0.5, 0.5, 0.5, cx, cy).
 * fx and fy default to cx and cy respectively when not specified.
 * Plus a "stops" element containing color stop sub-tuples.
 */
class RadialGradient: public TypedGradient<RadialGradient, TupleKind::RadialGradient>
{
public:
    using TypedGradient::TypedGradient;

    double cx() const
    {
        return tuple_.get<Numeric>("cx").value;
    }
    double cy() const
    {
        return tuple_.get<Numeric>("cy").value;
    }
    double radius() const
    {
        return tuple_.get<Numeric>("radius").value;
    }
    double fx() const
    {
        return tuple_.get<Numeric>("fx").value;
    }
    double fy() const
    {
        return tuple_.get<Numeric>("fy").value;
    }

private:
    friend class TypedGradient<RadialGradient, TupleKind::RadialGradient>;

    /// A transparent centered gradient, used when a value cannot be interpreted as a radial
    /// gradient.
    static Tuple zeros()
    {
        constexpr double defaultCenter = 0.5;
        constexpr double defaultRadius = 0.5;

        return Tuple(
            {
                Element::named("cx", Numeric {.value = defaultCenter, .unit = ""}),
                Element::named("cy", Numeric {.value = defaultCenter, .unit = ""}),
                Element::named("radius", Numeric {.value = defaultRadius, .unit = ""}),
                Element::named("fx", Numeric {.value = defaultCenter, .unit = ""}),
                Element::named("fy", Numeric {.value = defaultCenter, .unit = ""}),
                transparentStops(),
            },
            TupleKind::RadialGradient
        );
    }

    static std::optional<Tuple> expand(const Tuple& args)
    {
        // fx defaults to cx, fy defaults to cy
        constexpr double defaultCenter = 0.5;
        constexpr double defaultRadius = 0.5;

        double cxValue = numericOrDefault(args.find("cx"), "cx", defaultCenter);
        double cyValue = numericOrDefault(args.find("cy"), "cy", defaultCenter);
        double fxValue = numericOrDefault(args.find("fx"), "fx", cxValue);
        double fyValue = numericOrDefault(args.find("fy"), "fy", cyValue);

        auto stops = buildStopsElement(args);
        if (!stops) {
            return std::nullopt;
        }

        return Tuple(
            {
                makeGeometryElement("cx", defaultCenter, args),
                makeGeometryElement("cy", defaultCenter, args),
                makeGeometryElement("radius", defaultRadius, args),
                Element::named("fx", Numeric {.value = fxValue, .unit = ""}),
                Element::named("fy", Numeric {.value = fyValue, .unit = ""}),
                std::move(*stops),
            },
            TupleKind::RadialGradient
        );
    }
};

/**
 * @brief Applies a color transform to a gradient value's stops, or nothing when it is not a
 *        gradient.
 *
 * Tries the value as a LinearGradient, then a RadialGradient; the mapping only ever runs on
 * a wrapper that has already passed tryFrom's validation, so this is the sole path from a
 * caller-supplied Value to a stop transform.
 */
inline std::optional<Tuple> mapGradientStops(
    const Value& value,
    const std::function<Base::Color(const Base::Color&)>& transform
)
{
    if (auto linear = LinearGradient::tryFrom(value)) {
        return linear->mapStopColors(transform);
    }
    if (auto radial = RadialGradient::tryFrom(value)) {
        return radial->mapStopColors(transform);
    }
    return std::nullopt;
}

}  // namespace Gui::StyleParameters
