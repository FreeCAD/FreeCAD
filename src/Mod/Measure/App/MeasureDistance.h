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


#ifndef MEASUREAPP_MEASUREDISTANCE_H
#define MEASUREAPP_MEASUREDISTANCE_H

#include <Mod/Measure/MeasureGlobal.h>

#include <TopoDS_Shape.hxx>

#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>

#include <Mod/Part/App/MeasureInfo.h>

#include "MeasureBase.h"

namespace Measure
{



class MeasureExport MeasureDistance : public Measure::MeasureBaseExtendable<Part::MeasureDistanceInfo>
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureDistance);

public:
    /// Constructor
    MeasureDistance();
    ~MeasureDistance() override;

    App::PropertyLinkSub Element1;
    App::PropertyLinkSub Element2;
    App::PropertyDistance Distance;

    // Position properties for the viewprovider
    App::PropertyVector Position1;
    App::PropertyVector Position2;

    App::DocumentObjectExecReturn *execute() override;

    const char* getViewProviderName() const override {
        return "MeasureGui::ViewProviderMeasureDistance";
    }

    static bool isValidSelection(const App::MeasureSelection& selection);
    static bool isPrioritizedSelection(const App::MeasureSelection& selection);
    void parseSelection(const App::MeasureSelection& selection) override;

    std::vector<std::string> getInputProps() override {return {"Element1", "Element2"};}
    App::Property* getResultProp() override {return &this->Distance;}

    bool getShape(App::PropertyLinkSub* prop, TopoDS_Shape& rShape);

    // Return the object we are measuring
    std::vector<App::DocumentObject*> getSubject() const override;


private:

    void onChanged(const App::Property* prop) override;
};

} //namespace Measure


#endif // MEASUREAPP_MEASUREDISTANCE_H
