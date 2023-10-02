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

#include "FemConstraintPulley.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintPulley, Fem::ConstraintGear)

ConstraintPulley::ConstraintPulley()
{
    ADD_PROPERTY(OtherDiameter, (100.0));
    ADD_PROPERTY(CenterDistance, (500.0));
    ADD_PROPERTY(IsDriven, (0));
    ADD_PROPERTY(TensionForce, (0.0));

    ADD_PROPERTY_TYPE(BeltAngle,
                      (0),
                      "ConstraintPulley",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Angle of belt forces");
    ADD_PROPERTY_TYPE(BeltForce1,
                      (0.0),
                      "ConstraintPulley",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "First belt force");
    ADD_PROPERTY_TYPE(BeltForce2,
                      (0.0),
                      "ConstraintPulley",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Second belt force");
    ForceAngle.setValue(0.0);
    Diameter.setValue(300.0);
    // calculate initial values of read-only properties
    onChanged(&Force);
}

App::DocumentObjectExecReturn* ConstraintPulley::execute()
{
    return ConstraintGear::execute();
}

void ConstraintPulley::onChanged(const App::Property* prop)
{
    ConstraintGear::onChanged(prop);

    if ((prop == &Diameter) || (prop == &OtherDiameter) || (prop == &CenterDistance)) {
        if (CenterDistance.getValue() > Precision::Confusion()) {
            BeltAngle.setValue(asin((Diameter.getValue() - OtherDiameter.getValue()) / 2
                                    / CenterDistance.getValue()));
            BeltAngle.touch();
        }
    }
    else if ((prop == &Force) || (prop == &TensionForce) || (prop == &IsDriven)) {
        double radius = Diameter.getValue() / 2.0;
        if (radius < Precision::Confusion()) {
            return;
        }
        double force = Force.getValue() / (radius / 1000);
        if (fabs(force) < Precision::Confusion()) {
            return;
        }
        bool neg = (force < 0.0);
        if (neg) {
            force *= -1.0;
        }

        if (IsDriven.getValue() == neg) {
            BeltForce1.setValue(force + TensionForce.getValue());
            BeltForce2.setValue(TensionForce.getValue());
        }
        else {
            BeltForce2.setValue(force + TensionForce.getValue());
            BeltForce1.setValue(TensionForce.getValue());
        }
        BeltForce1.touch();
    }
}
