/***************************************************************************
 *   Copyright (c) 2024 Kavin Teenakul <andythe_great@protonmail.com>                *
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
 ***************************************************************************/


#pragma once

#include <Mod/Measure/MeasureGlobal.h>

#include <App/Application.h>
#include <App/GeoFeature.h>
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>
#include <Base/Placement.h>

#include <Mod/Part/App/MeasureInfo.h>

#include "MeasureBase.h"

namespace Measure
{

class MeasureExport MeasureDiameter: public Measure::MeasureBaseExtendable<Part::MeasureRadiusInfo>
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureDiameter);

public:
    MeasureDiameter();
    ~MeasureDiameter() override;

    App::PropertyLinkSub Element;
    App::PropertyDistance Diameter;

    App::DocumentObjectExecReturn* execute() override;
    const char* getViewProviderName() const override
    {
        return "MeasureGui::ViewProviderMeasureDiameter";
    }

    static bool isValidSelection(const App::MeasureSelection& selection);
    static bool isPrioritizedSelection(const App::MeasureSelection& selection);
    void parseSelection(const App::MeasureSelection& selection) override;

    std::vector<std::string> getInputProps() override
    {
        return {"Element"};
    }
    App::Property* getResultProp() override
    {
        return &this->Diameter;
    }

    Base::Placement getPlacement() const override;
    Base::Vector3d getPointOnCurve() const;

    std::vector<App::DocumentObject*> getSubject() const override;

private:
    void onChanged(const App::Property* prop) override;
    Part::MeasureRadiusInfoPtr getMeasureInfoFirst() const;
};

}  // namespace Measure
