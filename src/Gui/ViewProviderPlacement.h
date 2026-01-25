/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "AxisOrigin.h"
#include "ViewProviderGeometryObject.h"
#include "ViewProviderFeaturePython.h"


class SoFontStyle;
class SoText2;
class SoBaseColor;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;
class SoEventCallback;
class SoMaterial;

namespace Gui
{

class GuiExport ViewProviderPlacement: public ViewProviderGeometryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderPlacement);

public:
    /// Constructor
    ViewProviderPlacement();
    ~ViewProviderPlacement() override;

    void attach(App::DocumentObject*) override;
    void updateData(const App::Property*) override;
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;

    /// indicates if the ViewProvider use the new Selection model
    bool useNewSelectionModel() const override
    {
        return true;
    }
    /// indicates if the ViewProvider can be selected
    bool isSelectable() const override;

    bool getElementPicked(const SoPickedPoint* pp, std::string& subname) const override;
    bool getDetailPath(const char*, SoFullPath*, bool, SoDetail*&) const override;

protected:
    void onChanged(const App::Property* prop) override;
};

using ViewProviderPlacementPython = ViewProviderFeaturePythonT<ViewProviderPlacement>;

}  // namespace Gui
