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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QObject>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/elements/SoCacheElement.h>
# include <Inventor/elements/SoLazyElement.h>
# include <Inventor/elements/SoModelMatrixElement.h>
# include <Inventor/elements/SoProjectionMatrixElement.h>
# include <Inventor/elements/SoViewingMatrixElement.h>
# include <Inventor/elements/SoViewportRegionElement.h>
#endif

#ifdef FC_OS_MACOSX
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "SoDrawingGrid.h"


using namespace Gui::Inventor;

/*
from pivy import coin
grid=coin.SoType.fromName("SoDrawingGrid").createInstance()
Gui.ActiveDocument.ActiveView.getSceneGraph().addChild(grid)
*/

SO_NODE_SOURCE(SoDrawingGrid)

void
SoDrawingGrid::initClass()
{
    SO_NODE_INIT_CLASS(SoDrawingGrid, SoShape, "Shape");
}

SoDrawingGrid::SoDrawingGrid()
{
    SO_NODE_CONSTRUCTOR(SoDrawingGrid);
}

void
SoDrawingGrid::renderGrid(SoGLRenderAction *action)
{
    if (!shouldGLRender(action))
        return;

    SoState*  state = action->getState();
    state->push();
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);

    const SbMatrix & mat = SoModelMatrixElement::get(state);
    //const SbViewVolume & vv = SoViewVolumeElement::get(state);
    //const SbMatrix & projmatrix = (mat * SoViewingMatrixElement::get(state) *
    //                               SoProjectionMatrixElement::get(state));
    const SbViewportRegion & vp = SoViewportRegionElement::get(state);
    //SbVec2s vpsize = vp.getViewportSizePixels();
    float fRatio = vp.getViewportAspectRatio();

    //float width = vv.getWidth();
    //float height = vv.getHeight();
    SbVec3f worldcenter(0,0,0);
    mat.multVecMatrix(worldcenter, worldcenter);

    //float dist = (vv.getProjectionPoint() - worldcenter).length();

    SoModelMatrixElement::set(state,this,SbMatrix::identity());
    SoViewingMatrixElement::set(state,this,SbMatrix::identity());
    SoProjectionMatrixElement::set(state,this,SbMatrix::identity());

    glColor3f(1.0f,0.0f,0.0f);
    glBegin(GL_LINES);
    float p[3];
    p[2] = 0.0f;

    int numX = 20;
    for (int i=-numX; i<numX; i++) {
        p[0] = (float)i/numX;
        p[1] = -1.0f;
        glVertex3fv(p);
        p[1] = 1.0f;
        glVertex3fv(p);
    }
    int numY = numX / fRatio;
    for (int i=-numY; i<numY; i++) {
        p[0] = -1.0f;
        p[1] = (float)i/numY;
        glVertex3fv(p);
        p[0] = 1.0;
        glVertex3fv(p);
    }
    glEnd();

    state->pop();
}

void
SoDrawingGrid::GLRender(SoGLRenderAction *action)
{
    //renderGrid(action);
    //return;
    switch (action->getCurPathCode()) {
    case SoAction::NO_PATH:
    case SoAction::BELOW_PATH:
        this->GLRenderBelowPath(action);
        break;
    case SoAction::OFF_PATH:
        // do nothing. Separator will reset state.
        break;
    case SoAction::IN_PATH:
        this->GLRenderInPath(action);
        break;
    }
}

void
SoDrawingGrid::GLRenderBelowPath(SoGLRenderAction * action)
{
    //inherited::GLRenderBelowPath(action);
    //return;
    if (action->isRenderingDelayedPaths()) {
        SbBool zbenabled = glIsEnabled(GL_DEPTH_TEST);
        if (zbenabled) glDisable(GL_DEPTH_TEST);
        renderGrid(action);
        if (zbenabled) glEnable(GL_DEPTH_TEST);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        action->addDelayedPath(action->getCurPath()->copy());
    }
}

void
SoDrawingGrid::GLRenderInPath(SoGLRenderAction * action)
{
    //inherited::GLRenderInPath(action);
    //return;
    if (action->isRenderingDelayedPaths()) {
        SbBool zbenabled = glIsEnabled(GL_DEPTH_TEST);
        if (zbenabled) glDisable(GL_DEPTH_TEST);
        renderGrid(action);
        if (zbenabled) glEnable(GL_DEPTH_TEST);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        action->addDelayedPath(action->getCurPath()->copy());
    }
}

void
SoDrawingGrid::GLRenderOffPath(SoGLRenderAction *)
{
}

void
SoDrawingGrid::generatePrimitives(SoAction* action)
{
    Q_UNUSED(action); 
}

void
SoDrawingGrid::computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center)
{
    Q_UNUSED(action); 
    Q_UNUSED(box); 
    Q_UNUSED(center); 
    //SoState*  state = action->getState();
}
