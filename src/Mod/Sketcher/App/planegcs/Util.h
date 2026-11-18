// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Konstantinos Poulios
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <map>
#include <set>
#include <vector>


namespace GCS
{
using VEC_pD = std::vector<double*>;
using VEC_D = std::vector<double>;
using VEC_I = std::vector<int>;
using MAP_pD_pD = std::map<double*, double*>;
using MAP_pD_D = std::map<double*, double>;
using MAP_pD_I = std::map<double*, int>;
using SET_pD = std::set<double*>;
using SET_I = std::set<int>;
}  // namespace GCS
