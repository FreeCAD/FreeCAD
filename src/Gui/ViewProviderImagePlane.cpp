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
# include <sstream>
# include <QAction>
# include <QFileInfo>
# include <QImage>
# include <QMenu>
# include <QString>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoTexture2.h>
# include <Inventor/nodes/SoTextureCoordinate2.h>
#endif

#include <App/Document.h>
#include <Gui/ActionFunction.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/TaskView/TaskOrientation.h>
#include <Gui/TaskView/TaskImageScale.h>
#include <App/ImagePlane.h>

#include "ViewProviderImagePlane.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderImagePlane, Gui::ViewProviderGeometryObject)
const char* ViewProviderImagePlane::LightingEnums[]= {"One side", "Two side", nullptr};

ViewProviderImagePlane::ViewProviderImagePlane()
{
    ADD_PROPERTY_TYPE(Lighting,(1L), "Object Style", App::Prop_None, "Set object lighting.");
    Lighting.setEnums(LightingEnums);

    texture = new SoTexture2;
    texture->ref();

    pcCoords = new SoCoordinate3();
    pcCoords->ref();

    shapeHints = new SoShapeHints;
    shapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapeHints->ref();

    sPixmap = "image-plane";
}

ViewProviderImagePlane::~ViewProviderImagePlane()
{
    pcCoords->unref();
    texture->unref();
    shapeHints->unref();
}

void ViewProviderImagePlane::attach(App::DocumentObject *pcObj)
{
    ViewProviderGeometryObject::attach(pcObj);

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

    planesep->addChild(shapeHints);
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

std::vector<std::string> ViewProviderImagePlane::getDisplayModes() const
{
    std::vector<std::string> StrList;
    StrList.emplace_back("ImagePlane");
    return StrList;
}

void ViewProviderImagePlane::onChanged(const App::Property* prop)
{
    if (prop == &Lighting) {
        if (Lighting.getValue() == 0)
            shapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        else
            shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    }
    ViewProviderGeometryObject::onChanged(prop);
}

void ViewProviderImagePlane::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* action = menu->addAction(QObject::tr("Change image..."));
    action->setIcon(QIcon(QLatin1String("images:image-scaling.svg")));
    func->trigger(action, std::bind(&ViewProviderImagePlane::manipulateImage, this));

    ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);
}

bool ViewProviderImagePlane::doubleClicked()
{
    manipulateImage();
    return true;
}

void ViewProviderImagePlane::manipulateImage()
{
    auto dialog = new TaskOrientationDialog(
        dynamic_cast<App::GeoFeature*>(getObject())
    );
    dialog->addTaskBox(new TaskImageScale(
        dynamic_cast<App::ImagePlane*>(getObject())
    ));

    Gui::Control().showDialog(dialog);
}

void ViewProviderImagePlane::resizePlane(float xsize, float ysize)
{
    pcCoords->point.set1Value(0,-(xsize/2),-(ysize/2),0.0);
    pcCoords->point.set1Value(1,+(xsize/2),-(ysize/2),0.0);
    pcCoords->point.set1Value(2,+(xsize/2),+(ysize/2),0.0);
    pcCoords->point.set1Value(3,-(xsize/2),+(ysize/2),0.0);
}

void ViewProviderImagePlane::loadImage()
{
    App::ImagePlane* imagePlane = static_cast<App::ImagePlane*>(pcObject);
    std::string fileName = imagePlane->ImageFile.getValue();

    if (!fileName.empty()) {
        double xsize = imagePlane->XSize.getValue();
        double ysize = imagePlane->YSize.getValue();

        QImage impQ;
        if (!loadSvg(fileName.c_str(), xsize, ysize, impQ)) {
            QSizeF size = loadRaster(fileName.c_str(), impQ);
            if (!impQ.isNull()) {
                imagePlane->XSize.setValue(size.width());
                imagePlane->YSize.setValue(size.height());

                imagePlane->XPixelsPerMeter = impQ.dotsPerMeterX();
                imagePlane->YPixelsPerMeter = impQ.dotsPerMeterY();
            }
        }

        convertToSFImage(impQ);
    }
}

QSizeF ViewProviderImagePlane::loadRaster(const char* fileName, QImage& img)
{
    QSizeF sizef;
    if (img.load(QString::fromUtf8(fileName))) {
        double xPixelsPerM = img.dotsPerMeterX();
        double width = img.width();
        width = width * 1000 / xPixelsPerM;

        double yPixelsPerM = img.dotsPerMeterY();
        double height = img.height();
        height = height * 1000 / yPixelsPerM;

        sizef.setWidth(width);
        sizef.setHeight(height);
    }

    return sizef;
}

void ViewProviderImagePlane::reloadIfSvg()
{
    App::ImagePlane* imagePlane = static_cast<App::ImagePlane*>(pcObject);
    std::string fileName = imagePlane->ImageFile.getValue();

    double xsize = imagePlane->XSize.getValue();
    double ysize = imagePlane->YSize.getValue();

    QImage impQ;
    loadSvg(fileName.c_str(), xsize, ysize, impQ);
    convertToSFImage(impQ);
}

bool ViewProviderImagePlane::loadSvg(const char* filename, double width, double height, QImage& img)
{
    QFileInfo fi(QString::fromUtf8(filename));
    if (fi.suffix().toLower() == QLatin1String("svg")) {
        QImage tmp;
        if (tmp.load(QString::fromUtf8(filename))) {
            double xPixelsPerM = tmp.dotsPerMeterX();
            double yPixelsPerM = tmp.dotsPerMeterY();
            width = width * xPixelsPerM / 1000;
            height = height * yPixelsPerM / 1000;
        }

        QPixmap px = BitmapFactory().pixmapFromSvg(filename, QSizeF(width, height));
        img = px.toImage();
        return true;
    }

    return false;
}

void ViewProviderImagePlane::convertToSFImage(const QImage& img)
{
    if (!img.isNull()) {
        SoSFImage sfimg;
        // convert to Coin bitmap
        BitmapFactory().convert(img, sfimg);
        texture->image = sfimg;
    }
}

void ViewProviderImagePlane::updateData(const App::Property* prop)
{
    App::ImagePlane* pcPlaneObj = static_cast<App::ImagePlane*>(pcObject);
    if (prop == &pcPlaneObj->XSize || prop == &pcPlaneObj->YSize) {
        float xsize = pcPlaneObj->XSize.getValue();
        float ysize = pcPlaneObj->YSize.getValue();

        resizePlane(xsize, ysize);
        reloadIfSvg();
    }
    else if (prop == &pcPlaneObj->ImageFile) {
        loadImage();
    }
    else {
        Gui::ViewProviderGeometryObject::updateData(prop);
    }
}
