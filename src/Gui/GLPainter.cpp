/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "GLPainter.h"
#include "View3DInventorViewer.h"

using namespace Gui;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::GLGraphicsItem, Base::BaseClass);

GLPainter::GLPainter() : viewer(0), logicOp(false), lineStipple(false)
{
}

GLPainter::~GLPainter()
{
    end();
}

bool GLPainter::begin(View3DInventorViewer* v)
{
    if (viewer)
        return false;
    viewer = v;

    // Make current context
    SbVec2s view = viewer->getGLSize();
    this->width = view[0];
    this->height = view[1];

    viewer->glLockNormal();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, this->width, 0, this->height, -1, 1);

    // Store GL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glGetFloatv(GL_DEPTH_RANGE, this->depthrange);
    glGetDoublev(GL_PROJECTION_MATRIX, this->projectionmatrix);

    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glDepthRange(0,0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);

    glLineWidth(1.0f);
    glColor4f(1.0, 1.0, 1.0, 0.0);
    glViewport(0, 0, this->width, this->height);
    glDrawBuffer(GL_FRONT);

    return true;
}

bool GLPainter::end()
{
    if (!viewer)
        return false;

    glFlush();
    if (this->logicOp) {
        this->logicOp = false;
        glDisable(GL_COLOR_LOGIC_OP);
    }
    if (this->lineStipple) {
        this->lineStipple = false;
        glDisable(GL_LINE_STIPPLE);
    }

    // Reset original state
    glDepthRange(this->depthrange[0], this->depthrange[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(this->projectionmatrix);

    glPopAttrib();
    glPopMatrix();
    
    // Release the context
    viewer->glUnlockNormal();

    viewer = 0;
    return true;
}

bool GLPainter::isActive() const
{
    return viewer != 0;
}

void GLPainter::setLineWidth(float w)
{
    glLineWidth(w);
}

void GLPainter::setPointSize(float s)
{
    glPointSize(s);
}

void GLPainter::setColor(float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
}

void GLPainter::setLogicOp(GLenum mode)
{
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(mode);
    this->logicOp = true;
}

void GLPainter::resetLogicOp()
{
    glDisable(GL_COLOR_LOGIC_OP);
    this->logicOp = false;
}

void GLPainter::setDrawBuffer(GLenum mode)
{
    glDrawBuffer(mode);
}

void GLPainter::setLineStipple(GLint factor, GLushort pattern)
{
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(factor, pattern);
    this->lineStipple = true;
}

void GLPainter::resetLineStipple()
{
    glDisable(GL_LINE_STIPPLE);
    this->lineStipple = false;
}

// Draw routines
void GLPainter::drawRect(int x1, int y1, int x2, int y2)
{
    if (!viewer)
        return;

    glBegin(GL_LINE_LOOP);
        glVertex3i(x1, this->height-y1, 0);
        glVertex3i(x2, this->height-y1, 0);
        glVertex3i(x2, this->height-y2, 0);
        glVertex3i(x1, this->height-y2, 0);
    glEnd();
}

void GLPainter::drawLine (int x1, int y1, int x2, int y2)
{
    if (!viewer)
        return;

    glBegin(GL_LINES);
        glVertex3i(x1, this->height-y1, 0);
        glVertex3i(x2, this->height-y2, 0);
    glEnd();
}

void GLPainter::drawPoint(int x, int y)
{
    if (!viewer)
        return;

    glBegin(GL_POINTS);
        glVertex3i(x, this->height-y, 0);
    glEnd();
}
