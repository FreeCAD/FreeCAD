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

#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <FCGlobal.h>

#include "Exception.h"

namespace Base
{

constexpr auto unitSymbols = std::to_array<std::string_view>(
    {"mm", "kg", "s", "A", "K", "mol", "cd", "deg"}
);

constexpr auto unitNumExponents {unitSymbols.size()};
using UnitExponents = std::array<int8_t, unitNumExponents>;

constexpr auto unitExponentLimit {8};

class BaseExport Unit final
{
public:
    Unit() = default;

    explicit constexpr Unit(const UnitExponents exps, const std::string_view name = "")
        : _exps {exps}
        , _name {name}
    {
        checkRange();
    }

    /// helper constructor to ease Unit construction from Python
    explicit Unit(
        const int length,
        const int mass = 0,
        const int time = 0,
        const int electricCurrent = 0,
        const int thermodynamicTemperature = 0,
        const int amountOfSubstance = 0,
        const int luminousIntensity = 0,
        const int angle = 0
    );

    bool operator==(const Unit&) const;
    bool operator!=(const Unit& that) const;
    Unit& operator*=(const Unit& that);
    Unit& operator/=(const Unit& that);
    Unit operator*(const Unit&) const;
    Unit operator/(const Unit&) const;

    [[nodiscard]] Unit pow(const double exp) const;
    [[nodiscard]] Unit root(const uint8_t num) const;

    [[nodiscard]] UnitExponents exponents() const;
    [[nodiscard]] int length() const;

    [[nodiscard]] std::string getString() const;       // E.g. kg, mm^2, mm*kg/s^2
    [[nodiscard]] std::string getTypeString() const;   // E.g. "Area", "Length", "Pressure"
    [[nodiscard]] std::string representation() const;  // E.g. "Unit: mm (1,0,0,0,0,0,0,0) [Length]"

    Unit sqrt() const
    {
        return root(2);
    }
    Unit cbrt() const
    {
        return root(3);
    }

private:
    UnitExponents _exps {};
    std::string_view _name;

    constexpr void checkRange()
    {
        for (const auto exp : _exps) {
            if (exp >= unitExponentLimit) {
                throw OverflowError("Unit exponent overflow");
            }
            if (exp < -unitExponentLimit) {
                throw UnderflowError("Unit exponent underflow");
            }
        }
    }

    /** Returns posIndexes, negIndexes*/
    std::pair<std::vector<size_t>, std::vector<size_t>> nonZeroValsIndexes() const;

public:
    static const Unit Acceleration;
    static const Unit AmountOfSubstance;
    static const Unit Angle;
    static const Unit AngleOfFriction;
    static const Unit Area;
    static const Unit CompressiveStrength;
    static const Unit CurrentDensity;
    static const Unit Density;
    static const Unit DissipationRate;
    static const Unit DynamicViscosity;
    static const Unit ElectricalCapacitance;
    static const Unit ElectricalConductance;
    static const Unit ElectricalConductivity;
    static const Unit ElectricalInductance;
    static const Unit ElectricalResistance;
    static const Unit ElectricCharge;
    static const Unit ElectricCurrent;
    static const Unit ElectricPotential;
    static const Unit ElectromagneticPotential;
    static const Unit Force;
    static const Unit Frequency;
    static const Unit HeatFlux;
    static const Unit InverseArea;
    static const Unit InverseLength;
    static const Unit InverseVolume;
    static const Unit KinematicViscosity;
    static const Unit Length;
    static const Unit LuminousIntensity;
    static const Unit MagneticFieldStrength;
    static const Unit MagneticFlux;
    static const Unit MagneticFluxDensity;
    static const Unit Magnetization;
    static const Unit Mass;
    static const Unit Moment;
    static const Unit One;
    static const Unit Pressure;
    static const Unit Power;
    static const Unit ShearModulus;
    static const Unit SpecificEnergy;
    static const Unit SpecificHeat;
    static const Unit Stiffness;
    static const Unit StiffnessDensity;
    static const Unit Stress;
    static const Unit SurfaceChargeDensity;
    static const Unit Temperature;
    static const Unit TimeSpan;
    static const Unit ThermalConductivity;
    static const Unit ThermalExpansionCoefficient;
    static const Unit ThermalTransferCoefficient;
    static const Unit UltimateTensileStrength;
    static const Unit VacuumPermittivity;
    static const Unit Velocity;
    static const Unit Volume;
    static const Unit VolumeChargeDensity;
    static const Unit VolumeFlowRate;
    static const Unit VolumetricThermalExpansionCoefficient;
    static const Unit Work;
    static const Unit YieldStrength;
    static const Unit YoungsModulus;
};

}  // namespace Base
