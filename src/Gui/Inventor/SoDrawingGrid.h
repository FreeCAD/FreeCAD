/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_INVENTOR_SODRAWINGGRID_H
#define GUI_INVENTOR_SODRAWINGGRID_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoShape.h>

namespace Gui { namespace Inventor {

class GuiExport SoDrawingGrid : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(SoDrawingGrid);

public:
    static void initClass();
    SoDrawingGrid();

public:
    virtual void GLRender(SoGLRenderAction *action);
    virtual void GLRenderBelowPath(SoGLRenderAction *action);
    virtual void GLRenderInPath(SoGLRenderAction *action);
    virtual void GLRenderOffPath(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
    virtual void generatePrimitives(SoAction *action);

private:
    void renderGrid(SoGLRenderAction *action);
    // Force using the reference count mechanism.
    virtual ~SoDrawingGrid() {}
};

} // namespace Inventor

} // namespace Gui

#endif // GUI_INVENTOR_SODRAWINGGRID_H
