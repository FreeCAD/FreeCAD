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
#include <Inventor/nodes/SoAsciiText.h>
#include "ViewProviderPlane.h"
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

PROPERTY_SOURCE(Gui::ViewProviderPlane, Gui::ViewProviderGeometryObject)


ViewProviderPlane::ViewProviderPlane()
{

    ADD_PROPERTY(Size,(1.0));

    pMat = new SoMaterial();
    pMat->ref();

    float size = Size.getValue(); // Note: If you change this, you need to also adapt App/Plane.cpp getBoundBox()

    SbVec3f verts[4] =
    {
        SbVec3f(size,size,0), SbVec3f(size,-size,0),
        SbVec3f(-size,-size,0), SbVec3f(-size,size,0),
    };

    // indexes used to create the edges
    static const int32_t lines[6] =
    {
        0,1,2,3,0,-1
    };

    pMat->diffuseColor.setNum(1);
    pMat->diffuseColor.set1Value(0, SbColor(1.0f, 1.0f, 1.0f));

    pCoords = new SoCoordinate3();
    pCoords->ref();
    pCoords->point.setNum(4);
    pCoords->point.setValues(0, 4, verts);

    pLines  = new SoIndexedLineSet();
    pLines->ref();
    pLines->coordIndex.setNum(6);
    pLines->coordIndex.setValues(0, 6, lines);

    pFont = new SoFont();
    pFont->size.setValue(Size.getValue()/10.);

    pTranslation = new SoTranslation();
    pTranslation->translation.setValue(SbVec3f(-1,9./10.,0));

    pText = new SoAsciiText();
    pText->width.setValue(-1);

    sPixmap = "view-measurement";
}

ViewProviderPlane::~ViewProviderPlane()
{
    pCoords->unref();
    pLines->unref();
    pMat->unref();
}

void ViewProviderPlane::onChanged(const App::Property* prop)
{
        if (prop == &Size){
                float size = Size.getValue(); // Note: If you change this, you need to also adapt App/Plane.cpp getBoundBox()

                SbVec3f verts[4] =
                {
                    SbVec3f(size,size,0), SbVec3f(size,-size,0),
                    SbVec3f(-size,-size,0), SbVec3f(-size,size,0),
                };

                pCoords->point.setValues(0, 4, verts);
                pFont->size.setValue(Size.getValue()/10.);
                pTranslation->translation.setValue(SbVec3f(-size,size*9./10.,0));
        }
        else
         ViewProviderGeometryObject::onChanged(prop);
}

std::vector<std::string> ViewProviderPlane::getDisplayModes(void) const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.push_back("Base");
    return StrList;
}

void ViewProviderPlane::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

void ViewProviderPlane::attach(App::DocumentObject* pcObject)
{
    ViewProviderGeometryObject::attach(pcObject);

    SoSeparator  *sep = new SoSeparator();
    SoAnnotation *lineSep = new SoAnnotation();
    SoFCSelection *highlight = new SoFCSelection();

    SoDrawStyle* style = new SoDrawStyle();
    style->lineWidth = 2.0f;

    SoMaterialBinding* matBinding = new SoMaterialBinding;
    matBinding->value = SoMaterialBinding::OVERALL;

    sep->addChild(matBinding);
    sep->addChild(pMat);
    sep->addChild(highlight);
    highlight->addChild(style);
    highlight->addChild(pCoords);
    highlight->addChild(pLines);

    style = new SoDrawStyle();
    style->lineWidth = 2.0f;
    style->linePattern.setValue(0x00FF);
    lineSep->addChild(style);
    lineSep->addChild(pLines);
    lineSep->addChild(pFont);
    pText->string.setValue(SbString(pcObject->Label.getValue()));
    lineSep->addChild(pTranslation);
    lineSep->addChild(pText);
    highlight->addChild(lineSep);

    highlight->style = SoFCSelection::EMISSIVE_DIFFUSE;
    addDisplayMaskMode(sep, "Base");
}

void ViewProviderPlane::updateData(const App::Property* prop)
{
    pText->string.setValue(SbString(pcObject->Label.getValue()));
    ViewProviderGeometryObject::updateData(prop);
}

std::string ViewProviderPlane::getElement(const SoDetail* detail) const
{
    if (detail) {
        if (detail->getTypeId() == SoLineDetail::getClassTypeId()) {
            const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
            int edge = line_detail->getLineIndex();
            if (edge == 0)
            {
                return std::string("Main");
            }
        }
    }

    return std::string("");
}

SoDetail* ViewProviderPlane::getDetail(const char* subelement) const
{
    SoLineDetail* detail = 0;
    std::string subelem(subelement);
    int edge = -1;

    if(subelem == "Main") edge = 0;

    if(edge >= 0) {
         detail = new SoLineDetail();
         detail->setPartIndex(edge);
    }

    return detail;
}

bool ViewProviderPlane::isSelectable(void) const
{
    return true;
}

bool ViewProviderPlane::setEdit(int ModNum)
{
    return true;
}

void ViewProviderPlane::unsetEdit(int ModNum)
{

}

// ----------------------------------------------------------------------------


