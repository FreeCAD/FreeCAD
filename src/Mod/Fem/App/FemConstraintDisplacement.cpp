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

#ifndef _PreComp_
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#endif

#include "FemConstraintDisplacement.h"

using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintDisplacement, Fem::Constraint);

ConstraintDisplacement::ConstraintDisplacement()
{
    // six degree of freedoms are possible
    // each dof has three attributes, but only one of them should evaluate to True!
    // Free is True, Free, Fix should be False and Value should be 0.0
    // Fix is True, totally restrained, Free should be False and Value should be 0.0
    // Displacment or Rotation not 0.0, prescribed displacement, Free and Fix should be False

    // x displacement
    ADD_PROPERTY(xFix,(0));
    ADD_PROPERTY(xFree,(1));
    ADD_PROPERTY(xDisplacement,(0.0));

    // y displacement
    ADD_PROPERTY(yFix,(0));
    ADD_PROPERTY(yFree,(1));
    ADD_PROPERTY(yDisplacement,(0.0));

    // z displacement
    ADD_PROPERTY(zFix,(0));
    ADD_PROPERTY(zFree,(1));
    ADD_PROPERTY(zDisplacement,(0.0));

    // x rotation
    ADD_PROPERTY(rotxFix,(0));
    ADD_PROPERTY(rotxFree,(1));
    ADD_PROPERTY(xRotation,(0.0));

    // y rotation
    ADD_PROPERTY(rotyFix,(0));
    ADD_PROPERTY(rotyFree,(1));
    ADD_PROPERTY(yRotation,(0.0));

    // z rotation
    ADD_PROPERTY(rotzFix,(0));
    ADD_PROPERTY(rotzFree,(1));
    ADD_PROPERTY(zRotation,(0.0));

    ADD_PROPERTY_TYPE(Points,(Base::Vector3d()),"ConstraintFixed",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Points where symbols are drawn");
    ADD_PROPERTY_TYPE(Normals,(Base::Vector3d()),"ConstraintFixed",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                                                                             "Normals where symbols are drawn");
    Points.setValues(std::vector<Base::Vector3d>());
    Normals.setValues(std::vector<Base::Vector3d>());
}

App::DocumentObjectExecReturn *ConstraintDisplacement::execute(void)
{
    return Constraint::execute();
}

const char* ConstraintDisplacement::getViewProviderName(void) const
{
    return "FemGui::ViewProviderFemConstraintDisplacement";
}

void ConstraintDisplacement::onChanged(const App::Property* prop)
{
    // Note: If we call this at the end, then the arrows are not oriented correctly initially
    // because the NormalDirection has not been calculated yet
    Constraint::onChanged(prop);

    if (prop == &References) {
        std::vector<Base::Vector3d> points;
        std::vector<Base::Vector3d> normals;
        int scale = 1; //OvG: Enforce use of scale
        if (getPoints(points, normals, &scale)) {
            Points.setValues(points);
            Normals.setValues(normals);
            Scale.setValue(scale); //OvG: Scale
            Points.touch(); // This triggers ViewProvider::updateData()
        }
    }
}
