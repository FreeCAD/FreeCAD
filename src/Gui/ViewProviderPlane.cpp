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
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
#endif

#include "ViewProviderOrigin.h"

#include "ViewProviderPlane.h"

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderPlane, Gui::ViewProviderOriginFeature)


ViewProviderPlane::ViewProviderPlane()
{
    sPixmap = "Std_Plane";
}

ViewProviderPlane::~ViewProviderPlane()
{ }

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

    SoCoordinate3 *pCoords = new SoCoordinate3 ();
    pCoords->point.setNum (4);
    pCoords->point.setValues ( 0, 4, verts );
    sep->addChild ( pCoords );

    SoIndexedLineSet *pLines  = new SoIndexedLineSet ();
    pLines->ref();
    pLines->coordIndex.setNum(6);
    pLines->coordIndex.setValues(0, 6, lines);
    sep->addChild ( pLines );

    SoTranslation *textTranslation = new SoTranslation ();
    textTranslation->ref ();
    textTranslation->translation.setValue ( SbVec3f ( -size * 49. / 50., size * 9./10., 0 ) );
    sep->addChild ( textTranslation );

    sep->addChild ( getLabel () );
}
