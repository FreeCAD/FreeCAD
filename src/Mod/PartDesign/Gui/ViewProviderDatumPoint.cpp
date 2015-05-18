/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoVertexProperty.h>
#endif

#include "ViewProviderDatumPoint.h"
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Mod/PartDesign/App/DatumPoint.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPoint,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPoint::ViewProviderDatumPoint()
{
    sPixmap = "PartDesign_Point.svg";
    
    SoMFVec3f v;
    v.setNum(1);
    v.set1Value(0, 0,0,0);
    SoVertexProperty* vprop = new SoVertexProperty();
    vprop->vertex = v;
    // Using a marker gives a larger point but it doesn't do highlighting automatically like the SoBrepPointSet
    SoMarkerSet* marker = new SoMarkerSet();
    marker->markerIndex = SoMarkerSet::DIAMOND_FILLED_9_9;
    marker->vertexProperty = vprop;
    marker->numPoints = 1;
    PartGui::SoBrepPointSet* points = new PartGui::SoBrepPointSet();
    points->vertexProperty = vprop;
    points->numPoints = 1;
    pShapeSep->addChild(points);
    pShapeSep->addChild(marker);
}

ViewProviderDatumPoint::~ViewProviderDatumPoint()
{
}

void ViewProviderDatumPoint::updateData(const App::Property* prop)
{
    if (strcmp(prop->getName(),"Placement") == 0) {
        // The only reason to do this is to display the point in the correct position after loading the document
        SoMarkerSet* marker = static_cast<SoMarkerSet*>(pShapeSep->getChild(1));
        marker->touch();
        PartGui::SoBrepPointSet* points = static_cast<PartGui::SoBrepPointSet*>(pShapeSep->getChild(0));
        points->touch();
    }

    ViewProviderDatum::updateData(prop);
}
