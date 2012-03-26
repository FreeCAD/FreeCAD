/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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


#ifndef FEM_VIEWPROVIDERFEMMESH_H
#define FEM_VIEWPROVIDERFEMMESH_H

#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/ViewProviderBuilder.h>

class SoCoordinate3;
class SoDrawStyle;  
class SoIndexedFaceSet; 
class SoShapeHints;
class SoMaterialBinding;

namespace FemGui
{

class ViewProviderFEMMeshBuilder : public Gui::ViewProviderBuilder
{
public:
    ViewProviderFEMMeshBuilder(){}
    ~ViewProviderFEMMeshBuilder(){}
    virtual void buildNodes(const App::Property*, std::vector<SoNode*>&) const;
    void createMesh(const App::Property*, SoCoordinate3*, SoIndexedFaceSet*) const;
};

class FemGuiExport ViewProviderFemMesh : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(FemGui::ViewProviderFemMesh);

public:
    /// constructor.
    ViewProviderFemMesh();

    /// destructor.
    ~ViewProviderFemMesh();

    // Display properties
    App::PropertyColor PointColor;
    App::PropertyFloatConstraint PointSize;
    App::PropertyFloatConstraint LineWidth;
    App::PropertyMaterial PointMaterial;

    void attach(App::DocumentObject *pcObject);
    void setDisplayMode(const char* ModeName);
    std::vector<std::string> getDisplayModes() const;
    void updateData(const App::Property*);

private:
    static App::PropertyFloatConstraint::Constraints floatRange;

protected:
    /// get called by the container whenever a property has been changed
    virtual void onChanged(const App::Property* prop);

    SoMaterial            * pcPointMaterial;
    SoDrawStyle           * pcPointStyle;

    SoDrawStyle           * pcDrawStyle;
    SoShapeHints          * pShapeHints;
    SoMaterialBinding     * pcMatBinding;
    SoCoordinate3         * pcCoords;
    SoIndexedFaceSet      * pcFaces;
};

} //namespace FemGui


#endif // FEM_VIEWPROVIDERFEMMESH_H
