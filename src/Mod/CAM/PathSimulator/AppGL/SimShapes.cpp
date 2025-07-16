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

#include "SimShapes.h"
#include "Shader.h"
#include "GlUtils.h"
#include <math.h>
#include <cstddef>
#include <vector>

using namespace MillSim;
using std::numbers::pi;

int Shape::lastNumSlices = 0;
std::vector<float> Shape::sinTable;
std::vector<float> Shape::cosTable;

void Shape::GenerateSinTable(int nSlices)
{
    if (nSlices == lastNumSlices) {
        return;
    }

    float slice = (float)(2 * pi / nSlices);
    int nvals = nSlices + 1;
    sinTable.resize(nvals);
    cosTable.resize(nvals);
    for (int i = 0; i < nvals; i++) {
        sinTable[i] = sinf(slice * i);
        cosTable[i] = cosf(slice * i);
    }
    lastNumSlices = nvals;
}


void Shape::RotateProfile(float* profPoints,
                          int nPoints,
                          float distance,
                          float /* deltaHeight */,
                          int nSlices,
                          bool isHalfTurn)
{
    int vidx = 0;
    int iidx = 0;
    int numVerts, numIndices;
    int vstart;

    numVerts = nPoints * 2 * (nSlices + 1);
    numIndices = (nPoints - 1) * nSlices * 6;

    std::vector<Vertex> vbuffer(numVerts);
    std::vector<GLushort> ibuffer(numIndices);
    int nsinvals = nSlices;
    if (isHalfTurn) {
        nsinvals *= 2;
    }
    GenerateSinTable(nsinvals);

    for (int i = 0; i < nPoints; i++) {
        int i2 = i * 2;

        float prevy = i > 0 ? profPoints[i2 - 2] : 0;
        float prevz = i > 0 ? profPoints[i2 - 1] : profPoints[i2 + 1];
        float prevrad = fabsf(prevy);
        float rad = fabsf(profPoints[i2]);
        float z2 = profPoints[i2 + 1];
        float diffy = profPoints[i2] - prevy;
        float diffz = z2 - prevz;
        float len = sqrtf(diffy * diffy + diffz * diffz);
        float nz = diffy / len;
        vstart = i * 2 * (nSlices + 1);

        for (int j = 0; j <= nSlices; j++) {
            // generate vertices
            float sx = sinTable[j];
            float sy = cosTable[j];
            float x1 = prevrad * sx + distance;
            float y1 = prevrad * sy;
            float x2 = rad * sx + distance;
            float y2 = rad * sy;

            // generate normals
            float ny = -diffz / len;
            float nx = ny * sx;
            ny *= sy;

            vbuffer[vidx++] = {x1, y1, prevz, nx, ny, nz};
            vbuffer[vidx++] = {x2, y2, z2, nx, ny, nz};

            if (j != nSlices) {
                // generate indices { 0, 3, 1, 0, 2, 3 }
                int pos = vstart + 2 * j;
                if (i < (nPoints - 1)) {
                    SET_TRIPLE(ibuffer, iidx, pos, pos + 3, pos + 1);
                }
                if (i > 0) {
                    SET_TRIPLE(ibuffer, iidx, pos, pos + 2, pos + 3);
                }
            }
        }
    }

    SetModelData(vbuffer, ibuffer);
}

void Shape::CalculateExtrudeBufferSizes(int nProfilePoints,
                                        bool capStart,
                                        bool capEnd,
                                        int* numVerts,
                                        int* numIndices,
                                        int* vc1idx,
                                        int* vc2idx,
                                        int* ic1idx,
                                        int* ic2idx)
{
    *numVerts = nProfilePoints * 4;        // one face per profile point times 4 vertex per face
    *numIndices = nProfilePoints * 2 * 3;  // 2 triangles per face times 3 indices per triangle
    if (capStart) {
        *vc1idx = *numVerts;
        *numVerts += nProfilePoints;
        *ic1idx = *numIndices;
        *numIndices += (nProfilePoints - 2) * 3;
    }
    if (capEnd) {
        *vc2idx = *numVerts;
        *numVerts += nProfilePoints;
        *ic2idx = *numIndices;
        *numIndices += (nProfilePoints - 2) * 3;
    }
}

void Shape::ExtrudeProfileRadial(float* profPoints,
                                 int nPoints,
                                 float radius,
                                 float angleRad,
                                 float deltaHeight,
                                 bool capStart,
                                 bool capEnd)
{
    int vidx = 0, vc1idx, vc2idx;
    int iidx = 0, ic1idx, ic2idx;
    int numVerts, numIndices;

    CalculateExtrudeBufferSizes(nPoints,
                                capStart,
                                capEnd,
                                &numVerts,
                                &numIndices,
                                &vc1idx,
                                &vc2idx,
                                &ic1idx,
                                &ic2idx);
    int vc1start = vc1idx;
    int vc2start = vc2idx;

    std::vector<Vertex> vbuffer(numVerts);
    std::vector<GLushort> ibuffer(numIndices);

    bool is_clockwise = angleRad > 0;
    angleRad = (float)fabs(angleRad);
    float dir = is_clockwise ? 1.0f : -1.0f;
    int offs1 = is_clockwise ? -1 : 0;
    int offs2 = is_clockwise ? 0 : -1;

    float cosAng = cosf(angleRad);
    float sinAng = sinf(angleRad);
    for (int i = 0; i < nPoints; i++) {
        int p1 = i * 2;
        float y1 = profPoints[p1] + radius;
        float z1 = profPoints[p1 + 1];
        int p2 = (p1 + 2) % (nPoints * 2);
        float y2 = profPoints[p2] + radius;
        float z2 = profPoints[p2 + 1];

        // normals
        float ydiff = y2 - y1;
        float zdiff = z2 - z1;
        float len = sqrtf(ydiff * ydiff + zdiff * zdiff);
        float ny = -zdiff / len;
        float nz = ydiff / len;
        float nx = -sinAng * ny;
        ny *= cosAng;

        // start verts
        vbuffer[vidx++] = {0, y1, z1, nx, ny, nz};
        vbuffer[vidx++] = {0, y2, z2, nx, ny, nz};

        if (capStart) {
            vbuffer[vc1idx++] = {0, y1, z1, -1 * dir, 0, 0};
            if (i > 1) {
                SET_TRIPLE(ibuffer, ic1idx, vc1start, vc1start + i + offs1, vc1start + i + offs2);
            }
        }

        float x1 = y1 * sinAng * dir;
        float x2 = y2 * sinAng * dir;
        y1 *= cosAng;
        y2 *= cosAng;
        z1 += deltaHeight;
        z2 += deltaHeight;
        vbuffer[vidx++] = {x1, y1, z1, nx, ny, nz};
        vbuffer[vidx++] = {x2, y2, z2, nx, ny, nz};

        // face have 2 triangles { 0, 2, 3, 0, 3, 1 };
        GLushort vistart = i * 4;
        if (is_clockwise) {
            SET_TRIPLE(ibuffer, iidx, vistart, vistart + 2, vistart + 3);
            SET_TRIPLE(ibuffer, iidx, vistart, vistart + 3, vistart + 1);
        }
        else {
            SET_TRIPLE(ibuffer, iidx, vistart, vistart + 3, vistart + 2);
            SET_TRIPLE(ibuffer, iidx, vistart, vistart + 1, vistart + 3);
        }

        if (capEnd) {
            vbuffer[vc2idx++] = {x1, y1, z1, cosAng * dir, -sinAng, 0};
            if (i > 1) {
                SET_TRIPLE(ibuffer, ic2idx, vc2start, vc2start + i + offs2, vc2start + i + offs1);
            }
        }
    }

    SetModelData(vbuffer, ibuffer);
}

void Shape::ExtrudeProfileLinear(float* profPoints,
                                 int nPoints,
                                 float fromX,
                                 float toX,
                                 float fromZ,
                                 float toZ,
                                 bool capStart,
                                 bool capEnd)
{
    int vidx = 0, vc1idx, vc2idx;
    int iidx = 0, ic1idx, ic2idx;
    int numVerts, numIndices;

    CalculateExtrudeBufferSizes(nPoints,
                                capStart,
                                capEnd,
                                &numVerts,
                                &numIndices,
                                &vc1idx,
                                &vc2idx,
                                &ic1idx,
                                &ic2idx);
    int vc1start = vc1idx;
    int vc2start = vc2idx;

    std::vector<Vertex> vbuffer(numVerts);
    std::vector<GLushort> ibuffer(numIndices);

    for (int i = 0; i < nPoints; i++) {
        // hollow pipe verts
        int p1 = i * 2;
        float y1 = profPoints[p1];
        float z1 = profPoints[p1 + 1];
        int p2 = (p1 + 2) % (nPoints * 2);
        float y2 = profPoints[p2];
        float z2 = profPoints[p2 + 1];

        // nornal
        float ydiff = y2 - y1;
        float zdiff = z2 - z1;
        float len = sqrtf(ydiff * ydiff + zdiff * zdiff);
        float ny = -zdiff / len;
        float nz = ydiff / len;

        vbuffer[vidx++] = {fromX, y1, z1 + fromZ, 0, ny, nz};
        vbuffer[vidx++] = {fromX, y2, z2 + fromZ, 0, ny, nz};
        vbuffer[vidx++] = {toX, y1, z1 + toZ, 0, ny, nz};
        vbuffer[vidx++] = {toX, y2, z2 + toZ, 0, ny, nz};

        // face have 2 triangles { 0, 2, 3, 0, 3, 1 };
        GLushort vistart = i * 4;
        SET_TRIPLE(ibuffer, iidx, vistart, vistart + 2, vistart + 3);
        SET_TRIPLE(ibuffer, iidx, vistart, vistart + 3, vistart + 1);

        if (capStart) {
            vbuffer[vc1idx++] = {fromX, profPoints[p1], profPoints[p1 + 1] + fromZ, -1, 0, 0};
            if (i > 1) {
                SET_TRIPLE(ibuffer, ic1idx, vc1start, vc1start + i - 1, vc1start + i);
            }
        }
        if (capEnd) {
            vbuffer[vc2idx++] = {toX, profPoints[p1], profPoints[p1 + 1] + toZ, 1, 0, 0};
            if (i > 1) {
                SET_TRIPLE(ibuffer, ic2idx, vc2start, vc2start + i, vc2start + i - 1);
            }
        }
    }

    SetModelData(vbuffer, ibuffer);
}

void Shape::GenerateModel(float* vbuffer, GLushort* ibuffer, int numVerts, int nIndices)
{
    // GLuint vbo, ibo, vao;

    // vertex buffer
    glGenBuffers(1, &vbo);
    GLClearError();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLLogError();
    glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(Vertex), vbuffer, GL_STATIC_DRAW);

    // index buffer
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nIndices * sizeof(GLushort), ibuffer, GL_STATIC_DRAW);

    // vertex array
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));

    numIndices = nIndices;
}

void MillSim::Shape::SetModelData(std::vector<Vertex>& vbuffer, std::vector<GLushort>& ibuffer)
{
    GenerateModel((float*)vbuffer.data(), ibuffer.data(), (int)vbuffer.size(), (int)ibuffer.size());
}

void Shape::Render()
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, nullptr);
}

void Shape::Render(mat4x4 modelMat, mat4x4 normallMat)  // normals are rotated only
{
    CurrentShader->UpdateModelMat(modelMat, normallMat);
    Render();
}

void Shape::FreeResources()
{
    glBindVertexArray(0);
    GLDELETE_BUFFER(vbo);
    GLDELETE_BUFFER(ibo);
    GLDELETE_VERTEXARRAY(vao);
}

Shape::~Shape()
{
    FreeResources();
}
