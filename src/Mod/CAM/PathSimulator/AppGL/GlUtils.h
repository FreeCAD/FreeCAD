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

#ifndef __glutils_h__
#define __glutils_h__
#include "OpenGlWrapper.h"
#include "linmath.h"

#define PI 3.14159265f
#define PI2 (PI * 2)


constexpr auto EPSILON = 0.00001f;
#define EQ_FLOAT(x, y) (fabs((x) - (y)) < EPSILON)

#define MS_MOUSE_LEFT  0x01
#define MS_MOUSE_RIGHT 0x02
#define MS_MOUSE_MID   0x04
#define MS_KBD_SHIFT   0x08
#define MS_KBD_CONTROL 0x10
#define MS_KBD_ALT     0x20

#define GL(x)                                                                                      \
    {                                                                                              \
        GLClearError();                                                                            \
        x;                                                                                         \
        if (GLLogError())                                                                          \
            __debugbreak();                                                                        \
    }
#define RadToDeg(x) (x * 180.0f / PI)

#define GLDELETE(type, x)                                                                          \
    {                                                                                              \
        if (x != 0)                                                                                \
            glDelete##type(1, &x);                                                                 \
        x = 0;                                                                                     \
    }

#define GLDELETE_FRAMEBUFFER(x) GLDELETE(Framebuffers, x)
#define GLDELETE_TEXTURE(x) GLDELETE(Textures, x)
#define GLDELETE_VERTEXARRAY(x) GLDELETE(VertexArrays, x)
#define GLDELETE_RENDERBUFFER(x) GLDELETE(Renderbuffers, x)
#define GLDELETE_BUFFER(x) GLDELETE(Buffers, x)

namespace MillSim
{
void GLClearError();
bool GLLogError();
extern mat4x4 identityMat;
extern int gDebug;
extern int gWindowSizeW;
extern int gWindowSizeH;
}  // namespace MillSim
#endif  // !__glutils_h__
