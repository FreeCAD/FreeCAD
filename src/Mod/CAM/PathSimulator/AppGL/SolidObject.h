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

#ifndef __solid_object_h__
#define __solid_object_h__
#include "SimShapes.h"
#include "linmath.h"
#include <vector>

namespace MillSim
{

class SolidObject
{
public:
    SolidObject();
    virtual ~SolidObject();
    void SetPosition(vec3 position);

    /// Calls the display list.
    virtual void render();
    Shape shape;
    void GenerateSolid(std::vector<Vertex> & verts, std::vector<GLushort>& indices);
    vec3 center = {};
    vec3 size = {};
    vec3 position = {};
    bool isValid = false;

protected:
    mat4x4 mModelMat;
};
}  // namespace MillSim

#endif
