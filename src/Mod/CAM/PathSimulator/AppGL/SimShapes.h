// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once
#include "OpenGlWrapper.h"
#include "linmath.h"

#define SET_DUAL(var, idx, y, z) \
    { \
        var[idx++] = y; \
        var[idx++] = z; \
    }
#define SET_TRIPLE(var, idx, x, y, z) \
    { \
        var[idx++] = x; \
        var[idx++] = y; \
        var[idx++] = z; \
    }

#define SET_TRIPLE_OFFS(var, idx, offs, x, y, z) \
    { \
        var[idx++] = x + offs; \
        var[idx++] = y + offs; \
        var[idx++] = z + offs; \
    }

namespace MillSim
{
typedef unsigned int uint;

struct Vertex
{
    Vertex()
        : Vertex(0, 0, 0)
    {}
    Vertex(float _x, float _y, float _z, float _nx = 0, float _ny = 0, float _nz = 0)
    {
        x = _x;
        y = _y;
        z = _z;
        nx = _nx;
        ny = _ny;
        nz = _nz;
    }
    float x, y, z;
    float nx, ny, nz;
};

class Shape
{
public:
    Shape()
    {}
    ~Shape();

public:
    uint vao = 0;
    uint vbo = 0;
    uint ibo = 0;
    int numIndices = 0;

public:
    void Render();
    void Render(const mat4x4& modelMat, const mat4x4& normallMat);
    void FreeResources();
    void SetModelData(const std::vector<Vertex>& vbuffer, const std::vector<GLushort>& ibuffer);
    void RotateProfile(
        float* profPoints,
        int nPoints,
        float distance,
        float deltaHeight,
        int nSlices,
        bool isHalfTurn
    );
    void ExtrudeProfileRadial(
        float* profPoints,
        int nPoints,
        float radius,
        float angleRad,
        float deltaHeight,
        bool capStart,
        bool capEnd
    );
    void ExtrudeProfileLinear(
        float* profPoints,
        int nPoints,
        float fromX,
        float toX,
        float fromZ,
        float toZ,
        bool capStart,
        bool capEnd
    );

    static void GenerateSinTable(int nSlices);
    static std::vector<float> sinTable;
    static std::vector<float> cosTable;
    static int lastNumSlices;

protected:
    void GenerateModel(const float* vbuffer, const GLushort* ibuffer, int numVerts, int numIndices);
    void CalculateExtrudeBufferSizes(
        int nProfilePoints,
        bool capStart,
        bool capEnd,
        int* numVerts,
        int* numIndices,
        int* vc1idx,
        int* vc2idx,
        int* ic1idx,
        int* ic2idx
    );
};

}  // namespace MillSim
