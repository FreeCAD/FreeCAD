/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef UNITS_H
#define UNITS_H

#include <algorithm>
#include <stdexcept>
#include "Unit.h"

namespace Base::Units
{


struct UnitSpec
{
    std::string_view name;
    UnitVals vals;
};

constexpr auto numUnitSpecs {57};
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
    { "NullUnit"                              , {  0,  0,  0,  0,  0,  0,  0,  0 } },
    { "AmountOfSubstance"                     , {  0,  0,  0,  0,  0,  1         } },
    { "ElectricCurrent"                       , {  0,  0,  0,  1                 } },
    { "Length"                                , {  1                             } },
    { "LuminousIntensity"                     , {  0,  0,  0,  0,  0,  0,  1     } },
    { "Mass"                                  , {  0,  1                         } },
    { "Temperature"                           , {  0,  0,  0,  0,  1             } },
    { "TimeSpan"                              , {  0,  0,  1                     } },
    { "Acceleration"                          , {  1,  0, -2                     } },
    { "Angle"                                 , {  0,  0,  0,  0,  0,  0,  0,  1 } },
    { "AngleOfFriction"                       , {  0,  0,  0,  0,  0,  0,  0,  1 } },
    { "Area"                                  , {  2                             } },
    { "CompressiveStrength"                   , { -1,  1, -2                     } },
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
    { "ThermalConductivity"                   , {  1,  1, -3,  0, -1             } },
    { "ThermalExpansionCoefficient"           , {  0,  0,  0,  0, -1             } },
    { "ThermalTransferCoefficient"            , {  0,  1, -3,  0, -1             } },
    { "UltimateTensileStrength"               , { -1,  1, -2                     } },
    { "VacuumPermittivity"                    , { -3, -1,  4,  2                 } },
    { "Velocity"                              , {  1,  0, -1                     } },
    { "Volume"                                , {  3                             } },
    { "VolumeFlowRate"                        , {  3,  0, -1                     } },
    { "VolumetricThermalExpansionCoefficient" , {  0,  0,  0,  0, -1             } },
    { "Work"                                  , {  2,  1, -2                     } },
    { "YieldStrength"                         , { -1,  1, -2                     } },
    { "YoungsModulus"                         , { -1,  1, -2                     } },
}};  // clang-format on

constexpr Unit makeFromSpec(const std::pair<std::string_view, UnitVals>& unitSpec)
{
    return Unit {unitSpec.second, unitSpec.first};
}

constexpr Unit make(const std::string_view name = "NullUnit")
{
    for (const auto& spec : unitSpecs) {  // algorithms not constexpr until C++20
        if (spec.first == name) {
            return makeFromSpec(spec);
        }
    }
    throw std::invalid_argument("Invalid unit name");
}

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

// clang-format off
constexpr Unit NullUnit                              = make("NullUnit"                    );
constexpr Unit AmountOfSubstance                     = make("AmountOfSubstance"           );
constexpr Unit ElectricCurrent                       = make("ElectricCurrent"             );
constexpr Unit Length                                = make("Length"                      );
constexpr Unit LuminousIntensity                     = make("LuminousIntensity"           );
constexpr Unit Mass                                  = make("Mass"                        );
constexpr Unit Temperature                           = make("Temperature"                 );
constexpr Unit TimeSpan                              = make("TimeSpan"                    );
constexpr Unit Acceleration                          = make("Acceleration"                );
constexpr Unit Angle                                 = make("Angle"                       );
constexpr Unit AngleOfFriction                       = make("Angle"                       );
constexpr Unit Area                                  = make("Area"                        );
constexpr Unit CompressiveStrength                   = make("CompressiveStrength"         );
constexpr Unit CurrentDensity                        = make("CurrentDensity"              );
constexpr Unit Density                               = make("Density"                     );
constexpr Unit DissipationRate                       = make("DissipationRate"             );
constexpr Unit DynamicViscosity                      = make("DynamicViscosity"            );
constexpr Unit ElectricalCapacitance                 = make("ElectricalCapacitance"       );
constexpr Unit ElectricalConductance                 = make("ElectricalConductance"       );
constexpr Unit ElectricalConductivity                = make("ElectricalConductivity"      );
constexpr Unit ElectricalInductance                  = make("ElectricalInductance"        );
constexpr Unit ElectricalResistance                  = make("ElectricalResistance"        );
constexpr Unit ElectricCharge                        = make("ElectricCharge"              );
constexpr Unit ElectricPotential                     = make("ElectricPotential"           );
constexpr Unit ElectromagneticPotential              = make("ElectromagneticPotential"    );
constexpr Unit Force                                 = make("Force"                       );
constexpr Unit Frequency                             = make("Frequency"                   );
constexpr Unit HeatFlux                              = make("HeatFlux"                    );
constexpr Unit InverseArea                           = make("InverseArea"                 );
constexpr Unit InverseLength                         = make("InverseLength"               );
constexpr Unit InverseVolume                         = make("InverseVolume"               );
constexpr Unit KinematicViscosity                    = make("KinematicViscosity"          );
constexpr Unit MagneticFieldStrength                 = make("Magnetization"               );
constexpr Unit MagneticFlux                          = make("MagneticFlux"                );
constexpr Unit MagneticFluxDensity                   = make("MagneticFluxDensity"         );
constexpr Unit Magnetization                         = make("MagneticFieldStrength"       );
constexpr Unit Moment                                = make("Moment"                      );
constexpr Unit Pressure                              = make("Pressure"                    );
constexpr Unit Power                                 = make("Power"                       );
constexpr Unit ShearModulus                          = make("Pressure"                    );
constexpr Unit SpecificEnergy                        = make("SpecificEnergy"              );
constexpr Unit SpecificHeat                          = make("SpecificHeat"                );
constexpr Unit Stiffness                             = make("Stiffness"                   );
constexpr Unit StiffnessDensity                      = make("StiffnessDensity"            );
constexpr Unit Stress                                = make("Pressure"                    );
constexpr Unit ThermalConductivity                   = make("ThermalConductivity"         );
constexpr Unit ThermalExpansionCoefficient           = make("ThermalExpansionCoefficient" );
constexpr Unit ThermalTransferCoefficient            = make("ThermalTransferCoefficient"  );
constexpr Unit UltimateTensileStrength               = make("Pressure"                    );
constexpr Unit VacuumPermittivity                    = make("VacuumPermittivity"          );
constexpr Unit Velocity                              = make("Velocity"                    );
constexpr Unit Volume                                = make("Volume"                      );
constexpr Unit VolumeFlowRate                        = make("VolumeFlowRate"              );
constexpr Unit VolumetricThermalExpansionCoefficient = make("ThermalExpansionCoefficient" );
constexpr Unit Work                                  = make("Work"                        );
constexpr Unit YieldStrength                         = make("Pressure"                    );
constexpr Unit YoungsModulus                         = make("Pressure"                    );
// clang-format on


}  // namespace Base::Units
#endif  // UNITS_H
