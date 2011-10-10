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
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoShape.h>
# include <Inventor/nodes/SoScale.h>
# include <Inventor/nodes/SoCone.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodekits/SoShapeKit.h>
# include <Inventor/elements/SoViewportRegionElement.h>
# include <Inventor/elements/SoViewVolumeElement.h>
# include <Inventor/elements/SoModelMatrixElement.h>
#endif


#include "SoAxisCrossKit.h"

using namespace Gui;


SO_KIT_SOURCE(SoShapeScale);

//  Constructor.
SoShapeScale::SoShapeScale(void)
{
    SO_KIT_CONSTRUCTOR(SoShapeScale);

    SO_KIT_ADD_FIELD(active, (TRUE));
    SO_KIT_ADD_FIELD(scaleFactor, (1.0f));

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, "", FALSE);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SoNode, SoCube, TRUE, topSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(scale, SoScale, FALSE, topSeparator, shape, FALSE);

    SO_KIT_INIT_INSTANCE();
}

// Destructor.
SoShapeScale::~SoShapeScale()
{
}

void
SoShapeScale::initClass(void)
{
    SO_KIT_INIT_CLASS(SoShapeScale, SoBaseKit, "BaseKit");
}

void
SoShapeScale::GLRender(SoGLRenderAction * action)
{
    SoState * state = action->getState();

    SoScale * scale = static_cast<SoScale*>(this->getAnyPart(SbName("scale"), TRUE));
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

SO_KIT_SOURCE(SoAxisCrossKit);

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
                            TRUE, this,"", TRUE);
   SO_KIT_ADD_CATALOG_ENTRY(xHead, SoShapeKit, 
                            TRUE, this,"", TRUE);
   SO_KIT_ADD_CATALOG_ENTRY(yAxis, SoShapeKit, 
                            TRUE, this,"", TRUE);
   SO_KIT_ADD_CATALOG_ENTRY(yHead, SoShapeKit, 
                            TRUE, this,"", TRUE);
   SO_KIT_ADD_CATALOG_ENTRY(zAxis, SoShapeKit, 
                            TRUE, this,"", TRUE);
   SO_KIT_ADD_CATALOG_ENTRY(zHead, SoShapeKit, 
                            TRUE, this,"", TRUE);

   SO_KIT_INIT_INSTANCE();

   createAxes();
}

SoAxisCrossKit::~SoAxisCrossKit()
{
}

// This kit is made up entirely of SoShapeKits.
// Since SoShapeKits do not affect state, neither does this.
SbBool
SoAxisCrossKit::affectsState() const
{
   return FALSE;
}

void SoAxisCrossKit::addWriteReference(SoOutput * out, SbBool isfromfield)
{
    // this node should not be written out to a file
}

void SoAxisCrossKit::getBoundingBox(SoGetBoundingBoxAction * action)
{
    inherited::getBoundingBox(action);
    action->resetCenter();
    action->setCenter(SbVec3f(0,0,0), FALSE);
}

// Set up parts for default configuration of the jumping jack
void
SoAxisCrossKit::createAxes()
{
   // Create the heads.
   SoCone *head = new SoCone;
   head->bottomRadius.setValue(5);
   head->height.setValue(10);
   setPart("xHead.shape", head);
   setPart("yHead.shape", head);
   setPart("zHead.shape", head);

   // Create the axes.
   SoCoordinate3* coords = new SoCoordinate3;
   coords->point.set1Value(0, SbVec3f(0,0,0));
   coords->point.set1Value(1, SbVec3f(90,0,0));
   setPart("xAxis.coordinate3", coords);
   setPart("yAxis.coordinate3", coords);
   setPart("zAxis.coordinate3", coords);

   SoLineSet *shape = new SoLineSet;
   setPart("xAxis.shape", shape);
   setPart("yAxis.shape", shape);
   setPart("zAxis.shape", shape);

   // Place the axes ând heads
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
