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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>
#endif

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "Exception.h"
#include "Unit.h"

using namespace Base;

bool Unit::operator==(const Unit& that) const
{
    return _vals == that._vals;
}

bool Unit::operator!=(const Unit& that) const
{
    return _vals != that._vals;
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

    UnitVals res {};
    std::transform(_vals.begin(), _vals.end(), right._vals.begin(), res.begin(), mult);

    return Unit {res};
}

Unit Unit::operator/(const Unit& right) const
{
    auto div = [&](auto leftExponent, auto rightExponent) mutable {
        return leftExponent - rightExponent;
    };

    UnitVals res {};
    std::transform(_vals.begin(), _vals.end(), right._vals.begin(), res.begin(), div);

    return Unit {res};
}

Unit Unit::root(const uint8_t num) const
{
    auto apply = [&](auto val) {
        if (val % num != 0) {
            throw UnitsMismatchError("unit values must be divisible by root");
        }
        return static_cast<int8_t>(val / num);
    };

    if (num < 1) {
        throw UnitsMismatchError("root must be > 0");
    }

    UnitVals res {};
    std::transform(_vals.begin(), _vals.end(), res.begin(), apply);

    return Unit {res};
}

Unit Unit::pow(const double exp) const
{
    auto apply = [&](const auto val) {
        const auto num {val * exp};
        if (std::fabs(std::round(num) - num) >= std::numeric_limits<double>::epsilon()) {
            throw UnitsMismatchError("pow() of unit not possible");
        }

        return static_cast<int>(val * exp);
    };

    UnitVals res {};
    std::transform(_vals.begin(), _vals.end(), res.begin(), apply);

    return Unit {res};
}

UnitVals Unit::vals() const
{
    return _vals;
}

int Unit::length() const
{
    return _vals[0];
}

std::string Unit::getString() const
{
    auto buildSubStr = [&](auto index) {
        const std::string unitStrString {valNames.at(index)};
        const auto absol {abs(_vals.at(index))};

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
    auto inParen = fmt::format("Unit: {} ({})", getString(), fmt::join(_vals, ","));
    return _name.empty() ? inParen : fmt::format("{} [{}]", inParen, _name);
}

std::string Unit::getTypeString() const
{
    if (_name.empty()) {
        return Base::getString(_vals);
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

    std::for_each(_vals.begin(), _vals.end(), posNeg);

    return {pos, neg};
}


constexpr Unit make(const std::string_view name)
{
    for (const auto& spec : unitSpecs) {  // algorithms not constexpr until C++20
        if (spec.first == name) {
            return Unit {spec.second, spec.first};
        }
    }
    throw std::invalid_argument("Invalid unit name");
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
