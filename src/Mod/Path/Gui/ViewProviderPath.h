/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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


#ifndef PATH_ViewProviderPath_H
#define PATH_ViewProviderPath_H

#include <App/PropertyGeo.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/SoFCSelection.h>
#include <Gui/ViewProviderPythonFeature.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>

class SoCoordinate3;
class SoDrawStyle;  
class SoMaterial;
class SoBaseColor;
class SoMaterialBinding;
class SoTransform;
class SoSwitch;

namespace PathGui
{

class PathSelectionObserver;

class PathGuiExport ViewProviderPath : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(PathGui::ViewProviderPath);
    typedef ViewProviderGeometryObject inherited;

public:
    /// constructor.
    ViewProviderPath();

    /// destructor.
    ~ViewProviderPath();
    
    // Display properties
    App::PropertyInteger LineWidth;
    App::PropertyColor   NormalColor;
    App::PropertyColor   MarkerColor;
    App::PropertyBool    ShowNodes;
    App::PropertyVector  StartPosition;

    App::PropertyIntegerConstraint StartIndex;
    App::PropertyIntegerConstraint::Constraints  StartIndexConstraints;
    App::PropertyIntegerConstraint ShowCount;
    App::PropertyIntegerConstraint::Constraints  ShowCountConstraints;

    void attach(App::DocumentObject *pcObject);
    void setDisplayMode(const char* ModeName);
    std::vector<std::string> getDisplayModes() const;
    void updateData(const App::Property*);
    void recomputeBoundingBox();
    virtual QIcon getIcon() const;

    virtual bool useNewSelectionModel(void) const;
    virtual std::string getElement(const SoDetail *) const;
    SoDetail* getDetail(const char* subelement) const;

    void updateShowConstraints();
    void updateVisual(bool rebuild = false);
    void hideSelection();

    virtual void showBoundingBox(bool show);

    friend class PathSelectionObserver;

protected:

    virtual void onChanged(const App::Property* prop);
    virtual unsigned long getBoundColor() const;
 
    SoCoordinate3         * pcLineCoords;
    SoCoordinate3         * pcMarkerCoords;
    SoDrawStyle           * pcDrawStyle;
    SoDrawStyle           * pcMarkerStyle;
    PartGui::SoBrepEdgeSet         * pcLines;
    SoMaterial            * pcLineColor;
    SoBaseColor           * pcMarkerColor;
    SoMaterialBinding     * pcMatBind;
    std::vector<int>        colorindex;
    SoSwitch              * pcMarkerSwitch;
    SoSwitch              * pcArrowSwitch;
    SoTransform           * pcArrowTransform;

    std::vector<int>   command2Edge;
    std::deque<int>   edge2Command;
    std::deque<int>   edgeIndices;

    mutable int pt0Index;
    bool blockPropertyChange;
    int edgeStart;
    int coordStart;
    int coordEnd;

 };
 
 typedef Gui::ViewProviderPythonFeatureT<ViewProviderPath> ViewProviderPathPython;

} //namespace PathGui


#endif // PATH_VIEWPROVIDERPATH_H
