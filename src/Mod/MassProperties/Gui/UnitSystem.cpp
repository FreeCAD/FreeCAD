// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "UnitSystem.h"

#include <App/Application.h>
#include <App/Document.h>

#include <Base/Parameter.h>
#include <Base/UnitsApi.h>

using namespace MassPropertiesGui;

const char* UnitSystem::metric_mm_kg = "mm, kg, kg·mm²";
const char* UnitSystem::metric_m_kg = "m, kg, kg·m²";
const char* UnitSystem::imperial_inch_lb = "in, lb, lb·in²";
const char* UnitSystem::imperial_ft_lb = "ft, lb, lb·ft²";

UnitConversions UnitSystem::getConversions(const std::string& systemName)
{
    UnitConversions conv;
    
    if (systemName == metric_mm_kg) {
        conv.lengthFactor = 1.0;
        conv.volumeFactor = 1.0;
        conv.massFactor = 1.0;
        conv.densityFactor = 1.0;
        conv.areaFactor = 1.0;
        conv.inertiaFactor = 1.0;
        
        conv.lengthUnit = "mm";
        conv.volumeUnit = "mm³";
        conv.massUnit = "kg";
        conv.densityUnit = "kg/m³";
        conv.areaUnit = "mm²";
        conv.inertiaUnit = "kg·mm²";
    }
    else if (systemName == metric_m_kg) {
        conv.lengthFactor = 0.001;
        conv.volumeFactor = 1.0e-9;
        conv.massFactor = 1.0;
        conv.densityFactor = 1.0;
        conv.areaFactor = 1.0e-6;
        conv.inertiaFactor = 1.0e-6;
        
        conv.lengthUnit = "m";
        conv.volumeUnit = "m³";
        conv.massUnit = "kg";
        conv.densityUnit = "kg/m³";
        conv.areaUnit = "m²";
        conv.inertiaUnit = "kg·m²";
    }
    else if (systemName == imperial_inch_lb) {
        conv.lengthFactor = 0.0393701;      
        conv.volumeFactor = 6.10237e-5;     
        conv.massFactor = 2.20462;         
        conv.densityFactor = 0.06242796;  
        conv.areaFactor = 0.00155;          
        conv.inertiaFactor = 0.0034171719;    
        
        conv.lengthUnit = "in";
        conv.volumeUnit = "in³";
        conv.massUnit = "lb";
        conv.densityUnit = "lb/ft³";
        conv.areaUnit = "in²";
        conv.inertiaUnit = "lb·in²";
    }
    else if (systemName == imperial_ft_lb) {
        conv.lengthFactor = 0.00328084;      
        conv.volumeFactor = 3.53147e-8;     
        conv.massFactor = 2.20462;         
        conv.densityFactor = 0.06242796;  
        conv.areaFactor = 1.07639e-5;          
        conv.inertiaFactor = 2.371056e-5;    
        
        conv.lengthUnit = "ft";
        conv.volumeUnit = "ft³";
        conv.massUnit = "lb";
        conv.densityUnit = "lb/ft³";
        conv.areaUnit = "ft²";
        conv.inertiaUnit = "lb·ft²";
    }
    else {
        conv.lengthFactor = 1.0;
        conv.volumeFactor = 1.0;
        conv.massFactor = 1.0;
        conv.densityFactor = 1.0;
        conv.areaFactor = 1.0;
        conv.inertiaFactor = 1.0;
        
        conv.lengthUnit = "mm";
        conv.volumeUnit = "mm³";
        conv.massUnit = "kg";
        conv.densityUnit = "kg/m³";
        conv.areaUnit = "mm²";
        conv.inertiaUnit = "kg·mm²";
    }
    
    return conv;
}

std::string UnitSystem::getPreferredSystemName()
{
    ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Units"
    );
    bool ignoreProjectSchema = hGrpu->GetBool("IgnoreProjectSchema", false);
    int userSchema = hGrpu->GetInt("UserSchema", 0);

    if (App::Document* doc = App::GetApplication().getActiveDocument()) {
        if (!ignoreProjectSchema) {
            userSchema = doc->UnitSystem.getValue();
        }
    }

    if (userSchema < 0 || static_cast<std::size_t>(userSchema) >= Base::UnitsApi::count()) {
        userSchema = 0;
    }

    auto schema = Base::UnitsApi::createSchema(static_cast<std::size_t>(userSchema));
    const std::string lengthUnit = schema ? schema->getBasicLengthUnit() : "mm";

    if (lengthUnit == "mm") {
        return metric_mm_kg;
    }
    if (lengthUnit == "m") {
        return metric_m_kg;
    }
    if (lengthUnit == "in") {
        return imperial_inch_lb;
    }
    if (lengthUnit == "ft") {
        return imperial_ft_lb;
    }

    return metric_mm_kg;
}

int UnitSystem::getSystemIndex(const std::string& systemName)
{
    if (systemName == metric_mm_kg) {
        return 0;
    }
    if (systemName == metric_m_kg) {
        return 1;
    }
    if (systemName == imperial_inch_lb) {
        return 2;
    }
    if (systemName == imperial_ft_lb) {
        return 3;
    }
    return 0;
}