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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# endif

# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/bundles/SoMaterialBundle.h>
# include <Inventor/bundles/SoTextureCoordinateBundle.h>
# include <Inventor/elements/SoLazyElement.h>
# include <Inventor/elements/SoModelMatrixElement.h>
# include <Inventor/elements/SoViewportRegionElement.h>
# include <Inventor/elements/SoViewVolumeElement.h>
# include <Inventor/nodekits/SoShapeKit.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCone.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoFontStyle.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoScale.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTranslation.h>
#endif

#include "SoAxisCrossKit.h"


using namespace Gui;


SO_KIT_SOURCE(SoShapeScale)

//  Constructor.
SoShapeScale::SoShapeScale()
{
    SO_KIT_CONSTRUCTOR(SoShapeScale);

    SO_KIT_ADD_FIELD(active, (true));
    SO_KIT_ADD_FIELD(scaleFactor, (1.0f));

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, false, this, "", false);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SoNode, SoCube, true, topSeparator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(scale, SoScale, false, topSeparator, shape, false);

    SO_KIT_INIT_INSTANCE();
}

// Destructor.
SoShapeScale::~SoShapeScale() = default;

void
SoShapeScale::initClass()
{
    SO_KIT_INIT_CLASS(SoShapeScale, SoBaseKit, "BaseKit");
}

void
SoShapeScale::GLRender(SoGLRenderAction * action)
{
    SoState * state = action->getState();

    SoScale * scale = static_cast<SoScale*>(this->getAnyPart(SbName("scale"), true));
    if (!this->active.getValue()) {
        SbVec3f v(1.0f, 1.0f, 1.0f);
        if (scale->scaleFactor.getValue() != v)
            scale->scaleFactor = v;
    }
    else {
        const SbViewportRegion & vp = SoViewportRegionElement::get(state);
        const SbViewVolume & vv = SoViewVolumeElement::get(state);
        SbVec3f center(0.0f, 0.0f, 0.0f);
        float nsize = this->scaleFactor.getValue() / float(vp.getViewportSizePixels()[1]);
        SoModelMatrixElement::get(state).multVecMatrix(center, center); // world coords
        float sf = vv.getWorldToScreenScale(center, nsize);
        SbVec3f v(sf, sf, sf);
        if (scale->scaleFactor.getValue() != v)
            scale->scaleFactor = v;
    }

    inherited::GLRender(action);
}

// --------------------------------------------------------------

SO_KIT_SOURCE(SoAxisCrossKit)

void
SoAxisCrossKit::initClass()
{
   SO_KIT_INIT_CLASS(SoAxisCrossKit,SoBaseKit, "BaseKit");
}

SoAxisCrossKit::SoAxisCrossKit()
{
   SO_KIT_CONSTRUCTOR(SoAxisCrossKit);

   // Add the parts to the catalog...
   SO_KIT_ADD_CATALOG_ENTRY(xAxis, SoShapeKit,
                            true, this,"", true);
   SO_KIT_ADD_CATALOG_ENTRY(xHead, SoShapeKit,
                            true, this,"", true);
   SO_KIT_ADD_CATALOG_ENTRY(yAxis, SoShapeKit,
                            true, this,"", true);
   SO_KIT_ADD_CATALOG_ENTRY(yHead, SoShapeKit,
                            true, this,"", true);
   SO_KIT_ADD_CATALOG_ENTRY(zAxis, SoShapeKit,
                            true, this,"", true);
   SO_KIT_ADD_CATALOG_ENTRY(zHead, SoShapeKit,
                            true, this,"", true);

   SO_KIT_INIT_INSTANCE();

   createAxes();
}

SoAxisCrossKit::~SoAxisCrossKit() = default;

// This kit is made up entirely of SoShapeKits.
// Since SoShapeKits do not affect state, neither does this.
SbBool
SoAxisCrossKit::affectsState() const
{
   return false;
}

void SoAxisCrossKit::addWriteReference(SoOutput * /*out*/, SbBool /*isfromfield*/)
{
    // this node should not be written out to a file
}

void SoAxisCrossKit::getBoundingBox(SoGetBoundingBoxAction * action)
{
    inherited::getBoundingBox(action);
    action->resetCenter();
    action->setCenter(SbVec3f(0,0,0), false);
}

// Set up parts for default configuration of the jumping jack
void
SoAxisCrossKit::createAxes()
{
   // Create the heads.
   auto head = new SoCone;
   head->bottomRadius.setValue(5);
   head->height.setValue(10);
   setPart("xHead.shape", head);
   setPart("yHead.shape", head);
   setPart("zHead.shape", head);

   // Create the axes.
   auto coords = new SoCoordinate3;
   coords->point.set1Value(0, SbVec3f(0,0,0));
   coords->point.set1Value(1, SbVec3f(90,0,0));
   setPart("xAxis.coordinate3", coords);
   setPart("yAxis.coordinate3", coords);
   setPart("zAxis.coordinate3", coords);

   auto shape = new SoLineSet;
   setPart("xAxis.shape", shape);
   setPart("yAxis.shape", shape);
   setPart("zAxis.shape", shape);

   // Place the axes and heads
   set("yAxis.transform", "rotation 0 0 1 1.5707999");
   set("zAxis.transform", "rotation 0 1 0 -1.5707999");

   set("xHead.transform", "translation 95 0 0");
   set("xHead.transform", "scaleFactor 0.5 1.5 0.5");
   set("xHead.transform", "rotation 0 0 -1  1.5707999");

   set("yHead.transform", "translation 0 95 0");
   set("yHead.transform", "scaleFactor 0.5 1.5 0.5");
   set("yHead.transform", "rotation 0 0 1 0");

   set("zHead.transform", "translation 0 0 95");
   set("zHead.transform", "scaleFactor 0.5 1.5 0.5");
   set("zHead.transform", "rotation 1 0 0  1.5707999");

   // Set colors & styles
   set("xAxis.appearance.lightModel", "model BASE_COLOR");
   set("xHead.appearance.lightModel", "model BASE_COLOR");
   set("yAxis.appearance.lightModel", "model BASE_COLOR");
   set("yHead.appearance.lightModel", "model BASE_COLOR");
   set("zAxis.appearance.lightModel", "model BASE_COLOR");
   set("zHead.appearance.lightModel", "model BASE_COLOR");
   set("xAxis.appearance.drawStyle", "lineWidth 1");
   set("yAxis.appearance.drawStyle", "lineWidth 1");
   set("zAxis.appearance.drawStyle", "lineWidth 1");
   set("xAxis.appearance.material", "diffuseColor 0.5 0.125 0.125");
   set("xHead.appearance.material", "diffuseColor 0.5 0.125 0.125");
   set("yAxis.appearance.material", "diffuseColor 0.125 0.5 0.125");
   set("yHead.appearance.material", "diffuseColor 0.125 0.5 0.125");
   set("zAxis.appearance.material", "diffuseColor 0.125 0.125 0.5");
   set("zHead.appearance.material", "diffuseColor 0.125 0.125 0.5");

   // Make unpickable
   set("xAxis.pickStyle", "style UNPICKABLE");
   set("xHead.pickStyle", "style UNPICKABLE");
   set("yAxis.pickStyle", "style UNPICKABLE");
   set("yHead.pickStyle", "style UNPICKABLE");
   set("zAxis.pickStyle", "style UNPICKABLE");
   set("zHead.pickStyle", "style UNPICKABLE");
}

// --------------------------------------------------------------

SO_NODE_SOURCE(SoRegPoint)

void SoRegPoint::initClass()
{
    SO_NODE_INIT_CLASS(SoRegPoint, SoShape, "Shape");
}

SoRegPoint::SoRegPoint()
{
    SO_NODE_CONSTRUCTOR(SoRegPoint);

    SO_NODE_ADD_FIELD(base, (SbVec3f(0,0,0)));
    SO_NODE_ADD_FIELD(normal, (SbVec3f(1,1,1)));
    SO_NODE_ADD_FIELD(length, (3.0));
    SO_NODE_ADD_FIELD(color, (1.0f, 0.447059f, 0.337255f));
    SO_NODE_ADD_FIELD(text, (""));

    root = new SoSeparator();
    root->ref();

    // translation
    auto move = new SoTranslation();
    move->translation.setValue(base.getValue() + normal.getValue() * length.getValue());
    root->addChild(move);

    // sub-group
    auto col = new SoBaseColor();
    col->rgb.setValue(this->color.getValue());

    auto font = new SoFontStyle;
    font->size = 14;

    auto sub = new SoSeparator();
    sub->addChild(col);
    sub->addChild(font);
    sub->addChild(new SoText2());
    root->addChild(sub);
}

SoRegPoint::~SoRegPoint()
{
    root->unref();
}

/**
 * Renders the probe with text label and a bullet at the base point.
 */
void SoRegPoint::GLRender(SoGLRenderAction *action)
{
    if (shouldGLRender(action))
    {
        SoState*  state = action->getState();
        state->push();
        SoMaterialBundle mb(action);
        SoTextureCoordinateBundle tb(action, true, false);
        SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
        mb.sendFirst();  // make sure we have the correct material

        SbVec3f p1 = base.getValue();
        SbVec3f p2 = p1 + normal.getValue() * length.getValue();

        glLineWidth(1.0f);
        glColor3fv(color.getValue().getValue());
        glBegin(GL_LINE_STRIP);
            glVertex3d(p1[0], p1[1], p1[2]);
            glVertex3d(p2[0], p2[1], p2[2]);
        glEnd();
        glPointSize(5.0f);
        glBegin(GL_POINTS);
            glVertex3fv(p1.getValue());
        glEnd();
        glPointSize(2.0f);
        glBegin(GL_POINTS);
            glVertex3fv(p2.getValue());
        glEnd();

        root->GLRender(action);
        state->pop();
    }
}

void SoRegPoint::generatePrimitives(SoAction* /*action*/)
{
}

/**
 * Sets the bounding box of the probe to \a box and its center to \a center.
 */
void SoRegPoint::computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center)
{
    root->doAction(action);
    if (action->getTypeId().isDerivedFrom(SoGetBoundingBoxAction::getClassTypeId()))
        static_cast<SoGetBoundingBoxAction*>(action)->resetCenter();

    SbVec3f p1 = base.getValue();
    SbVec3f p2 = p1 + normal.getValue() * length.getValue();

    box.extendBy(p1);
    box.extendBy(p2);

    center = box.getCenter();
}

void SoRegPoint::notify(SoNotList * node)
{
    SoField * f = node->getLastField();
    if (f == &this->base || f == &this->normal || f == &this->length) {
        auto move = static_cast<SoTranslation*>(root->getChild(0));
        move->translation.setValue(base.getValue() + normal.getValue() * length.getValue());
    }
    else if (f == &this->color) {
        auto sub = static_cast<SoSeparator*>(root->getChild(1));
        auto col = static_cast<SoBaseColor*>(sub->getChild(0));
        col->rgb = this->color.getValue();
    }
    else if (f == &this->text) {
        auto sub = static_cast<SoSeparator*>(root->getChild(1));
        auto label = static_cast<SoText2*>(sub->getChild(2));
        label->string = this->text.getValue();
    }

    SoShape::notify(node);
}
