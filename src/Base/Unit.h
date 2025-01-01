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

#ifndef BASE_Unit_H
#define BASE_Unit_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <FCGlobal.h>

namespace Base
{

constexpr auto unitNumVals {8};
using UnitVals = std::array<int8_t, unitNumVals>;
using NameVal = std::pair<int8_t, std::string_view>;

constexpr std::array<std::string_view, unitNumVals>
    valNames {"mm", "kg", "s", "A", "K", "mol", "cd", "deg"};
constexpr auto unitValueLimit {8};

class BaseExport Unit final
{
public:
    Unit() = default;

    explicit constexpr Unit(const UnitVals vals, const std::string_view name = "")
        : _vals {vals}
        , _name {name}
    {
        checkRange();
    }

    bool operator==(const Unit&) const;
    bool operator!=(const Unit& that) const;
    Unit& operator*=(const Unit& that);
    Unit& operator/=(const Unit& that);
    Unit operator*(const Unit&) const;
    Unit operator/(const Unit&) const;

    [[nodiscard]] Unit pow(const double exp) const;
    [[nodiscard]] Unit root(const uint8_t num) const;

    [[nodiscard]] UnitVals vals() const;
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
    UnitVals _vals {};
    std::string_view _name;

    constexpr void checkRange()
    {
        for (const auto val : _vals) {
            if (val >= unitValueLimit || val < -unitValueLimit) {
                throw std::out_of_range("Unit exponent out of range");
            }
        }
    }

    /** Returns posIndexes, NegIndexes*/
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


struct UnitSpec
{
    std::string_view name;
    UnitVals vals;
};

constexpr auto numUnitSpecs {58};
constexpr std::array<std::pair<std::string_view, UnitVals>, numUnitSpecs> unitSpecs {{
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
}};  // clang-format on

inline bool isUnitName(const std::string_view str)
{
    auto match = [&](const std::pair<std::string_view, UnitVals>& pair) {
        return pair.first == str;
    };

    const auto found = std::find_if(unitSpecs.begin(), unitSpecs.end(), match);
    return found != unitSpecs.end();
}

inline std::string getString(const UnitVals vals)
{
    auto match = [&](const std::pair<std::string_view, UnitVals>& pair) {
        return pair.second == vals;
    };

    const auto found = std::find_if(unitSpecs.begin(), unitSpecs.end(), match);
    return std::string(found == unitSpecs.end() ? "" : found->first);
}

}  // namespace Base

#endif  // BASE_Unit_H
