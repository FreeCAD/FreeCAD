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


#ifndef GUI_GLPAINTER_H
#define GUI_GLPAINTER_H

#ifdef FC_OS_WIN32
#include <windows.h>
#endif
#ifdef FC_OS_MACOSX
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <Base/BaseClass.h>

namespace Gui {
class View3DInventorViewer;
class GuiExport GLPainter
{
public:
    GLPainter();
    virtual ~GLPainter();

    bool begin(View3DInventorViewer*);
    bool end();
    bool isActive() const;

    /** @name Setter methods */
    //@{
    void setLineWidth(float);
    void setPointSize(float);
    void setColor(float, float, float, float=0);
    void setLogicOp(GLenum);
    void setDrawBuffer(GLenum);
    void setLineStipple(GLint factor, GLushort pattern);
    //@}

    /** @name Draw routines */
    //@{
    void drawRect (int x, int y, int w, int h);
    void drawLine (int x1, int y1, int x2, int y2);
    void drawPoint(int x, int y);
    //@}

private:
    View3DInventorViewer* viewer;
    GLfloat depthrange[2];
    GLdouble projectionmatrix[16];
    GLint width, height;
    bool logicOp;
    bool lineStipple;
};

class GuiExport GLGraphicsItem : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    GLGraphicsItem()
    {
    }
    virtual ~GLGraphicsItem()
    {
    }
    virtual void paintGL() = 0;
};

} // namespace Gui

#endif  // GUI_GLPAINTER_H

