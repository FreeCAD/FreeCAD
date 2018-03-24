/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <Inventor/nodes/SoAsciiText.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoRotation.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
#endif

#include "TaskDatumParameters.h"
#include <Mod/Part/Gui/SoBrepEdgeSet.h>

#include "ViewProviderDatumCS.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumCoordinateSystem,PartDesignGui::ViewProviderDatum)

ViewProviderDatumCoordinateSystem::ViewProviderDatumCoordinateSystem()
{
    sPixmap = "PartDesign_CoordinateSystem.svg";

    coord = new SoCoordinate3();
    coord->ref();
    font = new SoFont();
    font->ref();
    axisLabelXTrans = new SoTranslation();
    axisLabelXTrans->ref();
    axisLabelXToYTrans = new SoTranslation();
    axisLabelXToYTrans->ref();
    axisLabelYToZTrans = new SoTranslation();
    axisLabelYToZTrans->ref();
}

ViewProviderDatumCoordinateSystem::~ViewProviderDatumCoordinateSystem()
{
    coord->unref();
    font->unref();
    axisLabelXTrans->unref();
    axisLabelXToYTrans->unref();
    axisLabelYToZTrans->unref();
}

void ViewProviderDatumCoordinateSystem::attach ( App::DocumentObject *obj ) {
    ViewProviderDatum::attach ( obj );

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setNum(4);
    material->diffuseColor.set1Value(0, SbColor(0.f, 0.f, 0.f));
    material->diffuseColor.set1Value(1, SbColor(1.f, 0.f, 0.f));
    material->diffuseColor.set1Value(2, SbColor(0.f, 1.f, 0.f));
    material->diffuseColor.set1Value(3, SbColor(0.f, 0.f, 1.f));
    SoMaterialBinding* binding = new SoMaterialBinding();
    binding->value = SoMaterialBinding::PER_FACE_INDEXED;

    getShapeRoot ()->addChild(binding);
    getShapeRoot ()->addChild(material);

    coord->point.setNum(4);

    ViewProviderDatum::setExtents ( defaultBoundBox () );

    getShapeRoot ()->addChild(coord);

    SoDrawStyle* style = new SoDrawStyle ();
    style->lineWidth = 1.5f;
    getShapeRoot ()->addChild(style);

    PartGui::SoBrepEdgeSet* lineSet = new PartGui::SoBrepEdgeSet();
    lineSet->coordIndex.setNum(9);
    // X
    lineSet->coordIndex.set1Value(0, 0);
    lineSet->coordIndex.set1Value(1, 1);
    lineSet->coordIndex.set1Value(2, SO_END_LINE_INDEX);
    // Y
    lineSet->coordIndex.set1Value(3, 0);
    lineSet->coordIndex.set1Value(4, 2);
    lineSet->coordIndex.set1Value(5, SO_END_LINE_INDEX);
    // Z
    lineSet->coordIndex.set1Value(6, 0);
    lineSet->coordIndex.set1Value(7, 3);
    lineSet->coordIndex.set1Value(8, SO_END_LINE_INDEX);

    lineSet->materialIndex.setNum(3);
    lineSet->materialIndex.set1Value(0,1);
    lineSet->materialIndex.set1Value(1,2);
    lineSet->materialIndex.set1Value(2,3);
    getShapeRoot ()->addChild(lineSet);

    getShapeRoot ()->addChild(font);

    // Transformation for axis labels are relative so no need in separators
    getShapeRoot ()->addChild(axisLabelXTrans);
    SoAsciiText* t = new SoAsciiText();
    t->string = "X";
    getShapeRoot ()->addChild(t);

    getShapeRoot ()->addChild(axisLabelXToYTrans);
    t = new SoAsciiText();
    t->string = "Y";
    getShapeRoot ()->addChild(t);

    getShapeRoot ()->addChild(axisLabelYToZTrans);
    SoRotation *rot = new SoRotation();
    rot->rotation = SbRotation(SbVec3f(1,1,1), static_cast<float>(2*M_PI/3));
    getShapeRoot ()->addChild(rot);
    t = new SoAsciiText();
    t->string = "Z";
    getShapeRoot ()->addChild(t);
}

void ViewProviderDatumCoordinateSystem::updateData(const App::Property* prop)
{
    if (strcmp(prop->getName(),"Placement") == 0) {
        updateExtents ();
    }

    ViewProviderDatum::updateData(prop);
}

void ViewProviderDatumCoordinateSystem::setExtents (Base::BoundBox3d bbox) {
    // Axis length of the CS is 1/3 of maximum bbox dimension, any smarter sizing will make it only worse
    double axisLength = std::max ( { bbox.LengthX (), bbox.LengthY(), bbox.LengthZ() } );
    axisLength *= (1 + marginFactor ()) / 3;

    coord->point.set1Value ( 0, 0, 0, 0 );
    coord->point.set1Value ( 1, axisLength, 0, 0 );
    coord->point.set1Value ( 2, 0, axisLength, 0 );
    coord->point.set1Value ( 3, 0, 0, axisLength );

    double fontSz = axisLength / 10.;
    font->size = fontSz;

    double labelPos = 9./10.*axisLength;
    double labelOffset = fontSz/8.;

    // offset 1 pixel
    axisLabelXTrans->translation.setValue ( SbVec3f( labelPos, labelOffset, 0) );
    axisLabelXToYTrans->translation.setValue ( SbVec3f( -labelPos + labelOffset, labelPos - labelOffset, 0) );
    axisLabelYToZTrans->translation.setValue ( SbVec3f( -labelOffset, -labelPos + labelOffset, labelPos) );
}
