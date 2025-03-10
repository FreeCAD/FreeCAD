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
#ifndef _PreComp_
#include <algorithm>
#include <array>
#endif

#include "Quantity.h"
#include "Unit.h"
#include "UnitsSchemaCentimeters.h"

using namespace Base;

std::string UnitsSchemaCentimeters::schemaTranslate(const Base::Quantity& quant,
                                                    double& factor,
                                                    std::string& unitString)
{
    static std::array<std::pair<Unit, std::pair<std::string, double>>, 7> unitSpecs {{
        {Unit::Length, {"cm", 10.0}},
        {Unit::Area, {"m^2", 1000000.0}},
        {Unit::Volume, {"m^3", 1000000000.0}},
        {Unit::Power, {"W", 1000000.0}},
        {Unit::ElectricPotential, {"V", 1000000.0}},
        {Unit::HeatFlux, {"W/m^2", 1.0}},
        {Unit::Velocity, {"mm/min", 1.0 / 60}},
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
