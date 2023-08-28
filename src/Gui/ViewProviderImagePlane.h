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

#ifndef GUI_ViewProviderImagePlane_H
#define GUI_ViewProviderImagePlane_H

#include <Gui/ViewProviderGeometryObject.h>


class SoCoordinate3;
class SoDrawStyle;
class SoShapeHints;
class SoTexture2;
class QImage;

namespace Gui
{

class GuiExport ViewProviderImagePlane : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderImagePlane);

public:
    ViewProviderImagePlane();
    ~ViewProviderImagePlane() override;

    App::PropertyEnumeration Lighting;

    void attach(App::DocumentObject *pcObject) override;
    void setDisplayMode(const char* ModeName) override;
    std::vector<std::string> getDisplayModes() const override;
    void updateData(const App::Property*) override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool doubleClicked() override;
    void onChanged(const App::Property* prop) override;

private:
    void resizePlane(float xsize, float ysize);
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
    SoCoordinate3         * pcCoords;
    SoTexture2            * texture;
    SoShapeHints          * shapeHints;
    static const char     * LightingEnums[];
};

} //namespace Gui


#endif // GUI_ViewProviderImagePlane_H
