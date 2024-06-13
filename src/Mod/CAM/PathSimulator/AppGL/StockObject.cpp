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

#include "StockObject.h"
#include "Shader.h"
#include <stdlib.h>

#define NUM_PROFILE_POINTS 4

MillSim::StockObject::StockObject()
{
    mat4x4_identity(mModelMat);
    vec3_set(center, 0, 0, 0);
}

MillSim::StockObject::~StockObject()
{
    shape.FreeResources();
}

void MillSim::StockObject::render()
{
    // glCallList(mDisplayListId);
    // UpdateObjColor(color);
    shape.Render(mModelMat, mModelMat);  // model is not rotated hence both are identity matrix
}

void MillSim::StockObject::SetPosition(vec3 position)
{
    mat4x4_translate(mModelMat, position[0], position[1], position[2]);
}

void MillSim::StockObject::GenerateBoxStock(float x, float y, float z, float l, float w, float h)
{
    int idx = 0;
    SET_DUAL(mProfile, idx, y + w, z + h);
    SET_DUAL(mProfile, idx, y + w, z);
    SET_DUAL(mProfile, idx, y, z);
    SET_DUAL(mProfile, idx, y, z + h);

    vec3_set(center, x + l / 2, y + w / 2, z + h / 2);
    vec3_set(size, l, w, h);

    shape.ExtrudeProfileLinear(mProfile, NUM_PROFILE_POINTS, x, x + l, 0, 0, true, true);
}
