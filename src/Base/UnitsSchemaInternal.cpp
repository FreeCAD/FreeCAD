/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel (FreeCAD@juergen-riegel.net)        *
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
#include <QLocale>
#include "Exception.h"
#include "UnitsApi.h"
#include "UnitsSchemaInternal.h"
#include <cmath>


using namespace Base;


QString UnitsSchemaInternal::schemaTranslate(const Quantity &quant, double &factor, QString &unitString)
{
    double UnitValue = std::abs(quant.getValue());
    Unit unit = quant.getUnit();

    // In order to get the right factor always express the target
    // units as internal units where length is in mm and mass in kg
    // Example:
    // For W/mm/K we get the factor of 1000000.0 because
    // W/mm/K = kg*m^2/s^3/mm/K
    // = 10e6 * kg*mm^2/s^3/mm/K
    // = 10e6 * kg*mm/s^3/K

    // now do special treatment on all cases seems necessary:
    if (unit == Unit::Length) {  // Length handling ============================
        if (UnitValue < 0.000000001) {// smaller then 0.001 nm -> scientific notation
            unitString = QString::fromLatin1("mm");
            factor = 1.0;
        }
        else if(UnitValue < 0.001) {
            unitString = QString::fromLatin1("nm");
            factor = 0.000001;
        }
        else if (UnitValue < 0.1) {
            unitString = QString::fromUtf8("\xC2\xB5m");
            factor = 0.001;
        }
        else if (UnitValue < 10000.0) {
            unitString = QString::fromLatin1("mm");
            factor = 1.0;
        }
        else if (UnitValue < 10000000.0) {
            unitString = QString::fromLatin1("m");
            factor = 1000.0;
        }
        else if (UnitValue < 100000000000.0) {
            unitString = QString::fromLatin1("km");
            factor = 1000000.0;
        }
        else { // bigger then 1000 km -> scientific notation
            unitString = QString::fromLatin1("mm");
            factor = 1.0;
        }
    }
    else if (unit == Unit::Area) {
        // TODO Cascade for the Areas
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }
    else if (unit == Unit::Angle) {
        // TODO Cascade for the Areas
        // default action for all cases without special treatment:
        unitString = QString::fromUtf8("\xC2\xB0");
        factor = 1.0;
    }
    else if (unit == Unit::Mass) {
        // TODO Cascade for the weights
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
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
            unitString = QString::fromLatin1("mm/mm/K");
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
        else { // bigger -> scientific notation
            unitString = QString::fromLatin1("Pa");
            factor = 0.001;
        }
    }
    else if (unit == Unit::FractureToughness) {
        unitString = QString::fromLatin1("MPa*m^0.5");
        factor = 1000.0 * pow(1000.0, 0.5);  // MPa = * 1000 and m^0.5 = * 1000^0.5
    }
    else if (unit == Unit::Power) {
        unitString = QString::fromLatin1("W");
        factor = 1000000;
    }
    else if (unit == Unit::ElectricPotential) {
        unitString = QString::fromLatin1("V");
        factor = 1000000;
    }
    else if (unit == Unit::SpecificEnergy) {
        unitString = QString::fromLatin1("m^2/s^2");
        factor = 1000000;
    }
    else if (unit == Unit::HeatFlux) {
        unitString = QString::fromLatin1("W/m^2");
        factor = 1.0;
    }
    else if (unit == Unit::Velocity) {
        unitString = QString::fromLatin1("mm/s");
        factor = 1.0;
    }
    else if (unit == Unit::DynamicViscosity) {
        unitString = QString::fromLatin1("kg/(m*s)");
        factor = 0.001;
    }
    else {
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}
