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
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Precision.hxx>
#endif

#include <Base/Tools.h>
#include "Gui/Control.h"
#include "FemGuiTools.h"
#include "TaskFemConstraintPulley.h"
#include "ViewProviderFemConstraintPulley.h"
#include <Mod/Fem/App/FemConstraintPulley.h>


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintPulley, FemGui::ViewProviderFemConstraint)


ViewProviderFemConstraintPulley::ViewProviderFemConstraintPulley()
{
    sPixmap = "FEM_ConstraintPulley";
}

ViewProviderFemConstraintPulley::~ViewProviderFemConstraintPulley() = default;

bool ViewProviderFemConstraintPulley::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgFemConstraintPulley(this));

        return true;
    }
    else {
        return ViewProviderFemConstraint::setEdit(ModNum);
    }
}

void ViewProviderFemConstraintPulley::updateData(const App::Property* prop)
{
    using std::numbers::pi;

    // Gets called whenever a property of the attached object changes
    Fem::ConstraintPulley* pcConstraint = this->getObject<Fem::ConstraintPulley>();

    if (prop == &pcConstraint->BasePoint) {
        if (pcConstraint->Height.getValue() > Precision::Confusion()) {
            // Remove and recreate the symbol
            Gui::coinRemoveAllChildren(pShapeSep);

            // This should always point outside of the cylinder
            Base::Vector3d base = pcConstraint->BasePoint.getValue();
            Base::Vector3d axis = pcConstraint->Axis.getValue();
            double radius = pcConstraint->Radius.getValue();
            double dia = pcConstraint->Diameter.getValue();
            if (dia < 2 * radius) {
                dia = 2 * radius;
            }
            double forceAngle = Base::toRadians(pcConstraint->ForceAngle.getValue());
            double beltAngle = pcConstraint->BeltAngle.getValue();
            double rat1 = 0.8, rat2 = 0.2;
            double f1 = pcConstraint->BeltForce1.getValue();
            double f2 = pcConstraint->BeltForce2.getValue();
            if (f1 + f2 > Precision::Confusion()) {
                rat1 = f1 / (f1 + f2);
                rat2 = f2 / (f1 + f2);
            }

            SbVec3f b(base.x, base.y, base.z);
            SbVec3f ax(axis.x, axis.y, axis.z);

            GuiTools::createPlacement(pShapeSep,
                                      b,
                                      SbRotation(SbVec3f(0, 1, 0), ax));  // child 0 and 1
            pShapeSep->addChild(GuiTools::createCylinder(pcConstraint->Height.getValue() * 0.8,
                                                         dia / 2));  // child 2
            SoSeparator* sep = new SoSeparator();
            GuiTools::createPlacement(sep,
                                      SbVec3f(dia / 2 * sin(forceAngle + beltAngle),
                                              0,
                                              dia / 2 * cos(forceAngle + beltAngle)),
                                      SbRotation(SbVec3f(0, 1, 0),
                                                 SbVec3f(sin(forceAngle + beltAngle + pi / 2),
                                                         0,
                                                         cos(forceAngle + beltAngle + pi / 2))));
            GuiTools::createPlacement(sep, SbVec3f(0, dia / 8 + dia / 2 * rat1, 0), SbRotation());
            sep->addChild(GuiTools::createArrow(dia / 8 + dia / 2 * rat1, dia / 8));
            pShapeSep->addChild(sep);  // child 3
            sep = new SoSeparator();
            GuiTools::createPlacement(sep,
                                      SbVec3f(-dia / 2 * sin(forceAngle - beltAngle),
                                              0,
                                              -dia / 2 * cos(forceAngle - beltAngle)),
                                      SbRotation(SbVec3f(0, 1, 0),
                                                 SbVec3f(-sin(forceAngle - beltAngle - pi / 2),
                                                         0,
                                                         -cos(forceAngle - beltAngle - pi / 2))));
            GuiTools::createPlacement(sep, SbVec3f(0, dia / 8 + dia / 2 * rat2, 0), SbRotation());
            sep->addChild(GuiTools::createArrow(dia / 8 + dia / 2 * rat2, dia / 8));
            pShapeSep->addChild(sep);  // child 4
        }
    }
    else if (prop == &pcConstraint->Diameter) {
        if (pShapeSep->getNumChildren() > 0) {
            // Change the symbol
            double radius = pcConstraint->Radius.getValue();
            double dia = pcConstraint->Diameter.getValue();
            if (dia < 2 * radius) {
                dia = 2 * radius;
            }
            double forceAngle = Base::toRadians(pcConstraint->ForceAngle.getValue());
            double beltAngle = pcConstraint->BeltAngle.getValue();
            double rat1 = 0.8, rat2 = 0.2;
            double f1 = pcConstraint->BeltForce1.getValue();
            double f2 = pcConstraint->BeltForce2.getValue();
            if (f1 + f2 > Precision::Confusion()) {
                rat1 = f1 / (f1 + f2);
                rat2 = f2 / (f1 + f2);
            }

            const SoSeparator* sep = static_cast<SoSeparator*>(pShapeSep->getChild(2));
            GuiTools::updateCylinder(sep, 0, pcConstraint->Height.getValue() * 0.8, dia / 2);
            sep = static_cast<SoSeparator*>(pShapeSep->getChild(3));
            GuiTools::updatePlacement(sep,
                                      0,
                                      SbVec3f(dia / 2 * sin(forceAngle + beltAngle),
                                              0,
                                              dia / 2 * cos(forceAngle + beltAngle)),
                                      SbRotation(SbVec3f(0, 1, 0),
                                                 SbVec3f(sin(forceAngle + beltAngle + pi / 2),
                                                         0,
                                                         cos(forceAngle + beltAngle + pi / 2))));
            GuiTools::updatePlacement(sep,
                                      2,
                                      SbVec3f(0, dia / 8 + dia / 2 * rat1, 0),
                                      SbRotation());
            const SoSeparator* subsep = static_cast<SoSeparator*>(sep->getChild(4));
            GuiTools::updateArrow(subsep, 0, dia / 8 + dia / 2 * rat1, dia / 8);
            sep = static_cast<SoSeparator*>(pShapeSep->getChild(4));
            GuiTools::updatePlacement(sep,
                                      0,
                                      SbVec3f(-dia / 2 * sin(forceAngle - beltAngle),
                                              0,
                                              -dia / 2 * cos(forceAngle - beltAngle)),
                                      SbRotation(SbVec3f(0, 1, 0),
                                                 SbVec3f(-sin(forceAngle - beltAngle - pi / 2),
                                                         0,
                                                         -cos(forceAngle - beltAngle - pi / 2))));
            GuiTools::updatePlacement(sep,
                                      2,
                                      SbVec3f(0, dia / 8 + dia / 2 * rat2, 0),
                                      SbRotation());
            subsep = static_cast<SoSeparator*>(sep->getChild(4));
            GuiTools::updateArrow(subsep, 0, dia / 8 + dia / 2 * rat2, dia / 8);
        }
    }
    else if ((prop == &pcConstraint->ForceAngle) || (prop == &pcConstraint->BeltAngle)) {
        if (pShapeSep->getNumChildren() > 0) {
            double radius = pcConstraint->Radius.getValue();
            double dia = pcConstraint->Diameter.getValue();
            if (dia < 2 * radius) {
                dia = 2 * radius;
            }
            double forceAngle = Base::toRadians(pcConstraint->ForceAngle.getValue());
            double beltAngle = pcConstraint->BeltAngle.getValue();

            const SoSeparator* sep = static_cast<SoSeparator*>(pShapeSep->getChild(3));
            GuiTools::updatePlacement(sep,
                                      0,
                                      SbVec3f(dia / 2 * sin(forceAngle + beltAngle),
                                              0,
                                              dia / 2 * cos(forceAngle + beltAngle)),
                                      SbRotation(SbVec3f(0, 1, 0),
                                                 SbVec3f(sin(forceAngle + beltAngle + pi / 2),
                                                         0,
                                                         cos(forceAngle + beltAngle + pi / 2))));
            sep = static_cast<SoSeparator*>(pShapeSep->getChild(4));
            GuiTools::updatePlacement(sep,
                                      0,
                                      SbVec3f(-dia / 2 * sin(forceAngle - beltAngle),
                                              0,
                                              -dia / 2 * cos(forceAngle - beltAngle)),
                                      SbRotation(SbVec3f(0, 1, 0),
                                                 SbVec3f(-sin(forceAngle - beltAngle - pi / 2),
                                                         0,
                                                         -cos(forceAngle - beltAngle - pi / 2))));
        }
    }
    else if ((prop == &pcConstraint->BeltForce1) || (prop == &pcConstraint->BeltForce2)) {
        if (pShapeSep->getNumChildren() > 0) {
            double radius = pcConstraint->Radius.getValue();
            double dia = pcConstraint->Diameter.getValue();
            if (dia < 2 * radius) {
                dia = 2 * radius;
            }
            double rat1 = 0.8, rat2 = 0.2;
            double f1 = pcConstraint->BeltForce1.getValue();
            double f2 = pcConstraint->BeltForce2.getValue();
            if (f1 + f2 > Precision::Confusion()) {
                rat1 = f1 / (f1 + f2);
                rat2 = f2 / (f1 + f2);
            }

            const SoSeparator* sep = static_cast<SoSeparator*>(pShapeSep->getChild(3));
            GuiTools::updatePlacement(sep,
                                      2,
                                      SbVec3f(0, dia / 8 + dia / 2 * rat1, 0),
                                      SbRotation());
            const SoSeparator* subsep = static_cast<SoSeparator*>(sep->getChild(4));
            GuiTools::updateArrow(subsep, 0, dia / 8 + dia / 2 * rat1, dia / 8);
            sep = static_cast<SoSeparator*>(pShapeSep->getChild(4));
            GuiTools::updatePlacement(sep,
                                      2,
                                      SbVec3f(0, dia / 8 + dia / 2 * rat2, 0),
                                      SbRotation());
            subsep = static_cast<SoSeparator*>(sep->getChild(4));
            GuiTools::updateArrow(subsep, 0, dia / 8 + dia / 2 * rat2, dia / 8);
        }
    }

    ViewProviderFemConstraint::updateData(prop);
}
