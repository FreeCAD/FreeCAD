/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
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

#include "FemConstraintDisplacement.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintDisplacement, Fem::Constraint)

ConstraintDisplacement::ConstraintDisplacement()
{
    // six degree of freedoms are possible
    // each dof has three attributes, but only one of them should evaluate to True!
    // Free is True, Free, Fix should be False and Value should be 0.0
    // Fix is True, totally restrained, Free should be False and Value should be 0.0
    // Displacement or Rotation not 0.0, prescribed displacement, Free and Fix should be False

    // x displacement
    ADD_PROPERTY_TYPE(xFree,
                      (true),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Use free translation in X direction");
    ADD_PROPERTY_TYPE(xDisplacement,
                      (0.0),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Translation in local X direction");
    ADD_PROPERTY_TYPE(hasXFormula,
                      (false),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Define translation in X direction as a formula");
    ADD_PROPERTY_TYPE(xDisplacementFormula,
                      (""),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Formula for translation in X direction");

    // y displacement
    ADD_PROPERTY_TYPE(yFree,
                      (true),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Use free translation in Y direction");
    ADD_PROPERTY_TYPE(yDisplacement,
                      (0.0),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Translation in local Y direction");
    ADD_PROPERTY_TYPE(hasYFormula,
                      (false),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Define translation in Y direction as a formula");
    ADD_PROPERTY_TYPE(yDisplacementFormula,
                      (""),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Formula for translation in Y direction");

    // z displacement
    ADD_PROPERTY_TYPE(zFree,
                      (true),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Use free translation in Z direction");
    ADD_PROPERTY_TYPE(zDisplacement,
                      (0.0),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Translation in local Z direction");
    ADD_PROPERTY_TYPE(hasZFormula,
                      (false),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Define translation in Z direction as a formula");
    ADD_PROPERTY_TYPE(zDisplacementFormula,
                      (""),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Formula for translation in Z direction");

    // flow surface force
    ADD_PROPERTY_TYPE(useFlowSurfaceForce,
                      (false),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Use flow surface force");

    // x rotation
    ADD_PROPERTY_TYPE(rotxFree,
                      (true),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Use free rotation in X direction");
    ADD_PROPERTY_TYPE(xRotation,
                      (0.0),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Rotation in local X direction");

    // y rotation
    ADD_PROPERTY_TYPE(rotyFree,
                      (true),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Use free rotation in Y direction");
    ADD_PROPERTY_TYPE(yRotation,
                      (0.0),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Rotation in local Y direction");

    // z rotation
    ADD_PROPERTY_TYPE(rotzFree,
                      (true),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Use free rotation in Z direction");
    ADD_PROPERTY_TYPE(zRotation,
                      (0.0),
                      "ConstraintDisplacement",
                      App::Prop_None,
                      "Rotation in local Z direction");
    ADD_PROPERTY_TYPE(EnableAmplitude,
                      (false),
                      "ConstraintDisplacement",
                      (App::PropertyType)(App::Prop_None),
                      "Amplitude of the displacement boundary condition");
    ADD_PROPERTY_TYPE(AmplitudeValues,
                      (std::vector<std::string> {"0, 0", "1, 1"}),
                      "ConstraintFDisplacement",
                      (App::PropertyType)(App::Prop_None),
                      "Amplitude values");
}

App::DocumentObjectExecReturn* ConstraintDisplacement::execute()
{
    return Constraint::execute();
}

const char* ConstraintDisplacement::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintDisplacement";
}

void ConstraintDisplacement::handleChangedPropertyType(Base::XMLReader& reader,
                                                       const char* TypeName,
                                                       App::Property* prop)
{
    // properties _Displacement had App::PropertyFloat and were changed to App::PropertyDistance
    if (prop == &xDisplacement && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat xDisplacementProperty;
        // restore the PropertyFloat to be able to set its value
        xDisplacementProperty.Restore(reader);
        xDisplacement.setValue(xDisplacementProperty.getValue());
    }
    else if (prop == &yDisplacement && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat yDisplacementProperty;
        yDisplacementProperty.Restore(reader);
        yDisplacement.setValue(yDisplacementProperty.getValue());
    }
    else if (prop == &zDisplacement && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat zDisplacementProperty;
        zDisplacementProperty.Restore(reader);
        zDisplacement.setValue(zDisplacementProperty.getValue());
    }
    // properties _Displacement had App::PropertyFloat and were changed to App::PropertyAngle
    else if (prop == &xRotation && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat xRotationProperty;
        xRotationProperty.Restore(reader);
        xRotation.setValue(xRotationProperty.getValue());
    }
    else if (prop == &yRotation && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat yRotationProperty;
        yRotationProperty.Restore(reader);
        yRotation.setValue(yRotationProperty.getValue());
    }
    else if (prop == &zRotation && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat zRotationProperty;
        zRotationProperty.Restore(reader);
        zRotation.setValue(zRotationProperty.getValue());
    }
    else {
        Constraint::handleChangedPropertyType(reader, TypeName, prop);
    }
}

void ConstraintDisplacement::onChanged(const App::Property* prop)
{
    Constraint::onChanged(prop);
}
