/***************************************************************************
 *   Copyright (c) 2021 FreeCAD Developers                                 *
 *   Author: Preslav Aleksandrov <preslav.aleksandrov@protonmail.com>      *
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

#include "FemConstraintSpring.h"


static const char* Stiffnesses[] = {"Normal Stiffness", "Tangential Stiffness", nullptr};

using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintSpring, Fem::Constraint)

ConstraintSpring::ConstraintSpring()
{
    ADD_PROPERTY(NormalStiffness, (0.0));
    ADD_PROPERTY(TangentialStiffness, (0.0));
    ADD_PROPERTY(ElmerStiffness, (1));
    ADD_PROPERTY_TYPE(Points,
                      (Base::Vector3d()),
                      "ConstraintSpring",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Points where arrows are drawn");
    ADD_PROPERTY_TYPE(Normals,
                      (Base::Vector3d()),
                      "ConstraintSpring",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Normals where symbols are drawn");

    ElmerStiffness.setEnums(Stiffnesses);
    Points.setValues(std::vector<Base::Vector3d>());
    Normals.setValues(std::vector<Base::Vector3d>());
}

App::DocumentObjectExecReturn* ConstraintSpring::execute()
{
    return Constraint::execute();
}

const char* ConstraintSpring::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintSpring";
}

void ConstraintSpring::onChanged(const App::Property* prop)
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
}
