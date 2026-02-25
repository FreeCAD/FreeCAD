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

#include "DlgCAMSimulator.h"
#define gSimWindow CAMSimulator::DlgCAMSimulator::instance()
#define glClearColor gSimWindow->glClearColor
#define glBlendFunc gSimWindow->glBlendFunc
#define glClear gSimWindow->glClear
#define glGenBuffers gSimWindow->glGenBuffers
#define glBindBuffer gSimWindow->glBindBuffer
#define glBufferData gSimWindow->glBufferData
#define glGenVertexArrays gSimWindow->glGenVertexArrays
#define glBindVertexArray gSimWindow->glBindVertexArray
#define glEnableVertexAttribArray gSimWindow->glEnableVertexAttribArray
#define glVertexAttribPointer gSimWindow->glVertexAttribPointer
#define glShaderSource gSimWindow->glShaderSource
#define glCompileShader gSimWindow->glCompileShader
#define glDeleteShader gSimWindow->glDeleteShader
#define glDeleteProgram gSimWindow->glDeleteProgram
#define glAttachShader gSimWindow->glAttachShader
#define glLinkProgram gSimWindow->glLinkProgram
#define glGetProgramiv gSimWindow->glGetProgramiv
#define glGetUniformLocation gSimWindow->glGetUniformLocation
#define glGetError gSimWindow->glGetError
#define glEnable gSimWindow->glEnable
#define glColorMask gSimWindow->glColorMask
#define glCullFace gSimWindow->glCullFace
#define glDepthFunc gSimWindow->glDepthFunc
#define glStencilFunc gSimWindow->glStencilFunc
#define glStencilOp gSimWindow->glStencilOp
#define glDepthMask gSimWindow->glDepthMask
#define glDisable gSimWindow->glDisable
#define glMatrixMode gSimWindow->glMatrixMode
#define glUseProgram gSimWindow->glUseProgram
#define glDrawElements gSimWindow->glDrawElements
#define glDeleteVertexArrays gSimWindow->glDeleteVertexArrays
#define glUniformMatrix4fv gSimWindow->glUniformMatrix4fv
#define glUniform3fv gSimWindow->glUniform3fv
#define glUniform1i gSimWindow->glUniform1i
#define glCreateShader gSimWindow->glCreateShader
#define glCreateProgram gSimWindow->glCreateProgram
#define glDeleteBuffers gSimWindow->glDeleteBuffers
#define glActiveTexture gSimWindow->glActiveTexture
#define glBindTexture gSimWindow->glBindTexture
#define glGenTextures gSimWindow->glGenTextures
#define glTexParameteri gSimWindow->glTexParameteri
#define glTexImage2D gSimWindow->glTexImage2D
#define glDeleteTextures gSimWindow->glDeleteTextures
#define glPolygonOffset gSimWindow->glPolygonOffset

#define glBindFramebuffer gSimWindow->glBindFramebuffer
#define glUniform1f gSimWindow->glUniform1f
#define glGenFramebuffers gSimWindow->glGenFramebuffers
#define glFramebufferTexture2D gSimWindow->glFramebufferTexture2D
#define glDrawBuffers gSimWindow->glDrawBuffers
#define glGenRenderbuffers gSimWindow->glGenRenderbuffers
#define glBindRenderbuffer gSimWindow->glBindRenderbuffer
#define glRenderbufferStorage gSimWindow->glRenderbufferStorage
#define glFramebufferRenderbuffer gSimWindow->glFramebufferRenderbuffer
#define glCheckFramebufferStatus gSimWindow->glCheckFramebufferStatus
#define glDeleteFramebuffers gSimWindow->glDeleteFramebuffers
#define glDeleteRenderbuffers gSimWindow->glDeleteRenderbuffers
#define glVertexAttribIPointer gSimWindow->glVertexAttribIPointer
#define glUniform4fv gSimWindow->glUniform4fv
#define glLineWidth gSimWindow->glLineWidth
#define glGetShaderiv gSimWindow->glGetShaderiv
#define glGetShaderInfoLog gSimWindow->glGetShaderInfoLog
