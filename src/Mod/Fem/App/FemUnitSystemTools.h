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


#pragma once

#include <Base/Quantity.h>


namespace Fem
{

namespace Units
{
struct BasicUnits
{
    std::string length;
    std::string mass;
    std::string time;
    std::string current;
    std::string temperature;
    std::string amountOfSubstance;
    std::string luminousIntensity;
    std::string angle;
    double lengthScale;
};

const std::map<std::string, const BasicUnits> coherentSymbols = {
    {"FEM", {"mm", "t", "s", "A", "K", "mol", "cd", "rad", 1.0}},
    {"Internal", {"mm", "kg", "s", "A", "K", "mol", "cd", "rad", 1.0}},
    {"MKS", {"m", "kg", "s", "A", "K", "mol", "cd", "rad", 1000}},
    {"US", {"in", "lbf*s^2/in", "s", "A", "K", "mol", "cd", "rad", 25.4}},
};

double getCoherentLengthScale(const std::string& system);
double getCoherentValue(const Base::Quantity& q, const std::string& system);

}  // namespace Units
}  // namespace Fem
