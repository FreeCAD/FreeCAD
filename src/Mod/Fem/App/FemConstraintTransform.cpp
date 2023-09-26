/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include "FemConstraintTransform.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintTransform, Fem::Constraint)

static const char* TransformTypes[] = {"Cylindrical", "Rectangular", nullptr};

ConstraintTransform::ConstraintTransform()
{
    ADD_PROPERTY(X_rot, (0.0));
    ADD_PROPERTY(Y_rot, (0.0));
    ADD_PROPERTY(Z_rot, (0.0));
    ADD_PROPERTY_TYPE(TransformType,
                      (1),
                      "ConstraintTransform",
                      (App::PropertyType)(App::Prop_None),
                      "Type of transform, rectangular or cylindrical");
    TransformType.setEnums(TransformTypes);
    ADD_PROPERTY_TYPE(RefDispl,
                      (nullptr, nullptr),
                      "ConstraintTransform",
                      (App::PropertyType)(App::Prop_None),
                      "Elements where the constraint is applied");
    // RefDispl must get a global scope, see
    // https://forum.freecad.org/viewtopic.php?p=671402#p671402
    RefDispl.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(NameDispl,
                      (nullptr),
                      "ConstraintTransform",
                      (App::PropertyType)(App::Prop_None),
                      "Elements where the constraint is applied");
    ADD_PROPERTY_TYPE(BasePoint,
                      (Base::Vector3d(0, 0, 0)),
                      "ConstraintTransform",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Base point of cylindrical surface");
    ADD_PROPERTY_TYPE(Axis,
                      (Base::Vector3d(0, 1, 0)),
                      "ConstraintTransform",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Axis of cylindrical surface");
    ADD_PROPERTY_TYPE(Points,
                      (Base::Vector3d()),
                      "ConstraintTransform",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Points where symbols are drawn");
    ADD_PROPERTY_TYPE(Normals,
                      (Base::Vector3d()),
                      "ConstraintTransform",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Normals where symbols are drawn");
    Points.setValues(std::vector<Base::Vector3d>());
    Normals.setValues(std::vector<Base::Vector3d>());
}

App::DocumentObjectExecReturn* ConstraintTransform::execute()
{
    return Constraint::execute();
}

const char* ConstraintTransform::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintTransform";
}

void ConstraintTransform::handleChangedPropertyType(Base::XMLReader& reader,
                                                    const char* TypeName,
                                                    App::Property* prop)
{
    // properties _rot had App::PropertyFloat and were changed to App::PropertyAngle
    if (prop == &X_rot && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat X_rotProperty;
        X_rotProperty.Restore(reader);
        X_rot.setValue(X_rotProperty.getValue());
    }
    else if (prop == &Y_rot && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat Y_rotProperty;
        Y_rotProperty.Restore(reader);
        Y_rot.setValue(Y_rotProperty.getValue());
    }
    else if (prop == &Z_rot && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat Z_rotProperty;
        Z_rotProperty.Restore(reader);
        Z_rot.setValue(Z_rotProperty.getValue());
    }
}

void ConstraintTransform::onChanged(const App::Property* prop)
{
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
            std::string transform_type = TransformType.getValueAsString();
            if (transform_type == "Cylindrical") {
                // Find data of cylinder
                double radius, height;
                Base::Vector3d base, axis;
                if (!getCylinder(radius, height, base, axis)) {
                    return;
                }
                Axis.setValue(axis);
                // Update base point
                base = base + axis * height / 2;
                BasePoint.setValue(base);
                BasePoint.touch();  // This triggers ViewProvider::updateData()
            }
        }
    }
}
