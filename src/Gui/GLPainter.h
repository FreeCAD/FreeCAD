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
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#ifdef FC_OS_MACOSX
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <Base/BaseClass.h>
#include <FCGlobal.h>
#include <QtOpenGL.h>
#include <QPoint>

class QPaintDevice;

namespace Gui {
class View3DInventorViewer;
class GuiExport GLPainter
{
public:
    GLPainter();
    virtual ~GLPainter();

    bool begin(QPaintDevice * device);
    bool end();
    bool isActive() const;

    /** @name Setter methods */
    //@{
    void setLineWidth(float);
    void setPointSize(float);
    void setColor(float, float, float, float=0);
    void setLogicOp(GLenum);
    void resetLogicOp();
    void setDrawBuffer(GLenum);
    void setLineStipple(GLint factor, GLushort pattern);
    void resetLineStipple();
    //@}

    /** @name Draw routines */
    //@{
    void drawRect (int x, int y, int w, int h);
    void drawLine (int x1, int y1, int x2, int y2);
    void drawPoint(int x, int y);
    //@}

private:
    QtGLWidget* viewer{nullptr};
    GLfloat depthrange[2];
    GLdouble projectionmatrix[16];
    GLint width{0}, height{0};
    bool logicOp{false};
    bool lineStipple{false};
};

class GuiExport GLGraphicsItem : public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    GLGraphicsItem() = default;
    ~GLGraphicsItem() override  = default;
    virtual void paintGL() = 0;
};

class GuiExport Rubberband : public Gui::GLGraphicsItem
{
    View3DInventorViewer* viewer;
    int x_old, y_old, x_new, y_new;
    float rgb_r, rgb_g, rgb_b, rgb_a;
    bool working, stipple;

public:
    explicit Rubberband(View3DInventorViewer* v);
    Rubberband();
    ~Rubberband() override;
    void setWorking(bool on);
    void setLineStipple(bool on);
    bool isWorking();
    void setViewer(View3DInventorViewer* v);
    void setCoords(int x1, int y1, int x2, int y2);
    void setColor(float r, float g, float b, float a);
    void paintGL() override;
};

class GuiExport Polyline : public Gui::GLGraphicsItem
{
    View3DInventorViewer* viewer;
    std::vector<QPoint> _cNodeVector;
    int x_new, y_new;
    float rgb_r, rgb_g, rgb_b, rgb_a, line;
    bool working, closed, stippled;
    GLPainter p;

public:
    explicit Polyline(View3DInventorViewer* v);
    Polyline();
    ~Polyline() override;
    void setWorking(bool on);
    bool isWorking() const;
    void setViewer(View3DInventorViewer* v);
    void setCoords(int x, int y);
    void setColor(int r, int g, int b, int a=0);
    void setLineWidth(float l);
    void setClosed(bool c);
    void setCloseStippled(bool c);
    void addNode(const QPoint& p);
    void popNode();
    void clear();
    void paintGL() override;
};

} // namespace Gui

#endif  // GUI_GLPAINTER_H

