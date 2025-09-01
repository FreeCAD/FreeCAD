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

#ifndef _PreComp_
#include <Precision.hxx>
#endif

#include "FemConstraintForce.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintForce, Fem::Constraint)

ConstraintForce::ConstraintForce()
{
    ADD_PROPERTY(Force, (0.0));
    ADD_PROPERTY_TYPE(Direction,
                      (nullptr),
                      "ConstraintForce",
                      (App::PropertyType)(App::Prop_None),
                      "Element giving direction of constraint");
    // RefDispl must get a global scope, see
    Direction.setScope(App::LinkScope::Global);

    ADD_PROPERTY(Reversed, (0));
    ADD_PROPERTY_TYPE(DirectionVector,
                      (Base::Vector3d(0, 0, 1)),
                      "ConstraintForce",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Direction of arrows");

    // by default use the null vector to indicate an invalid value
    naturalDirectionVector = Base::Vector3d(0, 0, 0);
    ADD_PROPERTY_TYPE(EnableAmplitude,
                      (false),
                      "ConstraintForce",
                      (App::PropertyType)(App::Prop_None),
                      "Amplitude of the force load");
    ADD_PROPERTY_TYPE(AmplitudeValues,
                      (std::vector<std::string> {"0, 0", "1, 1"}),
                      "ConstraintForce",
                      (App::PropertyType)(App::Prop_None),
                      "Amplitude values");
}

App::DocumentObjectExecReturn* ConstraintForce::execute()
{
    return Constraint::execute();
}

void ConstraintForce::handleChangedPropertyType(Base::XMLReader& reader,
                                                const char* TypeName,
                                                App::Property* prop)
{
    // property Force had App::PropertyFloat, was changed to App::PropertyForce
    if (prop == &Force && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat ForceProperty;
        // restore the PropertyFloat to be able to set its value
        ForceProperty.Restore(reader);
        // force uses m while FreeCAD uses internally mm thus
        // e.g. "2.5" must become 2500 to result in 2.5 N
        Force.setValue(ForceProperty.getValue() * 1000);
    }
    else {
        Constraint::handleChangedPropertyType(reader, TypeName, prop);
    }
}

void ConstraintForce::onChanged(const App::Property* prop)
{
    // Note: If we call this at the end, then the arrows are not oriented correctly initially
    // because the NormalDirection has not been calculated yet
    Constraint::onChanged(prop);

    if (prop == &Direction) {
        Base::Vector3d direction = getDirection(Direction);
        if (direction.Length() < Precision::Confusion()) {
            return;
        }
        naturalDirectionVector = direction;
        if (Reversed.getValue()) {
            direction = -direction;
        }
        DirectionVector.setValue(direction);
    }
    else if (prop == &Reversed) {
        // if the direction is invalid try to compute it again
        if (naturalDirectionVector.Length() < Precision::Confusion()) {
            naturalDirectionVector = getDirection(Direction);
        }
        if (naturalDirectionVector.Length() >= Precision::Confusion()) {
            if (Reversed.getValue() && (DirectionVector.getValue() == naturalDirectionVector)) {
                DirectionVector.setValue(-naturalDirectionVector);
            }
            else if (!Reversed.getValue()
                     && (DirectionVector.getValue() != naturalDirectionVector)) {
                DirectionVector.setValue(naturalDirectionVector);
            }
        }
    }
    else if (prop == &NormalDirection) {
        // Set a default direction if no direction reference has been given
        if (!Direction.getValue()) {
            Base::Vector3d direction = NormalDirection.getValue();
            if (Reversed.getValue()) {
                direction = -direction;
            }
            DirectionVector.setValue(direction);
            naturalDirectionVector = direction;
        }
    }
}
