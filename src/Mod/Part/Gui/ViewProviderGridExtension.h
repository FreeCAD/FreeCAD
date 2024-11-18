/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef PARTGUI_VIEWPROVIDERGRIDEXTENSION_H
#define PARTGUI_VIEWPROVIDERGRIDEXTENSION_H

#include <Base/Vector3D.h>
#include <Base/Rotation.h>
#include <App/Material.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Gui/ViewProviderExtensionPython.h>

#include <Mod/Part/PartGlobal.h>

namespace PartGui {

class GridExtensionP;

class PartGuiExport ViewProviderGridExtension : public Gui::ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderGridExtension);

public:
    App::PropertyBool ShowGrid;
    App::PropertyLength GridSize;
    App::PropertyBool GridAuto;

    /// constructor
    ViewProviderGridExtension();
    ~ViewProviderGridExtension() override;

    void setGridEnabled(bool enable);

    void drawGrid(bool cameraUpdate);

    void extensionUpdateData(const App::Property*) override;

    SoSeparator* getGridNode();

    void setGridOrientation(Base::Vector3d origin, Base::Rotation rotation);

    /** Return the distance to the closest point in the grid.
    *   The point closer to the grid is returned by reference
    */
    void getClosestGridPoint(double &x, double &y) const;
    double getGridSize() const;

protected:
    void extensionOnChanged(const App::Property*) override;

    // configuration parameters
    void setGridSizePixelThreshold(int value);
    void setGridNumberSubdivision(int value);
    void setGridLinePattern(int pattern);
    void setGridDivLinePattern(int pattern);
    void setGridLineWidth(int width);
    void setGridDivLineWidth(int width);
    void setGridLineColor(const App::Color & color);
    void setGridDivLineColor(const App::Color & color);

    bool extensionHandleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop) override;


private:
    static const char* GridStyleEnums[];
    static App::PropertyQuantityConstraint::Constraints GridSizeRange;

    std::unique_ptr<GridExtensionP> pImpl;


};

using ViewProviderGridExtensionPython = Gui::ViewProviderExtensionPythonT<PartGui::ViewProviderGridExtension>;


} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERGRIDEXTENSION_H

