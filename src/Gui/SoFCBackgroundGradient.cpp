/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "SoFCBackgroundGradient.h"

using namespace Gui;

SO_NODE_SOURCE(SoFCBackgroundGradient);

void SoFCBackgroundGradient::finish()
{
    atexit_cleanup();
}

/*!
  Constructor.
*/
SoFCBackgroundGradient::SoFCBackgroundGradient()
{
    SO_NODE_CONSTRUCTOR(SoFCBackgroundGradient);
    fCol.setValue(0.5f, 0.5f, 0.8f);
    tCol.setValue(0.7f, 0.7f, 0.9f);
    mCol.setValue(1.0f, 1.0f, 1.0f);
}

/*!
  Destructor.
*/
SoFCBackgroundGradient::~SoFCBackgroundGradient()
{
}

// doc from parent
void SoFCBackgroundGradient::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCBackgroundGradient,SoNode,"Node");
}

void SoFCBackgroundGradient::GLRender (SoGLRenderAction *action)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLE_STRIP);
    if (mCol[0] < 0) {
        glColor3f(fCol[0],fCol[1],fCol[2]); glVertex2f(-1, 1);
        glColor3f(tCol[0],tCol[1],tCol[2]); glVertex2f(-1,-1);
        glColor3f(fCol[0],fCol[1],fCol[2]); glVertex2f( 1, 1);
        glColor3f(tCol[0],tCol[1],tCol[2]); glVertex2f( 1,-1);
    }
    else {
        glColor3f(fCol[0],fCol[1],fCol[2]); glVertex2f(-1, 1);
        glColor3f(mCol[0],mCol[1],mCol[2]); glVertex2f(-1, 0);
        glColor3f(fCol[0],fCol[1],fCol[2]); glVertex2f( 1, 1);
        glColor3f(mCol[0],mCol[1],mCol[2]); glVertex2f( 1, 0);
        glEnd();
        glBegin(GL_TRIANGLE_STRIP);
        glColor3f(mCol[0],mCol[1],mCol[2]); glVertex2f(-1, 0);
        glColor3f(tCol[0],tCol[1],tCol[2]); glVertex2f(-1,-1);
        glColor3f(mCol[0],mCol[1],mCol[2]); glVertex2f( 1, 0);
        glColor3f(tCol[0],tCol[1],tCol[2]); glVertex2f( 1,-1);
    }
    glEnd();

    glPopAttrib();
    glPopMatrix(); // restore modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void SoFCBackgroundGradient::setColorGradient(const SbColor& fromColor,
                                              const SbColor& toColor)
{
    fCol = fromColor;
    tCol = toColor;
    mCol[0] = -1.0f;
}

void SoFCBackgroundGradient::setColorGradient(const SbColor& fromColor,
                                              const SbColor& toColor,
                                              const SbColor& midColor)
{
    fCol = fromColor;
    tCol = toColor;
    mCol = midColor;
}
