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

#include "FemConstraintContact.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintContact, Fem::Constraint)

ConstraintContact::ConstraintContact()
{
    /*Note: Initialise parameters here*/
    ADD_PROPERTY_TYPE(Slope,
                      (0.0),
                      "ConstraintContact",
                      App::PropertyType(App::Prop_None),
                      "Contact stiffness");
    ADD_PROPERTY_TYPE(Adjust,
                      (0.0),
                      "ConstraintContact",
                      App::PropertyType(App::Prop_None),
                      "Node clearance adjustment limit");
    ADD_PROPERTY_TYPE(Friction,
                      (false),
                      "ConstraintContact",
                      App::PropertyType(App::Prop_None),
                      "Enable friction interaction");
    ADD_PROPERTY_TYPE(FrictionCoefficient,
                      (0.0),
                      "ConstraintContact",
                      App::PropertyType(App::Prop_None),
                      "Friction coefficient");
    ADD_PROPERTY_TYPE(StickSlope,
                      (0.0),
                      "ConstraintContact",
                      App::PropertyType(App::Prop_None),
                      "Stick slope");
    ADD_PROPERTY_TYPE(EnableThermalContact,
                      (false),
                      "ConstraintContact",
                      App::PropertyType(App::Prop_None),
                      "Enable thermal contact");
    ADD_PROPERTY_TYPE(ThermalContactConductance,
                      (std::vector<std::string> {}),
                      "ConstraintContact",
                      App::PropertyType(App::Prop_None),
                      "Thermal contact conductance");
    ADD_PROPERTY_TYPE(HardContact,
                      (false),
                      "ConstraintContact",
                      App::PropertyType(App::Prop_None),
                      "Enable hard contact");
}

App::DocumentObjectExecReturn* ConstraintContact::execute()
{
    return Constraint::execute();
}

const char* ConstraintContact::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintContact";
}

void ConstraintContact::onChanged(const App::Property* prop)
{
    Constraint::onChanged(prop);
}

void ConstraintContact::handleChangedPropertyType(Base::XMLReader& reader,
                                                  const char* typeName,
                                                  App::Property* prop)
{
    if (prop == &Slope && strcmp(typeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat oldSlope;
        oldSlope.Restore(reader);
        // old slope value stored as MPa/mm equivalent to 1e3 kg/(mm^2*s^2)
        Slope.setValue(oldSlope.getValue() * 1000);

        // stick slope internally generated as slope/10
        StickSlope.setValue(Slope.getValue() / 10);
    }
    else if (prop == &Friction && strcmp(typeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat oldFriction;
        oldFriction.Restore(reader);
        FrictionCoefficient.setValue(oldFriction.getValue());

        Friction.setValue(oldFriction.getValue() > 0 ? true : false);
    }
    else {
        Constraint::handleChangedPropertyType(reader, typeName, prop);
    }
}
