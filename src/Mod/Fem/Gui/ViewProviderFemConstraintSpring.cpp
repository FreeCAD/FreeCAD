/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Preslav Aleksandrov <preslav.aleksandrov@protonmail.com>      *
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
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoMultipleCopy.h>
#include <Inventor/nodes/SoSeparator.h>
#endif

#include "Mod/Fem/App/FemConstraintSpring.h"
#include <Gui/Control.h>

#include "TaskFemConstraintSpring.h"  //TODO  do next
#include "ViewProviderFemConstraintSpring.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintSpring,
                FemGui::ViewProviderFemConstraintOnBoundary)

ViewProviderFemConstraintSpring::ViewProviderFemConstraintSpring()
{
    sPixmap = "FEM_ConstraintSpring";
    ADD_PROPERTY(FaceColor, (0.0f, 0.2f, 0.8f));
}

ViewProviderFemConstraintSpring::~ViewProviderFemConstraintSpring() = default;

// FIXME setEdit needs a careful review
bool ViewProviderFemConstraintSpring::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintSpring* constrDlg =
            qobject_cast<TaskDlgFemConstraintSpring*>(dlg);  // check this out too
        if (constrDlg && constrDlg->getConstraintView() != this) {
            constrDlg = nullptr;  // another constraint left open its task panel
        }
        if (dlg && !constrDlg) {
            if (constraintDialog) {
                // Ignore the request to open another dialog
                return false;
            }
            else {
                constraintDialog = new TaskFemConstraintSpring(this);
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
            Gui::Control().showDialog(new TaskDlgFemConstraintSpring(this));
        }
        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);  // clazy:exclude=skipped-base-method
    }
}

#define WIDTH (1)
#define LENGTH (2)
// #define USE_MULTIPLE_COPY //OvG: MULTICOPY fails to update scaled arrows on initial drawing - so
// disable

void ViewProviderFemConstraintSpring::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    Fem::ConstraintSpring* pcConstraint = static_cast<Fem::ConstraintSpring*>(this->getObject());
    float scaledwidth =
        WIDTH * pcConstraint->Scale.getValue();  // OvG: Calculate scaled values once only
    float scaledlength = LENGTH * pcConstraint->Scale.getValue();

#ifdef USE_MULTIPLE_COPY
    // OvG: always need access to cp for scaling
    SoMultipleCopy* cp = new SoMultipleCopy();
    if (pShapeSep->getNumChildren() == 0) {
        // Set up the nodes
        cp->matrix.setNum(0);
        cp->addChild((SoNode*)createSpring(scaledlength, scaledwidth));  // OvG: Scaling
        pShapeSep->addChild(cp);
    }
#endif

    if (prop == &pcConstraint->Points) {
        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();
        const std::vector<Base::Vector3d>& normals = pcConstraint->Normals.getValues();
        if (points.size() != normals.size()) {
            return;
        }
        std::vector<Base::Vector3d>::const_iterator n = normals.begin();

#ifdef USE_MULTIPLE_COPY
        cp = static_cast<SoMultipleCopy*>(pShapeSep->getChild(0));  // OvG: Use top cp
        cp->matrix.setNum(points.size());
        SbMatrix* matrices = cp->matrix.startEditing();
        int idx = 0;
#else
        // Redraw all cylinders
        Gui::coinRemoveAllChildren(pShapeSep);
#endif

        for (const auto& point : points) {
            SbVec3f base(point.x, point.y, point.z);
            SbVec3f dir(n->x, n->y, n->z);
            SbRotation rot(SbVec3f(0, -1.0, 0), dir);
#ifdef USE_MULTIPLE_COPY
            SbMatrix m;
            m.setTransform(base, rot, SbVec3f(1, 1, 1));
            matrices[idx] = m;
            idx++;
#else
            SoSeparator* sep = new SoSeparator();
            createPlacement(sep, base, rot);
            createSpring(sep, scaledlength, scaledwidth);  // OvG: Scaling
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
