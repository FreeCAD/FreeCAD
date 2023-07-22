/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDERMEASUREDISTANCE_H
#define GUI_VIEWPROVIDERMEASUREDISTANCE_H

#include "ViewProviderDocumentObject.h"
#include <QObject>

class SoFontStyle;
class SoText2;
class SoBaseColor;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;
class SoEventCallback;
class SoMarkerSet;

namespace Gui
{

class View3DInventorViewer;
class ViewProviderPointMarker;
class PointMarker : public QObject
{
public:
    explicit PointMarker(View3DInventorViewer* view);
    ~PointMarker() override;

    void addPoint(const SbVec3f&);
    int countPoints() const;

protected:
    void customEvent(QEvent* e) override;

private:
    View3DInventorViewer *view;
    ViewProviderPointMarker *vp;
    bool previousSelectionEn;
};

class GuiExport ViewProviderPointMarker : public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderPointMarker);

public:
    ViewProviderPointMarker();
    ~ViewProviderPointMarker() override;
    bool isPartOfPhysicalObject() const override;

protected:
    SoCoordinate3    * pCoords;
    SoMarkerSet      * pMarker;
    friend class PointMarker;
};

class GuiExport ViewProviderMeasureDistance : public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderMeasureDistance);

public:
    /// Constructor
    ViewProviderMeasureDistance();
    ~ViewProviderMeasureDistance() override;
    bool isPartOfPhysicalObject() const override;

    // Display properties
    App::PropertyColor          TextColor;
    App::PropertyColor          LineColor;
    App::PropertyInteger        FontSize;
    App::PropertyFloat          DistFactor;
    App::PropertyBool           Mirror;

    void attach(App::DocumentObject *) override;
    void updateData(const App::Property*) override;
    bool useNewSelectionModel() const override {return true;}
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;

    static void measureDistanceCallback(void * ud, SoEventCallback * n);

protected:
    void onChanged(const App::Property* prop) override;

private:
    SoFontStyle      * pFont;
    SoText2          * pLabel;
    SoBaseColor      * pColor;
    SoBaseColor      * pTextColor;
    SoTranslation    * pTranslation;
    SoCoordinate3    * pCoords;
    SoIndexedLineSet * pLines;

    static void endMeasureDistanceMode(void * ud, Gui::View3DInventorViewer* view, SoEventCallback * n, PointMarker *pm);
};

} //namespace Gui


#endif // GUI_VIEWPROVIDERMEASUREDISTANCE_H
