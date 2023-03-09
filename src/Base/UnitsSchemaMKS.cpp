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
#ifdef __GNUC__
# include <unistd.h>
#endif

#include <QString>

#include "UnitsSchemaMKS.h"
#include <cmath>


using namespace Base;


QString UnitsSchemaMKS::schemaTranslate(const Quantity &quant, double &factor, QString &unitString)
{
    double UnitValue = std::abs(quant.getValue());
    Unit unit = quant.getUnit();

    // now do special treatment on all cases seems necessary:
    if (unit == Unit::Length) {// Length handling ============================
        if (UnitValue < 1e-6) {// smaller than 0.001 nm -> scientific notation
            unitString = QString::fromLatin1("mm");
            factor = 1.0;
        }
        else if (UnitValue < 1e-3) {
            unitString = QString::fromLatin1("nm");
            factor = 1e-6;
        }
        else if (UnitValue < 0.1) {
            unitString = QString::fromUtf8("\xC2\xB5m");
            factor = 1e-3;
        }
        else if (UnitValue < 1e4) {
            unitString = QString::fromLatin1("mm");
            factor = 1.0;
        }
        else if (UnitValue < 1e7) {
            unitString = QString::fromLatin1("m");
            factor = 1e3;
        }
        else if (UnitValue < 1e10) {
            unitString = QString::fromLatin1("km");
            factor = 1e6;
        }
        else {// bigger than 1000 km -> scientific notation
            unitString = QString::fromLatin1("m");
            factor = 1e3;
        }
    }
    else if (unit == Unit::Area) {
        if (UnitValue < 100) {
            unitString = QString::fromLatin1("mm^2");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("cm^2");
            factor = 100;
        }
        else if (UnitValue < 1e12) {
            unitString = QString::fromLatin1("m^2");
            factor = 1e6;
        }
        else { // bigger than 1 square kilometer
            unitString = QString::fromLatin1("km^2");
            factor = 1e12;
        }
    }
    else if (unit == Unit::Volume) {
        if (UnitValue < 1e3) {// smaller than 1 ul
            unitString = QString::fromLatin1("mm^3");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("ml");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("l");
            factor = 1e6;
        }
        else { // bigger than 1000 l
            unitString = QString::fromLatin1("m^3");
            factor = 1e9;
        }
    }
    else if (unit == Unit::Mass) {
        if (UnitValue < 1e-6) {
            unitString = QString::fromUtf8("\xC2\xB5g");
            factor = 1e-9;
        }
        else if (UnitValue < 1e-3) {
            unitString = QString::fromLatin1("mg");
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = QString::fromLatin1("g");
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = QString::fromLatin1("kg");
            factor = 1.0;
        }
        else {
            unitString = QString::fromLatin1("t");
            factor = 1e3;
        }
    }
    else if (unit == Unit::Density) {
        if (UnitValue < 0.0001) {
            unitString = QString::fromLatin1("kg/m^3");
            factor = 0.000000001;
        }
        else if (UnitValue < 1.0) {
            unitString = QString::fromLatin1("kg/cm^3");
            factor = 0.001;
        }
        else {
            unitString = QString::fromLatin1("kg/mm^3");
            factor = 1.0;
        }
    }
    else if (unit == Unit::Acceleration) {
        unitString = QString::fromLatin1("m/s^2");
        factor = 1000.0;
    }
    else if ((unit == Unit::Pressure) || (unit == Unit::Stress)) {
        if (UnitValue < 10.0) {// Pa is the smallest
            unitString = QString::fromLatin1("Pa");
            factor = 0.001;
        }
        else if (UnitValue < 10000.0) {
            unitString = QString::fromLatin1("kPa");
            factor = 1.0;
        }
        else if (UnitValue < 10000000.0) {
            unitString = QString::fromLatin1("MPa");
            factor = 1000.0;
        }
        else if (UnitValue < 10000000000.0) {
            unitString = QString::fromLatin1("GPa");
            factor = 1000000.0;
        }
        else { // bigger then 1000 GPa -> scientific notation
            unitString = QString::fromLatin1("Pa");
            factor = 0.001;
        }
    }
    else if ((unit == Unit::Stiffness)) {
        if (UnitValue < 1){// mN/m is the smallest
            unitString = QString::fromLatin1("mN/m");
            factor = 1e-3;
        }
        if (UnitValue < 1e3) {
            unitString = QString::fromLatin1("N/m");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("kN/m");
            factor = 1e3;
        }
        else {
            unitString = QString::fromLatin1("MN/m");
            factor = 1e6;
        }
    }
    else if (unit == Unit::ThermalConductivity) {
        if (UnitValue > 1000000) {
            unitString = QString::fromLatin1("W/mm/K");
            factor = 1000000.0;
        }
        else {
            unitString = QString::fromLatin1("W/m/K");
            factor = 1000.0;
        }
    }
    else if (unit == Unit::ThermalExpansionCoefficient) {
        if (UnitValue < 0.001) {
            unitString = QString::fromUtf8("\xC2\xB5m/m/K");
            factor = 0.000001;
        }
        else {
            unitString = QString::fromLatin1("m/m/K");
            factor = 1.0;
        }
    }
    else if (unit == Unit::VolumetricThermalExpansionCoefficient) {
        if (UnitValue < 0.001) {
            unitString = QString::fromUtf8("mm^3/m^3/K");
            factor = 1e-9;
        }
        else {
            unitString = QString::fromLatin1("m^3/m^3/K");
            factor = 1.0;
        }
    }
    else if (unit == Unit::SpecificHeat) {
        unitString = QString::fromLatin1("J/kg/K");
        factor = 1000000.0;
    }
    else if (unit == Unit::ThermalTransferCoefficient) {
        unitString = QString::fromLatin1("W/m^2/K");
        factor = 1.0;
    }
    else if (unit == Unit::Force) {
        if (UnitValue < 1e3) {
            unitString = QString::fromLatin1("mN");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("N");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("kN");
            factor = 1e6;
        }
        else {
            unitString = QString::fromLatin1("MN");
            factor = 1e9;
        }
    }
    else if (unit == Unit::Power) {
        if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("mW");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("W");
            factor = 1e6;
        }
        else {
            unitString = QString::fromLatin1("kW");
            factor = 1e9;
        }
    }
    else if (unit == Unit::ElectricPotential) {
        if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("mV");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("V");
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = QString::fromLatin1("kV");
            factor = 1e9;
        }
        else { // > 1000 kV scientificc notation
            unitString = QString::fromLatin1("V");
            factor = 1e6;
        }
    }
    else if (unit == Unit::ElectricCharge) {
        unitString = QString::fromLatin1("C");
        factor = 1.0;
    }
    else if (unit == Unit::CurrentDensity) {
        if (UnitValue <= 1e3) {
            unitString = QString::fromLatin1("A/m^2");
            factor = 1e-6;
        }
        else {
            unitString = QString::fromLatin1("A/mm^2");
            factor = 1;
        }
    }
    else if (unit == Unit::MagneticFluxDensity) {
        if (UnitValue <= 1e-3) {
            unitString = QString::fromLatin1("G");
            factor = 1e-4;
        }
        else {
            unitString = QString::fromLatin1("T");
            factor = 1.0;
        }
    }
    else if (unit == Unit::MagneticFieldStrength) {
        unitString = QString::fromLatin1("A/m");
        factor = 1e-3;
    }
    else if (unit == Unit::MagneticFlux) {
        unitString = QString::fromLatin1("Wb");
        factor = 1e6;
    }
    else if (unit == Unit::Magnetization) {
        unitString = QString::fromLatin1("A/m");
        factor = 1e-3;
    } 
    else if (unit == Unit::ElectricalConductance) {
        if (UnitValue < 1e-9) {
            unitString = QString::fromUtf8("\xC2\xB5S");
            factor = 1e-12;
        }
        else if (UnitValue < 1e-6) {
            unitString = QString::fromLatin1("mS");
            factor = 1e-9;
        }
        else {
            unitString = QString::fromLatin1("S");
            factor = 1e-6;
        }
    }
    else if (unit == Unit::ElectricalResistance) {
        if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("Ohm");
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = QString::fromLatin1("kOhm");
            factor = 1e9;
        }
        else {
            unitString = QString::fromLatin1("MOhm");
            factor = 1e12;
        }
    }
    else if (unit == Unit::ElectricalConductivity) {
        if (UnitValue < 1e-3) {
            unitString = QString::fromLatin1("mS/m");
            factor = 1e-12;
        }
        else if (UnitValue < 1.0) {
            unitString = QString::fromLatin1("S/m");
            factor = 1e-9;
        }
        else if (UnitValue < 1e3) {
            unitString = QString::fromLatin1("kS/m");
            factor = 1e-6;
        }
        else {
            unitString = QString::fromLatin1("MS/m");
            factor = 1e-3;
        }
    }
    else if (unit == Unit::ElectricalCapacitance) {
        if (UnitValue < 1e-15) {
            unitString = QString::fromLatin1("pF");
            factor = 1e-18;
        }
        else if (UnitValue < 1e-12) {
            unitString = QString::fromLatin1("nF");
            factor = 1e-15;
        }
        else if (UnitValue < 1e-9) {
            // \x reads everything to the end, therefore split
            unitString = QString::fromUtf8("\xC2\xB5""F");
            factor = 1e-12;
        }
        else if (UnitValue < 1e-6) {
            unitString = QString::fromLatin1("mF");
            factor = 1e-9;
        }
        else {
            unitString = QString::fromLatin1("F");
            factor = 1e-6;
        }
    }
    else if (unit == Unit::ElectricalInductance) {
        if (UnitValue < 1e-6) {
            unitString = QString::fromLatin1("nH");
            factor = 1e-3;
        }
        else if (UnitValue < 1e-3) {
            unitString = QString::fromUtf8("\xC2\xB5H");
            factor = 1.0;
        }
        else if (UnitValue < 1.0) {
            unitString = QString::fromLatin1("mH");
            factor = 1e3;
        }
        else {
            unitString = QString::fromLatin1("H");
            factor = 1e6;
        }
    }
    else if (unit == Unit::VacuumPermittivity) {
            unitString = QString::fromLatin1("F/m");
            factor = 1e-9;
    }
    else if (unit == Unit::Work) {
        if (UnitValue < 1.602176634e-10) {
            unitString = QString::fromLatin1("eV");
            factor = 1.602176634e-13;
        }
        else if (UnitValue < 1.602176634e-7) {
            unitString = QString::fromLatin1("keV");
            factor = 1.602176634e-10;
        }
        else if (UnitValue < 1.602176634e-4) {
            unitString = QString::fromLatin1("MeV");
            factor = 1.602176634e-7;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("mJ");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("J");
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = QString::fromLatin1("kJ");
            factor = 1e9;
        }
        else if (UnitValue < 3.6e+15) {
            unitString = QString::fromLatin1("kWh");
            factor = 3.6e+12;
        }
        else { // bigger than 1000 kWh -> scientific notation
            unitString = QString::fromLatin1("J");
            factor = 1e6;
        }
    }
    else if (unit == Unit::SpecificEnergy) {
        unitString = QString::fromLatin1("m^2/s^2");
        factor = 1000000;
    }
    else if (unit == Unit::HeatFlux) {
        unitString = QString::fromLatin1("W/m^2");
        factor = 1.0;
    }
    else if (unit == Unit::Frequency) {
        if (UnitValue < 1e3) {
            unitString = QString::fromLatin1("Hz");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("kHz");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("MHz");
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = QString::fromLatin1("GHz");
            factor = 1e9;
        }
        else {
            unitString = QString::fromLatin1("THz");
            factor = 1e12;
        }
    }
    else if (unit == Unit::Velocity) {
        unitString = QString::fromLatin1("m/s");
        factor = 1000.0;
    }
    else if (unit == Unit::DynamicViscosity) {
        unitString = QString::fromLatin1("Pa*s");
        factor = 0.001;
    }
    else if (unit == Unit::KinematicViscosity) {
        unitString = QString::fromLatin1("m^2/s");
        factor = 1e6;
    }
    else if (unit == Unit::VolumeFlowRate) {
        if (UnitValue < 1e-3) {  // smaller than 0.001 mm^3/s -> scientific notation
            unitString = QString::fromLatin1("m^3/s");
            factor = 1e9;
        }
        else if (UnitValue < 1e3) {
            unitString = QString::fromLatin1("mm^3/s");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromLatin1("ml/s");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("l/s");
            factor = 1e6;
        }
        else {
            unitString = QString::fromLatin1("m^3/s");
            factor = 1e9;
        }
    }
    else if (unit == Unit::DissipationRate) {
        unitString = QString::fromLatin1("m^2/s^3");
        factor = 1e6;
    }
    else if (unit == Unit::InverseLength) {
        if (UnitValue < 1e-6) {// smaller than 0.001 1/km -> scientific notation
            unitString = QString::fromLatin1("1/m");
            factor = 1e-3;
        }
        else if (UnitValue < 1e-3) {
            unitString = QString::fromLatin1("1/km");
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = QString::fromLatin1("1/m");
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = QString::fromLatin1("1/mm");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromUtf8("1/\xC2\xB5m");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QString::fromLatin1("1/nm");
            factor = 1e6;
        }
        else {// larger -> scientific notation
            unitString = QString::fromLatin1("1/m");
            factor = 1e-3;
        }
    }
    else if (unit == Unit::InverseArea) {
        if (UnitValue < 1e-12) {// smaller than 0.001 1/km^2 -> scientific notation
            unitString = QString::fromLatin1("1/m^2");
            factor = 1e-6;
        }
        else if (UnitValue < 1e-6) {
            unitString = QString::fromLatin1("1/km^2");
            factor = 1e-12;
        }
        else if (UnitValue < 1.0) {
            unitString = QString::fromLatin1("1/m^2");
            factor = 1e-6;
        }
        else if (UnitValue < 1e2) {
            unitString = QString::fromLatin1("1/cm^2");
            factor = 1e-2;
        }
        else {
            unitString = QString::fromLatin1("1/mm^2");
            factor = 1.0;
        }
    }
    else if (unit == Unit::InverseVolume) {
        if (UnitValue < 1e-6) {
            unitString = QString::fromLatin1("1/m^3");
            factor = 1e-9;
        }
        else if (UnitValue < 1e-3) {
            unitString = QString::fromLatin1("1/l");
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = QString::fromLatin1("1/ml");
            factor = 1e-3;
        }
        else {
            unitString = QString::fromLatin1("1/mm^3");
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
