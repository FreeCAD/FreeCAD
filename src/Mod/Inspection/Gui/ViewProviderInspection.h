// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <limits>

#include <App/ComplexGeoData.h>
#include <Base/Observer.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderDocumentObjectGroup.h>


class SoGroup;
class SoMaterial;
class SoMaterialBinding;
class SoDrawStyle;
class SoSeparator;
class SoCoordinate3;

namespace Gui
{
class SoFCColorBar;
class View3DInventorViewer;
}  // namespace Gui

namespace InspectionGui
{

/**
 * @author Werner Mayer
 */
class ViewProviderInspection: public Gui::ViewProviderDocumentObject, public Base::Observer<int>
{
    using inherited = ViewProviderDocumentObject;

    PROPERTY_HEADER_WITH_OVERRIDE(InspectionGui::ViewProviderInspection);

public:
    ViewProviderInspection();
    ~ViewProviderInspection() override;

    App::PropertyBool OutsideGrayed;
    App::PropertyFloatConstraint PointSize;

    void attach(App::DocumentObject* pcFeat) override;
    /// Sets the viewing mode
    void setDisplayMode(const char* ModeName) override;
    /// Returns a list of all possible modes
    std::vector<std::string> getDisplayModes() const override;
    /// Update colorbar after recomputation of distances.
    void updateData(const App::Property*) override;
    /// Once the color bar settings has been changed this method gets called to update the feature's
    /// representation
    void OnChange(Base::Subject<int>& rCaller, int rcReason) override;
    QIcon getIcon() const override;
    /// Returns a color bar
    SoSeparator* getFrontRoot() const override;
    /// Hide the object in the view
    void hide() override;
    /// Show the object in the view
    void show() override;

    static void inspectCallback(void* ud, SoEventCallback* n);

protected:
    void onChanged(const App::Property* prop) override;
    void setDistances();
    QString inspectDistance(const SoPickedPoint* pp) const;

private:
    bool setupFaces(const Data::ComplexGeoData*);
    bool setupLines(const Data::ComplexGeoData*);
    bool setupPoints(const Data::ComplexGeoData*, App::PropertyContainer* container);
    void setupCoords(const std::vector<Base::Vector3d>&);
    void setupNormals(const std::vector<Base::Vector3f>&);
    void setupLineIndexes(const std::vector<Data::ComplexGeoData::Line>&);
    void setupFaceIndexes(const std::vector<Data::ComplexGeoData::Facet>&);
    void deleteColorBar();

private:
    SoMaterial* pcColorMat;
    SoMaterialBinding* pcMatBinding;
    SoGroup* pcLinkRoot;
    Gui::SoFCColorBar* pcColorBar;
    SoDrawStyle* pcColorStyle;
    SoDrawStyle* pcPointStyle;
    SoSeparator* pcColorRoot;
    SoCoordinate3* pcCoords;

private:
    float search_radius {std::numeric_limits<float>::max()};
    static bool addflag;
    static App::PropertyFloatConstraint::Constraints floatRange;
};

class ViewProviderInspectionGroup: public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(InspectionGui::ViewProviderInspectionGroup);

public:
    /// constructor
    ViewProviderInspectionGroup();
    /// destructor
    ~ViewProviderInspectionGroup() override;
    QIcon getIcon() const override;
};

}  // namespace InspectionGui
