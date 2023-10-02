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
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
#endif

#include "ViewProviderLine.h"
#include "ViewProviderOrigin.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderLine, Gui::ViewProviderOriginFeature)


ViewProviderLine::ViewProviderLine()
{
    sPixmap = "Std_Axis";
}

ViewProviderLine::~ViewProviderLine() = default;

void ViewProviderLine::attach ( App::DocumentObject *obj ) {
    ViewProviderOriginFeature::attach ( obj );

    static const float size = ViewProviderOrigin::defaultSize ();

    static const SbVec3f verts[2] = { SbVec3f(size, 0, 0),   SbVec3f ( -size, 0, 0 ) };

    // indexes used to create the edges
    static const int32_t lines[4] = { 0, 1, -1 };

    SoSeparator *sep = getOriginFeatureRoot ();

    auto pCoords = new SoCoordinate3 ();
    pCoords->point.setNum (2);
    pCoords->point.setValues ( 0, 2, verts );
    sep->addChild ( pCoords );

    auto pLines  = new SoIndexedLineSet ();
    pLines->coordIndex.setNum(3);
    pLines->coordIndex.setValues(0, 3, lines);
    sep->addChild ( pLines );

    auto textTranslation = new SoTranslation ();
    textTranslation->translation.setValue ( SbVec3f ( -size * 49. / 50., size / 30., 0 ) );
    sep->addChild ( textTranslation );

    auto ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::BOUNDING_BOX);
    sep->addChild(ps);

    sep->addChild ( getLabel () );
}
