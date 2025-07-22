/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef PARTGUI_ViewProviderDatumCoordinateSystem_H
#define PARTGUI_ViewProviderDatumCoordinateSystem_H

#include "ViewProviderDatum.h"

class SoCoordinate3;
class SoFont;
class SoTranslation;

namespace Gui {
class SoAutoZoomTranslation;
}

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderDatumCoordinateSystem : public PartDesignGui::ViewProviderDatum
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderDatumCoordinateSystem);

public:
    App::PropertyFloatConstraint Zoom;
    App::PropertyIntegerConstraint FontSize;
    App::PropertyBool ShowLabel;

    /// Constructor
    ViewProviderDatumCoordinateSystem();
    ~ViewProviderDatumCoordinateSystem() override;

    void attach ( App::DocumentObject *obj ) override;
    void updateData(const App::Property*) override;
    void onChanged(const App::Property*) override;

    void setExtents (Base::BoundBox3d bbox) override;

    SoDetail* getDetail(const char* subelement) const override;
    std::string getElement(const SoDetail* detail) const override;

private:
    void setupLabels();

private:
    SoCoordinate3 *coord;
    SoTranslation *axisLabelXTrans;
    SoTranslation *axisLabelXToYTrans;
    SoTranslation *axisLabelYToZTrans;
    SoFont* font;
    SoSwitch *labelSwitch;
    Gui::SoAutoZoomTranslation *autoZoom;
};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderDatumCoordinateSystem_H
