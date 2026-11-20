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

#include "SimShapes.h"
#include "linmath.h"
#include <vector>

namespace CAMSimulator
{

class SolidObject
{
public:
    SolidObject();
    virtual ~SolidObject();

    void Clear();

    void SetPosition(vec3 position);

    /// Calls the display list.
    virtual void render();
    Shape shape;
    void GenerateSolid(const std::vector<Vertex>& verts, const std::vector<GLushort>& indices);
    vec3 center = {};
    vec3 size = {};
    vec3 position = {};
    bool isValid = false;

protected:
    mat4x4 mModelMat;
};

}  // namespace CAMSimulator
