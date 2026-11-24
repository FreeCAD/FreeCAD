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

#pragma once

#include "linmath.h"

constexpr auto EPSILON = 0.00001f;
#define EQ_FLOAT(x, y) (fabs((x) - (y)) < EPSILON)

#define MS_MOUSE_LEFT 0x01
#define MS_MOUSE_RIGHT 0x02
#define MS_MOUSE_MID 0x04
#define MS_KBD_SHIFT 0x08
#define MS_KBD_CONTROL 0x10
#define MS_KBD_ALT 0x20

namespace CAMSimulator
{

extern const mat4x4 identityMat;

}  // namespace CAMSimulator
