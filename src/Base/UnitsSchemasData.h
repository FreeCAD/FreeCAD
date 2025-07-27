/************************************************************************
 *                                                                      *
 *   This file is part of the FreeCAD CAx development system.           *
 *                                                                      *
 *   This library is free software; you can redistribute it and/or      *
 *   modify it under the terms of the GNU Library General Public        *
 *   License as published by the Free Software Foundation; either       *
 *   version 2 of the License, or (at your option) any later version.   *
 *                                                                      *
 *   This library  is distributed in the hope that it will be useful,   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.               *
 *                                                                      *
 *   You should have received a copy of the GNU Library General Public  *
 *   License along with this library; see the file COPYING.LIB. If not, *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,      *
 *   Suite 330, Boston, MA  02111-1307, USA                             *
 *                                                                      *
 ************************************************************************/

#ifndef BASE_UNITSSCHEMASDATA_H
#define BASE_UNITSSCHEMASDATA_H

#include <map>
#include <vector>

#include <QtGlobal>

#include "fmt/format.h"
#include "fmt/ranges.h"

#include "UnitsSchemasSpecs.h"

/**
 * UnitSchemas raw data
 */

namespace Base::UnitsSchemasData
{

constexpr std::size_t defDecimals {2};
constexpr std::size_t defDenominator {8};

// NOLINTBEGIN
// clang-format off
inline const UnitsSchemaSpec s0
{ 6, "MmMin", "mm" , false, false , QT_TRANSLATE_NOOP("UnitsApi", "Metric small parts & CNC (mm, mm/min)"), false,
    {
        { "Length",   {{ 0 , "mm"     , 1.0        }}},
        { "Angle",    {{ 0 , "°"      , 1.0        }}},
        { "Velocity", {{ 0 , "mm/min" , 1.0 / 60.0 }}}
    }
};

inline const UnitsSchemaSpec s1
{ 9, "MeterDecimal", "m", false, false, QT_TRANSLATE_NOOP("UnitsApi", "Meter decimal (m, m², m³)"), false,
    {
        { "Length",             {{ 0 , "m"    , 1e3 }}},
        { "Area",               {{ 0 , "m^2"  , 1e6 }}},
        { "Volume",             {{ 0 , "m^3"  , 1e9 }}},
        { "Power",              {{ 0 , "W"    , 1e6 }}},
        { "ElectricPotential",  {{ 0 , "V"    , 1e6 }}},
        { "HeatFlux",           {{ 0 , "W/m^2", 1.0 }}},
        { "Velocity",           {{ 0 , "m/s"  , 1e3 }}}
    }
};

inline const UnitsSchemaSpec s2
{ 3, "ImperialDecimal", "in", false, false, QT_TRANSLATE_NOOP("UnitsApi", "Imperial decimal (in, lb)"), false,
    {
        { "Length",       {{ 0 , "in"      , 25.4              }}},
        { "Angle",        {{ 0 , "°"       , 1.0               }}},
        { "Area",         {{ 0 , "in^2"    , 645.16            }}},
        { "Volume",       {{ 0 , "in^3"    , 16387.064         }}},
        { "Mass",         {{ 0 , "lb"      , 0.45359237        }}},
        { "Pressure",     {{ 0 , "psi"     , 6.894744825494    }}},
        { "Stiffness",    {{ 0 , "lbf/in"  , 4.448222 / 0.0254 }}},
        { "Velocity",     {{ 0 , "in/min"  , 25.4 / 60         }}},
        { "Acceleration", {{ 0 , "in/min^2", 25.4 / 3600       }}}
    }
};

inline const UnitsSchemaSpec s3
{ 0, "Internal", "mm", false, false, QT_TRANSLATE_NOOP("UnitsApi", "Standard (mm, kg, s, °)"), true,
    {
        { "Length", {
            { 1e-6            , "mm"         , 1.0             },
            { 1e-3            , "nm"         , 1e-6            },
            { 1e-1            , "\xC2\xB5m"  , 1e-3            },
            { 1e4             , "mm"         , 1.0             },
            { 1e7             , "m"          , 1e3             },
            { 1e10            , "km"         , 1e6             },
            { 0               , "m"          , 1.0             }}
        },
        { "Area", {
            { 1e2             , "mm^2"       , 1.0             },
            { 1e6             , "cm^2"       , 1e2             },
            { 1e12            , "m^2"        , 1e6             },
            { 0               , "km^2"       , 1e12            }}
        },
        { "Volume", {
            { 1e3             , "mm^3"       , 1.0             },
            { 1e6             , "ml"         , 1e3             },
            { 1e9             , "l"          , 1e6             },
            { 0               , "m^3"        , 1e9             }}
        },
        { "Angle", {
            { 0               , "°"          , 1.0             }}
        },
        { "Mass", {
            { 1e-6            , "\xC2\xB5g"  , 1.0             },
            { 1e-3            , "mg"         , 1e-6            },
            { 1.0             , "g"          , 1e-3            },
            { 1e3             , "kg"         , 1.0             },
            { 0               , "t"          , 1e3             }}
        },
        { "Density", {
            { 1e-4            , "kg/m^3"     , 1e-9            },
            { 1.0             , "kg/cm^3"    , 1e-3            },
            { 0               , "kg/mm^3"    , 1.0             }}
        },
        { "ThermalConductivity", {
            { 0               , "W/m/K"      , 1e3             },
            { 1e6             , "W/mm/K"     , 1e6             }}
        },
        { "ThermalExpansionCoefficient", {
            { 1e-3            , "\xC2\xB5m/m/K" , 1e-6         },
            { 0               , "mm/mm/K"    , 1.0             }}
        },
        { "VolumetricThermalExpansionCoefficient", {
            { 1e-3            , "mm^3/m^3/K" , 1e-9            },
            { 0               , "m^3/m^3/K"  , 1.0             }}
        },
        { "SpecificHeat", {
            { 0               , "J/kg/K"     , 1e6             }}
        },
        { "ThermalTransferCoefficient", {
            { 0               , "W/m^2/K"    , 1.0             }}
        },
        { "Pressure", {
            { 10.0            , "Pa"         , 1e-3            },
            { 1e4             , "kPa"        , 1.0             },
            { 1e7             , "MPa"        , 1e3             },
            { 1e10            , "GPa"        , 1e6             },
            { 0               , "Pa"         , 1e-3            }}
        },
        { "Stress", {
            { 10.0            , "Pa"         , 1e-3            },
            { 1e4             , "kPa"        , 1.0             },
            { 1e7             , "MPa"        , 1e3             },
            { 1e10            , "GPa"        , 1e6             },
            { 0               , "Pa"         , 1e-3            }}
        },
        { "Stiffness", {
            { 1               , "mN/m"       , 1e-3            },
            { 1e3             , "N/m"        , 1.0             },
            { 1e6             , "kN/m"       , 1e3             },
            { 0               , "MN/m"       , 1e6             }}
        },
        { "StiffnessDensity", {
            { 1e-3            , "Pa/m"       , 1e-6            },
            { 1               , "kPa/m"      , 1e-3            },
            { 1e3             , "MPa/m"      , 1.0             },
            { 0               , "GPa/m"      , 1e3             }}
        },
        { "Force", {
            { 1e3             , "mN"         , 1.0             },
            { 1e6             , "N"          , 1e3             },
            { 1e9             , "kN"         , 1e6             },
            { 0               , "MN"         , 1e9             }}
        },
        { "Power", {
            { 1e6             , "mW"         , 1e3             },
            { 1e9             , "W"          , 1e6             },
            { 0               , "kW"         , 1e9             }}
        },
        { "ElectricPotential", {
            { 1e6             , "mV"         , 1e3             },
            { 1e9             , "V"          , 1e6             },
            { 1e12            , "kV"         , 1e9             },
            { 0               , "V"          , 1e6             }}
        },
        { "Work", {
            { 1.602176634e-10 , "eV"         , 1.602176634e-13 },
            { 1.602176634e-7  , "keV"        , 1.602176634e-10 },
            { 1.602176634e-4  , "MeV"        , 1.602176634e-7  },
            { 1e6             , "mJ"         , 1e3             },
            { 1e9             , "J"          , 1e6             },
            { 1e12            , "kJ"         , 1e9             },
            { 3.6e+15         , "kWh"        , 3.6e+12         },
            { 0               , "J"          , 1e6             }}
        },
        { "SpecificEnergy", {
            { 0               , "m^2/s^2"    , 1e6             }}
        },
        { "HeatFlux", {
            { 0               , "W/m^2"      , 1.0             }}
        },
        { "ElectricCharge", {
            { 0               , "C"          , 1.0             }}
        },
        { "SurfaceChargeDensity", {
            { 1e-4            , "C/m^2"      , 1e-6            },
            { 1e-2            , "C/cm^2"     , 1e-2            },
            { 0               , "C/mm^2"     , 1.0             }}
        },
        { "VolumeChargeDensity", {
            { 1e-4            , "C/m^3"      , 1e-9            },
            { 1e-2            , "C/cm^3"     , 1e-3            },
            { 0               , "C/mm^3"     , 1.0             }}
        },
        { "CurrentDensity", {
            { 1e-4            , "A/m^2"      , 1e-6            },
            { 1e-2            , "A/cm^2"     , 1e-2            },
            { 0               , "A/mm^2"     , 1               }}
        },
        { "MagneticFluxDensity", {
            { 1e-3            , "G"          , 1e-4            },
            { 0               , "T"          , 1.0             }}
        },
        { "MagneticFieldStrength", {
            { 0               , "A/m"        , 1e-3            }}
        },
        { "MagneticFlux", {
            { 0               , "Wb"         , 1e6             }}
        },
        { "Magnetization", {
            { 0               , "A/m"        , 1e-3            }}
        },
        { "ElectricalConductance", {
            { 1e-9            , "\xC2\xB5S"  , 1e-12           },
            { 1e-6            , "mS"         , 1e-9            },
            { 0               , "S"          , 1e-6            }}
        },
        { "ElectricalResistance", {
            { 1e9             , "Ohm"        , 1e6             },
            { 1e12            , "kOhm"       , 1e9             },
            { 0               , "MOhm"       , 1e12            }}
        },
        { "ElectricalConductivity", {
            { 0               , "MS/m"       , 1e-3            },
            { 1e-3            , "mS/m"       , 1e-12           },
            { 1.0             , "S/m"        , 1e-9            },
            { 1e3             , "kS/m"       , 1e-6            }}
        },
        { "ElectricalCapacitance", {
            { 1e-15           , "pF"         , 1e-18           },
            { 1e-12           , "nF"         , 1e-15           },
            { 1e-9            , "\xC2\xB5""F", 1e-12           },
            { 1e-6            , "mF"         , 1e-9            },
            { 0               , "F"          , 1e-6            }}
        },
        { "ElectricalInductance", {
            { 1.0             , "nH"         , 1e-3            },
            { 1e3             , "\xC2\xB5H"  , 1.0             },
            { 1e6             , "mH"         , 1e3             },
            { 0               , "H"          , 1e6             }}
        },
        { "VacuumPermittivity", {
            { 0               , "F/m"        , 1e-9            }}
        },
        { "Frequency", {
            { 1e3             , "Hz"         , 1.0             },
            { 1e6             , "kHz"        , 1e3             },
            { 1e9             , "MHz"        , 1e6             },
            { 1e12            , "GHz"        , 1e9             },
            { 0               , "THz"        , 1e12            }}
        },
        { "Velocity", {
            { 0               , "mm/s"       , 1.0             }}
        },
        { "DynamicViscosity", {
            { 0               , "Pa*s"       , 1e-3            }}
        },
        { "KinematicViscosity", {
            { 1e3             , "mm^2/s"     , 1.0             },
            { 0               , "m^2/s"      , 1e6             }}
        },
        { "VolumeFlowRate", {
            { 1e3             , "mm^3/s"     , 1.0             },
            { 1e6             , "ml/s"       , 1e3             },
            { 1e9             , "l/s"        , 1e6             },
            { 0               , "m^3/s"      , 1e9             }}
        },
        { "DissipationRate", {
            { 0               , "W/kg"       , 1e6             }}
        },
        { "InverseLength", {
            { 1e-6            , "1/m"        , 1e-3            },
            { 1e-3            , "1/km"       , 1e-6            },
            { 1.0             , "1/m"        , 1e-3            },
            { 1e3             , "1/mm"       , 1.0             },
            { 1e6             , "1/\xC2\xB5m", 1e3             },
            { 1e9             , "1/nm"       , 1e6             },
            { 0               , "1/m"        , 1e-3            }}
        },
        { "InverseArea", {
            { 1e-12           , "1/m^2"      , 1e-6            },
            { 1e-6            , "1/km^2"     , 1e-12           },
            { 1.0             , "1/m^2"      , 1e-6            },
            { 1e2             , "1/cm^2"     , 1e-2            },
            { 0               , "1/mm^2"     , 1.0             }}
        },
        { "InverseVolume", {
            { 1e-6            , "1/m^3"      , 1e-9            },
            { 1e-3            , "1/l"        , 1e-6            },
            { 1.0             , "1/ml"       , 1e-3            },
            { 0               , "1/mm^3"     , 1.0             }}
        }
    }
};

inline const UnitsSchemaSpec s4
{ 1, "MKS", "m", false, false, QT_TRANSLATE_NOOP("UnitsApi", "MKS (m, kg, s, °)") , false,
    {
        { "Length", {
            { 1e-6            , "mm"         , 1.0             },
            { 1e-3            , "nm"         , 1e-6            },
            { 0.1             , "\xC2\xB5m"  , 1e-3            },
            { 1e4             , "mm"         , 1.0             },
            { 1e7             , "m"          , 1e3             },
            { 1e10            , "km"         , 1e6             },
            { 0               , "m"          , 1e3             }}
        },
        { "Area", {
            { 100             , "mm^2"       , 1.0             },
            { 1e6             , "cm^2"       , 100             },
            { 1e12            , "m^2"        , 1e6             },
            { 0               , "km^2"       , 1e12            }}
        },
        { "Volume", {
            { 1e3             , "mm^3"       , 1.0             },
            { 1e6             , "ml"         , 1e3             },
            { 1e9             , "l"          , 1e6             },
            { 0               , "m^3"        , 1e9             }}
        },
        { "Mass", {
            { 1e-6            , "\xC2\xB5g"  , 1e-9            },
            { 1e-3            , "mg"         , 1e-6            },
            { 1.0             , "g"          , 1e-3            },
            { 1e3             , "kg"         , 1.0             },
            { 0               , "t"          , 1e3             }}
        },
        { "Density", {
            { 0.0001          , "kg/m^3"     , 0.000000001     },
            { 1.0             , "kg/cm^3"    , 0.001           },
            { 0               , "kg/mm^3"    , 1.0             }}
        },
        { "Acceleration", {
            { 0               , "m/s^2"      , 1000.0          }}
        },
        { "Pressure", {
            { 10.0             , "Pa"        , 0.001           },
            { 10'000.0         , "kPa"       , 1.0             },
            { 10'000'000.0     , "MPa"       , 1'000.0         },
            { 10'000'000'000.0 , "GPa"       , 1'000'000.0     },
            { 0                , "Pa"        , 1000.0          }}
        },
        { "Stress", {
            { 10.0             , "Pa"        , 0.001           },
            { 10'000.0         , "kPa"       , 1.0             },
            { 10'000'000.0     , "MPa"       , 1'000.0         },
            { 10'000'000'000.0 , "GPa"       , 1'000'000.0     },
            { 0                , "Pa"        , 0.001           }}
        },
        { "Stiffness", {
            { 1               , "mN/m"       , 1e-3            },
            { 1e3             , "N/m"        , 1.0             },
            { 1e6             , "kN/m"       , 1e3             },
            { 0               , "MN/m"       , 1e6             }}
        },
        { "StiffnessDensity", {
            { 1e-3            , "Pa/m"       , 1e-6            },
            { 1               , "kPa/m"      , 1e-3            },
            { 1e3             , "MPa/m"      , 1.0             },
            { 0               , "GPa/m"      , 1e3             }}
        },
        { "ThermalConductivity", {
            { 1'000'000       , "W/mm/K"     , 1'000'000.0     },
            { 0               , "W/m/K"      , 1'000.0         }}
        },
        { "ThermalExpansionCoefficient", {
            { 0.001           , "\xC2\xB5m/m/K" , 0.000001     },
            { 0               , "m/m/K"      , 1.0             }}
        },
        { "VolumetricThermalExpansionCoefficient", {
            { 0.001           , "mm^3/m^3/K" , 1e-9            },
            { 0               , "m^3/m^3/K"  , 1.0             }}
        },
        { "SpecificHeat", {
            { 0               , "J/kg/K"     , 1'000'000.0     }}
        },
        { "ThermalTransferCoefficient", {
            { 0               , "W/m^2/K"    , 1.0             }}
        },
        { "Force", {
            { 1e3             , "mN"         , 1.0             },
            { 1e6             , "N"          , 1e3             },
            { 1e9             , "kN"         , 1e6             },
            { 0               , "MN"         , 1e9             }}
        },
        { "Power", {
            { 1e6             , "mW"         , 1e3             },
            { 1e9             , "W"          , 1e6             },
            { 0               , "kW"         , 1e9             }}
        },
        { "ElectricPotential", {
            { 1e6             , "mV"         , 1e3             },
            { 1e9             , "V"          , 1e6             },
            { 1e12            , "kV"         , 1e9             },
            { 0               , "V"          , 1e6             }}
        },
        { "ElectricCharge", {
            { 0               , "C"          , 1.0             }}
        },
        { "SurfaceChargeDensity", {
            { 0               , "C/m^2"      , 1e-6            }}
        },
        { "VolumeChargeDensity", {
            { 0               , "C/m^3"      , 1e-9            }}
        },
        { "CurrentDensity", {
            { 1e3             , "A/m^2"      , 1e-6            },
            { 0               , "A/mm^2"     , 1.0             }}
        },
        { "MagneticFluxDensity", {
            { 1e-3            , "G"          , 1e-4            },
            { 0               , "T"          , 1.0             }}
        },
        { "MagneticFieldStrength", {
            { 0               , "A/m"        , 1e-3            }}
        },
        { "MagneticFlux", {
            { 0               , "Wb"         , 1e6             }}
        },
        { "Magnetization", {
            { 0               , "A/m"        , 1e-3            }}
        },
        { "ElectricalConductance", {
            { 1e-9            , "\xC2\xB5S"  , 1e-12           },
            { 1e-6            , "mS"         , 1e-9            },
            { 0               , "S"          , 1e-6            }}
        },
        { "ElectricalResistance", {
            { 1e9             , "Ohm"        , 1e6             },
            { 1e12            , "kOhm"       , 1e9             },
            { 0               , "MOhm"       , 1e12            }}
        },
        { "ElectricalConductivity", {
            { 1e-3            , "mS/m"       , 1e-12           },
            { 1.0             , "S/m"        , 1e-9            },
            { 1e3             , "kS/m"       , 1e-6            },
            { 0               , "MS/m"       , 1e-3            }}
        },
        { "ElectricalCapacitance", {
            { 1e-15           , "pF"         , 1e-18           },
            { 1e-12           , "nF"         , 1e-15           },
            { 1e-9            , "\xC2\xB5""F", 1e-12           },
            { 1e-6            , "mF"         , 1e-9            },
            { 0               , "F"          , 1e-6            }}
        },
        { "ElectricalInductance", {
            { 1e-6            , "nH"         , 1e-3            },
            { 1e-3            , "\xC2\xB5H"  , 1.0             },
            { 1.0             , "mH"         , 1e3             },
            { 0               , "H"          , 1e6             }}
        },
        { "VacuumPermittivity", {
            { 0               , "F/m"        , 1e-9            }}
        },
        { "Work", {
            { 1.602176634e-10 , "eV"         , 1.602176634e-13 },
            { 1.602176634e-7  , "keV"        , 1.602176634e-10 },
            { 1.602176634e-4  , "MeV"        , 1.602176634e-7  },
            { 1e6             , "mJ"         , 1e3             },
            { 1e9             , "J"          , 1e6             },
            { 1e12            , "kJ"         , 1e9             },
            { 3.6e+15         , "kWh"        , 3.6e+12         },
            { 0               , "J"          , 1e6             }}
        },
        { "SpecificEnergy", {
            { 0               , "m^2/s^2"    , 1000000         }}
        },
        { "HeatFlux", {
            { 0               , "W/m^2"      , 1.0             }}
        },
        { "Frequency", {
            { 1e3             , "Hz"         , 1.0             },
            { 1e6             , "kHz"        , 1e3             },
            { 1e9             , "MHz"        , 1e6             },
            { 1e12            , "GHz"        , 1e9             },
            { 0               , "THz"        , 1e12            }}
        },
        { "Velocity", {
            { 0               , "m/s"        , 1000.0          }}
        },
        { "DynamicViscosity", {
            { 0               , "Pa*s"       , 0.001           }}
        },
        { "KinematicViscosity", {
            { 0               , "m^2/s"      , 1e6             }}
        },
        { "VolumeFlowRate", {
            { 1e-3            , "m^3/s"      , 1e9             },
            { 1e3             , "mm^3/s"     , 1.0             },
            { 1e6             , "ml/s"       , 1e3             },
            { 1e9             , "l/s"        , 1e6             },
            { 0               , "m^3/s"      , 1e9             }}
        },
        { "DissipationRate", {
            { 0               , "W/kg"       , 1e6             }}
        },
        { "InverseLength", {
            { 1e-6            , "1/m"        , 1e-3             },
            { 1e-3            , "1/km"       , 1e-6             },
            { 1.0             , "1/m"        , 1e-3             },
            { 1e3             , "1/mm"       , 1.0              },
            { 1e6             , "1/\xC2\xB5m", 1e3              },
            { 1e9             , "1/nm"       , 1e6              },
            { 0               , "1/m"        , 1e-3             }}
        },
        { "InverseArea", {
            { 1e-12           , "1/m^2"      , 1e-6             },
            { 1e-6            , "1/km^2"     , 1e-12            },
            { 1.0             , "1/m^2"      , 1e-6             },
            { 1e2             , "1/cm^2"     , 1e-2             },
            { 0               , "1/mm^2"     , 1.0              }}
        },
        { "InverseVolume", {
            { 1e-6            , "1/m^3"      , 1e-9             },
            { 1e-3            , "1/l"        , 1e-6             },
            { 1.0             , "1/ml"       , 1e-3             },
            { 0               , "1/mm^3"     , 1.0              }}
        }
    }
};

inline const UnitsSchemaSpec s5
{ 4, "Centimeter", "cm", false, false, QT_TRANSLATE_NOOP("UnitsApi", "Building Euro (cm, m², m³)") , false,
    {
        { "Length", {
            { 0              , "cm"          , 10.0             }}
        },
        { "Area", {
            { 0              , "m^2"         , 1e6              }}
        },
        { "Volume", {
            { 0              , "m^3"         , 1e9              }}
        },
        { "Power", {
            { 0              , "W"           , 1e6              }}
        },
        { "ElectricPotential", {
            { 0              , "V"           , 1e6              }}
        },
        { "HeatFlux", {
            { 0              , "W/m^2"       , 1.0              }}
        },
        { "Velocity", {
            { 0              , "mm/min"      , 1.0 / 60         }}
        }
    }
};

inline const UnitsSchemaSpec s6
{ 8, "FEM", "mm", false , false , QT_TRANSLATE_NOOP("UnitsApi", "FEM (mm, N, s)"), false,
    {
        { "Length", {
            { 0             , "mm"           , 1.0               }}
        },
        { "Mass",   {
            { 0             , "t"            , 1e3               }}
        }
    }
};

inline const UnitsSchemaSpec s7
{ 2, "Imperial", "in", false, false, QT_TRANSLATE_NOOP("UnitsApi", "US customary (in, lb)"), false,
    {
        { "Length", {
            { 0.00000254      , "in"       , 25.4              },
            { 2.54            , "thou"     , 0.0254            },
            { 304.8           , "\""       , 25.4              },
            { 914.4           , "'"        , 304.8             },
            { 1'609'344.0     , "yd"       , 914.4             },
            { 1'609'344'000.0 , "mi"       , 1'609'344.0       },
            { 0               , "in"       , 25.4              }}
        },
        { "Angle", {
            { 0               , "°"        , 1.0               }}
        },
        { "Area", {
            { 0               , "in^2"     , 645.16            }}
        },
        { "Volume", {
            { 0               , "in^3"     , 16'387.064        }}
        },
        { "Mass", {
            { 0               , "lb"       , 0.45359237        }}
        },
        { "Pressure", {
            { 6'894.744       , "psi"      , 6.894744825494    },
            { 6'894'744.825   , "ksi"      , 6'894.744825494   },
            { 0               , "psi"      , 6.894744825494    }}
        },
        { "Stiffness", {
            { 0               , "lbf/in"   , 4.448222 / 0.0254 }}
        },
        { "Velocity", {
            { 0               , "in/min"   , 25.4 / 60         }}
        }
    }
};

inline const UnitsSchemaSpec s8
{ 5, "ImperialBuilding", "ft", true, false , QT_TRANSLATE_NOOP("UnitsApi", "Building US (ft-in, sqft, cft)"), false,
    {
        { "Length"   , {{ 0   , "toFractional"    , 0              }}},  // <== !
        { "Angle"    , {{ 0   , "°"               , 1.0            }}},
        { "Area"     , {{ 0   , "sqft"            , 92'903.04      }}},
        { "Volume"   , {{ 0   , "cft"             , 28'316'846.592 }}},
        { "Velocity" , {{ 0   , "in/min"          , 25.4 / 60      }}}
    }
};

inline const UnitsSchemaSpec s9
{ 7, "ImperialCivil", "ft", false, true, QT_TRANSLATE_NOOP("UnitsApi", "Imperial for Civil Eng (ft, ft/s)"), false,
    {
        { "Angle"    , {{ 0   , "toDMS"           , 0              }}}  // <== !
    }
};

// clang-format on
// NOLINTEND
inline const std::vector schemaSpecs {s3, s4, s5, s6, s7, s8, s9, s0, s1, s2};

/**
 * Special functions
 *
 * A schema unit can have custom formatting via a special function
 * Such functions must be included here and also registered in special functions caller (below)
 */

/** utility function for toFractional */
inline std::size_t greatestCommonDenominator(const std::size_t a, const std::size_t b)
{
    return b == 0 ? a : greatestCommonDenominator(b, a % b);  // Euclid's algorithm
}

/**
 * double -> [feet'] [inches" [+ fraction]"], e.g.: 3' 4" + 3/8"
 */
inline std::string toFractional(const double value)
{
    constexpr auto inchPerFoot {12};
    constexpr auto mmPerInch {25.4};

    auto numFractUnits =
        static_cast<std::size_t>(std::round(std::abs(value) / mmPerInch * defDenominator));
    if (numFractUnits == 0) {
        return "0";
    }

    const auto feet =
        static_cast<std::size_t>(std::floor(numFractUnits / (inchPerFoot * defDenominator)));
    numFractUnits -= inchPerFoot * defDenominator * feet;

    const auto inches = static_cast<std::size_t>(std::floor(numFractUnits / defDenominator));
    const std::size_t fractNumerator = numFractUnits - (defDenominator * inches);

    const std::size_t common_denom = greatestCommonDenominator(fractNumerator, defDenominator);
    const std::size_t numerator = fractNumerator / common_denom;
    const std::size_t denominator = defDenominator / common_denom;

    bool addSpace {false};
    std::string result;

    if (value < 0) {
        result += "-";
    }

    if (feet > 0) {
        result += fmt::format("{}'", feet);
        addSpace = true;
    }

    if (inches > 0) {
        result += fmt::format("{}{}\"", addSpace ? " " : "", inches);
        addSpace = false;
    }

    if (numerator > 0) {
        if (inches > 0) {
            result += fmt::format(" {} ", value < 0 ? "-" : "+");
            addSpace = false;
        }
        result += fmt::format("{}{}/{}\"", addSpace ? " " : "", numerator, denominator);
    }

    return result;
}

/**
 * double -> degrees°[minutes′[seconds″]]
 */
inline std::string toDms(const double value)
{
    constexpr auto dmsRatio {60.0};

    auto calc = [&](const double total) -> std::pair<int, double> {
        const double whole = std::floor(total);
        return {static_cast<int>(whole), dmsRatio * (total - whole)};
    };

    auto [degrees, totalMinutes] = calc(value);
    std::string out = fmt::format("{}°", degrees);

    if (totalMinutes > 0) {
        auto [minutes, totalSeconds] = calc(totalMinutes);
        out += fmt::format("{}′", minutes);

        if (totalSeconds > 0) {
            out += fmt::format("{}″", std::round(totalSeconds));
        }
    }

    return out;
}


/**
 * Special functions caller
 */

// clang-format off
inline const std::map<std::string, std::function<std::string(double, double&, std::string&)>> specials
{
    {
        { "toDMS"        , [](const double val, double& factor, std::string& unitString) {
            factor = 1.0;
            unitString = "deg";
            return toDms(val);
        }},
        { "toFractional" , [](const double val, double& factor, std::string& unitString) {
            factor = 25.4;
            unitString = "in";
            return toFractional(val);
        }}
    }
};  // clang-format on

inline std::string
runSpecial(const std::string& name, const double value, double& factor, std::string& unitString)
{
    return specials.contains(name) ? specials.at(name)(value, factor, unitString) : "";
}


/**
 * Build data pack
 */

inline const UnitsSchemasDataPack unitSchemasDataPack {schemaSpecs, defDecimals, defDenominator};


}  // namespace Base::UnitsSchemasData
#endif  // BASE_UNITSSCHEMASDATA_H
