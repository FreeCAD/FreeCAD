/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/SbColor.h>
#endif

#include "ViewProviderPlane.h"
#include "ViewProviderOrigin.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderPlane, Gui::ViewProviderOriginFeature)


ViewProviderPlane::ViewProviderPlane()
{
    sPixmap = "Std_Plane";
}

ViewProviderPlane::~ViewProviderPlane() = default;

void ViewProviderPlane::attach ( App::DocumentObject *obj ) {
    ViewProviderOriginFeature::attach ( obj );
    static const float size = ViewProviderOrigin::defaultSize ();

    static const SbVec3f verts[4] = {
        SbVec3f(size,size,0),   SbVec3f(size,-size,0),
        SbVec3f(-size,-size,0), SbVec3f(-size,size,0),
    };

    // indexes used to create the edges
    static const int32_t lines[6] = { 0, 1, 2, 3, 0, -1 };

    SoSeparator *sep = getOriginFeatureRoot ();

    auto pCoords = new SoCoordinate3 ();
    pCoords->point.setNum (4);
    pCoords->point.setValues ( 0, 4, verts );
    sep->addChild ( pCoords );

    auto pLines  = new SoIndexedLineSet ();
    pLines->coordIndex.setNum(6);
    pLines->coordIndex.setValues(0, 6, lines);
    sep->addChild ( pLines );

    // add semi transparent face
    auto faceSeparator = new SoSeparator();
    sep->addChild(faceSeparator);

    auto material = new SoMaterial();
    material->transparency.setValue(0.95f);
    auto color = new SbColor();
    float alpha = 0.0f;
    color->setPackedValue(ViewProviderOrigin::defaultColor, alpha);
    material->ambientColor.setValue(*color);
    material->diffuseColor.setValue(*color);
    faceSeparator->addChild(material);

    // disable backface culling and render with two-sided lighting
    auto shapeHints = new SoShapeHints();
    shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    faceSeparator->addChild(shapeHints);

    // disable picking
    auto pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    faceSeparator->addChild(pickStyle);

    auto faceSet = new SoFaceSet();
    auto vertexProperty = new SoVertexProperty();
    vertexProperty->vertex.setValues(0, 4, verts);
    faceSet->vertexProperty.setValue(vertexProperty);
    faceSeparator->addChild(faceSet);

    auto textTranslation = new SoTranslation ();
    textTranslation->translation.setValue ( SbVec3f ( -size * 49. / 50., size * 9./10., 0 ) );
    sep->addChild ( textTranslation );

    auto ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::BOUNDING_BOX);
    sep->addChild(ps);

    sep->addChild ( getLabel () );
}
