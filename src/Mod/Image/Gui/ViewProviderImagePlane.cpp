/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <Inventor/SbVec3f.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoTextureCoordinate2.h>
# include <Inventor/nodes/SoTexture2.h>
# include <QFile>
# include <QFileInfo>
# include <QImage>
# include <QString>
#endif

#include "ViewProviderImagePlane.h"

#include <Mod/Image/App/ImagePlane.h>
#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Console.h>
#include <sstream>

using namespace Gui;
using namespace ImageGui;
using namespace Image;


PROPERTY_SOURCE(ImageGui::ViewProviderImagePlane, Gui::ViewProviderGeometryObject)

ViewProviderImagePlane::ViewProviderImagePlane()
{
    texture = new SoTexture2;
    texture->ref();

    pcCoords = new SoCoordinate3();
    pcCoords->ref();
}

ViewProviderImagePlane::~ViewProviderImagePlane()
{
    pcCoords->unref();
    texture->unref();
}

void ViewProviderImagePlane::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);

    // NOTE: SoFCSelection node has beem removed because it led to
    // problems using the image as a construction plane with the
    // draft commands
    SoSeparator* planesep = new SoSeparator;
    planesep->addChild(pcCoords);

    SoTextureCoordinate2 *textCoord = new SoTextureCoordinate2;
    textCoord->point.set1Value(0,0,0);
    textCoord->point.set1Value(1,1,0);
    textCoord->point.set1Value(2,1,1);
    textCoord->point.set1Value(3,0,1);
    planesep->addChild(textCoord);

    // texture
    texture->model = SoTexture2::MODULATE;
    planesep->addChild(texture);

    planesep->addChild(pcShapeMaterial);

    // plane
    pcCoords->point.set1Value(0,0,0,0);
    pcCoords->point.set1Value(1,1,0,0);
    pcCoords->point.set1Value(2,1,1,0);
    pcCoords->point.set1Value(3,0,1,0);
    SoFaceSet *faceset = new SoFaceSet;
    faceset->numVertices.set1Value(0,4);
    planesep->addChild(faceset);

    addDisplayMaskMode(planesep, "ImagePlane");
}

void ViewProviderImagePlane::setDisplayMode(const char* ModeName)
{
    if (strcmp("ImagePlane",ModeName) == 0)
        setDisplayMaskMode("ImagePlane");
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderImagePlane::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("ImagePlane");
    return StrList;
}

bool ViewProviderImagePlane::loadSvg(const char* filename, float x, float y, QImage& img)
{
    QFileInfo fi(QString::fromUtf8(filename));
    if (fi.suffix().toLower() == QLatin1String("svg")) {
        QPixmap px = BitmapFactory().pixmapFromSvg(filename, QSize((int)x,(int)y));
        img = px.toImage();
        return true;
    }

    return false;
}

void ViewProviderImagePlane::updateData(const App::Property* prop)
{
    Image::ImagePlane* pcPlaneObj = static_cast<Image::ImagePlane*>(pcObject);
    if (prop == &pcPlaneObj->XSize || prop == &pcPlaneObj->YSize) {
        float x = pcPlaneObj->XSize.getValue();
        float y = pcPlaneObj->YSize.getValue();

        //pcCoords->point.setNum(4);
        pcCoords->point.set1Value(0,-(x/2),-(y/2),0.0);
        pcCoords->point.set1Value(1,+(x/2),-(y/2),0.0);
        pcCoords->point.set1Value(2,+(x/2),+(y/2),0.0);
        pcCoords->point.set1Value(3,-(x/2),+(y/2),0.0);

        QImage impQ;
        loadSvg(pcPlaneObj->ImageFile.getValue(), x, y, impQ);
        if (!impQ.isNull()) {
            SoSFImage img;
            // convert to Coin bitmap
            BitmapFactory().convert(impQ,img);
            texture->image = img;
        }
    }
    else if (prop == &pcPlaneObj->ImageFile) {
        float x = pcPlaneObj->XSize.getValue();
        float y = pcPlaneObj->YSize.getValue();
        QImage impQ;
        if (!loadSvg(pcPlaneObj->ImageFile.getValue(),x,y, impQ))
            impQ.load(QString::fromUtf8(pcPlaneObj->ImageFile.getValue()));
        if (!impQ.isNull()) {
            SoSFImage img;
            // convert to Coin bitmap
            BitmapFactory().convert(impQ,img);
            texture->image = img;
        }
    }
    else {
        Gui::ViewProviderGeometryObject::updateData(prop);
    }
}
