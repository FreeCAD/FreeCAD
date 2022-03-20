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
# include <unistd.h>
#endif

#include <QString>

#include "UnitsSchemaCentimeters.h"


using namespace Base;


QString UnitsSchemaCentimeters::schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString)
{
    Unit unit = quant.getUnit();
    if (unit == Unit::Length) {
        // all length units in centimeters
        unitString = QString::fromLatin1("cm");
        factor = 10.0;
    }
    else if (unit == Unit::Area) {
        // all area units in square meters
        unitString = QString::fromLatin1("m^2");
        factor = 1000000.0;
    }
    else if (unit == Unit::Volume) {
        // all area units in cubic meters
        unitString = QString::fromLatin1("m^3");
        factor = 1000000000.0;
    }
    else if (unit == Unit::Power) {
        unitString = QString::fromLatin1("W");
        factor = 1000000;
    }
    else if (unit == Unit::ElectricPotential) {
        unitString = QString::fromLatin1("V");
        factor = 1000000;
    }
    else if (unit == Unit::HeatFlux) {
        unitString = QString::fromLatin1("W/m^2");
        factor = 1.0;
    }
    else if (unit == Unit::Velocity) {
        unitString = QString::fromLatin1("mm/min");
        factor = 1.0/60;
    }
    else {
        // default action for all cases without special treatment:
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}
