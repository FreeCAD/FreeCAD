/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
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
#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#endif

#include "Mod/Fem/App/FemConstraintPressure.h"
#include <Gui/Control.h>

#include "TaskFemConstraintPressure.h"
#include "ViewProviderFemConstraintPressure.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintPressure,
                FemGui::ViewProviderFemConstraintOnBoundary)

ViewProviderFemConstraintPressure::ViewProviderFemConstraintPressure()
{
    sPixmap = "FEM_ConstraintPressure";
    loadSymbol((resourceSymbolDir + "ConstraintPressure.iv").c_str());
    ShapeAppearance.setDiffuseColor(0.0f, 0.2f, 0.8f);
}

ViewProviderFemConstraintPressure::~ViewProviderFemConstraintPressure() = default;

// FIXME setEdit needs a careful review
bool ViewProviderFemConstraintPressure::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintPressure* constrDlg = qobject_cast<TaskDlgFemConstraintPressure*>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this) {
            constrDlg = nullptr;  // another constraint left open its task panel
        }
        if (dlg && !constrDlg) {
            if (constraintDialog) {
                // Ignore the request to open another dialog
                return false;
            }
            else {
                constraintDialog = new TaskFemConstraintPressure(this);
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
            Gui::Control().showDialog(new TaskDlgFemConstraintPressure(this));
        }
        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);  // clazy:exclude=skipped-base-method
    }
}

void ViewProviderFemConstraintPressure::updateData(const App::Property* prop)
{
    auto pcConstraint = static_cast<Fem::ConstraintPressure*>(this->getObject());

    if (prop == &pcConstraint->Reversed) {
        updateSymbol();
    }
    else {
        ViewProviderFemConstraint::updateData(prop);
    }
}

void ViewProviderFemConstraintPressure::transformSymbol(const Base::Vector3d& point,
                                                        const Base::Vector3d& normal,
                                                        SbMatrix& mat) const
{
    auto obj = static_cast<const Fem::ConstraintPressure*>(this->getObject());
    float rotAngle = obj->Reversed.getValue() ? F_PI : 0.0f;
    float s = obj->getScaleFactor();
    // Symbol length from .iv file
    float symLen = 4.0f;
    SbMatrix mat0, mat1;
    mat0.setTransform(SbVec3f(0, 0, 0),
                      SbRotation(SbVec3f(0, 0, 1), rotAngle),
                      SbVec3f(1, 1, 1),
                      SbRotation(SbVec3f(0, 0, 1), 0),
                      SbVec3f(0, symLen / 2.0f, 0));

    mat1.setTransform(SbVec3f(point.x, point.y, point.z),
                      SbRotation(SbVec3f(0, 1, 0), SbVec3f(normal.x, normal.y, normal.z)),
                      SbVec3f(s, s, s));

    mat = mat0 * mat1;
}
