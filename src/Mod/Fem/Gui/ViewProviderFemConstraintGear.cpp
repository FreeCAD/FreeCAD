/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
# include <Standard_math.hxx>
# include <Precision.hxx>

# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoRotation.h>
# include <Inventor/SbMatrix.h>

# include <QMessageBox>
#endif

#include "ViewProviderFemConstraintGear.h"
#include <Mod/Fem/App/FemConstraintGear.h>
#include "TaskFemConstraintGear.h"
#include "Gui/Control.h"

#include <Base/Console.h>

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintGear, FemGui::ViewProviderFemConstraint)


ViewProviderFemConstraintGear::ViewProviderFemConstraintGear()
{
    sPixmap = "fem-constraint-gear";
}

ViewProviderFemConstraintGear::~ViewProviderFemConstraintGear()
{
}

bool ViewProviderFemConstraintGear::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintGear *constrDlg = qobject_cast<TaskDlgFemConstraintGear *>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this)
            constrDlg = 0; // another constraint left open its task panel
        if (dlg && !constrDlg) {
            // This case will occur in the ShaftWizard application
            checkForWizard();
            if ((wizardWidget == NULL) || (wizardSubLayout == NULL)) {
                // No shaft wizard is running
                QMessageBox msgBox;
                msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
                msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                int ret = msgBox.exec();
                if (ret == QMessageBox::Yes)
                    Gui::Control().reject();
                else
                    return false;
            } else if (constraintDialog != NULL) {
                // Another FemConstraint* dialog is already open inside the Shaft Wizard
                // Ignore the request to open another dialog
                return false;
            } else {
                constraintDialog = new TaskFemConstraintGear(this);
                return true;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg)
            Gui::Control().showDialog(constrDlg);
        else
            Gui::Control().showDialog(new TaskDlgFemConstraintGear(this));

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemConstraintGear::updateData(const App::Property* prop)
{
    Fem::ConstraintGear* pcConstraint = static_cast<Fem::ConstraintGear*>(this->getObject());

    // Gets called whenever a property of the attached object changes
    if (strcmp(prop->getName(),"BasePoint") == 0) {
        if (pcConstraint->Height.getValue() > Precision::Confusion()) {
            // Remove and recreate the symbol
            Gui::coinRemoveAllChildren(pShapeSep);

            Base::Vector3d base = pcConstraint->BasePoint.getValue();
            Base::Vector3d axis = pcConstraint->Axis.getValue();
            Base::Vector3d direction = pcConstraint->DirectionVector.getValue();
            if (direction.Length() < Precision::Confusion())
                direction = Base::Vector3d(0,1,0);
            double radius = pcConstraint->Radius.getValue();
            double dia = pcConstraint->Diameter.getValue();
            if (dia < 2 * radius)
                dia = 2 * radius;
            double angle = pcConstraint->ForceAngle.getValue() / 180 * M_PI;

            SbVec3f b(base.x, base.y, base.z);
            SbVec3f ax(axis.x, axis.y, axis.z);
            SbVec3f dir(direction.x, direction.y, direction.z);
            //Base::Console().Error("DirectionVector: %f, %f, %f\n", direction.x, direction.y, direction.z);

            createPlacement(pShapeSep, b, SbRotation(SbVec3f(0,1,0), ax));
            pShapeSep->addChild(createCylinder(pcConstraint->Height.getValue() * 0.8, dia/2));
            createPlacement(pShapeSep, SbVec3f(dia/2 * sin(angle), 0, dia/2 * cos(angle)), SbRotation(ax, dir));
            pShapeSep->addChild(createArrow(dia/2, dia/8));
        }
    } else if (strcmp(prop->getName(),"Diameter") == 0) {
        if (pShapeSep->getNumChildren() > 0) {
            // Change the symbol
            Base::Vector3d axis = pcConstraint->Axis.getValue();
            Base::Vector3d direction = pcConstraint->DirectionVector.getValue();
            if (direction.Length() < Precision::Confusion())
                direction = Base::Vector3d(0,1,0);
            double dia = pcConstraint->Diameter.getValue();
            double radius = pcConstraint->Radius.getValue();
            if (dia < 2 * radius)
                dia = 2 * radius;
            double angle = pcConstraint->ForceAngle.getValue() / 180 * M_PI;

            SbVec3f ax(axis.x, axis.y, axis.z);
            SbVec3f dir(direction.x, direction.y, direction.z);

            const SoSeparator* sep = static_cast<SoSeparator*>(pShapeSep->getChild(2));
            updateCylinder(sep, 0, pcConstraint->Height.getValue() * 0.8, dia/2);
            updatePlacement(pShapeSep, 3, SbVec3f(dia/2 * sin(angle), 0, dia/2 * cos(angle)), SbRotation(ax, dir));
            sep = static_cast<SoSeparator*>(pShapeSep->getChild(5));
            updateArrow(sep, 0, dia/2, dia/8);
        }
    } else if ((strcmp(prop->getName(),"DirectionVector") == 0) || (strcmp(prop->getName(),"ForceAngle") == 0))  {
        // Note: "Reversed" also triggers "DirectionVector"
        if (pShapeSep->getNumChildren() > 0) {
            // Re-orient the symbol
            Base::Vector3d axis = pcConstraint->Axis.getValue();
            Base::Vector3d direction = pcConstraint->DirectionVector.getValue();
            if (direction.Length() < Precision::Confusion())
                direction = Base::Vector3d(0,1,0);
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
            // directions when ax = (1,0,0) (but for ax=(0,1,0) it points in 3 different directions!)
            */

            updatePlacement(pShapeSep, 3, SbVec3f(dia/2 * sin(angle), 0, dia/2 * cos(angle)), SbRotation(ax, dir));
        }
    }

    ViewProviderFemConstraint::updateData(prop);
}
