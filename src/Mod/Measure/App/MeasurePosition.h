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


#ifndef APP_MEASUREPOSITION_H
#define APP_MEASUREPOSITION_H

#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <App/PropertyUnits.h>

#include <QTextStream>

#include <Mod/Measure/MeasureGlobal.h>
#include "MeasureBase.h"


namespace Measure
{


class MeasureExport MeasurePosition : public Measure::MeasureBaseExtendable<Base::Vector3d>
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasurePosition);

public:
    /// Constructor
    MeasurePosition();
    ~MeasurePosition() override;

    App::PropertyLinkSub Element;
    App::PropertyPosition Position;

    App::DocumentObjectExecReturn *execute() override;
    void recalculatePosition();

    const char* getViewProviderName() const override {
        return "MeasureGui::ViewProviderMeasure";
    }

    static bool isValidSelection(const App::MeasureSelection& selection);
    void parseSelection(const App::MeasureSelection& selection);
    
    std::vector<std::string> getInputProps() override {return {"Element"};}
    App::Property* getResultProp() override {return &this->Position;}
    QString getResultString() override;

    Base::Placement getPlacement();

    // Return the object we are measuring
    std::vector<App::DocumentObject*> getSubject() const override;

private:

    void onChanged(const App::Property* prop) override;
};

} //namespace Measure


#endif // APP_MEASUREPOSITION_H
