/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef PARTGUI_VIEWPROVIDER2DOBJECT_H
#define PARTGUI_VIEWPROVIDER2DOBJECT_H

#include "ViewProvider.h"
#include <App/PropertyUnits.h>
#include <Gui/ViewProviderPythonFeature.h>

class TopoDS_Shape;
class TopoDS_Face;
class SoSeparator;
class SbVec3f;
class SoTransform;

namespace PartGui {


class PartGuiExport ViewProvider2DObject : public PartGui::ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProvider2DObject);

public:
    /// constructor
    ViewProvider2DObject();
    /// destructor
    ~ViewProvider2DObject() override;
    std::vector<std::string> getDisplayModes() const override;
    const char* getDefaultDisplayMode() const override;
};

class PartGuiExport ViewProvider2DObjectGrid : public ViewProvider2DObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProvider2DObjectGrid);

public:
    /// constructor
    ViewProvider2DObjectGrid();
    /// destructor
    ~ViewProvider2DObjectGrid() override;

    /// Property to switch the grid on and off
    App::PropertyBool ShowGrid;
    App::PropertyBool ShowOnlyInEditMode;
    App::PropertyLength GridSize;
    App::PropertyEnumeration GridStyle;
    App::PropertyBool TightGrid;
    App::PropertyBool GridSnap;
    App::PropertyBool GridAutoSize;
    App::PropertyInteger maxNumberOfLines;

    void attach(App::DocumentObject *) override;
    void updateData(const App::Property*) override;

    /// creates the grid
    SoSeparator* createGrid();

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop) override;
    void Restore(Base::XMLReader &reader) override;
    void handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop) override;

    SoSeparator  *GridRoot;

    void updateGridExtent(float minx, float maxx, float miny, float maxy);

    static const char* GridStyleEnums[];
    static App::PropertyQuantityConstraint::Constraints GridSizeRange;

private:
    float MinX;
    float MaxX;
    float MinY;
    float MaxY;
};

using ViewProvider2DObjectPython = Gui::ViewProviderPythonFeatureT<ViewProvider2DObject>;

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDER2DOBJECT_H

