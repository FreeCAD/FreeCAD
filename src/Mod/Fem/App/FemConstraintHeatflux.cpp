/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
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

#include "FemConstraintHeatflux.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintHeatflux, Fem::Constraint)

static const char* ConstraintTypes[] = {"DFlux", "Convection", nullptr};

ConstraintHeatflux::ConstraintHeatflux()
{
    ADD_PROPERTY(AmbientTemp, (0.0));
    /*ADD_PROPERTY(FaceTemp,(0.0));*/
    ADD_PROPERTY(FilmCoef, (0.0));
    ADD_PROPERTY(DFlux, (0.0));
    ADD_PROPERTY_TYPE(ConstraintType,
                      (1),
                      "ConstraintHeatflux",
                      (App::PropertyType)(App::Prop_None),
                      "Type of constraint, surface convection or surface heat flux");
    ConstraintType.setEnums(ConstraintTypes);

    ADD_PROPERTY_TYPE(Points,
                      (Base::Vector3d()),
                      "ConstraintHeatflux",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Points where symbols are drawn");
    ADD_PROPERTY_TYPE(Normals,
                      (Base::Vector3d()),
                      "ConstraintHeatflux",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Normals where symbols are drawn");
    Points.setValues(std::vector<Base::Vector3d>());
    Normals.setValues(std::vector<Base::Vector3d>());
}

App::DocumentObjectExecReturn* ConstraintHeatflux::execute()
{
    return Constraint::execute();
}

const char* ConstraintHeatflux::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintHeatflux";
}

void ConstraintHeatflux::onChanged(const App::Property* prop)
{
    // Note: If we call this at the end, then the arrows are not oriented correctly initially
    // because the NormalDirection has not been calculated yet
    Constraint::onChanged(prop);

    if (prop == &References) {
        std::vector<Base::Vector3d> points;
        std::vector<Base::Vector3d> normals;
        int scale = 1;  // OvG: Enforce use of scale
        if (getPoints(points, normals, &scale)) {
            Points.setValues(points);
            Normals.setValues(normals);
            Scale.setValue(scale);  // OvG: Scale
            Points.touch();         // This triggers ViewProvider::updateData()
        }
    }
}
