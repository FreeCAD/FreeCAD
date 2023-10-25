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

#ifndef FEM_CONSTRAINTDISPLACEMENT_H
#define FEM_CONSTRAINTDISPLACEMENT_H

#include "FemConstraint.h"


namespace Fem
{

class FemExport ConstraintDisplacement: public Fem::Constraint
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::ConstraintDisplacement);

public:
    /// Constructor
    ConstraintDisplacement();

    // Read-only (calculated values). These trigger changes in the ViewProvider
    App::PropertyVectorList Points;
    App::PropertyVectorList Normals;

    // Displacement parameters
    App::PropertyDistance xDisplacement;
    App::PropertyDistance yDisplacement;
    App::PropertyDistance zDisplacement;
    App::PropertyAngle xRotation;
    App::PropertyAngle yRotation;
    App::PropertyAngle zRotation;
    App::PropertyString xDisplacementFormula;
    App::PropertyString yDisplacementFormula;
    App::PropertyString zDisplacementFormula;
    App::PropertyBool xFree;
    App::PropertyBool yFree;
    App::PropertyBool zFree;
    App::PropertyBool xFix;
    App::PropertyBool yFix;
    App::PropertyBool zFix;
    App::PropertyBool rotxFree;
    App::PropertyBool rotyFree;
    App::PropertyBool rotzFree;
    App::PropertyBool rotxFix;
    App::PropertyBool rotyFix;
    App::PropertyBool rotzFix;
    App::PropertyBool hasXFormula;
    App::PropertyBool hasYFormula;
    App::PropertyBool hasZFormula;
    App::PropertyBool useFlowSurfaceForce;

    /// recalculate the object
    App::DocumentObjectExecReturn* execute() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override;

protected:
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   App::Property* prop) override;
    void onChanged(const App::Property* prop) override;
};

}  // namespace Fem


#endif  // FEM_CONSTRAINTDISPLACEMENT_H
