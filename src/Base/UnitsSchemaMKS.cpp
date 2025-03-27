/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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


#include "PreCompiled.h"
#ifndef _PreComp_
#include <cmath>
#endif
#ifdef __GNUC__
#include <unistd.h>
#endif

#include "Quantity.h"
#include "Unit.h"
#include "UnitsSchemaMKS.h"

using namespace Base;

std::string
UnitsSchemaMKS::schemaTranslate(const Quantity& quant, double& factor, std::string& unitString)
{
    double UnitValue = std::abs(quant.getValue());
    Unit unit = quant.getUnit();

    // now do special treatment on all cases seems necessary:
    if (unit == Unit::Length) {  // Length handling ============================
        if (UnitValue < 1e-6) {  // smaller than 0.001 nm -> scientific notation
            unitString = "mm";
            factor = 1.0;
        }
        else if (UnitValue < 1e-3) {
            unitString = "nm";
            factor = 1e-6;
        }
        else if (UnitValue < 0.1) {
            unitString = "\xC2\xB5m";
            factor = 1e-3;
        }
        else if (UnitValue < 1e4) {
            unitString = "mm";
            factor = 1.0;
        }
        else if (UnitValue < 1e7) {
            unitString = "m";
            factor = 1e3;
        }
        else if (UnitValue < 1e10) {
            unitString = "km";
            factor = 1e6;
        }
        else {  // bigger than 1000 km -> scientific notation
            unitString = "m";
            factor = 1e3;
        }
    }
    else if (unit == Unit::Area) {
        if (UnitValue < 100) {
            unitString = "mm^2";
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = "cm^2";
            factor = 100;
        }
        else if (UnitValue < 1e12) {
            unitString = "m^2";
            factor = 1e6;
        }
        else {  // bigger than 1 square kilometer
            unitString = "km^2";
            factor = 1e12;
        }
    }
    else if (unit == Unit::Volume) {
        if (UnitValue < 1e3) {  // smaller than 1 ul
            unitString = "mm^3";
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = "ml";
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = "l";
            factor = 1e6;
        }
        else {  // bigger than 1000 l
            unitString = "m^3";
            factor = 1e9;
        }
    }
    else if (unit == Unit::Mass) {
        if (UnitValue < 1e-6) {
            unitString = "\xC2\xB5g";
            factor = 1e-9;
        }
        else if (UnitValue < 1e-3) {
            unitString = "mg";
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = "g";
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = "kg";
            factor = 1.0;
        }
        else {
            unitString = "t";
            factor = 1e3;
        }
    }
    else if (unit == Unit::Density) {
        if (UnitValue < 0.0001) {
            unitString = "kg/m^3";
            factor = 0.000000001;
        }
        else if (UnitValue < 1.0) {
            unitString = "kg/cm^3";
            factor = 0.001;
        }
        else {
            unitString = "kg/mm^3";
            factor = 1.0;
        }
    }
    else if (unit == Unit::Acceleration) {
        unitString = "m/s^2";
        factor = 1000.0;
    }
    else if ((unit == Unit::Pressure) || (unit == Unit::Stress)) {
        if (UnitValue < 10.0) {  // Pa is the smallest
            unitString = "Pa";
            factor = 0.001;
        }
        else if (UnitValue < 10000.0) {
            unitString = "kPa";
            factor = 1.0;
        }
        else if (UnitValue < 10000000.0) {
            unitString = "MPa";
            factor = 1000.0;
        }
        else if (UnitValue < 10000000000.0) {
            unitString = "GPa";
            factor = 1000000.0;
        }
        else {  // bigger then 1000 GPa -> scientific notation
            unitString = "Pa";
            factor = 0.001;
        }
    }
    else if ((unit == Unit::Stiffness)) {
        if (UnitValue < 1) {  // mN/m is the smallest
            unitString = "mN/m";
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = "N/m";
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = "kN/m";
            factor = 1e3;
        }
        else {
            unitString = "MN/m";
            factor = 1e6;
        }
    }
    else if ((unit == Unit::StiffnessDensity)) {
        if (UnitValue < 1e-3) {
            unitString = "Pa/m";
            factor = 1e-6;
        }
        else if (UnitValue < 1) {
            unitString = "kPa/m";
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = "MPa/m";
            factor = 1.0;
        }
        else {
            unitString = "GPa/m";
            factor = 1e3;
        }
    }
    else if (unit == Unit::ThermalConductivity) {
        if (UnitValue > 1000000) {
            unitString = "W/mm/K";
            factor = 1000000.0;
        }
        else {
            unitString = "W/m/K";
            factor = 1000.0;
        }
    }
    else if (unit == Unit::ThermalExpansionCoefficient) {
        if (UnitValue < 0.001) {
            unitString = "\xC2\xB5m/m/K";
            factor = 0.000001;
        }
        else {
            unitString = "m/m/K";
            factor = 1.0;
        }
    }
    else if (unit == Unit::VolumetricThermalExpansionCoefficient) {
        if (UnitValue < 0.001) {
            unitString = "mm^3/m^3/K";
            factor = 1e-9;
        }
        else {
            unitString = "m^3/m^3/K";
            factor = 1.0;
        }
    }
    else if (unit == Unit::SpecificHeat) {
        unitString = "J/kg/K";
        factor = 1000000.0;
    }
    else if (unit == Unit::ThermalTransferCoefficient) {
        unitString = "W/m^2/K";
        factor = 1.0;
    }
    else if (unit == Unit::Force) {
        if (UnitValue < 1e3) {
            unitString = "mN";
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = "N";
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = "kN";
            factor = 1e6;
        }
        else {
            unitString = "MN";
            factor = 1e9;
        }
    }
    //    else if (unit == Unit::Moment) {
    //        if (UnitValue < 1e6) {
    //            unitString = "mNm";
    //            factor = 1e3;
    //        }
    //        else if (UnitValue < 1e9) {
    //            unitString = "Nm";
    //            factor = 1e6;
    //        }
    //        else if (UnitValue < 1e12) {
    //            unitString = "kNm";
    //            factor = 1e9;
    //        }
    //        else {
    //            unitString = "MNm";
    //            factor = 1e12;
    //        }
    //    }
    else if (unit == Unit::Power) {
        if (UnitValue < 1e6) {
            unitString = "mW";
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = "W";
            factor = 1e6;
        }
        else {
            unitString = "kW";
            factor = 1e9;
        }
    }
    else if (unit == Unit::ElectricPotential) {
        if (UnitValue < 1e6) {
            unitString = "mV";
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = "V";
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = "kV";
            factor = 1e9;
        }
        else {  // > 1000 kV scientificc notation
            unitString = "V";
            factor = 1e6;
        }
    }
    else if (unit == Unit::ElectricCharge) {
        unitString = "C";
        factor = 1.0;
    }
    else if (unit == Unit::SurfaceChargeDensity) {
        unitString = "C/m^2";
        factor = 1e-6;
    }
    else if (unit == Unit::VolumeChargeDensity) {
        unitString = "C/m^3";
        factor = 1e-9;
    }
    else if (unit == Unit::CurrentDensity) {
        if (UnitValue <= 1e3) {
            unitString = "A/m^2";
            factor = 1e-6;
        }
        else {
            unitString = "A/mm^2";
            factor = 1;
        }
    }
    else if (unit == Unit::MagneticFluxDensity) {
        if (UnitValue <= 1e-3) {
            unitString = "G";
            factor = 1e-4;
        }
        else {
            unitString = "T";
            factor = 1.0;
        }
    }
    else if (unit == Unit::MagneticFieldStrength) {
        unitString = "A/m";
        factor = 1e-3;
    }
    else if (unit == Unit::MagneticFlux) {
        unitString = "Wb";
        factor = 1e6;
    }
    else if (unit == Unit::Magnetization) {
        unitString = "A/m";
        factor = 1e-3;
    }
    else if (unit == Unit::ElectromagneticPotential) {
        unitString = "Wb/m";
        factor = 1e3;
    }
    else if (unit == Unit::ElectricalConductance) {
        if (UnitValue < 1e-9) {
            unitString = "\xC2\xB5S";
            factor = 1e-12;
        }
        else if (UnitValue < 1e-6) {
            unitString = "mS";
            factor = 1e-9;
        }
        else {
            unitString = "S";
            factor = 1e-6;
        }
    }
    else if (unit == Unit::ElectricalResistance) {
        if (UnitValue < 1e9) {
            unitString = "Ohm";
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = "kOhm";
            factor = 1e9;
        }
        else {
            unitString = "MOhm";
            factor = 1e12;
        }
    }
    else if (unit == Unit::ElectricalConductivity) {
        if (UnitValue < 1e-3) {
            unitString = "mS/m";
            factor = 1e-12;
        }
        else if (UnitValue < 1.0) {
            unitString = "S/m";
            factor = 1e-9;
        }
        else if (UnitValue < 1e3) {
            unitString = "kS/m";
            factor = 1e-6;
        }
        else {
            unitString = "MS/m";
            factor = 1e-3;
        }
    }
    else if (unit == Unit::ElectricalCapacitance) {
        if (UnitValue < 1e-15) {
            unitString = "pF";
            factor = 1e-18;
        }
        else if (UnitValue < 1e-12) {
            unitString = "nF";
            factor = 1e-15;
        }
        else if (UnitValue < 1e-9) {
            // \x reads everything to the end, therefore split
            unitString = "\xC2\xB5"
                         "F";
            factor = 1e-12;
        }
        else if (UnitValue < 1e-6) {
            unitString = "mF";
            factor = 1e-9;
        }
        else {
            unitString = "F";
            factor = 1e-6;
        }
    }
    else if (unit == Unit::ElectricalInductance) {
        if (UnitValue < 1e-6) {
            unitString = "nH";
            factor = 1e-3;
        }
        else if (UnitValue < 1e-3) {
            unitString = "\xC2\xB5H";
            factor = 1.0;
        }
        else if (UnitValue < 1.0) {
            unitString = "mH";
            factor = 1e3;
        }
        else {
            unitString = "H";
            factor = 1e6;
        }
    }
    else if (unit == Unit::VacuumPermittivity) {
        unitString = "F/m";
        factor = 1e-9;
    }
    else if (unit == Unit::Work) {
        if (UnitValue < 1.602176634e-10) {
            unitString = "eV";
            factor = 1.602176634e-13;
        }
        else if (UnitValue < 1.602176634e-7) {
            unitString = "keV";
            factor = 1.602176634e-10;
        }
        else if (UnitValue < 1.602176634e-4) {
            unitString = "MeV";
            factor = 1.602176634e-7;
        }
        else if (UnitValue < 1e6) {
            unitString = "mJ";
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = "J";
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = "kJ";
            factor = 1e9;
        }
        else if (UnitValue < 3.6e+15) {
            unitString = "kWh";
            factor = 3.6e+12;
        }
        else {  // bigger than 1000 kWh -> scientific notation
            unitString = "J";
            factor = 1e6;
        }
    }
    else if (unit == Unit::SpecificEnergy) {
        unitString = "m^2/s^2";
        factor = 1000000;
    }
    else if (unit == Unit::HeatFlux) {
        unitString = "W/m^2";
        factor = 1.0;
    }
    else if (unit == Unit::Frequency) {
        if (UnitValue < 1e3) {
            unitString = "Hz";
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = "kHz";
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = "MHz";
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = "GHz";
            factor = 1e9;
        }
        else {
            unitString = "THz";
            factor = 1e12;
        }
    }
    else if (unit == Unit::Velocity) {
        unitString = "m/s";
        factor = 1000.0;
    }
    else if (unit == Unit::DynamicViscosity) {
        unitString = "Pa*s";
        factor = 0.001;
    }
    else if (unit == Unit::KinematicViscosity) {
        unitString = "m^2/s";
        factor = 1e6;
    }
    else if (unit == Unit::VolumeFlowRate) {
        if (UnitValue < 1e-3) {  // smaller than 0.001 mm^3/s -> scientific notation
            unitString = "m^3/s";
            factor = 1e9;
        }
        else if (UnitValue < 1e3) {
            unitString = "mm^3/s";
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = "ml/s";
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = "l/s";
            factor = 1e6;
        }
        else {
            unitString = "m^3/s";
            factor = 1e9;
        }
    }
    else if (unit == Unit::DissipationRate) {
        unitString = "W/kg";
        factor = 1e6;
    }
    else if (unit == Unit::InverseLength) {
        if (UnitValue < 1e-6) {  // smaller than 0.001 1/km -> scientific notation
            unitString = "1/m";
            factor = 1e-3;
        }
        else if (UnitValue < 1e-3) {
            unitString = "1/km";
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = "1/m";
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = "1/mm";
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = "1/\xC2\xB5m";
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = "1/nm";
            factor = 1e6;
        }
        else {  // larger -> scientific notation
            unitString = "1/m";
            factor = 1e-3;
        }
    }
    else if (unit == Unit::InverseArea) {
        if (UnitValue < 1e-12) {  // smaller than 0.001 1/km^2 -> scientific notation
            unitString = "1/m^2";
            factor = 1e-6;
        }
        else if (UnitValue < 1e-6) {
            unitString = "1/km^2";
            factor = 1e-12;
        }
        else if (UnitValue < 1.0) {
            unitString = "1/m^2";
            factor = 1e-6;
        }
        else if (UnitValue < 1e2) {
            unitString = "1/cm^2";
            factor = 1e-2;
        }
        else {
            unitString = "1/mm^2";
            factor = 1.0;
        }
    }
    else if (unit == Unit::InverseVolume) {
        if (UnitValue < 1e-6) {
            unitString = "1/m^3";
            factor = 1e-9;
        }
        else if (UnitValue < 1e-3) {
            unitString = "1/l";
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = "1/ml";
            factor = 1e-3;
        }
        else {
            unitString = "1/mm^3";
            factor = 1.0;
        }
    }
    else {
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}
