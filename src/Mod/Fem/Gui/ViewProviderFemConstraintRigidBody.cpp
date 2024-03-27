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

#ifndef _PreComp_
#include <QMessageBox>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoMultipleCopy.h>
#include <Inventor/nodes/SoSeparator.h>
#endif

#include <Mod/Fem/App/FemConstraintRigidBody.h>
#include "Gui/Control.h"

#include "ViewProviderFemConstraintRigidBody.h"
#include "TaskFemConstraintRigidBody.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintRigidBody,
                FemGui::ViewProviderFemConstraintOnBoundary)


ViewProviderFemConstraintRigidBody::ViewProviderFemConstraintRigidBody()
{
    sPixmap = "FEM_ConstraintRigidBody";
}

ViewProviderFemConstraintRigidBody::~ViewProviderFemConstraintRigidBody()
{}

bool ViewProviderFemConstraintRigidBody::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintRigidBody* constrDlg =
            qobject_cast<TaskDlgFemConstraintRigidBody*>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this) {
            constrDlg = nullptr;  // another constraint left open its task panel
        }
        if (dlg && !constrDlg) {
            // This case will occur in the ShaftWizard application
            checkForWizard();
            if (!wizardWidget || !wizardSubLayout) {
                // No shaft wizard is running
                QMessageBox msgBox;
                msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
                msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                int ret = msgBox.exec();
                if (ret == QMessageBox::Yes) {
                    Gui::Control().reject();
                }
                else {
                    return false;
                }
            }
            else if (constraintDialog) {
                // Another FemConstraint* dialog is already open inside the Shaft Wizard
                // Ignore the request to open another dialog
                return false;
            }
            else {
                constraintDialog = new TaskFemConstraintRigidBody(this);
                return true;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg) {
            Gui::Control().showDialog(constrDlg);
        }
        else {
            Gui::Control().showDialog(new TaskDlgFemConstraintRigidBody(this));
        }

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);  // clazy:exclude=skipped-base-method
    }
}

#define WIDTH (2)
#define HEIGHT (1)
// #define USE_MULTIPLE_COPY  //OvG: MULTICOPY fails to update scaled display on initial drawing -
// so disable

void ViewProviderFemConstraintRigidBody::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    Fem::ConstraintRigidBody* pcConstraint =
        static_cast<Fem::ConstraintRigidBody*>(this->getObject());
    float scaledwidth =
        WIDTH * pcConstraint->Scale.getValue();  // OvG: Calculate scaled values once only
    float scaledheight = HEIGHT * pcConstraint->Scale.getValue();

#ifdef USE_MULTIPLE_COPY
    // OvG: always need access to cp for scaling
    SoMultipleCopy* cp = new SoMultipleCopy();
    if (pShapeSep->getNumChildren() == 0) {
        // Set up the nodes
        cp->matrix.setNum(0);
        cp->addChild((SoNode*)createRigidBody(scaledheight, scaledwidth));  // OvG: Scaling
        pShapeSep->addChild(cp);
    }
#endif

    if (strcmp(prop->getName(), "Points") == 0) {
        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();
        const std::vector<Base::Vector3d>& normals = pcConstraint->Normals.getValues();
        if (points.size() != normals.size()) {
            return;
        }
        std::vector<Base::Vector3d>::const_iterator n = normals.begin();

#ifdef USE_MULTIPLE_COPY
        cp = static_cast<SoMultipleCopy*>(pShapeSep->getChild(0));
        cp->matrix.setNum(points.size());
        SbMatrix* matrices = cp->matrix.startEditing();
        int idx = 0;
#else
        // Note: Points and Normals are always updated together
        Gui::coinRemoveAllChildren(pShapeSep);
#endif

        for (std::vector<Base::Vector3d>::const_iterator p = points.begin(); p != points.end();
             p++) {
            SbVec3f base(p->x, p->y, p->z);
            SbVec3f dir(n->x, n->y, n->z);
            SbRotation rot(SbVec3f(0, -1, 0), dir);
#ifdef USE_MULTIPLE_COPY
            SbMatrix m;
            m.setTransform(base, rot, SbVec3f(1, 1, 1));
            matrices[idx] = m;
            idx++;
#else
            SoSeparator* sep = new SoSeparator();
            createPlacement(sep, base, rot);
            createFixed(sep, scaledheight, scaledwidth);  // OvG: Scaling
            pShapeSep->addChild(sep);
#endif
            n++;
        }
#ifdef USE_MULTIPLE_COPY
        cp->matrix.finishEditing();
#endif
    }

    ViewProviderFemConstraint::updateData(prop);
}
