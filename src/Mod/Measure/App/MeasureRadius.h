// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

#include <App/Application.h>
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>
#include <App/GeoFeature.h>
#include <Base/Placement.h>

#include <Mod/Part/App/MeasureInfo.h>

#include "MeasureBase.h"


namespace Measure
{


class MeasureExport MeasureRadius: public Measure::MeasureBaseExtendable<Part::MeasureRadiusInfo>
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureRadius);

public:
    /// Constructor
    MeasureRadius();
    ~MeasureRadius() override;

    App::PropertyLinkSub Element;
    App::PropertyDistance Radius;

    App::DocumentObjectExecReturn* execute() override;
    const char* getViewProviderName() const override
    {
        return "MeasureGui::ViewProviderMeasureRadius";
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
        return &this->Radius;
    }

    // Return a placement for the viewprovider, just use the first element for now
    Base::Placement getPlacement() const override;
    // Return a point on curve for the viewprovider
    Base::Vector3d getPointOnCurve() const;

    // Return the object we are measuring
    std::vector<App::DocumentObject*> getSubject() const override;


private:
    void onChanged(const App::Property* prop) override;
    Part::MeasureRadiusInfoPtr getMeasureInfoFirst() const;
};

}  // namespace Measure
