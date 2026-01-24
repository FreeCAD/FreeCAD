// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer@users.sourceforge.net>        *
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

#include <Gui/MouseSelection.h>
#include <QColor>
#include <QPoint>
#include <QList>

namespace SandboxGui {
class DrawingPlane : public Gui::BaseMouseSelection
{
public:
    DrawingPlane();
    virtual ~DrawingPlane();

protected:
    void initialize();
    void terminate();
    virtual int mouseButtonEvent( const SoMouseButtonEvent * const e, const QPoint& pos );
    virtual int locationEvent   ( const SoLocation2Event   * const e, const QPoint& pos );
    virtual int keyboardEvent   ( const SoKeyboardEvent    * const e );
    void draw ();

private:
    void drawLineTo(const QPoint &endPoint);

    bool scribbling;
    int myPenWidth;
    float myRadius;
    QColor myPenColor;
    QPoint lastPoint;
    QList<QPoint> selection;

    QOpenGLFramebufferObject* fbo;
};

} // SandboxGui