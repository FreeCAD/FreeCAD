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


#include <algorithm>
#include <array>
#include <sstream>
#include <QAction>
#include <QFileInfo>
#include <QImage>
#include <QMenu>
#include <QString>
#include <QSvgRenderer>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>


#include <App/Document.h>
#include <App/ImagePlane.h>
#include <Gui/Document.h>
#include <Gui/ActionFunction.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/TaskView/TaskImage.h>

#include "ViewProviderImagePlane.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderImagePlane, Gui::ViewProviderGeometryObject)
const char* ViewProviderImagePlane::LightingEnums[] = {"One side", "Two side", nullptr};

ViewProviderImagePlane::ViewProviderImagePlane()
{
    ADD_PROPERTY_TYPE(Lighting, (1L), "Object Style", App::Prop_None, "Set object lighting.");
    Lighting.setEnums(LightingEnums);

    texture = new SoTexture2;
    texture->ref();

    pcCoords = new SoCoordinate3();
    pcCoords->ref();

    textCoord = new SoTextureCoordinate2;
    textCoord->ref();

    shapeHints = new SoShapeHints;
    shapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapeHints->ref();

    cropPreviewSwitch = new SoSwitch;
    cropPreviewSwitch->whichChild = SO_SWITCH_NONE;
    cropPreviewSwitch->ref();

    cropPreviewMaterial = new SoMaterial;
    cropPreviewMaterial->diffuseColor.setValue(1.0F, 1.0F, 1.0F);
    cropPreviewMaterial->transparency = 0.8F;
    cropPreviewMaterial->ref();

    cropPreviewCoords = new SoCoordinate3;
    cropPreviewCoords->ref();

    cropPreviewTexCoord = new SoTextureCoordinate2;
    cropPreviewTexCoord->ref();

    cropPreviewFaceSet = new SoFaceSet;
    cropPreviewFaceSet->numVertices.setValues(0, 4, std::array<int32_t, 4> {4, 4, 4, 4}.data());
    cropPreviewFaceSet->ref();

    sPixmap = "image-plane";

    ShapeAppearance.setDiffuseColor(1.0F, 1.0F, 1.0F);
}

ViewProviderImagePlane::~ViewProviderImagePlane()
{
    pcCoords->unref();
    textCoord->unref();
    texture->unref();
    shapeHints->unref();
    cropPreviewSwitch->unref();
    cropPreviewMaterial->unref();
    cropPreviewCoords->unref();
    cropPreviewTexCoord->unref();
    cropPreviewFaceSet->unref();
}

void ViewProviderImagePlane::attach(App::DocumentObject* pcObj)
{
    ViewProviderGeometryObject::attach(pcObj);

    // Own separator so the crop preview's material/texture-coordinate state doesn't leak
    // into whatever is traversed after it.
    SoSeparator* cropPreviewSep = new SoSeparator;
    cropPreviewSep->addChild(cropPreviewMaterial);
    cropPreviewSep->addChild(texture);
    cropPreviewSep->addChild(cropPreviewTexCoord);
    cropPreviewSep->addChild(cropPreviewCoords);
    cropPreviewSep->addChild(shapeHints);
    cropPreviewSep->addChild(cropPreviewFaceSet);
    cropPreviewSwitch->addChild(cropPreviewSep);

    // NOTE: SoFCSelection node has beem removed because it led to
    // problems using the image as a construction plane with the
    // draft commands
    SoSeparator* shading = new SoSeparator;
    shading->addChild(pcCoords);

    textCoord->point.set1Value(0, 0, 0);
    textCoord->point.set1Value(1, 1, 0);
    textCoord->point.set1Value(2, 1, 1);
    textCoord->point.set1Value(3, 0, 1);
    shading->addChild(textCoord);

    // texture
    texture->model = SoTexture2::MODULATE;
    shading->addChild(texture);

    shading->addChild(shapeHints);
    shading->addChild(pcShapeMaterial);

    // plane
    pcCoords->point.set1Value(0, 0, 0, 0);
    pcCoords->point.set1Value(1, 1, 0, 0);
    pcCoords->point.set1Value(2, 1, 1, 0);
    pcCoords->point.set1Value(3, 0, 1, 0);
    SoFaceSet* faceset = new SoFaceSet;
    faceset->numVertices.set1Value(0, 4);
    shading->addChild(faceset);
    shading->addChild(cropPreviewSwitch);

    addDisplayMaskMode(shading, "Shading");

    SoLightModel* lightmodel = new SoLightModel;
    lightmodel->model = SoLightModel::BASE_COLOR;

    SoSeparator* noshading = new SoSeparator;
    noshading->addChild(pcCoords);
    noshading->addChild(textCoord);
    noshading->addChild(texture);
    noshading->addChild(shapeHints);
    noshading->addChild(pcShapeMaterial);
    noshading->addChild(lightmodel);
    noshading->addChild(faceset);
    noshading->addChild(cropPreviewSwitch);

    addDisplayMaskMode(noshading, "No shading");
}

void ViewProviderImagePlane::setDisplayMode(const char* ModeName)
{
    if (strcmp("Shading", ModeName) == 0) {
        setDisplayMaskMode("Shading");
    }
    else if (strcmp("No shading", ModeName) == 0) {
        setDisplayMaskMode("No shading");
    }
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderImagePlane::getDisplayModes() const
{
    std::vector<std::string> StrList;
    StrList.emplace_back("Shading");
    StrList.emplace_back("No shading");
    return StrList;
}

void ViewProviderImagePlane::onChanged(const App::Property* prop)
{
    if (prop == &Lighting) {
        if (Lighting.getValue() == 0) {
            shapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        }
        else {
            shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        }
    }
    ViewProviderGeometryObject::onChanged(prop);
}

void ViewProviderImagePlane::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* action = menu->addAction(QObject::tr("Edit Image Plane"));
    action->setIcon(QIcon(QLatin1String("images:image-scaling.svg")));
    func->trigger(action, [this]() { this->manipulateImage(); });

    ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);
}

bool ViewProviderImagePlane::doubleClicked()
{
    manipulateImage();
    return true;
}

void ViewProviderImagePlane::manipulateImage()
{
    auto dialog = new TaskImageDialog(getObject<Image::ImagePlane>());

    Gui::Control().showDialog(dialog, getDocument()->getDocument());
}

ViewProviderImagePlane::CropFractions ViewProviderImagePlane::getCropFractions() const
{
    Image::ImagePlane* pcPlaneObj = static_cast<Image::ImagePlane*>(pcObject);
    // Crop is stored as a percentage of the image itself, not of the plane's physical
    // XSize/YSize, so it stays correct no matter how the plane is later scaled or calibrated.
    // PropertyFloatConstraint's constraint range only guides the property editor's widget, so
    // values set from Python or a hand-edited document are clamped defensively here as well.
    float left = std::clamp(float(pcPlaneObj->CropLeft.getValue()) / 100.0f, 0.0f, 1.0f);
    float right = std::clamp(float(pcPlaneObj->CropRight.getValue()) / 100.0f, 0.0f, 1.0f);
    float top = std::clamp(float(pcPlaneObj->CropTop.getValue()) / 100.0f, 0.0f, 1.0f);
    float bottom = std::clamp(float(pcPlaneObj->CropBottom.getValue()) / 100.0f, 0.0f, 1.0f);

    // Opposite edges can still sum past 100%, so squeeze them proportionally to leave a
    // sliver of the image visible.
    if (left + right >= 1.0f) {
        float total = left + right;
        left = (left / total) * (1.0f - 0.001f);
        right = (right / total) * (1.0f - 0.001f);
    }
    if (top + bottom >= 1.0f) {
        float total = top + bottom;
        top = (top / total) * (1.0f - 0.001f);
        bottom = (bottom / total) * (1.0f - 0.001f);
    }

    return {left, right, top, bottom};
}

void ViewProviderImagePlane::resizePlane(float xsize, float ysize)
{
    auto [left, right, top, bottom] = getCropFractions();

    float leftMM = left * xsize;
    float rightMM = right * xsize;
    float topMM = top * ysize;
    float bottomMM = bottom * ysize;

    pcCoords->point.set1Value(0, -(xsize / 2) + leftMM, -(ysize / 2) + bottomMM, 0.0);
    pcCoords->point.set1Value(1, +(xsize / 2) - rightMM, -(ysize / 2) + bottomMM, 0.0);
    pcCoords->point.set1Value(2, +(xsize / 2) - rightMM, +(ysize / 2) - topMM, 0.0);
    pcCoords->point.set1Value(3, -(xsize / 2) + leftMM, +(ysize / 2) - topMM, 0.0);

    float u0 = left;
    float u1 = 1.0f - right;
    float v0 = bottom;
    float v1 = 1.0f - top;

    textCoord->point.set1Value(0, u0, v0);
    textCoord->point.set1Value(1, u1, v0);
    textCoord->point.set1Value(2, u1, v1);
    textCoord->point.set1Value(3, u0, v1);
}

void ViewProviderImagePlane::updateCropPreview(float xsize, float ysize)
{
    auto [left, right, top, bottom] = getCropFractions();

    float leftMM = left * xsize;
    float rightMM = right * xsize;
    float topMM = top * ysize;
    float bottomMM = bottom * ysize;

    float xMin = -(xsize / 2);
    float xMax = +(xsize / 2);
    float yMin = -(ysize / 2);
    float yMax = +(ysize / 2);
    float keptXMin = xMin + leftMM;
    float keptXMax = xMax - rightMM;
    float keptYMin = yMin + bottomMM;
    float keptYMax = yMax - topMM;

    // Non-overlapping "picture frame" decomposition of the cropped-out area: left/right
    // strips span the full height (and cover the corners), top/bottom strips only span the
    // kept x-range in between them. A strip with a zero-fraction crop degenerates to a
    // zero-area quad, which is harmless to still emit.
    struct Quad
    {
        float x0, y0, x1, y1;
        float u0, v0, u1, v1;
    };
    std::array<Quad, 4> quads {
        {// Left
         {xMin, yMin, keptXMin, yMax, 0.0F, 0.0F, left, 1.0F},
         // Right
         {keptXMax, yMin, xMax, yMax, 1.0F - right, 0.0F, 1.0F, 1.0F},
         // Top
         {keptXMin, keptYMax, keptXMax, yMax, left, 1.0F - top, 1.0F - right, 1.0F},
         // Bottom
         {keptXMin, yMin, keptXMax, keptYMin, left, 0.0F, 1.0F - right, bottom}
        }
    };

    int idx = 0;
    for (const Quad& quad : quads) {
        cropPreviewCoords->point.set1Value(idx, quad.x0, quad.y0, 0.0);
        cropPreviewCoords->point.set1Value(idx + 1, quad.x1, quad.y0, 0.0);
        cropPreviewCoords->point.set1Value(idx + 2, quad.x1, quad.y1, 0.0);
        cropPreviewCoords->point.set1Value(idx + 3, quad.x0, quad.y1, 0.0);

        cropPreviewTexCoord->point.set1Value(idx, quad.u0, quad.v0);
        cropPreviewTexCoord->point.set1Value(idx + 1, quad.u1, quad.v0);
        cropPreviewTexCoord->point.set1Value(idx + 2, quad.u1, quad.v1);
        cropPreviewTexCoord->point.set1Value(idx + 3, quad.u0, quad.v1);

        idx += 4;
    }
}

void ViewProviderImagePlane::setCropPreviewActive(bool active)
{
    cropPreviewSwitch->whichChild = active ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}

void ViewProviderImagePlane::setCropPreviewTransparency(float transparency)
{
    cropPreviewMaterial->transparency = std::clamp(transparency, 0.0F, 1.0F);
}

void ViewProviderImagePlane::loadImage()
{
    Image::ImagePlane* imagePlane = static_cast<Image::ImagePlane*>(pcObject);
    std::string fileName = imagePlane->ImageFile.getValue();

    if (!fileName.empty()) {
        QImage impQ;
        if (isSvgFile(fileName.c_str())) {
            impQ = loadSvg(fileName.c_str());
        }
        else {
            impQ = loadRaster(fileName.c_str());
        }

        QSizeF size = getSizeInMM(impQ);
        setPlaneSize(size, impQ);
        convertToSFImage(impQ);
    }
}

void ViewProviderImagePlane::setPlaneSize(const QSizeF& size, const QImage& img)
{
    if (!img.isNull()) {
        Image::ImagePlane* imagePlane = static_cast<Image::ImagePlane*>(pcObject);
        // When restoring the document or importing a ImagePlane by eg. pasting it
        // preserve the X and Y size.
        if (!isRestoring() && !pcObject->testStatus(App::ObjectStatus::ObjImporting)) {
            imagePlane->XSize.setValue(size.width());
            imagePlane->YSize.setValue(size.height());
        }

        imagePlane->XPixelsPerMeter = img.dotsPerMeterX();
        imagePlane->YPixelsPerMeter = img.dotsPerMeterY();
    }
}

QImage ViewProviderImagePlane::loadRaster(const char* fileName) const
{
    QImage img;
    img.load(QString::fromUtf8(fileName));
    return img;
}

void ViewProviderImagePlane::reloadIfSvg()
{
    Image::ImagePlane* imagePlane = static_cast<Image::ImagePlane*>(pcObject);
    std::string fileName = imagePlane->ImageFile.getValue();

    if (isSvgFile(fileName.c_str())) {
        double xsize = imagePlane->XSize.getValue();
        double ysize = imagePlane->YSize.getValue();

        QImage impQ = loadSvgOfSize(fileName.c_str(), QSizeF(xsize, ysize));
        convertToSFImage(impQ);
    }
}

QImage ViewProviderImagePlane::loadSvg(const char* filename) const
{
    QSizeF defaultSize = defaultSizeOfSvg(filename);
    QPixmap px = BitmapFactory().pixmapFromSvg(filename, defaultSize);
    return px.toImage();
}

QImage ViewProviderImagePlane::loadSvgOfSize(const char* filename, const QSizeF& size) const
{
    QSizeF psize = pixelSize(filename, size);
    QPixmap px = BitmapFactory().pixmapFromSvg(filename, psize);
    return px.toImage();
}

bool ViewProviderImagePlane::isSvgFile(const char* filename) const
{
    QFileInfo fi(QString::fromUtf8(filename));
    return (fi.suffix().toLower() == QLatin1String("svg"));
}

QSizeF ViewProviderImagePlane::defaultSizeOfSvg(const char* filename) const
{
    QSvgRenderer svg;
    svg.load(QString::fromUtf8(filename));
    return svg.defaultSize();
}

QSizeF ViewProviderImagePlane::getSizeInMM(const QImage& img) const
{
    double xPixelsPerM = img.dotsPerMeterX();
    double width = img.width();
    width = width * 1000 / xPixelsPerM;

    double yPixelsPerM = img.dotsPerMeterY();
    double height = img.height();
    height = height * 1000 / yPixelsPerM;

    QSizeF sizef;
    sizef.setWidth(width);
    sizef.setHeight(height);
    return sizef;
}

QSizeF ViewProviderImagePlane::pixelSize(const char* filename, const QSizeF& size) const
{
    QImage tmp;
    if (tmp.load(QString::fromUtf8(filename))) {
        double xPixelsPerM = tmp.dotsPerMeterX();
        double yPixelsPerM = tmp.dotsPerMeterY();

        double width = size.width();
        double height = size.height();

        width = width * xPixelsPerM / 1000;
        height = height * yPixelsPerM / 1000;

        return {width, height};
    }

    return size;
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
    Image::ImagePlane* pcPlaneObj = static_cast<Image::ImagePlane*>(pcObject);
    if (prop == &pcPlaneObj->XSize || prop == &pcPlaneObj->YSize || prop == &pcPlaneObj->CropLeft
        || prop == &pcPlaneObj->CropRight || prop == &pcPlaneObj->CropTop
        || prop == &pcPlaneObj->CropBottom) {
        float xsize = pcPlaneObj->XSize.getValue();
        float ysize = pcPlaneObj->YSize.getValue();

        resizePlane(xsize, ysize);
        updateCropPreview(xsize, ysize);
        reloadIfSvg();
    }
    else if (prop == &pcPlaneObj->ImageFile) {
        loadImage();
    }
    else {
        Gui::ViewProviderGeometryObject::updateData(prop);
    }
}
