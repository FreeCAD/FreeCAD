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

#include <Geom_Circle.hxx>
#include <TopoDS_Shape.hxx>

#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>

#include <Mod/Part/App/MeasureInfo.h>

#include "MeasureBase.h"

class TopoDS_Edge;
class TopoDS_Wire;

namespace Measure
{

class MeasureDistanceType: public Base::BaseClass
{
public:
    static Base::Type getClassTypeId();
    Base::Type getTypeId() const override;
    static void init();
    static void* create();

private:
    static Base::Type classTypeId;
};


class MeasureExport MeasureDistance: public Measure::MeasureBaseExtendable<Part::MeasureDistanceInfo>
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureDistance);

public:
    /// Constructor
    MeasureDistance();
    ~MeasureDistance() override;

    App::PropertyLinkSub Element1;
    App::PropertyLinkSub Element2;
    App::PropertyDistance Distance;
    App::PropertyDistance DistanceX;
    App::PropertyDistance DistanceY;
    App::PropertyDistance DistanceZ;

    // Position properties for the viewprovider
    App::PropertyVector Position1;
    App::PropertyVector Position2;

    App::DocumentObjectExecReturn* execute() override;

    const char* getViewProviderName() const override
    {
        return "MeasureGui::ViewProviderMeasureDistance";
    }

    static bool isValidSelection(const App::MeasureSelection& selection);
    static bool isPrioritizedSelection(const App::MeasureSelection& selection);
    void parseSelection(const App::MeasureSelection& selection) override;

    std::vector<std::string> getInputProps() override
    {
        return {"Element1", "Element2"};
    }
    App::Property* getResultProp() override
    {
        return &this->Distance;
    }

    bool getShape(App::PropertyLinkSub* prop, TopoDS_Shape& rShape);

    // Return the object we are measuring
    std::vector<App::DocumentObject*> getSubject() const override;


private:
    bool distanceCircleCircle(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    void distanceGeneric(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    void setValues(const gp_Pnt& p1, const gp_Pnt& p2);
    void onChanged(const App::Property* prop) override;
    Handle(Geom_Circle) asCircle(const TopoDS_Shape& shape) const;
    Handle(Geom_Circle) asCircle(const TopoDS_Edge& edge) const;
    Handle(Geom_Circle) asCircle(const TopoDS_Wire& wire) const;
};


class MeasureExport MeasureDistanceDetached: public Measure::MeasureBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureDistanceDetached);

public:
    /// Constructor
    MeasureDistanceDetached();
    ~MeasureDistanceDetached() override;

    App::PropertyDistance Distance;
    App::PropertyDistance DistanceX;
    App::PropertyDistance DistanceY;
    App::PropertyDistance DistanceZ;

    App::PropertyVector Position1;
    App::PropertyVector Position2;

    App::DocumentObjectExecReturn* execute() override;
    void recalculateDistance();

    const char* getViewProviderName() const override
    {
        return "MeasureGui::ViewProviderMeasureDistance";
    }

    static bool isValidSelection(const App::MeasureSelection& selection);
    void parseSelection(const App::MeasureSelection& selection) override;

    std::vector<std::string> getInputProps() override
    {
        return {"Position1", "Position2"};
    }
    App::Property* getResultProp() override
    {
        return &this->Distance;
    }

    // Return the object we are measuring
    std::vector<App::DocumentObject*> getSubject() const override;

    void handleChangedPropertyName(
        Base::XMLReader& reader,
        const char* TypeName,
        const char* PropName
    ) override;

private:
    void onChanged(const App::Property* prop) override;
};


}  // namespace Measure
