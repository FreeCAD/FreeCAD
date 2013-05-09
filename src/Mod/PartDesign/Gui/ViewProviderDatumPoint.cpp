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
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <TopoDS_Vertex.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <Precision.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Line.hxx>
# include <GeomAPI_IntCS.hxx>
#endif

#include "ViewProviderDatumPoint.h"
#include "TaskDatumParameters.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPoint,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPoint::ViewProviderDatumPoint()
{
    SoMarkerSet* points = new SoMarkerSet();
    points->markerIndex = SoMarkerSet::DIAMOND_FILLED_9_9;    
    SoMFVec3f v;
    v.setNum(1);
    v.set1Value(0, 0,0,0);
    SoVertexProperty* vprop = new SoVertexProperty();
    vprop->vertex = v;
    points->vertexProperty = vprop;
    points->numPoints = 1;
    pShapeSep->addChild(points);
}

ViewProviderDatumPoint::~ViewProviderDatumPoint()
{
}

void ViewProviderDatumPoint::updateData(const App::Property* prop)
{
    if (strcmp(prop->getName(),"Placement") == 0) {
        // The only reason to do this is to display the point in the correct position after loading the document
        SoMarkerSet* points = static_cast<SoMarkerSet*>(pShapeSep->getChild(0));
        points->touch();
        //points->numPoints = 0;
        //points->numPoints = 1;
    }

    ViewProviderDatum::updateData(prop);
}
