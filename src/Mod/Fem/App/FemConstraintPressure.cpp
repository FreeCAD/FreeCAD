/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
 *   Based on Force constraint by Jan Rheinländer                          *
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

#include "PreCompiled.h"

#include "FemConstraintPressure.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintPressure, Fem::Constraint)

ConstraintPressure::ConstraintPressure()
{
    ADD_PROPERTY(Pressure, (0.0));
    ADD_PROPERTY(Reversed, (0));
    ADD_PROPERTY_TYPE(Points,
                      (Base::Vector3d()),
                      "ConstraintPressure",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Points where arrows are drawn");
    ADD_PROPERTY_TYPE(Normals,
                      (Base::Vector3d()),
                      "ConstraintPressure",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Normals where symbols are drawn");
    Points.setValues(std::vector<Base::Vector3d>());
    Normals.setValues(std::vector<Base::Vector3d>());
}

App::DocumentObjectExecReturn* ConstraintPressure::execute()
{
    return Constraint::execute();
}

const char* ConstraintPressure::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintPressure";
}

void ConstraintPressure::handleChangedPropertyType(Base::XMLReader& reader,
                                                   const char* TypeName,
                                                   App::Property* prop)
{
    // property Pressure had App::PropertyFloat and was changed to App::PropertyPressure
    if (prop == &Pressure && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat PressureProperty;
        // restore the PropertyFloat to be able to set its value
        PressureProperty.Restore(reader);
        // the old implementation or pressure stored the value as MPa
        // therefore we must convert the value with a factor 1000
        Pressure.setValue(PressureProperty.getValue() * 1000.0);
    }
}

void ConstraintPressure::onChanged(const App::Property* prop)
{
    Constraint::onChanged(prop);

    if (prop == &References) {
        std::vector<Base::Vector3d> points;
        std::vector<Base::Vector3d> normals;
        int scale = Scale.getValue();
        if (getPoints(points, normals, &scale)) {
            Points.setValues(points);
            Normals.setValues(normals);
            Scale.setValue(scale);
            Points.touch();
        }
    }
    else if (prop == &Reversed) {
        Points.touch();
    }
}
