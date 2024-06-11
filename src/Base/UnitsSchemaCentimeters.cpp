/***************************************************************************
 *   Copyright (c) 2016 Yorik van Havre <yorik@uncreated.net>              *
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

#include "UnitsSchemaCentimeters.h"


using namespace Base;


QString UnitsSchemaCentimeters::schemaTranslate(const Base::Quantity& quant,
                                                double& factor,
                                                QString& unitString)
{
    Unit unit = quant.getUnit();
    if (unit == Unit::Length) {
        // all length units in centimeters
        unitString = QLatin1String("cm");
        factor = 10.0;
    }
    else if (unit == Unit::Area) {
        // all area units in square meters
        unitString = QLatin1String("m^2");
        factor = 1000000.0;
    }
    else if (unit == Unit::Volume) {
        // all area units in cubic meters
        unitString = QLatin1String("m^3");
        factor = 1000000000.0;
    }
    else if (unit == Unit::Power) {
        unitString = QLatin1String("W");
        factor = 1000000;
    }
    else if (unit == Unit::ElectricPotential) {
        unitString = QLatin1String("V");
        factor = 1000000;
    }
    else if (unit == Unit::HeatFlux) {
        unitString = QLatin1String("W/m^2");
        factor = 1.0;
    }
    else if (unit == Unit::Velocity) {
        unitString = QLatin1String("mm/min");
        factor = 1.0 / 60;
    }
    else {
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}
