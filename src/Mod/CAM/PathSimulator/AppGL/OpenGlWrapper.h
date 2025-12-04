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

#ifndef __openglwrapper_h__
#define __openglwrapper_h__

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

}  // namespace CAMSimulator

#endif  // !__openglwrapper_h__
