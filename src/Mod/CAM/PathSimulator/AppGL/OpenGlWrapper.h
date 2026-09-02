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

// The defines in this file can mess up other includes that are using OpenGL function names as
// reported in #28950. For this reason this file must be included last. To avoid having to track
// multiple levels of includes, this file should only be included in source files and not in
// headers. If OpenGL defines are needed in headers, use <QOpenGLFunctions>.

#include <QOpenGLExtraFunctions>

namespace CAMSimulator
{

extern QOpenGLExtraFunctions gOpenGLFunctions;

#define glClearColor gOpenGLFunctions.glClearColor
#define glBlendFunc gOpenGLFunctions.glBlendFunc
#define glClear gOpenGLFunctions.glClear
#define glGenBuffers gOpenGLFunctions.glGenBuffers
#define glBindBuffer gOpenGLFunctions.glBindBuffer
#define glBufferData gOpenGLFunctions.glBufferData
#define glGenVertexArrays gOpenGLFunctions.glGenVertexArrays
#define glBindVertexArray gOpenGLFunctions.glBindVertexArray
#define glEnableVertexAttribArray gOpenGLFunctions.glEnableVertexAttribArray
#define glVertexAttribPointer gOpenGLFunctions.glVertexAttribPointer
#define glBindAttribLocation gOpenGLFunctions.glBindAttribLocation
#define glGetAttribLocation gOpenGLFunctions.glGetAttribLocation
#define glShaderSource gOpenGLFunctions.glShaderSource
#define glCompileShader gOpenGLFunctions.glCompileShader
#define glDeleteShader gOpenGLFunctions.glDeleteShader
#define glDeleteProgram gOpenGLFunctions.glDeleteProgram
#define glAttachShader gOpenGLFunctions.glAttachShader
#define glLinkProgram gOpenGLFunctions.glLinkProgram
#define glGetProgramiv gOpenGLFunctions.glGetProgramiv
#define glGetUniformLocation gOpenGLFunctions.glGetUniformLocation
#define glGetError gOpenGLFunctions.glGetError
#define glEnable gOpenGLFunctions.glEnable
#define glColorMask gOpenGLFunctions.glColorMask
#define glCullFace gOpenGLFunctions.glCullFace
#define glDepthFunc gOpenGLFunctions.glDepthFunc
#define glStencilFunc gOpenGLFunctions.glStencilFunc
#define glStencilOp gOpenGLFunctions.glStencilOp
#define glDepthMask gOpenGLFunctions.glDepthMask
#define glDisable gOpenGLFunctions.glDisable
#define glMatrixMode gOpenGLFunctions.glMatrixMode
#define glUseProgram gOpenGLFunctions.glUseProgram
#define glDrawElements gOpenGLFunctions.glDrawElements
#define glDeleteVertexArrays gOpenGLFunctions.glDeleteVertexArrays
#define glUniformMatrix4fv gOpenGLFunctions.glUniformMatrix4fv
#define glUniform3fv gOpenGLFunctions.glUniform3fv
#define glUniform1i gOpenGLFunctions.glUniform1i
#define glCreateShader gOpenGLFunctions.glCreateShader
#define glCreateProgram gOpenGLFunctions.glCreateProgram
#define glDeleteBuffers gOpenGLFunctions.glDeleteBuffers
#define glActiveTexture gOpenGLFunctions.glActiveTexture
#define glBindTexture gOpenGLFunctions.glBindTexture
#define glGenTextures gOpenGLFunctions.glGenTextures
#define glTexParameteri gOpenGLFunctions.glTexParameteri
#define glTexImage2D gOpenGLFunctions.glTexImage2D
#define glDeleteTextures gOpenGLFunctions.glDeleteTextures
#define glPolygonOffset gOpenGLFunctions.glPolygonOffset

#define glBindFramebuffer gOpenGLFunctions.glBindFramebuffer
#define glUniform1f gOpenGLFunctions.glUniform1f
#define glGenFramebuffers gOpenGLFunctions.glGenFramebuffers
#define glFramebufferTexture2D gOpenGLFunctions.glFramebufferTexture2D
#define glDrawBuffers gOpenGLFunctions.glDrawBuffers
#define glGenRenderbuffers gOpenGLFunctions.glGenRenderbuffers
#define glBindRenderbuffer gOpenGLFunctions.glBindRenderbuffer
#define glRenderbufferStorage gOpenGLFunctions.glRenderbufferStorage
#define glFramebufferRenderbuffer gOpenGLFunctions.glFramebufferRenderbuffer
#define glCheckFramebufferStatus gOpenGLFunctions.glCheckFramebufferStatus
#define glDeleteFramebuffers gOpenGLFunctions.glDeleteFramebuffers
#define glDeleteRenderbuffers gOpenGLFunctions.glDeleteRenderbuffers
#define glVertexAttribIPointer gOpenGLFunctions.glVertexAttribIPointer
#define glUniform4fv gOpenGLFunctions.glUniform4fv
#define glLineWidth gOpenGLFunctions.glLineWidth
#define glGetShaderiv gOpenGLFunctions.glGetShaderiv
#define glGetShaderInfoLog gOpenGLFunctions.glGetShaderInfoLog

#define GL(x) \
    { \
        GLClearError(); \
        x; \
        if (GLLogError()) \
            __debugbreak(); \
    }

#define GLDELETE(type, x) \
    { \
        if (x != 0) \
            glDelete##type(1, &x); \
        x = 0; \
    }

#define GLDELETE_FRAMEBUFFER(x) GLDELETE(Framebuffers, x)
#define GLDELETE_TEXTURE(x) GLDELETE(Textures, x)
#define GLDELETE_VERTEXARRAY(x) GLDELETE(VertexArrays, x)
#define GLDELETE_RENDERBUFFER(x) GLDELETE(Renderbuffers, x)
#define GLDELETE_BUFFER(x) GLDELETE(Buffers, x)

void GLClearError();
bool GLLogError();

}  // namespace CAMSimulator
