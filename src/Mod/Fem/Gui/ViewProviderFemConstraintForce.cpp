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
# include <Inventor/nodes/SoMultipleCopy.h>

# include <QMessageBox>
#endif

#include "ViewProviderFemConstraintForce.h"
#include <Mod/Fem/App/FemConstraintForce.h>
#include "TaskFemConstraintForce.h"
#include "Gui/Control.h"

#include <Base/Console.h>

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintForce, FemGui::ViewProviderFemConstraint)


ViewProviderFemConstraintForce::ViewProviderFemConstraintForce()
{
    sPixmap = "fem-constraint-force";
}

ViewProviderFemConstraintForce::~ViewProviderFemConstraintForce()
{
}

bool ViewProviderFemConstraintForce::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintForce *constrDlg = qobject_cast<TaskDlgFemConstraintForce *>(dlg);
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
                constraintDialog = new TaskFemConstraintForce(this);
                return true;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg)
            Gui::Control().showDialog(constrDlg);
        else
            Gui::Control().showDialog(new TaskDlgFemConstraintForce(this));

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

#define ARROWLENGTH (4)
#define ARROWHEADRADIUS (ARROWLENGTH/3)
//#define USE_MULTIPLE_COPY  //OvG: MULTICOPY fails to update scaled arrows on initial drawing - so disable

void ViewProviderFemConstraintForce::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(this->getObject());
    float scaledheadradius = ARROWHEADRADIUS * pcConstraint->Scale.getValue(); //OvG: Calculate scaled values once only
    float scaledlength = ARROWLENGTH * pcConstraint->Scale.getValue();

#ifdef USE_MULTIPLE_COPY
    //OvG: need access to cp for scaling
    SoMultipleCopy* cp = new SoMultipleCopy();
    if (pShapeSep->getNumChildren() == 0) {
        // Set up the nodes
        cp->matrix.setNum(0);
        cp->addChild((SoNode*)createArrow(scaledlength , scaledheadradius)); //OvG: Scaling
        pShapeSep->addChild(cp);
    }
#endif

    if (strcmp(prop->getName(),"Points") == 0) {
        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();

#ifdef USE_MULTIPLE_COPY
        cp = static_cast<SoMultipleCopy*>(pShapeSep->getChild(0));
        cp->matrix.setNum(points.size());
        SbMatrix* matrices = cp->matrix.startEditing();
        int idx = 0;
#else
        // Redraw all arrows
        Gui::coinRemoveAllChildren(pShapeSep);
#endif
        // This should always point outside of the solid
        Base::Vector3d normal = pcConstraint->NormalDirection.getValue();

        // Get default direction (on first call to method)
        Base::Vector3d forceDirection = pcConstraint->DirectionVector.getValue();
        if (forceDirection.Length() < Precision::Confusion())
            forceDirection = normal;

        SbVec3f dir(forceDirection.x, forceDirection.y, forceDirection.z);
        SbRotation rot(SbVec3f(0,1,0), dir);

        for (std::vector<Base::Vector3d>::const_iterator p = points.begin(); p != points.end(); p++) {
            SbVec3f base(p->x, p->y, p->z);
            if (forceDirection.GetAngle(normal) < M_PI_2) // Move arrow so it doesn't disappear inside the solid
                base = base + dir * scaledlength; //OvG: Scaling
#ifdef USE_MULTIPLE_COPY
            SbMatrix m;
            m.setTransform(base, rot, SbVec3f(1,1,1));
            matrices[idx] = m;
            idx++;
#else
            SoSeparator* sep = new SoSeparator();
            createPlacement(sep, base, rot);
            createArrow(sep, scaledlength, scaledheadradius); //OvG: Scaling
            pShapeSep->addChild(sep);
#endif
        }
#ifdef USE_MULTIPLE_COPY
        cp->matrix.finishEditing();
#endif
    }
    else if (strcmp(prop->getName(),"DirectionVector") == 0) { // Note: "Reversed" also triggers "DirectionVector"
        // Re-orient all arrows
        Base::Vector3d normal = pcConstraint->NormalDirection.getValue();
        Base::Vector3d forceDirection = pcConstraint->DirectionVector.getValue();
        if (forceDirection.Length() < Precision::Confusion())
            forceDirection = normal;

        SbVec3f dir(forceDirection.x, forceDirection.y, forceDirection.z);
        SbRotation rot(SbVec3f(0,1,0), dir);

        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();

#ifdef USE_MULTIPLE_COPY
        SoMultipleCopy* cp = static_cast<SoMultipleCopy*>(pShapeSep->getChild(0));
        cp->matrix.setNum(points.size());
        SbMatrix* matrices = cp->matrix.startEditing();
#endif
        int idx = 0;

        for (std::vector<Base::Vector3d>::const_iterator p = points.begin(); p != points.end(); p++) {
            SbVec3f base(p->x, p->y, p->z);
            if (forceDirection.GetAngle(normal) < M_PI_2)
                base = base + dir * scaledlength; //OvG: Scaling
#ifdef USE_MULTIPLE_COPY
            SbMatrix m;
            m.setTransform(base, rot, SbVec3f(1,1,1));
            matrices[idx] = m;
#else
            SoSeparator* sep = static_cast<SoSeparator*>(pShapeSep->getChild(idx));
            updatePlacement(sep, 0, base, rot);
            updateArrow(sep, 2, scaledlength, scaledheadradius); //OvG: Scaling
#endif
            idx++;
        }
#ifdef USE_MULTIPLE_COPY
        cp->matrix.finishEditing();
#endif
    }

    ViewProviderFemConstraint::updateData(prop);
}
