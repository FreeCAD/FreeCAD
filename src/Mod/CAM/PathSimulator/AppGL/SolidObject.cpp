/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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

#include "SolidObject.h"
#include "Shader.h"
#include <algorithm>

#define NUM_PROFILE_POINTS 4
using namespace MillSim;

SolidObject::SolidObject()
{
    mat4x4_identity(mModelMat);
    vec3_set(center, 0, 0, 0);
}

SolidObject::~SolidObject()
{
    isValid = false;
    shape.FreeResources();
}

void MillSim::SolidObject::SetPosition(vec3 position)
{
    mat4x4_translate(mModelMat, position[0], position[1], position[2]);
}

void SolidObject::render()
{
    if (!isValid) {
        return;
    }
    // UpdateObjColor(color);
    shape.Render(mModelMat, mModelMat);  // model is not rotated hence both are identity matrix
}

void SolidObject::GenerateSolid(std::vector<Vertex>& verts, std::vector<GLushort>& indices)
{
    shape.SetModelData(verts, indices);

    // calculate object's bounding box:
    float x = 999999.0f, y = 999999.0f, z = 999999.0f;
    float l = -999999.0f, w = -999999.0f, h = -999999.0f;
    for (auto& vert : verts) {
        x = std::fminf(x, vert.x);
        y = std::fminf(y, vert.y);
        z = std::fminf(z, vert.z);
        l = std::fmaxf(l, vert.x);
        w = std::fmaxf(w, vert.y);
        h = std::fmaxf(h, vert.z);
    }
    l -= x;
    w -= y;
    h -= z;
    vec3_set(position, x, y, z);
    vec3_set(center, x + l / 2, y + w / 2, z + h / 2);
    vec3_set(size, l, w, h);
    isValid = true;
}
