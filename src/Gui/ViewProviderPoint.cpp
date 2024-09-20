/***************************************************************************
 *   Copyright (c) 2024 Ondsel (PL Boyer) <development@ondsel.com>         *
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
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
#endif

#include "ViewProviderPoint.h"
#include "ViewProviderOrigin.h"

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderPoint, Gui::ViewProviderDatum)

ViewProviderPoint::ViewProviderPoint()
{
    sPixmap = "Std_Point";  // Set the icon for the point
}

ViewProviderPoint::~ViewProviderPoint() = default;

void ViewProviderPoint::attach(App::DocumentObject * obj) {
    ViewProviderDatum::attach(obj);

    // The coordinates for the point (single vertex at the origin)
    static const SbVec3f point = SbVec3f(0, 0, 0);

    // Get the local coordinate system root node (LCSRoot)
    SoSeparator* sep = getLCSRoot();

    auto pCoords = new SoCoordinate3();
    pCoords->point.setNum(1);
    pCoords->point.setValue(point);
    sep->addChild(pCoords);

    auto sphere = new SoSphere();
    sphere->radius.setValue(1.0);
    sep->addChild(sphere);

    // Add pick style to define how the point can be selected
    auto ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::BOUNDING_BOX);
    sep->addChild(ps);
}
