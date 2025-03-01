/***************************************************************************
 *   Copyright (c) WandererFan <wandererfan@gmail.com>                     *
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

/*  Metric units schema intended for design of large objects
 *  Lengths are always in metres.
 *  Areas are always in square metres
 *  Volumes are always in cubic metres
 *  Angles in decimal degrees (use degree symbol)
 *  Velocities in m/sec
 */

#include "PreCompiled.h"
#ifndef _PreComp_
#include <algorithm>
#include <array>
#endif

#include "Quantity.h"
#include "Unit.h"
#include "UnitsSchemaMeterDecimal.h"

using namespace Base;

std::string UnitsSchemaMeterDecimal::schemaTranslate(const Base::Quantity& quant,
                                                     double& factor,
                                                     std::string& unitString)
{
    static std::array<std::pair<Unit, std::pair<std::string, double>>, 7> unitSpecs {{
        {Unit::Length, {"m", 1e3}},
        {Unit::Area, {"m^2", 1e6}},
        {Unit::Volume, {"m^3", 1e9}},
        {Unit::Power, {"W", 1000000}},
        {Unit::ElectricPotential, {"V", 1000000}},
        {Unit::HeatFlux, {"W/m^2", 1.0}},
        {Unit::Velocity, {"m/s", 1e3}},
    }};

    const auto unit = quant.getUnit();
    const auto spec = std::find_if(unitSpecs.begin(), unitSpecs.end(), [&](const auto& pair) {
        return pair.first == unit;
    });

    if (spec != std::end(unitSpecs)) {
        unitString = spec->second.first;
        factor = spec->second.second;
    }
    else {
        unitString = quant.getUnit().getString();
        factor = 1.0;
    }

    return toLocale(quant, factor, unitString);
}
