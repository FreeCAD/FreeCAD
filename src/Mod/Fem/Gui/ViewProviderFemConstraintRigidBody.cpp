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
#include <Inventor/nodes/SoTransform.h>
#endif

#include "Gui/Control.h"
#include <Mod/Fem/App/FemConstraintRigidBody.h>

#include "TaskFemConstraintRigidBody.h"
#include "ViewProviderFemConstraintRigidBody.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintRigidBody,
                FemGui::ViewProviderFemConstraintOnBoundary)


ViewProviderFemConstraintRigidBody::ViewProviderFemConstraintRigidBody()
{
    sPixmap = "FEM_ConstraintRigidBody";
    loadSymbol((resourceSymbolDir + "ConstraintRigidBody.iv").c_str());
    ShapeAppearance.setDiffuseColor(0.0f, 0.5f, 0.0f);
}

ViewProviderFemConstraintRigidBody::~ViewProviderFemConstraintRigidBody() = default;

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

void ViewProviderFemConstraintRigidBody::updateData(const App::Property* prop)
{
    auto obj = static_cast<Fem::ConstraintRigidBody*>(this->getObject());

    if (prop == &obj->ReferenceNode) {
        updateSymbol();
    }

    ViewProviderFemConstraint::updateData(prop);
}

void ViewProviderFemConstraintRigidBody::transformExtraSymbol() const
{
    SoTransform* symTrans = getExtraSymbolTransform();
    if (symTrans) {
        auto obj = static_cast<const Fem::ConstraintRigidBody*>(this->getObject());
        float s = obj->getScaleFactor();
        const Base::Vector3d& refNode = obj->ReferenceNode.getValue();
        SbVec3f tra(refNode.x, refNode.y, refNode.z);
        SbVec3f sca(s, s, s);
        SbRotation rot(SbVec3f(0, 0, 1), 0);

        SbMatrix mat;
        mat.setTransform(tra, rot, sca);

        symTrans->setMatrix(mat);
    }
}
