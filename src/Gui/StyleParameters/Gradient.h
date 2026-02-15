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

#ifndef STYLEPARAMETERS_GRADIENT_H
#define STYLEPARAMETERS_GRADIENT_H

#include <algorithm>
#include <vector>

#include <Base/Exception.h>
#include <fmt/format.h>

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
     * @brief Extracts structured color stops from the tuple.
     */
    std::vector<ColorStop> colorStops() const
    {
        std::vector<ColorStop> result;
        const auto& stopsTuple = stops();
        for (size_t index = 0; index < stopsTuple.size(); ++index) {
            const auto& stopEntry = stopsTuple.at(index).get<Tuple>();
            result.push_back({
                .position = stopEntry.at(0).get<Numeric>(),
                .color = stopEntry.at(1).get<Base::Color>(),
            });
        }
        return result;
    }

    const Tuple& tuple() const
    {
        return tuple_;
    }

protected:
    Gradient(Tuple tuple, TupleKind expected)
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

    /**
     * @brief Creates a geometry element with a default value.
     *
     * Looks up a named element in the args tuple and uses the provided
     * default if not found.
     */
    static Tuple::Element makeGeometryElement(const char* name, double defaultValue, const Tuple& args)
    {
        const Value* found = args.find(name);
        double value = found ? found->get<Numeric>().value : defaultValue;
        return Tuple::Element {
            .name = std::string(name),
            .value = std::make_shared<const Value>(Numeric {.value = value, .unit = ""}),
        };
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
     * @throws Base::ExpressionError if fewer than 2 stops or invalid stop format.
     */
    static Tuple::Element buildStopsElement(const Tuple& args)
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
                    THROWM(Base::ExpressionError, "Gradient stop tuple must be (position, color)");
                }
                rawStops.push_back({
                    .position = stopTuple.at(0).get<Numeric>().value,
                    .color = stopTuple.at(1).get<Base::Color>(),
                });
            }
            else {
                THROWM(
                    Base::ExpressionError,
                    "Gradient arguments must be colors or (position, color) tuples"
                );
            }
        }

        if (rawStops.size() < 2) {
            THROWM(Base::ExpressionError, "Gradient requires at least 2 color stops");
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
            Tuple stopEntry;
            stopEntry.elements.push_back({
                .name = std::nullopt,
                .value = std::make_shared<const Value>(Numeric {.value = *stop.position, .unit = ""}),
            });
            stopEntry.elements.push_back({
                .name = std::nullopt,
                .value = std::make_shared<const Value>(stop.color),
            });
            stopsTuple.elements.push_back({
                .name = std::nullopt,
                .value = std::make_shared<const Value>(std::move(stopEntry)),
            });
        }

        return Tuple::Element {
            .name = std::string("stops"),
            .value = std::make_shared<const Value>(std::move(stopsTuple)),
        };
    }

    Tuple tuple_;
};

/**
 * @brief Wrapper for LinearGradient tuples.
 *
 * Geometry: x1, y1, x2, y2 (defaults: 0, 0, 0, 1 = top to bottom).
 * Plus a "stops" element containing color stop sub-tuples.
 */
class LinearGradient: public Gradient
{
public:
    explicit LinearGradient(Tuple tuple)
        : Gradient(
              tuple.kind == TupleKind::Generic ? expand(tuple) : std::move(tuple),
              TupleKind::LinearGradient
          )
    {}

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

    static constexpr TupleKind kind()
    {
        return TupleKind::LinearGradient;
    }

private:
    static Tuple expand(const Tuple& args)
    {
        Tuple result;
        result.kind = TupleKind::LinearGradient;
        result.elements.push_back(makeGeometryElement("x1", 0.0, args));
        result.elements.push_back(makeGeometryElement("y1", 0.0, args));
        result.elements.push_back(makeGeometryElement("x2", 0.0, args));
        result.elements.push_back(makeGeometryElement("y2", 1.0, args));
        result.elements.push_back(buildStopsElement(args));
        return result;
    }
};

/**
 * @brief Wrapper for RadialGradient tuples.
 *
 * Geometry: cx, cy, radius, fx, fy (defaults: 0.5, 0.5, 0.5, cx, cy).
 * fx and fy default to cx and cy respectively when not specified.
 * Plus a "stops" element containing color stop sub-tuples.
 */
class RadialGradient: public Gradient
{
public:
    explicit RadialGradient(Tuple tuple)
        : Gradient(
              tuple.kind == TupleKind::Generic ? expand(tuple) : std::move(tuple),
              TupleKind::RadialGradient
          )
    {}

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

    static constexpr TupleKind kind()
    {
        return TupleKind::RadialGradient;
    }

private:
    static Tuple expand(const Tuple& args)
    {
        // fx defaults to cx, fy defaults to cy
        constexpr double defaultCenter = 0.5;
        constexpr double defaultRadius = 0.5;

        const Value* cxFound = args.find("cx");
        const Value* cyFound = args.find("cy");
        double cxValue = cxFound ? cxFound->get<Numeric>().value : defaultCenter;
        double cyValue = cyFound ? cyFound->get<Numeric>().value : defaultCenter;

        const Value* fxFound = args.find("fx");
        const Value* fyFound = args.find("fy");
        double fxValue = fxFound ? fxFound->get<Numeric>().value : cxValue;
        double fyValue = fyFound ? fyFound->get<Numeric>().value : cyValue;

        Tuple result;
        result.kind = TupleKind::RadialGradient;
        result.elements.push_back(makeGeometryElement("cx", defaultCenter, args));
        result.elements.push_back(makeGeometryElement("cy", defaultCenter, args));
        result.elements.push_back(makeGeometryElement("radius", defaultRadius, args));
        result.elements.push_back({
            .name = std::string("fx"),
            .value = std::make_shared<const Value>(Numeric {.value = fxValue, .unit = ""}),
        });
        result.elements.push_back({
            .name = std::string("fy"),
            .value = std::make_shared<const Value>(Numeric {.value = fyValue, .unit = ""}),
        });
        result.elements.push_back(buildStopsElement(args));
        return result;
    }
};

}  // namespace Gui::StyleParameters

#endif  // STYLEPARAMETERS_GRADIENT_H
