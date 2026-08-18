// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include <format>

#include "FemUnitSystemTools.h"


namespace Fem::Units
{

double getCoherentLengthScale(const std::string& system)
{
    auto it = coherentSymbols.find(system);
    if (it == coherentSymbols.end()) {
        throw Base::ValueError(std::format("Invalid unit system: {}", system));
    }
    const BasicUnits& symbols = it->second;
    return symbols.lengthScale;
}

double getCoherentValue(const Base::Quantity& q, const std::string& system)
{
    auto exponents = q.getUnit().exponents();
    auto it = coherentSymbols.find(system);
    if (it == coherentSymbols.end()) {
        throw Base::ValueError(std::format("Invalid unit system: {}", system));
    }
    const BasicUnits& symbols = it->second;
    std::string unit = std::format(
        "({})^({})*({})^({})*({})^({})*({})^({})*({})^({})*({})^({})*({})^({})*({})^({})",
        symbols.length,
        exponents[0],
        symbols.mass,
        exponents[1],
        symbols.time,
        exponents[2],
        symbols.current,
        exponents[3],
        symbols.temperature,
        exponents[4],
        symbols.amountOfSubstance,
        exponents[5],
        symbols.luminousIntensity,
        exponents[6],
        symbols.angle,
        exponents[7]
    );
    return q.getValueAs(Base::Quantity(1, unit));
}

}  // namespace Fem::Units
