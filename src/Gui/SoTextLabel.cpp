/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# endif
# include <cfloat>
# include <QFontMetrics>
# include <QPainter>
# include <QPen>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/bundles/SoMaterialBundle.h>
# include <Inventor/elements/SoLazyElement.h>
# include <Inventor/misc/SoState.h>
#endif

#include <Inventor/C/basic.h>
#include <Inventor/draggers/SoTranslate2Dragger.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>

#if COIN_MAJOR_VERSION > 3
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#else
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#endif

#include <QtOpenGL.h>

#include "SoTextLabel.h"
#include "BitmapFactory.h"
#include "SoFCInteractiveElement.h"
#include "Tools.h"


using namespace Gui;

/*!
\code

s="""
  #Inventor V2.1 ascii

  Annotation {
    Translation { translation 4 0 0 }
    FontStyle {
        size 20
        style BOLD
    }
    BaseColor {
        rgb 0.0 0.0 0.0
    }


    SoTextLabel { string ["Text label", "Second line"] backgroundColor 1.0 0.447059 0.337255}
  }
"""

App.ActiveDocument.addObject("App::InventorObject","iv").Buffer=s

\endcode
*/

SO_NODE_SOURCE(SoTextLabel)

void SoTextLabel::initClass()
{
    SO_NODE_INIT_CLASS(SoTextLabel, SoText2, "Text2");
}

SoTextLabel::SoTextLabel()
{
    SO_NODE_CONSTRUCTOR(SoTextLabel);
    SO_NODE_ADD_FIELD(backgroundColor, (SbVec3f(1.0f,1.0f,1.0f)));
    SO_NODE_ADD_FIELD(background, (true));
    SO_NODE_ADD_FIELD(frameSize, (10.0f));
}

/**
 * Renders the label.
 */
void SoTextLabel::GLRender(SoGLRenderAction *action)
{
    if (!this->shouldGLRender(action))
        return;

    // only draw text without background
    if (!this->background.getValue()) {
        inherited::GLRender(action);
        return;
    }

    SoState * state = action->getState();

    state->push();
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);

    SbBox3f box;
    SbVec3f center;
    this->computeBBox(action, box, center);

    if (!SoCullElement::cullTest(state, box, true)) {
        SoMaterialBundle mb(action);
        mb.sendFirst();
        const SbMatrix & mat = SoModelMatrixElement::get(state);
        const SbMatrix & projmatrix = (mat * SoViewingMatrixElement::get(state) *
                                       SoProjectionMatrixElement::get(state));
        const SbViewportRegion & vp = SoViewportRegionElement::get(state);
        SbVec2s vpsize = vp.getViewportSizePixels();

        // font stuff
        SbName fontname = SoFontNameElement::get(state);
        int lines = this->string.getNum();

        // get left bottom corner of the label
        SbVec3f nilpoint(0.0f, 0.0f, 0.0f);
        projmatrix.multVecMatrix(nilpoint, nilpoint);
        nilpoint[0] = (nilpoint[0] + 1.0f) * 0.5f * vpsize[0];
        nilpoint[1] = (nilpoint[1] + 1.0f) * 0.5f * vpsize[1];

        // Unfortunately, the size of the label is stored in the pimpl class of
        // SoText2 which cannot be accessed directly. However, there is a trick
        // to get the required information: set model, viewing and projection
        // matrix to the identity matrix and also view volume to some default
        // values. SoText2::computeBBox() then calls SoText2P::getQuad which
        // returns the sizes in form of the bounding box. These values can be
        // reverse-engineered to get width and height.
        state->push();
        SoModelMatrixElement::set(state,this,SbMatrix::identity());
        SoViewingMatrixElement::set(state,this,SbMatrix::identity());
        SoProjectionMatrixElement::set(state,this,SbMatrix::identity());
        SbViewVolume vv;
        vv.ortho(-1,1,-1,1,-1,1);
        SoViewVolumeElement::set(state,this,vv);

        SbBox3f box;
        SbVec3f center;
        this->computeBBox(action, box, center);
        state->pop();

        float xmin,ymin,zmin,xmax,ymax,zmax;
        box.getBounds(xmin,ymin,zmin,xmax,ymax,zmax);
        SbVec3f v0(xmin,ymax,zmax);
        SbVec3f v1(xmax,ymax,zmax);
        SbVec3f v2(xmax,ymin,zmax);
        SbVec3f v3(xmin,ymin,zmax);
        vv.projectToScreen(v0,v0);
        vv.projectToScreen(v1,v1);
        vv.projectToScreen(v2,v2);
        vv.projectToScreen(v3,v3);

        float width,height;
        width  = (v1[0]-v0[0])*vpsize[0];
        height = (v1[1]-v3[1])*vpsize[1];
        switch (this->justification.getValue()) {
        case SoText2::RIGHT:
            nilpoint[0] -= width;
            break;
        case SoText2::CENTER:
            nilpoint[0] -= 0.5f*width;
            break;
        default:
            break;
        }

        if (lines > 1) {
            nilpoint[1] -= (float(lines-1)/(float)lines*height);
        }

        SbVec3f toppoint = nilpoint;
        toppoint[0] += width;
        toppoint[1] += height;

        // Set new state.
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, vpsize[0], 0, vpsize[1], -1.0f, 1.0f);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);

        state->push();

        // disable textures for all units
        SoGLTextureEnabledElement::set(state, this, false);
#if COIN_MAJOR_VERSION > 3
        SoMultiTextureEnabledElement::set(state, this, false);
#else
        SoGLTexture3EnabledElement::set(state, this, false);
#endif

        glPushAttrib(GL_ENABLE_BIT | GL_PIXEL_MODE_BIT | GL_COLOR_BUFFER_BIT);
        glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

        // color and frame size
        SbColor color = this->backgroundColor.getValue();
        float fs = this->frameSize.getValue();

        // draw background
        glColor3f(color[0], color[1], color[2]);
        glBegin(GL_QUADS);
        glVertex3f(nilpoint[0]-fs,nilpoint[1]-fs,0.0f);
        glVertex3f(toppoint[0]+fs,nilpoint[1]-fs,0.0f);
        glVertex3f(toppoint[0]+fs,toppoint[1]+fs,0.0f);
        glVertex3f(nilpoint[0]-fs,toppoint[1]+fs,0.0f);
        glEnd();

        // pop old state
        glPopClientAttrib();
        glPopAttrib();
        state->pop();

        glPixelStorei(GL_UNPACK_ALIGNMENT,4);
        // Pop old GL matrix state.
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    state->pop();

    inherited::GLRender(action);
}

// ------------------------------------------------------

SO_NODE_SOURCE(SoColorBarLabel)

void SoColorBarLabel::initClass()
{
    SO_NODE_INIT_CLASS(SoColorBarLabel, SoText2, "Text2");
}

SoColorBarLabel::SoColorBarLabel()
{
    SO_NODE_CONSTRUCTOR(SoColorBarLabel);
}

void SoColorBarLabel::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
    inherited::computeBBox(action, box, center);
    if (!box.hasVolume()) {
        SbViewVolume vv = SoViewVolumeElement::get(action->getState());
        // workaround for https://github.com/coin3d/coin/issues/417:
        // extend by 2 percent
        vv.scaleWidth(1.02f);
        SoViewVolumeElement::set(action->getState(), this, vv);
        inherited::computeBBox(action, box, center);
    }
}

// ------------------------------------------------------

SO_NODE_SOURCE(SoStringLabel)

void SoStringLabel::initClass()
{
    SO_NODE_INIT_CLASS(SoStringLabel, SoNode, "Node");
}

SoStringLabel::SoStringLabel()
{
    SO_NODE_CONSTRUCTOR(SoStringLabel);
    SO_NODE_ADD_FIELD(string, (""));
    SO_NODE_ADD_FIELD(textColor, (SbVec3f(1.0f,1.0f,1.0f)));
    SO_NODE_ADD_FIELD(name, ("Helvetica"));
    SO_NODE_ADD_FIELD(size, (12));
}

/**
 * Renders the open edges only.
 */
void SoStringLabel::GLRender(SoGLRenderAction *action)
{
    QtGLWidget* window;
    SoState * state = action->getState();
    state->push();
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoGLWidgetElement::get(state, window);
    if (!window) {
        state->pop();
        return;
    }

    // Enter in 2D screen mode
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
    glEnable(GL_BLEND);

    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    font.setFamily(QLatin1String(this->name.getValue()));
    font.setPixelSize(this->size.getValue());

    glBlendFunc(GL_ONE,GL_SRC_ALPHA);

    // text color
    SbColor color = this->textColor.getValue();
    glColor4f(color[0], color[1], color[2], 1);
    const SbMatrix & mat = SoModelMatrixElement::get(state);
    const SbMatrix & projmatrix = (mat * SoViewingMatrixElement::get(state) *
                                   SoProjectionMatrixElement::get(state));
    SbVec3f nil(0.0f, 0.0f, 0.0f);
    projmatrix.multVecMatrix(nil, nil);
    QStringList list;
    for (int i=0; i<this->string.getNum(); i++)
        list << QLatin1String(this->string[i].getString());

    // Leave 2D screen mode
    glPopAttrib();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    state->pop();
}

// ------------------------------------------------------

SO_NODE_SOURCE(SoFrameLabel)

void SoFrameLabel::initClass()
{
    SO_NODE_INIT_CLASS(SoFrameLabel, SoImage, "Image");
}

SoFrameLabel::SoFrameLabel()
{
    SO_NODE_CONSTRUCTOR(SoFrameLabel);
    SO_NODE_ADD_FIELD(string, (""));
    SO_NODE_ADD_FIELD(textColor, (SbVec3f(1.0f,1.0f,1.0f)));
    SO_NODE_ADD_FIELD(backgroundColor, (SbVec3f(0.0f,0.333f,1.0f)));
    SO_NODE_ADD_FIELD(justification, (LEFT));
    SO_NODE_ADD_FIELD(name, ("Helvetica"));
    SO_NODE_ADD_FIELD(size, (12));
    SO_NODE_ADD_FIELD(frame, (true));
  //SO_NODE_ADD_FIELD(image, (SbVec2s(0,0), 0, NULL));
}

void SoFrameLabel::notify(SoNotList * list)
{
    SoField *f = list->getLastField();
    if (f == &this->string ||
        f == &this->textColor ||
        f == &this->backgroundColor ||
        f == &this->justification ||
        f == &this->name ||
        f == &this->size ||
        f == &this->frame) {
        drawImage();
    }
    inherited::notify(list);
}

void SoFrameLabel::drawImage()
{
    const SbString* s = string.getValues(0);
    int num = string.getNum();
    if (num == 0) {
        this->image = SoSFImage();
        return;
    }

    QFont font(QString::fromLatin1(QByteArray(name.getValue())), size.getValue());
    QFontMetrics fm(font);
    int w = 0;
    int h = fm.height() * num;
    const SbColor& b = backgroundColor.getValue();
    QColor brush;
    brush.setRgbF(b[0],b[1],b[2]);
    const SbColor& t = textColor.getValue();
    QColor front;
    front.setRgbF(t[0],t[1],t[2]);

    QStringList lines;
    for (int i=0; i<num; i++) {
        QString line = QString::fromUtf8(s[i].getString());
        w = std::max<int>(w, QtTools::horizontalAdvance(fm, line));
        lines << line;
    }

    QImage image(w+10,h+10,QImage::Format_ARGB32_Premultiplied);
    image.fill(0x00000000);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    SbBool drawFrame = frame.getValue();
    if (drawFrame) {
        painter.setPen(QPen(QColor(0,0,127), 2, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
        painter.setBrush(QBrush(brush, Qt::SolidPattern));
        QRectF rectangle(0.0, 0.0, w+10, h+10);
        painter.drawRoundedRect(rectangle, 5, 5);
    }

    painter.setPen(front);

    Qt::Alignment align = Qt::AlignVCenter;
    if (justification.getValue() == 0)
        align = Qt::AlignVCenter | Qt::AlignLeft;
    else if (justification.getValue() == 1)
        align = Qt::AlignVCenter | Qt::AlignRight;
    else
        align = Qt::AlignVCenter | Qt::AlignHCenter;
    QString text = lines.join(QLatin1String("\n"));
    painter.setFont(font);
    painter.drawText(5,5,w,h,align,text);
    painter.end();

    SoSFImage sfimage;
    Gui::BitmapFactory().convert(image, sfimage);
    this->image = sfimage;
}

/**
 * Renders the open edges only.
 */
void SoFrameLabel::GLRender(SoGLRenderAction *action)
{
    inherited::GLRender(action);
}

// ------------------------------------------------------

SO_NODE_SOURCE(TranslateManip)

void
TranslateManip::initClass()
{
    SO_NODE_INIT_CLASS(TranslateManip, SoTransformManip,
                       "TransformManip");
}

TranslateManip::TranslateManip()
{
    SO_NODE_CONSTRUCTOR(TranslateManip);

    auto myDrag = new SoTranslate2Dragger;
    setDragger(myDrag);
}

TranslateManip::~TranslateManip() = default;
