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
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoRotation.h>
# include <Precision.hxx>
#endif

#include "ViewProviderFemConstraintPulley.h"
#include <Mod/Fem/App/FemConstraintPulley.h>
#include "TaskFemConstraintPulley.h"
#include "Gui/Control.h"

#include <Base/Console.h>

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintPulley, FemGui::ViewProviderFemConstraint)


ViewProviderFemConstraintPulley::ViewProviderFemConstraintPulley()
{
    sPixmap = "view-femconstraintpulley";
}

ViewProviderFemConstraintPulley::~ViewProviderFemConstraintPulley()
{
}

bool ViewProviderFemConstraintPulley::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintPulley *constrDlg = qobject_cast<TaskDlgFemConstraintPulley *>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this)
            constrDlg = 0; // another constraint left open its task panel
        if (dlg && !constrDlg) {
            // Allow stacking of dialogs, for ShaftWizard application
            // Note: If other features start to allow stacking, we need to check for oldDlg != NULL
            oldDlg = dlg;
            /*
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().closeDialog();
            else
                return false;
                */
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg)
            Gui::Control().showDialog(constrDlg);
        else
            Gui::Control().showDialog(new TaskDlgFemConstraintPulley(this));

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemConstraintPulley::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(this->getObject());
    if (this->getObject() != NULL)
        Base::Console().Error("%s: VP updateData: %s\n", this->getObject()->getNameInDocument(), prop->getName());
    else
        Base::Console().Error("Anonymous: VP updateData: %s\n", prop->getName());

    if (strcmp(prop->getName(),"BasePoint") == 0) {
        if (pcConstraint->Height.getValue() > Precision::Confusion()) {
            // Remove and recreate the symbol
            pShapeSep->removeAllChildren();

            // This should always point outside of the cylinder
            Base::Vector3f base = pcConstraint->BasePoint.getValue();
            Base::Vector3f axis = pcConstraint->Axis.getValue();
            float radius = pcConstraint->Radius.getValue();
            float dia = pcConstraint->Diameter.getValue();
            if (dia < 2 * radius)
                dia = 2 * radius;
            float angle = pcConstraint->Angle.getValue();

            SbVec3f b(base.x, base.y, base.z);
            SbVec3f dir(axis.x, axis.y, axis.z);
            SbRotation rot(SbVec3f(0,-1,0), dir);

            createPlacement(pShapeSep, b, rot); // child 0 and 1
            pShapeSep->addChild(createCylinder(pcConstraint->Height.getValue() * 0.8, dia/2)); // child 2
            SoSeparator* sep = new SoSeparator();
            createPlacement(sep, SbVec3f(dia/2,0,0), SbRotation(SbVec3f(0,1,0), SbVec3f(sin(angle),0,cos(angle))));
            sep->addChild(createArrow(dia/2, dia/8));
            pShapeSep->addChild(sep); // child 3
            sep = new SoSeparator();
            createPlacement(sep, SbVec3f(-dia/2,0,0), SbRotation(SbVec3f(0,1,0), SbVec3f(-sin(angle),0,cos(angle))));
            sep->addChild(createArrow(dia/2, dia/8));
            pShapeSep->addChild(sep); // child 4
        }
    } else if (strcmp(prop->getName(),"Angle") == 0) {
        if (pShapeSep->getNumChildren() > 0) {
            // Change the symbol
            Base::Vector3f base = pcConstraint->BasePoint.getValue();
            Base::Vector3f axis = pcConstraint->Axis.getValue();
            float radius = pcConstraint->Radius.getValue();
            float dia = pcConstraint->Diameter.getValue();
            if (dia < 2 * radius)
                dia = 2 * radius;
            float angle = pcConstraint->Angle.getValue();

            SbVec3f b(base.x, base.y, base.z);
            SbVec3f dir(axis.x, axis.y, axis.z);
            SbRotation rot(SbVec3f(0,-1,0), dir);

            updatePlacement(pShapeSep, 0, b, rot);
            const SoSeparator* sep = static_cast<SoSeparator*>(pShapeSep->getChild(2));
            updateCylinder(sep, 0, pcConstraint->Height.getValue() * 0.8, dia/2);
            sep = static_cast<SoSeparator*>(pShapeSep->getChild(3));
            updatePlacement(sep, 0, SbVec3f(0,0,dia/2), SbRotation(SbVec3f(0,1,0), SbVec3f(cos(angle),0,sin(angle))));
            const SoSeparator* subsep = static_cast<SoSeparator*>(sep->getChild(2));
            updateArrow(subsep, 0, dia/2, dia/8);
            sep = static_cast<SoSeparator*>(pShapeSep->getChild(4));
            updatePlacement(sep, 0, SbVec3f(0,0,-dia/2), SbRotation(SbVec3f(0,1,0), SbVec3f(cos(angle),0,-sin(angle))));
            subsep = static_cast<SoSeparator*>(sep->getChild(2));
            updateArrow(subsep, 0, dia/2, dia/8);
        }
    }

    ViewProviderFemConstraint::updateData(prop);
}
