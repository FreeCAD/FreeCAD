// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#pragma once

#include <Mod/Measure/MeasureGlobal.h>

#include <string>

#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>
#include <App/GeoFeature.h>

#include <Mod/Part/App/MeasureInfo.h>

#include "MeasureBase.h"


namespace Measure
{


class MeasureExport MeasureArea: public Measure::MeasureBaseExtendable<Part::MeasureAreaInfo>
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureArea);

public:
    /// Constructor
    MeasureArea();
    ~MeasureArea() override;

    App::PropertyLinkSubList Elements;
    App::PropertyArea Area;

    App::DocumentObjectExecReturn* execute() override;

    const char* getViewProviderName() const override
    {
        return "MeasureGui::ViewProviderMeasureArea";
    }

    static bool isValidSelection(const App::MeasureSelection& selection);
    void parseSelection(const App::MeasureSelection& selection) override;

    std::vector<std::string> getInputProps() override
    {
        return {"Elements"};
    }
    App::Property* getResultProp() override
    {
        return &this->Area;
    }

    // Return a placement for the viewprovider, just use the first element for now
    Base::Placement getPlacement() const override;

    // Return the object we are measuring
    std::vector<App::DocumentObject*> getSubject() const override;


private:
    static bool isSupported(App::MeasureElementType type);
    void onChanged(const App::Property* prop) override;
};

}  // namespace Measure
