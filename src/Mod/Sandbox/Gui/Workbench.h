// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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

#include <Gui/Workbench.h>
#include <Inventor/nodes/SoShape.h>
#include <QPointer>
#include <QImage>

namespace SandboxGui {

class Workbench : public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER();

public:
    Workbench();
    virtual ~Workbench();

protected:
    Gui::MenuItem* setupMenuBar() const;
    Gui::ToolBarItem* setupToolBars() const;
    Gui::ToolBarItem* setupCommandBars() const;
    Gui::DockWindowItems* setupDockWindows() const;
};

class SoWidgetShape : public SoShape {
    using inherited = SoShape;

    SO_NODE_HEADER(SoWidgetShape);

public:
    static void initClass();
    SoWidgetShape();
    void setWidget(QWidget* w);

protected:
    virtual void GLRender(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
    virtual void generatePrimitives(SoAction *action);
    void getQuad(SoState * state, SbVec3f & v0, SbVec3f & v1, SbVec3f & v2, SbVec3f & v3);

private:
    virtual ~SoWidgetShape(){};
    QPointer<QWidget> w;
    QImage image;
};

} // namespace SandboxGui