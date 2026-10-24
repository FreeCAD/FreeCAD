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

#pragma once

#include <Gui/ViewProviderGeometryObject.h>


class SoCoordinate3;
class SoDrawStyle;
class SoFaceSet;
class SoMaterial;
class SoShapeHints;
class SoSwitch;
class SoTexture2;
class SoTextureCoordinate2;
class QImage;

namespace Gui
{

class GuiExport ViewProviderImagePlane: public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderImagePlane);

public:
    ViewProviderImagePlane();
    ~ViewProviderImagePlane() override;

    App::PropertyEnumeration Lighting;

    void attach(App::DocumentObject* pcObject) override;
    void setDisplayMode(const char* ModeName) override;
    std::vector<std::string> getDisplayModes() const override;
    void updateData(const App::Property*) override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool doubleClicked() override;
    void onChanged(const App::Property* prop) override;

    /// Show/hide the semi-transparent overlay of the cropped-out area (used while the crop
    /// task dialog is open).
    void setCropPreviewActive(bool active);
    /// Set the overlay's transparency, in the range [0, 1].
    void setCropPreviewTransparency(float transparency);

private:
    struct CropFractions
    {
        float left;
        float right;
        float top;
        float bottom;
    };

    CropFractions getCropFractions() const;
    void resizePlane(float xsize, float ysize);
    void updateCropPreview(float xsize, float ysize);
    void loadImage();
    void setPlaneSize(const QSizeF& size, const QImage& img);
    void reloadIfSvg();
    bool isSvgFile(const char*) const;
    QSizeF getSizeInMM(const QImage&) const;
    QSizeF defaultSizeOfSvg(const char*) const;
    QSizeF pixelSize(const char*, const QSizeF&) const;
    QImage loadSvg(const char*) const;
    QImage loadSvgOfSize(const char*, const QSizeF&) const;
    QImage loadRaster(const char*) const;
    void convertToSFImage(const QImage& img);
    void manipulateImage();

private:
    SoCoordinate3* pcCoords;
    SoTexture2* texture;
    SoShapeHints* shapeHints;
    SoTextureCoordinate2* textCoord;

    // Semi-transparent overlay of the cropped-out area, shown only while the crop task
    // dialog is open. Reuses `texture` and `shapeHints` above; independent of `pcShapeMaterial`
    // so the kept image's own Transparency setting is never affected by the overlay.
    SoSwitch* cropPreviewSwitch;
    SoMaterial* cropPreviewMaterial;
    SoCoordinate3* cropPreviewCoords;
    SoTextureCoordinate2* cropPreviewTexCoord;
    SoFaceSet* cropPreviewFaceSet;

    static const char* LightingEnums[];
};

}  // namespace Gui
