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


#ifndef INSPECTIOGUI_VIEWPROVIDERINSPECTION_H
#define INSPECTIOGUI_VIEWPROVIDERINSPECTION_H

#include <Base/Observer.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderDocumentObjectGroup.h>

class SoGroup;
class SoMaterial;
class SoMaterialBinding;
class SoDrawStyle;
class SoSeparator;
class SoCoordinate3;

namespace Gui {
    class SoFCColorBar;
    class View3DInventorViewer;
}

namespace InspectionGui {

/**
 * @author Werner Mayer
 */
class ViewProviderInspection : public Gui::ViewProviderDocumentObject,
                               public Base::Observer<int>{
    typedef ViewProviderDocumentObject inherited;

    PROPERTY_HEADER(InspectionGui::ViewProviderInspection);

public:
    ViewProviderInspection();
    virtual ~ViewProviderInspection();

    App::PropertyBool OutsideGrayed;
    App::PropertyFloatConstraint PointSize;

    void attach(App::DocumentObject *pcFeat);
    /// Sets the viewing mode
    void setDisplayMode(const char* ModeName);
    /// Returns a list of all possible modes
    std::vector<std::string> getDisplayModes(void) const;
    /// Update colorbar after recomputation of distances.
    void updateData(const App::Property*);
    /// Once the color bar settings has been changed this method gets called to update the feature's representation
    void OnChange(Base::Subject<int> &rCaller,int rcReason);
    QIcon getIcon() const;
    /// Returns a color bar
    SoSeparator* getFrontRoot(void) const;
    /// Hide the object in the view
    virtual void hide(void);
    /// Show the object in the view
    virtual void show(void);

    static void inspectCallback(void * ud, SoEventCallback * n);

protected:
    void onChanged(const App::Property* prop);
    void setDistances();
    QString inspectDistance(const SoPickedPoint* pp) const;

protected:
    SoMaterial       * pcColorMat;
    SoMaterialBinding* pcMatBinding;
    SoGroup          * pcLinkRoot;
    Gui::SoFCColorBar* pcColorBar;
    SoDrawStyle      * pcColorStyle;
    SoDrawStyle      * pcPointStyle;
    SoSeparator      * pcColorRoot;
    SoCoordinate3    * pcCoords;

private:
    float search_radius;
    static bool addflag;
    static App::PropertyFloatConstraint::Constraints floatRange;
};

class ViewProviderInspectionGroup : public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER(InspectionGui::ViewProviderInspectionGroup);

public:
    /// constructor
    ViewProviderInspectionGroup();
    /// destructor
    virtual ~ViewProviderInspectionGroup();
    QIcon getIcon() const;
};

} // namespace InspectionGui


#endif // INSPECTIOGUI_VIEWPROVIDERINSPECTION_H

