// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Shai Seger
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

#include "StockObject.h"

#define NUM_PROFILE_POINTS 4

namespace CAMSimulator
{

StockObject::StockObject()
{}

void StockObject::GenerateBoxStock(float x, float y, float z, float l, float w, float h)
{
    int idx = 0;
    SET_DUAL(mProfile, idx, y + w, z + h);
    SET_DUAL(mProfile, idx, y + w, z);
    SET_DUAL(mProfile, idx, y, z);
    SET_DUAL(mProfile, idx, y, z + h);

    vec3_set(position, x, y, z);
    vec3_set(center, x + l / 2, y + w / 2, z + h / 2);
    vec3_set(size, l, w, h);

    shape.ExtrudeProfileLinear(mProfile, NUM_PROFILE_POINTS, x, x + l, 0, 0, true, true);
}

}  // namespace CAMSimulator
