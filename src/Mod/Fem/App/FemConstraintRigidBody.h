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

#ifndef FEM_CONSTRAINTRIGIDBODY_H
#define FEM_CONSTRAINTRIGIDBODY_H

#include "FemConstraint.h"


namespace Fem
{

class FemExport ConstraintRigidBody: public Fem::Constraint
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::ConstraintRigidBody);

public:
    /// Constructor
    ConstraintRigidBody();

    // Read-only (calculated values). These trigger changes in the ViewProvider
    App::PropertyVectorList Points;
    App::PropertyVectorList Normals;

    // Rigid Body parameters
    App::PropertyFloat xRefNode;
    App::PropertyFloat yRefNode;
    App::PropertyFloat zRefNode;
    App::PropertyFloat xDisplacement;
    App::PropertyFloat yDisplacement;
    App::PropertyFloat zDisplacement;
    App::PropertyFloat xRotation;
    App::PropertyFloat yRotation;
    App::PropertyFloat zRotation;
    App::PropertyFloat xLoad;
    App::PropertyFloat yLoad;
    App::PropertyFloat zLoad;
    App::PropertyFloat xMoment;
    App::PropertyFloat yMoment;
    App::PropertyFloat zMoment;
    App::PropertyBool DefineRefNode;

    /// recalculate the object
    App::DocumentObjectExecReturn* execute() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemConstraintRigidBody";
    }

protected:
    void onChanged(const App::Property* prop) override;
};

}  // namespace Fem


#endif  // FEM_CONSTRAINTRIGIDBODY_H
