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
#include <unistd.h>
#endif

#include <QString>

#include "UnitsSchemaMKS.h"
#include <cmath>


using namespace Base;


QString UnitsSchemaMKS::schemaTranslate(const Quantity& quant, double& factor, QString& unitString)
{
    double UnitValue = std::abs(quant.getValue());
    Unit unit = quant.getUnit();

    // now do special treatment on all cases seems necessary:
    if (unit == Unit::Length) {  // Length handling ============================
        if (UnitValue < 1e-6) {  // smaller than 0.001 nm -> scientific notation
            unitString = QLatin1String("mm");
            factor = 1.0;
        }
        else if (UnitValue < 1e-3) {
            unitString = QLatin1String("nm");
            factor = 1e-6;
        }
        else if (UnitValue < 0.1) {
            unitString = QString::fromUtf8("\xC2\xB5m");
            factor = 1e-3;
        }
        else if (UnitValue < 1e4) {
            unitString = QLatin1String("mm");
            factor = 1.0;
        }
        else if (UnitValue < 1e7) {
            unitString = QLatin1String("m");
            factor = 1e3;
        }
        else if (UnitValue < 1e10) {
            unitString = QLatin1String("km");
            factor = 1e6;
        }
        else {  // bigger than 1000 km -> scientific notation
            unitString = QLatin1String("m");
            factor = 1e3;
        }
    }
    else if (unit == Unit::Area) {
        if (UnitValue < 100) {
            unitString = QLatin1String("mm^2");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QLatin1String("cm^2");
            factor = 100;
        }
        else if (UnitValue < 1e12) {
            unitString = QLatin1String("m^2");
            factor = 1e6;
        }
        else {  // bigger than 1 square kilometer
            unitString = QLatin1String("km^2");
            factor = 1e12;
        }
    }
    else if (unit == Unit::Volume) {
        if (UnitValue < 1e3) {  // smaller than 1 ul
            unitString = QLatin1String("mm^3");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QLatin1String("ml");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QLatin1String("l");
            factor = 1e6;
        }
        else {  // bigger than 1000 l
            unitString = QLatin1String("m^3");
            factor = 1e9;
        }
    }
    else if (unit == Unit::Mass) {
        if (UnitValue < 1e-6) {
            unitString = QString::fromUtf8("\xC2\xB5g");
            factor = 1e-9;
        }
        else if (UnitValue < 1e-3) {
            unitString = QLatin1String("mg");
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = QLatin1String("g");
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = QLatin1String("kg");
            factor = 1.0;
        }
        else {
            unitString = QLatin1String("t");
            factor = 1e3;
        }
    }
    else if (unit == Unit::Density) {
        if (UnitValue < 0.0001) {
            unitString = QLatin1String("kg/m^3");
            factor = 0.000000001;
        }
        else if (UnitValue < 1.0) {
            unitString = QLatin1String("kg/cm^3");
            factor = 0.001;
        }
        else {
            unitString = QLatin1String("kg/mm^3");
            factor = 1.0;
        }
    }
    else if (unit == Unit::Acceleration) {
        unitString = QLatin1String("m/s^2");
        factor = 1000.0;
    }
    else if ((unit == Unit::Pressure) || (unit == Unit::Stress)) {
        if (UnitValue < 10.0) {  // Pa is the smallest
            unitString = QLatin1String("Pa");
            factor = 0.001;
        }
        else if (UnitValue < 10000.0) {
            unitString = QLatin1String("kPa");
            factor = 1.0;
        }
        else if (UnitValue < 10000000.0) {
            unitString = QLatin1String("MPa");
            factor = 1000.0;
        }
        else if (UnitValue < 10000000000.0) {
            unitString = QLatin1String("GPa");
            factor = 1000000.0;
        }
        else {  // bigger then 1000 GPa -> scientific notation
            unitString = QLatin1String("Pa");
            factor = 0.001;
        }
    }
    else if ((unit == Unit::Stiffness)) {
        if (UnitValue < 1) {  // mN/m is the smallest
            unitString = QLatin1String("mN/m");
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = QLatin1String("N/m");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QLatin1String("kN/m");
            factor = 1e3;
        }
        else {
            unitString = QLatin1String("MN/m");
            factor = 1e6;
        }
    }
    else if ((unit == Unit::StiffnessDensity)) {
        if (UnitValue < 1e-3) {
            unitString = QLatin1String("Pa/m");
            factor = 1e-6;
        }
        else if (UnitValue < 1) {
            unitString = QLatin1String("kPa/m");
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = QLatin1String("MPa/m");
            factor = 1.0;
        }
        else {
            unitString = QLatin1String("GPa/m");
            factor = 1e3;
        }
    }
    else if (unit == Unit::ThermalConductivity) {
        if (UnitValue > 1000000) {
            unitString = QLatin1String("W/mm/K");
            factor = 1000000.0;
        }
        else {
            unitString = QLatin1String("W/m/K");
            factor = 1000.0;
        }
    }
    else if (unit == Unit::ThermalExpansionCoefficient) {
        if (UnitValue < 0.001) {
            unitString = QString::fromUtf8("\xC2\xB5m/m/K");
            factor = 0.000001;
        }
        else {
            unitString = QLatin1String("m/m/K");
            factor = 1.0;
        }
    }
    else if (unit == Unit::VolumetricThermalExpansionCoefficient) {
        if (UnitValue < 0.001) {
            unitString = QString::fromUtf8("mm^3/m^3/K");
            factor = 1e-9;
        }
        else {
            unitString = QLatin1String("m^3/m^3/K");
            factor = 1.0;
        }
    }
    else if (unit == Unit::SpecificHeat) {
        unitString = QLatin1String("J/kg/K");
        factor = 1000000.0;
    }
    else if (unit == Unit::ThermalTransferCoefficient) {
        unitString = QLatin1String("W/m^2/K");
        factor = 1.0;
    }
    else if (unit == Unit::Force) {
        if (UnitValue < 1e3) {
            unitString = QLatin1String("mN");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QLatin1String("N");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QLatin1String("kN");
            factor = 1e6;
        }
        else {
            unitString = QLatin1String("MN");
            factor = 1e9;
        }
    }
    //    else if (unit == Unit::Moment) {
    //        if (UnitValue < 1e6) {
    //            unitString = QLatin1String("mNm");
    //            factor = 1e3;
    //        }
    //        else if (UnitValue < 1e9) {
    //            unitString = QLatin1String("Nm");
    //            factor = 1e6;
    //        }
    //        else if (UnitValue < 1e12) {
    //            unitString = QLatin1String("kNm");
    //            factor = 1e9;
    //        }
    //        else {
    //            unitString = QLatin1String("MNm");
    //            factor = 1e12;
    //        }
    //    }
    else if (unit == Unit::Power) {
        if (UnitValue < 1e6) {
            unitString = QLatin1String("mW");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QLatin1String("W");
            factor = 1e6;
        }
        else {
            unitString = QLatin1String("kW");
            factor = 1e9;
        }
    }
    else if (unit == Unit::ElectricPotential) {
        if (UnitValue < 1e6) {
            unitString = QLatin1String("mV");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QLatin1String("V");
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = QLatin1String("kV");
            factor = 1e9;
        }
        else {  // > 1000 kV scientificc notation
            unitString = QLatin1String("V");
            factor = 1e6;
        }
    }
    else if (unit == Unit::ElectricCharge) {
        unitString = QLatin1String("C");
        factor = 1.0;
    }
    else if (unit == Unit::CurrentDensity) {
        if (UnitValue <= 1e3) {
            unitString = QLatin1String("A/m^2");
            factor = 1e-6;
        }
        else {
            unitString = QLatin1String("A/mm^2");
            factor = 1;
        }
    }
    else if (unit == Unit::MagneticFluxDensity) {
        if (UnitValue <= 1e-3) {
            unitString = QLatin1String("G");
            factor = 1e-4;
        }
        else {
            unitString = QLatin1String("T");
            factor = 1.0;
        }
    }
    else if (unit == Unit::MagneticFieldStrength) {
        unitString = QLatin1String("A/m");
        factor = 1e-3;
    }
    else if (unit == Unit::MagneticFlux) {
        unitString = QLatin1String("Wb");
        factor = 1e6;
    }
    else if (unit == Unit::Magnetization) {
        unitString = QLatin1String("A/m");
        factor = 1e-3;
    }
    else if (unit == Unit::ElectricalConductance) {
        if (UnitValue < 1e-9) {
            unitString = QString::fromUtf8("\xC2\xB5S");
            factor = 1e-12;
        }
        else if (UnitValue < 1e-6) {
            unitString = QLatin1String("mS");
            factor = 1e-9;
        }
        else {
            unitString = QLatin1String("S");
            factor = 1e-6;
        }
    }
    else if (unit == Unit::ElectricalResistance) {
        if (UnitValue < 1e9) {
            unitString = QLatin1String("Ohm");
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = QLatin1String("kOhm");
            factor = 1e9;
        }
        else {
            unitString = QLatin1String("MOhm");
            factor = 1e12;
        }
    }
    else if (unit == Unit::ElectricalConductivity) {
        if (UnitValue < 1e-3) {
            unitString = QLatin1String("mS/m");
            factor = 1e-12;
        }
        else if (UnitValue < 1.0) {
            unitString = QLatin1String("S/m");
            factor = 1e-9;
        }
        else if (UnitValue < 1e3) {
            unitString = QLatin1String("kS/m");
            factor = 1e-6;
        }
        else {
            unitString = QLatin1String("MS/m");
            factor = 1e-3;
        }
    }
    else if (unit == Unit::ElectricalCapacitance) {
        if (UnitValue < 1e-15) {
            unitString = QLatin1String("pF");
            factor = 1e-18;
        }
        else if (UnitValue < 1e-12) {
            unitString = QLatin1String("nF");
            factor = 1e-15;
        }
        else if (UnitValue < 1e-9) {
            // \x reads everything to the end, therefore split
            unitString = QString::fromUtf8("\xC2\xB5"
                                           "F");
            factor = 1e-12;
        }
        else if (UnitValue < 1e-6) {
            unitString = QLatin1String("mF");
            factor = 1e-9;
        }
        else {
            unitString = QLatin1String("F");
            factor = 1e-6;
        }
    }
    else if (unit == Unit::ElectricalInductance) {
        if (UnitValue < 1e-6) {
            unitString = QLatin1String("nH");
            factor = 1e-3;
        }
        else if (UnitValue < 1e-3) {
            unitString = QString::fromUtf8("\xC2\xB5H");
            factor = 1.0;
        }
        else if (UnitValue < 1.0) {
            unitString = QLatin1String("mH");
            factor = 1e3;
        }
        else {
            unitString = QLatin1String("H");
            factor = 1e6;
        }
    }
    else if (unit == Unit::VacuumPermittivity) {
        unitString = QLatin1String("F/m");
        factor = 1e-9;
    }
    else if (unit == Unit::Work) {
        if (UnitValue < 1.602176634e-10) {
            unitString = QLatin1String("eV");
            factor = 1.602176634e-13;
        }
        else if (UnitValue < 1.602176634e-7) {
            unitString = QLatin1String("keV");
            factor = 1.602176634e-10;
        }
        else if (UnitValue < 1.602176634e-4) {
            unitString = QLatin1String("MeV");
            factor = 1.602176634e-7;
        }
        else if (UnitValue < 1e6) {
            unitString = QLatin1String("mJ");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QLatin1String("J");
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = QLatin1String("kJ");
            factor = 1e9;
        }
        else if (UnitValue < 3.6e+15) {
            unitString = QLatin1String("kWh");
            factor = 3.6e+12;
        }
        else {  // bigger than 1000 kWh -> scientific notation
            unitString = QLatin1String("J");
            factor = 1e6;
        }
    }
    else if (unit == Unit::SpecificEnergy) {
        unitString = QLatin1String("m^2/s^2");
        factor = 1000000;
    }
    else if (unit == Unit::HeatFlux) {
        unitString = QLatin1String("W/m^2");
        factor = 1.0;
    }
    else if (unit == Unit::Frequency) {
        if (UnitValue < 1e3) {
            unitString = QLatin1String("Hz");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QLatin1String("kHz");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QLatin1String("MHz");
            factor = 1e6;
        }
        else if (UnitValue < 1e12) {
            unitString = QLatin1String("GHz");
            factor = 1e9;
        }
        else {
            unitString = QLatin1String("THz");
            factor = 1e12;
        }
    }
    else if (unit == Unit::Velocity) {
        unitString = QLatin1String("m/s");
        factor = 1000.0;
    }
    else if (unit == Unit::DynamicViscosity) {
        unitString = QLatin1String("Pa*s");
        factor = 0.001;
    }
    else if (unit == Unit::KinematicViscosity) {
        unitString = QLatin1String("m^2/s");
        factor = 1e6;
    }
    else if (unit == Unit::VolumeFlowRate) {
        if (UnitValue < 1e-3) {  // smaller than 0.001 mm^3/s -> scientific notation
            unitString = QLatin1String("m^3/s");
            factor = 1e9;
        }
        else if (UnitValue < 1e3) {
            unitString = QLatin1String("mm^3/s");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QLatin1String("ml/s");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QLatin1String("l/s");
            factor = 1e6;
        }
        else {
            unitString = QLatin1String("m^3/s");
            factor = 1e9;
        }
    }
    else if (unit == Unit::DissipationRate) {
        unitString = QLatin1String("W/kg");
        factor = 1e6;
    }
    else if (unit == Unit::InverseLength) {
        if (UnitValue < 1e-6) {  // smaller than 0.001 1/km -> scientific notation
            unitString = QLatin1String("1/m");
            factor = 1e-3;
        }
        else if (UnitValue < 1e-3) {
            unitString = QLatin1String("1/km");
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = QLatin1String("1/m");
            factor = 1e-3;
        }
        else if (UnitValue < 1e3) {
            unitString = QLatin1String("1/mm");
            factor = 1.0;
        }
        else if (UnitValue < 1e6) {
            unitString = QString::fromUtf8("1/\xC2\xB5m");
            factor = 1e3;
        }
        else if (UnitValue < 1e9) {
            unitString = QLatin1String("1/nm");
            factor = 1e6;
        }
        else {  // larger -> scientific notation
            unitString = QLatin1String("1/m");
            factor = 1e-3;
        }
    }
    else if (unit == Unit::InverseArea) {
        if (UnitValue < 1e-12) {  // smaller than 0.001 1/km^2 -> scientific notation
            unitString = QLatin1String("1/m^2");
            factor = 1e-6;
        }
        else if (UnitValue < 1e-6) {
            unitString = QLatin1String("1/km^2");
            factor = 1e-12;
        }
        else if (UnitValue < 1.0) {
            unitString = QLatin1String("1/m^2");
            factor = 1e-6;
        }
        else if (UnitValue < 1e2) {
            unitString = QLatin1String("1/cm^2");
            factor = 1e-2;
        }
        else {
            unitString = QLatin1String("1/mm^2");
            factor = 1.0;
        }
    }
    else if (unit == Unit::InverseVolume) {
        if (UnitValue < 1e-6) {
            unitString = QLatin1String("1/m^3");
            factor = 1e-9;
        }
        else if (UnitValue < 1e-3) {
            unitString = QLatin1String("1/l");
            factor = 1e-6;
        }
        else if (UnitValue < 1.0) {
            unitString = QLatin1String("1/ml");
            factor = 1e-3;
        }
        else {
            unitString = QLatin1String("1/mm^3");
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
