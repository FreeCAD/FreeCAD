// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <ranges>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "Unit.h"

using namespace Base;

struct UnitSpec
{
    std::string_view name;
    UnitExponents exps;
};

constexpr auto unitSpecs = std::to_array<UnitSpec>({
    // clang-format off
    //                                             Length
    //                                             .   Mass
    //                                             .   .   Time
    //                                             .   .   .   ElectricCurrent
    //                                             .   .   .   .   ThermodynamicTemperature
    //                                             .   .   .   .   .   AmountOfSubstance
    //                                             .   .   .   .   .   .   LuminousIntensity
    //                                             .   .   .   .   .   .   .   Angle
    { "1"                                     , {  0,  0,  0,  0,  0,  0,  0,  0 } },
    { "Length"                                , {  1                             } },
    { "Mass"                                  , {  0,  1                         } },
    { "TimeSpan"                              , {  0,  0,  1                     } },
    { "ElectricCurrent"                       , {  0,  0,  0,  1                 } },
    { "Temperature"                           , {  0,  0,  0,  0,  1             } },
    { "AmountOfSubstance"                     , {  0,  0,  0,  0,  0,  1         } },
    { "LuminousIntensity"                     , {  0,  0,  0,  0,  0,  0,  1     } },
    { "Angle"                                 , {  0,  0,  0,  0,  0,  0,  0,  1 } },
    { "Acceleration"                          , {  1,  0, -2                     } },
    { "AngleOfFriction"                       , {  0,  0,  0,  0,  0,  0,  0,  1 } },
    { "Area"                                  , {  2                             } },
    { "CurrentDensity"                        , { -2,  0,  0,  1                 } },
    { "Density"                               , { -3,  1                         } },
    { "DissipationRate"                       , {  2,  0, -3                     } },
    { "DynamicViscosity"                      , { -1,  1, -1                     } },
    { "ElectricalCapacitance"                 , { -2, -1,  4,  2                 } },
    { "ElectricalConductance"                 , { -2, -1,  3,  2                 } },
    { "ElectricalConductivity"                , { -3, -1,  3,  2                 } },
    { "ElectricalInductance"                  , {  2,  1, -2, -2                 } },
    { "ElectricalResistance"                  , {  2,  1, -3, -2                 } },
    { "ElectricCharge"                        , {  0,  0,  1,  1                 } },
    { "ElectricPotential"                     , {  2,  1, -3, -1                 } },
    { "ElectromagneticPotential"              , {  1,  1, -2, -1                 } },
    { "Force"                                 , {  1,  1, -2                     } },
    { "Frequency"                             , {  0,  0, -1                     } },
    { "HeatFlux"                              , {  0,  1, -3                     } },
    { "InverseArea"                           , { -2                             } },
    { "InverseLength"                         , { -1                             } },
    { "InverseVolume"                         , { -3                             } },
    { "KinematicViscosity"                    , {  2,  0, -1                     } },
    { "MagneticFieldStrength"                 , { -1,  0,  0,  1                 } },
    { "MagneticFlux"                          , {  2,  1, -2, -1                 } },
    { "MagneticFluxDensity"                   , {  0,  1, -2, -1                 } },
    { "Magnetization"                         , { -1,  0,  0,  1                 } },
    { "Moment"                                , {  2,  1, -2                     } },
    { "Pressure"                              , { -1,  1, -2                     } },
    { "Power"                                 , {  2,  1, -3                     } },
    { "ShearModulus"                          , { -1,  1, -2                     } },
    { "SpecificEnergy"                        , {  2,  0, -2                     } },
    { "SpecificHeat"                          , {  2,  0, -2,  0, -1             } },
    { "Stiffness"                             , {  0,  1, -2                     } },
    { "StiffnessDensity"                      , { -2,  1, -2                     } },
    { "Stress"                                , { -1,  1, -2                     } },
    { "SurfaceChargeDensity"                  , { -2,  0,  1,  1                 } },
    { "ThermalConductivity"                   , {  1,  1, -3,  0, -1             } },
    { "ThermalExpansionCoefficient"           , {  0,  0,  0,  0, -1             } },
    { "ThermalTransferCoefficient"            , {  0,  1, -3,  0, -1             } },
    { "UltimateTensileStrength"               , { -1,  1, -2                     } },
    { "VacuumPermittivity"                    , { -3, -1,  4,  2                 } },
    { "Velocity"                              , {  1,  0, -1                     } },
    { "Volume"                                , {  3                             } },
    { "VolumeChargeDensity"                   , { -3,  0,  1,  1                 } },
    { "VolumeFlowRate"                        , {  3,  0, -1                     } },
    { "VolumetricThermalExpansionCoefficient" , {  0,  0,  0,  0, -1             } },
    { "Work"                                  , {  2,  1, -2                     } },
    { "YieldStrength"                         , { -1,  1, -2                     } },
    { "YoungsModulus"                         , { -1,  1, -2                     } },
});  // clang-format on

Unit::Unit(const int length,  // NOLINT
           const int mass,
           const int time,
           const int electricCurrent,
           const int thermodynamicTemperature,
           const int amountOfSubstance,
           const int luminousIntensity,
           const int angle)
    : _name {""}
{
    auto cast = [](auto val) {
        return static_cast<int8_t>(std::clamp<decltype(val)>(val,
                                                             std::numeric_limits<int8_t>::min(),
                                                             std::numeric_limits<int8_t>::max()));
    };

    _exps[0] = cast(length);
    _exps[1] = cast(mass);
    _exps[2] = cast(time);
    _exps[3] = cast(electricCurrent);
    _exps[4] = cast(thermodynamicTemperature);
    _exps[5] = cast(amountOfSubstance);
    _exps[6] = cast(luminousIntensity);
    _exps[7] = cast(angle);

    checkRange();
}

bool Unit::operator==(const Unit& that) const
{
    return _exps == that._exps;
}

bool Unit::operator!=(const Unit& that) const
{
    return _exps != that._exps;
}

Unit& Unit::operator*=(const Unit& that)
{
    *this = *this * that;
    return *this;
}

Unit& Unit::operator/=(const Unit& that)
{
    *this = *this / that;
    return *this;
}

Unit Unit::operator*(const Unit& right) const
{
    auto mult = [&](auto leftExponent, auto rightExponent) {
        return leftExponent + rightExponent;
    };

    UnitExponents res {};
    std::transform(_exps.begin(), _exps.end(), right._exps.begin(), res.begin(), mult);

    return Unit {res};
}

Unit Unit::operator/(const Unit& right) const
{
    auto div = [&](auto leftExponent, auto rightExponent) {
        return leftExponent - rightExponent;
    };

    UnitExponents res {};
    std::transform(_exps.begin(), _exps.end(), right._exps.begin(), res.begin(), div);

    return Unit {res};
}

Unit Unit::root(const uint8_t num) const
{
    auto apply = [&](auto val) {
        if (val % num != 0) {
            throw UnitsMismatchError("unit values must be divisible by root");
        }
        return static_cast<decltype(val)>(val / num);
    };

    if (num < 1) {
        throw UnitsMismatchError("root must be > 0");
    }

    UnitExponents res {};
    std::transform(_exps.begin(), _exps.end(), res.begin(), apply);

    return Unit {res};
}

Unit Unit::pow(const double exp) const
{
    auto apply = [&](const auto val) {
        const auto num {val * exp};
        if (std::fabs(std::round(num) - num) >= std::numeric_limits<double>::epsilon()) {
            throw UnitsMismatchError("pow() of unit not possible");
        }

        return static_cast<decltype(val)>(val * exp);
    };

    UnitExponents res {};
    std::transform(_exps.begin(), _exps.end(), res.begin(), apply);

    return Unit {res};
}

UnitExponents Unit::exponents() const
{
    return _exps;
}

int Unit::length() const
{
    return _exps[0];
}

std::string Unit::getString() const
{
    auto buildSubStr = [&](auto index) {
        const std::string unitStrString {unitSymbols.at(index)};
        const auto absol {abs(_exps.at(index))};

        return absol <= 1 ? unitStrString
                          : fmt::format("{}^{}", unitStrString, std::to_string(absol));
    };

    auto buildStr = [&](auto indexes) {
        std::vector<std::string> subStrings {};
        std::transform(indexes.begin(), indexes.end(), std::back_inserter(subStrings), buildSubStr);

        return fmt::format("{}", fmt::join(subStrings, "*"));
    };

    //------------------------------------------------------------------------------

    auto [posValIndexes, negValIndexes] = nonZeroValsIndexes();
    auto numeratorStr = buildStr(posValIndexes);
    if (negValIndexes.empty()) {
        return numeratorStr;
    }

    auto denominatorStr = buildStr(negValIndexes);

    return fmt::format("{}/{}",
                       numeratorStr.empty() ? "1" : numeratorStr,
                       negValIndexes.size() > 1 ? fmt::format("({})", denominatorStr)
                                                : denominatorStr);
}

std::string Unit::representation() const
{
    auto name = getTypeString();
    auto inParen = fmt::format("Unit: {} ({})", getString(), fmt::join(_exps, ","));
    return name.empty() ? inParen : fmt::format("{} [{}]", inParen, name);
}

std::string Unit::getTypeString() const
{
    if (_name.empty()) {
        const auto spec = std::ranges::find(unitSpecs, _exps, &UnitSpec::exps);
        return std::string(spec == unitSpecs.end() ? "" : spec->name);
    }

    return std::string {_name.data(), _name.size()};
}

std::pair<std::vector<size_t>, std::vector<size_t>> Unit::nonZeroValsIndexes() const
{
    std::vector<size_t> pos {};
    std::vector<size_t> neg {};

    auto posNeg = [&, index {0}](auto val) mutable {
        if (val != 0) {
            (val > 0 ? pos : neg).push_back(index);
        }
        ++index;
    };

    std::ranges::for_each(_exps, posNeg);

    return {pos, neg};
}


constexpr Unit make(const std::string_view name)
{
    if (const auto spec = std::ranges::find(unitSpecs, name, &UnitSpec::name);
        spec != unitSpecs.end()) {
        return Unit {spec->exps, spec->name};
    }
    throw NameError("Invalid unit name");
}

// clang-format off
constexpr Unit Unit::One                                   = make("1"                           );

constexpr Unit Unit::Length                                = make("Length"                      );
constexpr Unit Unit::Mass                                  = make("Mass"                        );
constexpr Unit Unit::TimeSpan                              = make("TimeSpan"                    );
constexpr Unit Unit::ElectricCurrent                       = make("ElectricCurrent"             );
constexpr Unit Unit::Temperature                           = make("Temperature"                 );
constexpr Unit Unit::AmountOfSubstance                     = make("AmountOfSubstance"           );
constexpr Unit Unit::LuminousIntensity                     = make("LuminousIntensity"           );
constexpr Unit Unit::Angle                                 = make("Angle"                       );

constexpr Unit Unit::Acceleration                          = make("Acceleration"                );
constexpr Unit Unit::AngleOfFriction                       = make("Angle"                       );
constexpr Unit Unit::Area                                  = make("Area"                        );
constexpr Unit Unit::CompressiveStrength                   = make("Pressure"                    );
constexpr Unit Unit::CurrentDensity                        = make("CurrentDensity"              );
constexpr Unit Unit::Density                               = make("Density"                     );
constexpr Unit Unit::DissipationRate                       = make("DissipationRate"             );
constexpr Unit Unit::DynamicViscosity                      = make("DynamicViscosity"            );
constexpr Unit Unit::ElectricalCapacitance                 = make("ElectricalCapacitance"       );
constexpr Unit Unit::ElectricalConductance                 = make("ElectricalConductance"       );
constexpr Unit Unit::ElectricalConductivity                = make("ElectricalConductivity"      );
constexpr Unit Unit::ElectricalInductance                  = make("ElectricalInductance"        );
constexpr Unit Unit::ElectricalResistance                  = make("ElectricalResistance"        );
constexpr Unit Unit::ElectricCharge                        = make("ElectricCharge"              );
constexpr Unit Unit::ElectricPotential                     = make("ElectricPotential"           );
constexpr Unit Unit::ElectromagneticPotential              = make("ElectromagneticPotential"    );
constexpr Unit Unit::Force                                 = make("Force"                       );
constexpr Unit Unit::Frequency                             = make("Frequency"                   );
constexpr Unit Unit::HeatFlux                              = make("HeatFlux"                    );
constexpr Unit Unit::InverseArea                           = make("InverseArea"                 );
constexpr Unit Unit::InverseLength                         = make("InverseLength"               );
constexpr Unit Unit::InverseVolume                         = make("InverseVolume"               );
constexpr Unit Unit::KinematicViscosity                    = make("KinematicViscosity"          );
constexpr Unit Unit::MagneticFieldStrength                 = make("Magnetization"               );
constexpr Unit Unit::MagneticFlux                          = make("MagneticFlux"                );
constexpr Unit Unit::MagneticFluxDensity                   = make("MagneticFluxDensity"         );
constexpr Unit Unit::Magnetization                         = make("MagneticFieldStrength"       );
constexpr Unit Unit::Moment                                = make("Moment"                      );
constexpr Unit Unit::Pressure                              = make("Pressure"                    );
constexpr Unit Unit::Power                                 = make("Power"                       );
constexpr Unit Unit::ShearModulus                          = make("Pressure"                    );
constexpr Unit Unit::SpecificEnergy                        = make("SpecificEnergy"              );
constexpr Unit Unit::SpecificHeat                          = make("SpecificHeat"                );
constexpr Unit Unit::Stiffness                             = make("Stiffness"                   );
constexpr Unit Unit::StiffnessDensity                      = make("StiffnessDensity"            );
constexpr Unit Unit::Stress                                = make("Pressure"                    );
constexpr Unit Unit::SurfaceChargeDensity                  = make("SurfaceChargeDensity"        );
constexpr Unit Unit::ThermalConductivity                   = make("ThermalConductivity"         );
constexpr Unit Unit::ThermalExpansionCoefficient           = make("ThermalExpansionCoefficient" );
constexpr Unit Unit::ThermalTransferCoefficient            = make("ThermalTransferCoefficient"  );
constexpr Unit Unit::UltimateTensileStrength               = make("Pressure"                    );
constexpr Unit Unit::VacuumPermittivity                    = make("VacuumPermittivity"          );
constexpr Unit Unit::Velocity                              = make("Velocity"                    );
constexpr Unit Unit::Volume                                = make("Volume"                      );
constexpr Unit Unit::VolumeChargeDensity                   = make("VolumeChargeDensity"         );
constexpr Unit Unit::VolumeFlowRate                        = make("VolumeFlowRate"              );
constexpr Unit Unit::VolumetricThermalExpansionCoefficient = make("ThermalExpansionCoefficient" );
constexpr Unit Unit::Work                                  = make("Work"                        );
constexpr Unit Unit::YieldStrength                         = make("Pressure"                    );
constexpr Unit Unit::YoungsModulus                         = make("Pressure"                    );
