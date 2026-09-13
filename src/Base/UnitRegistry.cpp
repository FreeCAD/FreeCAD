// SPDX-License-Identifier: LGPL-2.1-or-later

#include "UnitRegistry.h"

#include "Exception.h"
#include "Quantity.h"
#include "UnitsConvData.h"

#include <array>
#include <numbers>
#include <unordered_map>

namespace Base
{
namespace
{
enum Prefix : unsigned
{
    Pico = 1U << 0,
    Nano = 1U << 1,
    Micro = 1U << 2,
    Milli = 1U << 3,
    Centi = 1U << 4,
    Deci = 1U << 5,
    Kilo = 1U << 6,
    Mega = 1U << 7,
    Giga = 1U << 8,
    Tera = 1U << 9,
};

struct PrefixDefinition
{
    std::string_view symbol;
    double factor;
    unsigned flag;
};

// Longest spellings must be checked first.
constexpr std::array prefixes {
    PrefixDefinition {"\xC2\xB5", 1e-6, Micro},
    PrefixDefinition {"p", 1e-12, Pico},
    PrefixDefinition {"n", 1e-9, Nano},
    PrefixDefinition {"u", 1e-6, Micro},
    PrefixDefinition {"m", 1e-3, Milli},
    PrefixDefinition {"c", 1e-2, Centi},
    PrefixDefinition {"d", 1e-1, Deci},
    PrefixDefinition {"k", 1e3, Kilo},
    PrefixDefinition {"M", 1e6, Mega},
    PrefixDefinition {"G", 1e9, Giga},
    PrefixDefinition {"T", 1e12, Tera},
};

struct UnitDefinition
{
    Quantity quantity;
    unsigned prefixes {};
};

using Registry = std::unordered_map<std::string_view, UnitDefinition>;

const Registry& units()
{
    using namespace UnitsConvData;
    constexpr auto micro = Micro;
    static const Registry registry {
        {"m", {Quantity(1e3, Unit::Length), Nano | micro | Milli | Centi | Deci | Kilo}},
        {"l", {Quantity(1e6, Unit::Volume), Milli}},
        {"Hz", {Quantity(1, Unit::Frequency), Kilo | Mega | Giga | Tera}},
        {"g", {Quantity(1e-3, Unit::Mass), micro | Milli | Kilo}},
        {"t", {Quantity(1e3, Unit::Mass)}},
        {"s", {Quantity(1, Unit::TimeSpan)}},
        {"min", {Quantity(60, Unit::TimeSpan)}},
        {"h", {Quantity(3600, Unit::TimeSpan)}},
        {"A", {Quantity(1, Unit::ElectricCurrent), Nano | micro | Milli | Kilo | Mega}},
        {"K", {Quantity(1, Unit::Temperature), micro | Milli}},
        {"mol", {Quantity(1, Unit::AmountOfSubstance), Nano | micro | Milli}},
        {"cd", {Quantity(1, Unit::LuminousIntensity)}},
        {"in", {Quantity(in, Unit::Length)}},
        {"ft", {Quantity(ft, Unit::Length)}},
        {"thou", {Quantity(in / 1000, Unit::Length)}},
        {"yd", {Quantity(yd, Unit::Length)}},
        {"mi", {Quantity(mi, Unit::Length)}},
        {"mph", {Quantity(mi / 3600, Unit::Velocity)}},
        {"km/h", {Quantity(1e6 / 3600, Unit::Velocity)}},
        {"sqft", {Quantity(ft * ft, Unit::Area)}},
        {"cft", {Quantity(ft * ft * ft, Unit::Volume)}},
        {"lb", {Quantity(lb, Unit::Mass)}},
        {"oz", {Quantity(lb / 16, Unit::Mass)}},
        {"st", {Quantity(lb * 14, Unit::Mass)}},
        {"cwt", {Quantity(lb * 112, Unit::Mass)}},
        {"lbf", {Quantity(1000 * lbf, Unit::Force)}},
        {"N", {Quantity(1000, Unit::Force), Milli | Kilo | Mega}},
        {"N/m", {Quantity(1, Unit::Stiffness), Milli | Kilo | Mega}},
        {"Pa", {Quantity(0.001, Unit::Pressure), Kilo | Mega | Giga}},
        {"bar", {Quantity(100, Unit::Pressure), Milli}},
        {"Torr", {Quantity(101.325 / 760, Unit::Pressure), micro | Milli}},
        {"psi", {Quantity(psi, Unit::Pressure)}},
        {"ksi", {Quantity(psi * 1000, Unit::Pressure)}},
        {"Mpsi", {Quantity(psi * 1e6, Unit::Pressure)}},
        {"W", {Quantity(1e6, Unit::Power), Nano | micro | Milli | Kilo}},
        {"VA", {Quantity(1e6, Unit::Power)}},
        {"V", {Quantity(1e6, Unit::ElectricPotential), Milli | Kilo}},
        {"S", {Quantity(1e-6, Unit::ElectricalConductance), micro | Milli | Kilo | Mega}},
        {"Ohm", {Quantity(1e6, Unit::ElectricalResistance), Kilo | Mega}},
        {"C", {Quantity(1, Unit::ElectricCharge)}},
        {"T", {Quantity(1, Unit::MagneticFluxDensity), Milli}},
        {"G", {Quantity(1e-4, Unit::MagneticFluxDensity)}},
        {"Wb", {Quantity(1e6, Unit::MagneticFlux)}},
        {"F", {Quantity(1e-6, Unit::ElectricalCapacitance), Pico | Nano | micro | Milli}},
        {"H", {Quantity(1e6, Unit::ElectricalInductance), Nano | micro | Milli}},
        {"J", {Quantity(1e6, Unit::Work), Milli | Kilo}},
        {"Nm", {Quantity(1e6, Unit::Moment)}},
        {"VAs", {Quantity(1e6, Unit::Work)}},
        {"Ws", {Quantity(1e6, Unit::Work)}},
        {"kWh", {Quantity(3.6e12, Unit::Work)}},
        {"eV", {Quantity(1.602176634e-13, Unit::Work), Kilo | Mega}},
        {"cal", {Quantity(4.1868e6, Unit::Work), Kilo}},
        {"deg", {Quantity(1, Unit::Angle)}},
        {"rad", {Quantity(180 / std::numbers::pi, Unit::Angle)}},
        {"gon", {Quantity(360.0 / 400, Unit::Angle)}},
        {"M", {Quantity(1.0 / 60, Unit::Angle)}},
        {"AS", {Quantity(1.0 / 3600, Unit::Angle)}},
    };
    return registry;
}

const std::unordered_map<std::string_view, std::string_view>& aliases()
{
    static const std::unordered_map<std::string_view, std::string_view> registry {
        {"\"", "in"},
        {"'", "ft"},
        {"mil", "thou"},
        {"lbm", "lb"},
        {"CV", "Ws"},
        {"\xC2\xB0", "deg"},
        {"\xE2\x80\xB2", "M"},
        {"\xE2\x80\xB3", "AS"},
    };
    return registry;
}
}  // namespace

std::optional<Quantity> UnitRegistry::lookup(std::string_view symbol)
{
    if (const auto exact = units().find(symbol); exact != units().end()) {
        return exact->second.quantity;
    }
    if (const auto alias = aliases().find(symbol); alias != aliases().end()) {
        return units().at(alias->second).quantity;
    }

    for (const auto& prefix : prefixes) {
        if (!symbol.starts_with(prefix.symbol)) {
            continue;
        }
        const auto baseSymbol = symbol.substr(prefix.symbol.size());
        const auto base = units().find(baseSymbol);
        if (base != units().end() && (base->second.prefixes & prefix.flag) != 0) {
            return base->second.quantity * prefix.factor;
        }
    }
    return std::nullopt;
}

Quantity UnitRegistry::require(std::string_view symbol)
{
    if (auto quantity = lookup(symbol)) {
        return *quantity;
    }
    throw ParserError("Unknown unit: " + std::string(symbol));
}
}  // namespace Base
