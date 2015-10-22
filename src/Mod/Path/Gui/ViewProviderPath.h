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

#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/SoFCSelection.h>
#include <Gui/ViewProviderPythonFeature.h>

class SoCoordinate3;
class SoDrawStyle;  
class SoIndexedLineSet;
class SoMaterial;
class SoBaseColor;
class SoMaterialBinding;
class SoTransform;

namespace PathGui
{

class PathGuiExport ViewProviderPath : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(PathGui::ViewProviderPath);

public:
    /// constructor.
    ViewProviderPath();

    /// destructor.
    ~ViewProviderPath();
    
    // Display properties
    App::PropertyInteger LineWidth;
    App::PropertyColor   NormalColor;
    App::PropertyColor   MarkerColor;
    App::PropertyBool    ShowFirstRapid;

    void attach(App::DocumentObject *pcObject);
    void setDisplayMode(const char* ModeName);
    std::vector<std::string> getDisplayModes() const;
    void updateData(const App::Property*);
    void recomputeBoundingBox();
    virtual QIcon getIcon() const;

protected:

    virtual void onChanged(const App::Property* prop);
 
    Gui::SoFCSelection    * pcPathRoot;
    SoTransform           * pcTransform;
    SoCoordinate3         * pcLineCoords;
    SoCoordinate3         * pcMarkerCoords;
    SoDrawStyle           * pcDrawStyle;
    SoIndexedLineSet      * pcLines;
    SoMaterial            * pcLineColor;
    SoBaseColor           * pcMarkerColor;
    SoMaterialBinding     * pcMatBind;
    std::vector<int>        colorindex;

 };
 
 typedef Gui::ViewProviderPythonFeatureT<ViewProviderPath> ViewProviderPathPython;

} //namespace PathGui


#endif // PATH_VIEWPROVIDERPATH_H
