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

#include "GlUtils.h"
#include <iostream>

namespace MillSim
{
int gWindowSizeW = 800;
int gWindowSizeH = 600;

int gDebug = -1;

mat4x4 identityMat = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

void GLClearError()
{
    while (glGetError() != GL_NO_ERROR)
        ;
}

bool GLLogError()
{
    bool isError = false;
    while (GLenum err = glGetError()) {
        std::cout << "[Opengl Error] (" << err << ")" << std::endl;
        isError = true;
    }
    return isError;
}


typedef struct Vertex
{
    vec3 pos;
    vec3 col;
} Vertex;

}  // namespace MillSim
