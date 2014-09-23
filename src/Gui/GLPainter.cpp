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
#include <Base/Console.h>

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
    if(viewer)
        return false;

    viewer = v;

    // Make current context
    SbVec2s view = viewer->getSoRenderManager()->getSize();
    this->width = view[0];
    this->height = view[1];

    viewer->makeCurrent();

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
    if(!viewer)
        return false;

    glFlush();

    if(this->logicOp) {
        this->logicOp = false;
        glDisable(GL_COLOR_LOGIC_OP);
    }

    if(this->lineStipple) {
        this->lineStipple = false;
        glDisable(GL_LINE_STIPPLE);
    }

    // Reset original state
    glDepthRange(this->depthrange[0], this->depthrange[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(this->projectionmatrix);

    glPopAttrib();
    glPopMatrix();

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
    if(!viewer)
        return;

    glBegin(GL_LINE_LOOP);
    glVertex3i(x1, this->height-y1, 0);
    glVertex3i(x2, this->height-y1, 0);
    glVertex3i(x2, this->height-y2, 0);
    glVertex3i(x1, this->height-y2, 0);
    glEnd();
}

void GLPainter::drawLine(int x1, int y1, int x2, int y2)
{
    if(!viewer)
        return;

    glBegin(GL_LINES);
    glVertex3i(x1, this->height-y1, 0);
    glVertex3i(x2, this->height-y2, 0);
    glEnd();
}

void GLPainter::drawPoint(int x, int y)
{
    if(!viewer)
        return;

    glBegin(GL_POINTS);
    glVertex3i(x, this->height-y, 0);
    glEnd();
}



Rubberband::Rubberband(Gui::View3DInventorViewer* v) : viewer(v)
{
    x_old = y_old = x_new = y_new = 0;
    working = false;
}

Rubberband::Rubberband()
{
    x_old = y_old = x_new = y_new = 0;
    working = false;
}

Rubberband::~Rubberband()
{
}

void Rubberband::setWorking(bool on)
{
    working = on;
}

void Rubberband::setViewer(View3DInventorViewer* v)
{
    viewer = v;
}

void Rubberband::setCoords(int x1, int y1, int x2, int y2)
{
    x_old = x1;
    y_old = y1;
    x_new = x2;
    y_new = y2;
}

void Rubberband::paintGL()
{
    if(!working)
        return;

    const SbViewportRegion vp = viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();

    
    glMatrixMode(GL_PROJECTION);

    glOrtho(0, size[0], size[1], 0, 0, 100);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(4.0);
    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
    glRecti(x_old, y_old, x_new, y_new);
    glColor4f(0.0, 0.0, 1.0, 1.0);
    glLineStipple(3, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);

    glLineWidth(4.0);
    glColor4f(0.0, 0.0, 1.0, 1.0);
    glLineStipple(3, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINE_LOOP);
    glVertex2i(x_old, y_old);
    glVertex2i(x_old, y_new);
    glVertex2i(x_new, y_new);
    glVertex2i(x_new, y_old);
    glEnd();

    glLineWidth(1.0);
    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_BLEND);
}

