/***************************************************************************
 *   Copyright (c) 2022 Ajinkya Dahale <dahale.a.p@gmail.com>              *
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

#include "FemConstraintRigidBody.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintRigidBody, Fem::Constraint)

const char* ConstraintRigidBody::boundaryModeEnum[] = {"Free", "Constraint", "Load", nullptr};

ConstraintRigidBody::ConstraintRigidBody()
{
    ADD_PROPERTY_TYPE(ReferenceNode,
                      (0.0, 0.0, 0.0),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Reference node position");
    ADD_PROPERTY_TYPE(Displacement,
                      (0.0, 0.0, 0.0),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Reference node displacement");
    ADD_PROPERTY_TYPE(Rotation,
                      (Base::Rotation(0.0, 0.0, 0.0, 1.0)),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Reference node rotation");
    ADD_PROPERTY_TYPE(ForceX,
                      (0.0),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Applied force in X direction");
    ADD_PROPERTY_TYPE(ForceY,
                      (0.0),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Applied force in Y direction");
    ADD_PROPERTY_TYPE(ForceZ,
                      (0.0),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Applied force in Z direction");
    ADD_PROPERTY_TYPE(MomentX,
                      (0.0),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Applied moment in X direction");
    ADD_PROPERTY_TYPE(MomentY,
                      (0.0),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Applied moment in Y direction");
    ADD_PROPERTY_TYPE(MomentZ,
                      (0.0),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Applied moment in Z direction");
    ADD_PROPERTY_TYPE(TranslationalModeX,
                      ("Free"),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "X-direction displacement/force  mode");
    ADD_PROPERTY_TYPE(TranslationalModeY,
                      ("Free"),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Y-direction displacement/force  mode");
    ADD_PROPERTY_TYPE(TranslationalModeZ,
                      ("Free"),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Z-direction displacement/force  mode");
    ADD_PROPERTY_TYPE(RotationalModeX,
                      ("None"),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "X-direction rotation/moment mode");
    ADD_PROPERTY_TYPE(RotationalModeY,
                      ("None"),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Y-direction rotation/moment mode");
    ADD_PROPERTY_TYPE(RotationalModeZ,
                      ("None"),
                      "ConstraintRigidBody",
                      App::Prop_Output,
                      "Z-direction rotation/moment mode");

    TranslationalModeX.setEnums(boundaryModeEnum);
    TranslationalModeY.setEnums(boundaryModeEnum);
    TranslationalModeZ.setEnums(boundaryModeEnum);
    RotationalModeX.setEnums(boundaryModeEnum);
    RotationalModeY.setEnums(boundaryModeEnum);
    RotationalModeZ.setEnums(boundaryModeEnum);
}

App::DocumentObjectExecReturn* ConstraintRigidBody::execute()
{
    return Constraint::execute();
}

void ConstraintRigidBody::onChanged(const App::Property* prop)
{
    Constraint::onChanged(prop);
}
