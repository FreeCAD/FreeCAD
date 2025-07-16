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
#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#endif

#include "Gui/Control.h"
#include <Mod/Fem/App/FemConstraintForce.h>

#include "TaskFemConstraintForce.h"
#include "ViewProviderFemConstraintForce.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintForce, FemGui::ViewProviderFemConstraintOnBoundary)


ViewProviderFemConstraintForce::ViewProviderFemConstraintForce()
{
    sPixmap = "FEM_ConstraintForce";
    loadSymbol((resourceSymbolDir + "ConstraintForce.iv").c_str());
}

ViewProviderFemConstraintForce::~ViewProviderFemConstraintForce() = default;

bool ViewProviderFemConstraintForce::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgFemConstraintForce(this));

        return true;
    }
    else {
        return ViewProviderFemConstraintOnBoundary::setEdit(ModNum);
    }
}

void ViewProviderFemConstraintForce::updateData(const App::Property* prop)
{
    auto pcConstraint = this->getObject<Fem::ConstraintForce>();

    if (prop == &pcConstraint->Reversed || prop == &pcConstraint->DirectionVector) {
        updateSymbol();
    }
    else {
        ViewProviderFemConstraint::updateData(prop);
    }
}

void ViewProviderFemConstraintForce::transformSymbol(const Base::Vector3d& point,
                                                     const Base::Vector3d& normal,
                                                     SbMatrix& mat) const
{
    auto obj = this->getObject<const Fem::ConstraintForce>();
    bool rev = obj->Reversed.getValue();
    float s = obj->getScaleFactor();
    // Symbol length from .iv file
    float symLen = 4.0f;
    // Place each symbol outside the boundary
    Base::Vector3d dir = (rev ? -1.0 : 1.0) * obj->DirectionVector.getValue();
    float symTraY = dir.Dot(normal) < 0 ? -1 * symLen : 0.0f;
    float rotAngle = rev ? std::numbers::pi_v<float> : 0.0f;
    SbMatrix mat0, mat1;
    mat0.setTransform(SbVec3f(0, symTraY, 0),
                      SbRotation(SbVec3f(0, 0, 1), rotAngle),
                      SbVec3f(1, 1, 1),
                      SbRotation(SbVec3f(0, 0, 1), 0),
                      SbVec3f(0, symLen / 2.0f, 0));

    mat1.setTransform(SbVec3f(point.x, point.y, point.z),
                      SbRotation(SbVec3f(0, 1, 0), SbVec3f(dir.x, dir.y, dir.z)),
                      SbVec3f(s, s, s));

    mat = mat0 * mat1;
}
