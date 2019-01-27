/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2012    *
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
# include <sstream>
# include <QApplication>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoFontStyle.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoDrawStyle.h>
#endif

#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/details/SoLineDetail.h>
#include "ViewProviderPlacement.h"
#include "SoFCSelection.h"
#include "Application.h"
#include "Document.h"
#include "View3DInventorViewer.h"
#include "Inventor/SoAutoZoomTranslation.h"
#include "SoAxisCrossKit.h"
//#include <SoDepthBuffer.h>

#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <App/MeasureDistance.h>
#include <Base/Console.h>

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderPlacement, Gui::ViewProviderGeometryObject)


ViewProviderPlacement::ViewProviderPlacement()
{

    pMat = new SoMaterial();
    pMat->ref();

    const float dist = 2;
    const float size = 6;
    const float pSize = 4;

    static const SbVec3f verts[13] =
    {
        SbVec3f(0,0,0), SbVec3f(size,0,0),
        SbVec3f(0,size,0), SbVec3f(0,0,size),
        SbVec3f(dist,dist,0), SbVec3f(dist,pSize,0), SbVec3f(pSize,dist,0),  // XY Plane
        SbVec3f(dist,0,dist), SbVec3f(dist,0,pSize), SbVec3f(pSize,0,dist),  // XY Plane
        SbVec3f(0,dist,dist), SbVec3f(0,pSize,dist), SbVec3f(0,dist,pSize)  // XY Plane
    };

    // indexes used to create the edges
    static const int32_t lines[21] =
    {
        0,1,-1,
        0,2,-1,
        0,3,-1,
        5,4,6,-1,
        8,7,9,-1,
        11,10,12,-1
    };

    pMat->diffuseColor.setNum(6);
    pMat->diffuseColor.set1Value(0, SbColor(1.0f, 0.2f, 0.2f));
    pMat->diffuseColor.set1Value(1, SbColor(0.2f, 1.0f, 0.2f));
    pMat->diffuseColor.set1Value(2, SbColor(0.2f, 0.2f, 1.0f));

    pMat->diffuseColor.set1Value(3, SbColor(1.0f, 1.0f, 0.8f));
    pMat->diffuseColor.set1Value(4, SbColor(1.0f, 0.8f, 1.0f));
    pMat->diffuseColor.set1Value(5, SbColor(0.8f, 1.0f, 1.0f));

    pCoords = new SoCoordinate3();
    pCoords->ref();
    pCoords->point.setNum(13);
    pCoords->point.setValues(0, 13, verts);

    pLines  = new SoIndexedLineSet();
    pLines->ref();
    pLines->coordIndex.setNum(21);
    pLines->coordIndex.setValues(0, 21, lines);
    sPixmap = "view-measurement";
}

ViewProviderPlacement::~ViewProviderPlacement()
{
    pCoords->unref();
    pLines->unref();
    pMat->unref();
}

void ViewProviderPlacement::onChanged(const App::Property* prop)
{
        ViewProviderGeometryObject::onChanged(prop);
}

std::vector<std::string> ViewProviderPlacement::getDisplayModes(void) const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.push_back("Base");
    return StrList;
}

void ViewProviderPlacement::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

void ViewProviderPlacement::attach(App::DocumentObject* pcObject)
{
    ViewProviderGeometryObject::attach(pcObject);

    SoAnnotation *lineSep = new SoAnnotation();


    SoAutoZoomTranslation *zoom = new SoAutoZoomTranslation;

    SoDrawStyle* style = new SoDrawStyle();
    style->lineWidth = 2.0f;

    SoMaterialBinding* matBinding = new SoMaterialBinding;
    matBinding->value = SoMaterialBinding::PER_FACE;

    lineSep->addChild(zoom);
    lineSep->addChild(style);
    lineSep->addChild(matBinding);
    lineSep->addChild(pMat);
    lineSep->addChild(pCoords);
    lineSep->addChild(pLines);

    addDisplayMaskMode(lineSep, "Base");
}

void ViewProviderPlacement::updateData(const App::Property* prop)
{
    ViewProviderGeometryObject::updateData(prop);
}

std::string ViewProviderPlacement::getElement(const SoDetail* detail) const
{
    if (detail) {
        if (detail->getTypeId() == SoLineDetail::getClassTypeId()) {
            const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
            int edge = line_detail->getLineIndex();
            switch (edge)
            {
            case 0: return std::string("X-Axis");
            case 1: return std::string("Y-Axis");
            case 2: return std::string("Z-Axis");
            case 3: return std::string("XY-Plane");
            case 4: return std::string("XZ-Plane");
            case 5: return std::string("YZ-Plane");
            }
        }
    }

    return std::string("");
}

SoDetail* ViewProviderPlacement::getDetail(const char* subelement) const
{
    SoLineDetail* detail = 0;
    std::string subelem(subelement);
    int edge = -1;

    if(subelem == "X-Axis") edge = 0;
    else if(subelem == "Y-Axis") edge = 1;
    else if(subelem == "Z-Axis") edge = 2;
    else if(subelem == "XY-Plane") edge = 3;
    else if(subelem == "XZ-Plane") edge = 4;
    else if(subelem == "YZ-Plane") edge = 5;

    if(edge >= 0) {
         detail = new SoLineDetail();
         detail->setPartIndex(edge);
    }


    return detail;
}

bool ViewProviderPlacement::isSelectable(void) const
{
    return true;
}
// ----------------------------------------------------------------------------

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPlacementPython, Gui::ViewProviderPlacement)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderPlacement>;
}

