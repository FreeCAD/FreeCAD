/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
#  include <OpenGL/gl.h>
# else
#  include <GL/gl.h>
# endif
# include <float.h>
# include <algorithm>
# include <Inventor/actions/SoCallbackAction.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGetPrimitiveCountAction.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/actions/SoPickAction.h>
# include <Inventor/actions/SoWriteAction.h>
# include <Inventor/bundles/SoMaterialBundle.h>
# include <Inventor/bundles/SoTextureCoordinateBundle.h>
# include <Inventor/elements/SoCoordinateElement.h>
# include <Inventor/elements/SoGLCacheContextElement.h>
# include <Inventor/errors/SoReadError.h>
# include <Inventor/misc/SoState.h>
#endif

#include "SoFCShapeObject.h"

using namespace PartGui;


SO_NODE_SOURCE(SoFCControlPoints);

void SoFCControlPoints::initClass()
{
    SO_NODE_INIT_CLASS(SoFCControlPoints, SoShape, "Shape");
}

SoFCControlPoints::SoFCControlPoints()
{
    SO_NODE_CONSTRUCTOR(SoFCControlPoints);

    SbVec3f c(1.0f, 0.447059f, 0.337255f);
    SO_NODE_ADD_FIELD(numPolesU, (0));
    SO_NODE_ADD_FIELD(numPolesV, (0));
    SO_NODE_ADD_FIELD(numKnotsU, (0));
    SO_NODE_ADD_FIELD(numKnotsV, (0));
    SO_NODE_ADD_FIELD(lineColor, (c));
}

/**
 * Renders the control points.
 */
void SoFCControlPoints::GLRender(SoGLRenderAction *action)
{
    if (shouldGLRender(action))
    {
        SoState*  state = action->getState();
        const SoCoordinateElement * coords = SoCoordinateElement::getInstance(state);
        if (!coords) return;
        const SbVec3f * points = coords->getArrayPtr3();
        if (!points) return;

        SoMaterialBundle mb(action);
        SoTextureCoordinateBundle tb(action, true, false);
        SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
        mb.sendFirst();  // make sure we have the correct material

        int32_t len = coords->getNum();
        drawControlPoints(points, len);
    }
}

/**
 * Renders the control points and knots.
 */
void SoFCControlPoints::drawControlPoints(const SbVec3f * points,int32_t len) const
{
    glLineWidth(1.0f);
    glColor3fv(lineColor.getValue().getValue());

    uint32_t nCtU=numPolesU.getValue();
    uint32_t nCtV=numPolesV.getValue();
    uint32_t poles = nCtU * nCtV;
    if (poles > (uint32_t)len)
        return; // wrong setup, too few points
    // draw control mesh
    glBegin(GL_LINES);
    for (uint32_t u = 0; u < nCtU-1; ++u) {
        for (uint32_t v = 0; v < nCtV -1; ++v) {
            glVertex3fv(points[u*nCtV+v].getValue());
            glVertex3fv(points[u*nCtV+v+1].getValue());
            glVertex3fv(points[u*nCtV+v].getValue());
            glVertex3fv(points[(u+1)*nCtV+v].getValue());
        }
        glVertex3fv(points[(u+1)*nCtV-1].getValue());
        glVertex3fv(points[(u+2)*nCtV-1].getValue());
    }
    for (uint32_t v = 0; v < nCtV -1; ++v) {
        glVertex3fv(points[(nCtU-1)*nCtV+v].getValue());
        glVertex3fv(points[(nCtU-1)*nCtV+v+1].getValue());
    }
    glEnd();

    // draw poles
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (uint32_t p=0; p<poles; p++) {
        glVertex3fv(points[p].getValue());
    }
    glEnd();

    // draw knots if available
    uint32_t knots=numKnotsU.getValue() * numKnotsV.getValue();
    uint32_t index = poles + knots;
    if (index > (uint32_t)len)
        return; // wrong setup, too few points
    glColor3f(1.0f, 1.0f, 0.0f);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for (uint32_t p=poles; p<index; p++) {
        glVertex3fv(points[p].getValue());
    }
    glEnd();
}

void SoFCControlPoints::generatePrimitives(SoAction* /*action*/)
{
}

/**
 * Sets the bounding box of the mesh to \a box and its center to \a center.
 */
void SoFCControlPoints::computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center)
{
    SoState*  state = action->getState();
    const SoCoordinateElement * coords = SoCoordinateElement::getInstance(state);
    if (!coords) return;
    const SbVec3f * points = coords->getArrayPtr3();
    if (!points) return;
    float maxX=-FLT_MAX, minX=FLT_MAX,
          maxY=-FLT_MAX, minY=FLT_MAX,
          maxZ=-FLT_MAX, minZ=FLT_MAX;
    int32_t len = coords->getNum();
    if (len > 0) {
        for (int32_t i=0; i<len; i++) {
            maxX = std::max<float>(maxX,points[i][0]);
            minX = std::min<float>(minX,points[i][0]);
            maxY = std::max<float>(maxY,points[i][1]);
            minY = std::min<float>(minY,points[i][1]);
            maxZ = std::max<float>(maxZ,points[i][2]);
            minZ = std::min<float>(minZ,points[i][2]);
        }

        box.setBounds(minX,minY,minZ,maxX,maxY,maxZ);
        center.setValue(0.5f*(minX+maxX),0.5f*(minY+maxY),0.5f*(minZ+maxZ));
    }
    else {
        box.setBounds(SbVec3f(0,0,0), SbVec3f(0,0,0));
        center.setValue(0.0f,0.0f,0.0f);
    }
}
