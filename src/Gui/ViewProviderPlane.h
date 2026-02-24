/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include "ViewProviderDatum.h"

#include "Selection/Selection.h"
#include "ParamHandler.h"


class SoSwitch;
class SoTranslation;
class SoAsciiText;
class SoCoordinate3;

namespace Gui
{

class GuiExport ViewProviderPlane: public ViewProviderDatum, public SelectionObserver
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderPlane);

public:
    /// Constructor
    ViewProviderPlane();
    ~ViewProviderPlane() override;

    void attach(App::DocumentObject*) override;

    unsigned long getColor(const std::string& role) const;
    std::string getRole() const;
    std::string getLabelText(const std::string& role) const;
    void setLabelVisibility(bool val);

    void onSelectionChanged(const SelectionChanges&) override;

private:
    void updatePlaneSize();

    bool isHovered {false};
    bool isSelected {false};

    CoinPtr<SoSwitch> labelSwitch;
    CoinPtr<SoAsciiText> pLabel;
    CoinPtr<SoCoordinate3> pCoords;
    CoinPtr<SoTranslation> pTextTranslation;

    ParamHandlers handlers;
};

}  // namespace Gui
