/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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


#ifndef MEASURE_MEASUREAREA_H
#define MEASURE_MEASUREAREA_H

#include <Mod/Measure/MeasureGlobal.h>

#include <functional>
#include <string.h>
#include <map>
#include <tuple>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <TopExp.hxx>

#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>
#include <App/GeoFeature.h>

#include <Mod/Part/App/TopoShape.h>

#include "MeasureBase.h"
#include "MeasureInfo.h"


namespace Measure
{

class MeasureExport MeasureAreaInfo : public MeasureInfo {
public:
    MeasureAreaInfo() = default;
    MeasureAreaInfo(bool val, double a2, Base::Placement plm) { valid = val; area = a2; placement = plm;};
    ~MeasureAreaInfo() = default;

    double area{0};
    Base::Placement placement{};
};


class MeasureExport MeasureArea : public Measure::MeasureBaseExtendable<MeasureAreaInfo>
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureArea);

public:
    /// Constructor
    MeasureArea();
    ~MeasureArea() override;

    App::PropertyLinkSubList Elements;
    App::PropertyArea Area;

    App::DocumentObjectExecReturn *execute() override;
    void recalculateArea();

    const char* getViewProviderName() const override {
        return "MeasureGui::ViewProviderMeasure";
    }

    static bool isValidSelection(const App::MeasureSelection& selection);
    void parseSelection(const App::MeasureSelection& selection) override;

    std::vector<std::string> getInputProps() override {return {"Elements"};}
    App::Property* getResultProp() override {return &this->Area;}

    // Return a placement for the viewprovider, just use the first element for now
    Base::Placement getPlacement();

    // Return the object we are measuring
    std::vector<App::DocumentObject*> getSubject() const override;


private:
    void onChanged(const App::Property* prop) override;

};

} //namespace Measure


#endif // MEASURE_MEASUREAREA_H

