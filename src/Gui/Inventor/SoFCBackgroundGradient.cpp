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

#ifndef _PreComp_
#include <array>
#include <boost/math/constants/constants.hpp>
#ifdef FC_OS_WIN32
 #define _USE_MATH_DEFINES
#endif
#include <cmath>
#ifdef FC_OS_MACOSX
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif

#include "SoFCBackgroundGradient.h"

static const std::array <GLfloat[2], 32> big_circle = []{
    static const float pi2 = boost::math::constants::two_pi<float>();
    std::array <GLfloat[2], 32> result; int c = 0;
    for (GLfloat i = 0; i < pi2; i += pi2 / 32, c++) {
        result[c][0] = M_SQRT2*cosf(i); result[c][1] = M_SQRT2*sinf(i);
    }
    return result; }();
static const std::array <GLfloat[2], 32> small_oval = []{
    static const float pi2 = boost::math::constants::two_pi<float>();
    std::array <GLfloat[2], 32> result; int c = 0;
    for (GLfloat i = 0; i < pi2; i += pi2 / 32, c++) {
        result[c][0] = 0.3*M_SQRT2*cosf(i); result[c][1] = M_SQRT1_2*sinf(i);
    }
    return result; }();

using namespace Gui;

SO_NODE_SOURCE(SoFCBackgroundGradient)

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
    gradient = Gradient::LINEAR;
}

/*!
  Destructor.
*/
SoFCBackgroundGradient::~SoFCBackgroundGradient() = default;

// doc from parent
void SoFCBackgroundGradient::initClass()
{
    SO_NODE_INIT_CLASS(SoFCBackgroundGradient,SoNode,"Node");
}

void SoFCBackgroundGradient::GLRender (SoGLRenderAction * /*action*/)
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

    if (gradient == Gradient::LINEAR) {
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
    }
    else { // radial gradient
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(fCol[0], fCol[1], fCol[2]); glVertex2f(0.0f, 0.0f);

        if (mCol[0] < 0) { // simple radial gradient
            glColor3f(tCol[0], tCol[1], tCol[2]);
            for (const GLfloat *vertex : big_circle)
                glVertex2fv( vertex );
            glVertex2fv( big_circle.front() );
        } else {
            glColor3f(mCol[0], mCol[1], mCol[2]);
            for (const GLfloat *vertex : small_oval)
                glVertex2fv( vertex );
            glVertex2fv( small_oval.front() );
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);
            for (std::size_t i = 0; i < small_oval.size(); i++) {
                glColor3f(mCol[0], mCol[1], mCol[2]); glVertex2fv( small_oval[i] );
                glColor3f(tCol[0], tCol[1], tCol[2]); glVertex2fv( big_circle[i] );
            }

            glColor3f(mCol[0], mCol[1], mCol[2]); glVertex2fv( small_oval.front() );
            glColor3f(tCol[0], tCol[1], tCol[2]); glVertex2fv( big_circle.front() );
        }
    } // end of radial gradient
    glEnd();

    glPopAttrib();
    glPopMatrix(); // restore modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void SoFCBackgroundGradient::setGradient(SoFCBackgroundGradient::Gradient grad)
{
    gradient = grad;
}

SoFCBackgroundGradient::Gradient SoFCBackgroundGradient::getGradient() const
{
    return gradient;
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
