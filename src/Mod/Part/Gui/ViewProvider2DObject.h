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


#ifndef PARTGUI_IEWPROVIDER2DOBJECT_H
#define PARTGUI_IEWPROVIDER2DOBJECT_H

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
    PROPERTY_HEADER(PartGui::ViewProvider2DObject);

public:
    /// constructor
    ViewProvider2DObject();
    /// destructor
    virtual ~ViewProvider2DObject();
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual const char* getDefaultDisplayMode() const;
};

class PartGuiExport ViewProvider2DObjectGrid : public ViewProvider2DObject
{
    PROPERTY_HEADER(PartGui::ViewProvider2DObjectGrid);

public:
    /// constructor
    ViewProvider2DObjectGrid();
    /// destructor
    virtual ~ViewProvider2DObjectGrid();

    /// Property to switch the grid on and off
    App::PropertyBool ShowGrid;
    App::PropertyBool ShowOnlyInEditMode;
    App::PropertyLength GridSize;
    App::PropertyEnumeration GridStyle;
    App::PropertyBool TightGrid;
    App::PropertyBool GridSnap;
    App::PropertyBool GridAutoSize;
    App::PropertyInteger maxNumberOfLines;

    virtual void attach(App::DocumentObject *);
    virtual void updateData(const App::Property*);

    /// creates the grid
    SoSeparator* createGrid(void);

protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    /// get called by the container whenever a property has been changed
    virtual void onChanged(const App::Property* prop);
    virtual void Restore(Base::XMLReader &reader);
    virtual void handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop);

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

typedef Gui::ViewProviderPythonFeatureT<ViewProvider2DObject> ViewProvider2DObjectPython;

} // namespace PartGui


#endif // PARTGUI_IEWPROVIDER2DOBJECT_H

