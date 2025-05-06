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

#include <Base/Tools.h>
#include <Mod/Part/App/PartFeature.h>

#include "FemConstraintTransform.h"
#include "FemTools.h"

using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintTransform, Fem::Constraint)

static const char* TransformTypes[] = {"Cylindrical", "Rectangular", nullptr};

ConstraintTransform::ConstraintTransform()
{
    ADD_PROPERTY_TYPE(Rotation,
                      (Base::Rotation(0.0, 0.0, 0.0, 1.0)),
                      "ConstraintTransform",
                      App::Prop_Output,
                      "Rectangular system transform");
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
}

App::DocumentObjectExecReturn* ConstraintTransform::execute()
{
    return Constraint::execute();
}

const char* ConstraintTransform::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintTransform";
}

void ConstraintTransform::onChanged(const App::Property* prop)
{
    if (prop == &References) {
        std::string transform_type = TransformType.getValueAsString();
        if (transform_type == "Cylindrical") {
            // Extract geometry from References
            std::vector<App::DocumentObject*> ref = References.getValues();
            std::vector<std::string> subRef = References.getSubValues();
            if (ref.empty()) {
                return;
            }

            Part::Feature* feat = static_cast<Part::Feature*>(ref.front());
            TopoDS_Shape sh = Tools::getFeatureSubShape(feat, subRef.front().c_str(), true);

            Base::Vector3d axis, base;
            double height, radius;
            if (!Tools::getCylinderParams(sh, base, axis, height, radius)) {
                return;
            }

            BasePoint.setValue(base);
            Axis.setValue(axis);
        }
    }

    Constraint::onChanged(prop);
}

namespace
{

Base::Rotation anglesToRotation(double xAngle, double yAngle, double zAngle)
{
    static Base::Vector3d a(1, 0, 0);
    static Base::Vector3d b(0, 1, 0);
    static int count = 0;
    double xRad = Base::toRadians(xAngle);
    double yRad = Base::toRadians(yAngle);
    double zRad = Base::toRadians(zAngle);
    if (xAngle != 0) {
        a[1] = 0;
        a[2] = 0;
        b[1] = std::cos(xRad);
        b[2] = -std::sin(xRad);
    }
    if (yAngle != 0) {
        a[0] = std::cos(yRad);
        a[2] = std::sin(yRad);
        b[0] = 0;
        b[2] = 0;
    }
    if (zAngle != 0) {
        a[0] = std::cos(zRad);
        a[1] = -std::sin(zRad);
        b[0] = 0;
        b[1] = 0;
    }

    ++count;
    count %= 3;
    if (!count) {
        Base::Vector3d X = a.Normalize();
        Base::Vector3d Y = b.Normalize();
        Base::Vector3d Z = X.Cross(Y);
        Z.Normalize();
        Y = Z.Cross(X);

        a.x = 1;
        a.y = 0;
        a.z = 0;
        b.x = 0;
        b.y = 1;
        b.z = 0;

        Base::Matrix4D m;
        m.setCol(0, X);
        m.setCol(1, Y);
        m.setCol(2, Z);

        return Base::Rotation(m);
    }
    return Base::Rotation();
}

}  // namespace


void ConstraintTransform::handleChangedPropertyName(Base::XMLReader& reader,
                                                    const char* typeName,
                                                    const char* propName)
{
    if (strcmp(propName, "X_rot") == 0) {
        double xAngle {};
        if (strcmp(typeName, "App::PropertyFloat") == 0) {
            App::PropertyFloat X_rotProperty;
            X_rotProperty.Restore(reader);
            xAngle = X_rotProperty.getValue();
        }
        else if (strcmp(typeName, "App::PropertyAngle") == 0) {
            App::PropertyAngle X_rotProperty;
            X_rotProperty.Restore(reader);
            xAngle = X_rotProperty.getValue();
        }
        anglesToRotation(xAngle, 0, 0);
    }
    else if (strcmp(propName, "Y_rot") == 0) {
        double yAngle {};
        if (strcmp(typeName, "App::PropertyFloat") == 0) {
            App::PropertyFloat Y_rotProperty;
            Y_rotProperty.Restore(reader);
            yAngle = Y_rotProperty.getValue();
        }
        else if (strcmp(typeName, "App::PropertyAngle") == 0) {
            App::PropertyAngle Y_rotProperty;
            Y_rotProperty.Restore(reader);
            yAngle = Y_rotProperty.getValue();
        }
        anglesToRotation(0, yAngle, 0);
    }
    else if (strcmp(propName, "Z_rot") == 0) {
        double zAngle {};
        if (strcmp(typeName, "App::PropertyFloat") == 0) {
            App::PropertyFloat Z_rotProperty;
            Z_rotProperty.Restore(reader);
            zAngle = Z_rotProperty.getValue();
        }
        else if (strcmp(typeName, "App::PropertyAngle") == 0) {
            App::PropertyAngle Z_rotProperty;
            Z_rotProperty.Restore(reader);
            zAngle = Z_rotProperty.getValue();
        }
        Rotation.setValue(anglesToRotation(0, 0, zAngle));
    }
    else {
        Constraint::handleChangedPropertyName(reader, typeName, propName);
    }
}
