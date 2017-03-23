/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef QUARTER_QTOPENGL_H
#define QUARTER_QTOPENGL_H

/* QtOpenGL.h.  Generated from QtOpenGL.h.cmake by cmake.  */

#cmakedefine HAVE_QT5_OPENGL

#if defined(HAVE_QT5_OPENGL)

#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>
#include <QOpenGLVersionProfile>
#include <QOpenGLFunctions>

typedef QOpenGLContext QtGLContext;
typedef QSurfaceFormat QtGLFormat;
typedef QOpenGLWidget QtGLWidget;
typedef QOpenGLFramebufferObject QtGLFramebufferObject;
typedef QOpenGLFramebufferObjectFormat QtGLFramebufferObjectFormat;

#else // HAVE_QT5_OPENGL

#include <QGLContext>
#include <QGLFormat>
#include <QGLWidget>
#include <QGLPixelBuffer>
#include <QGLFramebufferObject>

typedef QGLContext QtGLContext;
typedef QGLFormat QtGLFormat;
typedef QGLWidget QtGLWidget;
typedef QGLPixelBuffer QtGLPixelBuffer;
typedef QGLFramebufferObject QtGLFramebufferObject;
typedef QGLFramebufferObjectFormat QtGLFramebufferObjectFormat;

#endif // HAVE_QT5_OPENGL

#endif //QUARTER_QTOPENGL_H
