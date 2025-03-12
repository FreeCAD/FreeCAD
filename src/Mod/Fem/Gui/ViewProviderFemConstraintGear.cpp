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
#include <Inventor/nodes/SoSeparator.h>
#include <Precision.hxx>
#endif

#include "Gui/Control.h"
#include <Base/Console.h>
#include <Mod/Fem/App/FemConstraintGear.h>

#include "FemGuiTools.h"
#include "TaskFemConstraintGear.h"
#include "ViewProviderFemConstraintGear.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintGear, FemGui::ViewProviderFemConstraint)


ViewProviderFemConstraintGear::ViewProviderFemConstraintGear()
{
    sPixmap = "FEM_ConstraintGear";
}

ViewProviderFemConstraintGear::~ViewProviderFemConstraintGear() = default;

bool ViewProviderFemConstraintGear::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgFemConstraintGear(this));

        return true;
    }
    else {
        return ViewProviderFemConstraint::setEdit(ModNum);
    }
}

void ViewProviderFemConstraintGear::updateData(const App::Property* prop)
{
    Fem::ConstraintGear* pcConstraint = this->getObject<Fem::ConstraintGear>();

    // Gets called whenever a property of the attached object changes
    if (prop == &pcConstraint->BasePoint) {
        if (pcConstraint->Height.getValue() > Precision::Confusion()) {
            // Remove and recreate the symbol
            Gui::coinRemoveAllChildren(pShapeSep);

            Base::Vector3d base = pcConstraint->BasePoint.getValue();
            Base::Vector3d axis = pcConstraint->Axis.getValue();
            Base::Vector3d direction = pcConstraint->DirectionVector.getValue();
            if (direction.Length() < Precision::Confusion()) {
                direction = Base::Vector3d(0, 1, 0);
            }
            double radius = pcConstraint->Radius.getValue();
            double dia = pcConstraint->Diameter.getValue();
            if (dia < 2 * radius) {
                dia = 2 * radius;
            }
            double angle = pcConstraint->ForceAngle.getValue() / 180 * M_PI;

            SbVec3f b(base.x, base.y, base.z);
            SbVec3f ax(axis.x, axis.y, axis.z);
            SbVec3f dir(direction.x, direction.y, direction.z);
            // Base::Console().Error("DirectionVector: %f, %f, %f\n", direction.x, direction.y,
            // direction.z);

            GuiTools::createPlacement(pShapeSep, b, SbRotation(SbVec3f(0, 1, 0), ax));
            pShapeSep->addChild(
                GuiTools::createCylinder(pcConstraint->Height.getValue() * 0.8, dia / 2));
            GuiTools::createPlacement(pShapeSep,
                                      SbVec3f(dia / 2 * sin(angle), 0, dia / 2 * cos(angle)),
                                      SbRotation(ax, dir));
            pShapeSep->addChild(GuiTools::createArrow(dia / 2, dia / 8));
        }
    }
    else if (prop == &pcConstraint->Diameter) {
        if (pShapeSep->getNumChildren() > 0) {
            // Change the symbol
            Base::Vector3d axis = pcConstraint->Axis.getValue();
            Base::Vector3d direction = pcConstraint->DirectionVector.getValue();
            if (direction.Length() < Precision::Confusion()) {
                direction = Base::Vector3d(0, 1, 0);
            }
            double dia = pcConstraint->Diameter.getValue();
            double radius = pcConstraint->Radius.getValue();
            if (dia < 2 * radius) {
                dia = 2 * radius;
            }
            double angle = pcConstraint->ForceAngle.getValue() / 180 * M_PI;

            SbVec3f ax(axis.x, axis.y, axis.z);
            SbVec3f dir(direction.x, direction.y, direction.z);

            const SoSeparator* sep = static_cast<SoSeparator*>(pShapeSep->getChild(2));
            GuiTools::updateCylinder(sep, 0, pcConstraint->Height.getValue() * 0.8, dia / 2);
            GuiTools::updatePlacement(pShapeSep,
                                      3,
                                      SbVec3f(dia / 2 * sin(angle), 0, dia / 2 * cos(angle)),
                                      SbRotation(ax, dir));
            sep = static_cast<SoSeparator*>(pShapeSep->getChild(5));
            GuiTools::updateArrow(sep, 0, dia / 2, dia / 8);
        }
    }
    else if ((prop == &pcConstraint->DirectionVector) || (prop == &pcConstraint->ForceAngle)) {
        // Note: "Reversed" also triggers "DirectionVector"
        if (pShapeSep->getNumChildren() > 0) {
            // Re-orient the symbol
            Base::Vector3d axis = pcConstraint->Axis.getValue();
            Base::Vector3d direction = pcConstraint->DirectionVector.getValue();
            if (direction.Length() < Precision::Confusion()) {
                direction = Base::Vector3d(0, 1, 0);
            }
            double dia = pcConstraint->Diameter.getValue();
            double angle = pcConstraint->ForceAngle.getValue() / 180 * M_PI;

            SbVec3f ax(axis.x, axis.y, axis.z);
            SbVec3f dir(direction.x, direction.y, direction.z);
            /*Base::Console().Error("Axis: %f, %f, %f\n", axis.x, axis.y, axis.z);
            Base::Console().Error("Direction: %f, %f, %f\n", direction.x, direction.y, direction.z);
            SbRotation rot = SbRotation(ax, dir);
            SbMatrix m;
            rot.getValue(m);
            SbMat m2;
            m.getValue(m2);
            Base::Console().Error("Matrix: %f, %f, %f, %f\n", m[0][0], m[1][0], m[2][0], m[3][0]);
            // Note: In spite of the fact that the rotation matrix takes on 3 different values if 3
            // normal directions are chosen, the resulting arrow will only point in two different
            // directions when ax = (1,0,0) (but for ax=(0,1,0) it points in 3 different
            directions!)
            */

            GuiTools::updatePlacement(pShapeSep,
                                      3,
                                      SbVec3f(dia / 2 * sin(angle), 0, dia / 2 * cos(angle)),
                                      SbRotation(ax, dir));
        }
    }

    ViewProviderFemConstraint::updateData(prop);
}
