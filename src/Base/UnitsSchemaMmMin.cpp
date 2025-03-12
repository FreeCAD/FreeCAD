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
#include <algorithm>
#include <array>
#endif

#include "Quantity.h"
#include "Unit.h"
#include "UnitsSchemaMmMin.h"

using namespace Base;

std::string
UnitsSchemaMmMin::schemaTranslate(const Quantity& quant, double& factor, std::string& unitString)
{
    static std::array<std::pair<Unit, std::pair<std::string, double>>, 3> unitSpecs {{
        {Unit::Length, {"mm", 1.0}},
        {Unit::Angle, {"\xC2\xB0", 1.0}},
        {Unit::Velocity, {"mm/min", 1.0 / 60.0}},
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
