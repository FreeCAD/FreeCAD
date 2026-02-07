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

#include "Value.h"

#include <ranges>
#include <fmt/ranges.h>

namespace Gui::StyleParameters
{

Numeric Numeric::operator+(const Numeric& rhs) const
{
    ensureEqualUnits(rhs);
    return {value + rhs.value, unit};
}

Numeric Numeric::operator-(const Numeric& rhs) const
{
    ensureEqualUnits(rhs);
    return {value - rhs.value, unit};
}

Numeric Numeric::operator-() const
{
    return {-value, unit};
}

Numeric Numeric::operator/(const Numeric& rhs) const
{
    if (rhs.value == 0) {
        THROWM(Base::RuntimeError, "Division by zero");
    }

    if (rhs.unit.empty() || unit.empty()) {
        return {value / rhs.value, unit};
    }

    ensureEqualUnits(rhs);
    return {value / rhs.value, unit};
}

Numeric Numeric::operator*(const Numeric& rhs) const
{
    if (rhs.unit.empty() || unit.empty()) {
        return {value * rhs.value, unit};
    }

    ensureEqualUnits(rhs);
    return {value * rhs.value, unit};
}

void Numeric::ensureEqualUnits(const Numeric& rhs) const
{
    if (unit != rhs.unit) {
        THROWM(
            Base::RuntimeError,
            fmt::format("Units mismatch left expression is '{}', right expression is '{}'", unit, rhs.unit)
        );
    }
}

std::string Value::toString() const
{
    if (holds<Numeric>()) {
        auto [value, unit] = get<Numeric>();
        return fmt::format("{}{}", value, unit);
    }

    if (holds<Base::Color>()) {
        auto color = get<Base::Color>();
        return fmt::format("#{:0>6x}", color.getPackedRGB() >> 8);  // NOLINT(*-magic-numbers)
    }

    if (holds<Tuple>()) {
        const auto& tuple = get<Tuple>();

        std::vector<std::string> parts;
        parts.reserve(tuple.elements.size());

        for (const auto& [name, value] : tuple.elements) {
            std::string string;

            if (name) {
                string = fmt::format("{}: {}", *name, value->toString());
            }
            else {
                string = value->toString();
            }

            parts.push_back(std::move(string));
        }

        return fmt::format("({})", fmt::join(parts, ", "));
    }

    return get<std::string>();
}

const Value& Tuple::at(size_t index) const
{
    if (index >= elements.size()) {
        THROWM(
            Base::RuntimeError,
            fmt::format("Tuple index {} out of range (size {})", index, elements.size())
        );
    }
    return *elements[index].value;
}

const Value* Tuple::find(const std::string& name) const
{
    auto it = std::ranges::find_if(elements, [&](const Element& elem) {
        return elem.name && *elem.name == name;
    });

    return it != elements.end() ? it->value.get() : nullptr;
}

size_t Tuple::size() const
{
    return elements.size();
}

ArgumentParser::ArgumentParser(std::initializer_list<ParamDef> params)
    : params_(params)
{}

Tuple ArgumentParser::resolve(const Tuple& args) const
{
    // Slots for resolved values, one per declared parameter
    std::vector<std::shared_ptr<const Value>> slots(params_.size());

    // 1. Match named arguments to their declared parameter
    for (const auto& elem : args.elements) {
        if (!elem.name) {
            continue;
        }

        auto it = std::ranges::find_if(params_, [&](const ParamDef& p) {
            return p.name == *elem.name;
        });

        if (it == params_.end()) {
            THROWM(Base::ExpressionError, fmt::format("Unknown argument '{}'", *elem.name));
        }

        auto index = static_cast<size_t>(std::distance(params_.begin(), it));

        if (slots[index]) {
            THROWM(Base::ExpressionError, fmt::format("Duplicate argument '{}'", *elem.name));
        }

        slots[index] = elem.value;
    }

    // 2. Fill remaining slots with unnamed arguments in order
    auto unnamed = args.elements
        | std::views::filter([](const Tuple::Element& e) { return !e.name.has_value(); });
    auto unnamedIt = unnamed.begin();

    for (size_t i = 0; i < params_.size(); ++i) {
        if (slots[i]) {
            continue;  // already claimed by name
        }

        if (unnamedIt != unnamed.end()) {
            slots[i] = unnamedIt->value;
            ++unnamedIt;
        }
        else if (params_[i].defaultValue) {
            slots[i] = std::make_shared<const Value>(*params_[i].defaultValue);
        }
        else {
            THROWM(
                Base::ExpressionError,
                fmt::format("Missing required argument '{}'", params_[i].name)
            );
        }
    }

    // 3. Check for excess positional arguments
    if (unnamedIt != unnamed.end()) {
        THROWM(
            Base::ExpressionError,
            fmt::format("Too many arguments: expected {}, got {}", params_.size(), args.size())
        );
    }

    // 4. Build result Tuple with all elements named per signature
    Tuple result;
    for (size_t i = 0; i < params_.size(); ++i) {
        result.elements.emplace_back(params_[i].name, std::move(slots[i]));
    }

    return result;
}

}  // namespace Gui::StyleParameters
