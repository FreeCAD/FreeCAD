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
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoRotation.h>
# include <Inventor/nodes/SoMultipleCopy.h>
# include <Precision.hxx>
#endif

#include "ViewProviderFemConstraintFixed.h"
#include <Mod/Fem/App/FemConstraintFixed.h>
#include "TaskFemConstraintFixed.h"

#include "Gui/Control.h"

#include <Base/Console.h>

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintFixed, FemGui::ViewProviderFemConstraint)


ViewProviderFemConstraintFixed::ViewProviderFemConstraintFixed()
{
    sPixmap = "Fem_ConstraintFixed";
}

ViewProviderFemConstraintFixed::~ViewProviderFemConstraintFixed()
{
}

bool ViewProviderFemConstraintFixed::setEdit(int ModNum)
{
    Base::Console().Error("ViewProviderFemConstraintFixed::setEdit()\n");
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintFixed *constrDlg = qobject_cast<TaskDlgFemConstraintFixed *>(dlg);
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
                    Gui::Control().closeDialog();
                else
                    return false;
            } else if (constraintDialog != NULL) {
                // Another FemConstraint* dialog is already open inside the Shaft Wizard
                // Ignore the request to open another dialog
                return false;
            } else {
                constraintDialog = new TaskFemConstraintFixed(this);
                return true;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg)
            Gui::Control().showDialog(constrDlg);
        else
            Gui::Control().showDialog(new TaskDlgFemConstraintFixed(this));

        return true;
    } else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

#define HEIGHT 4
#define WIDTH (1.5*HEIGHT)

void ViewProviderFemConstraintFixed::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    Fem::ConstraintFixed* pcConstraint = static_cast<Fem::ConstraintFixed*>(this->getObject());

    /*
    // This has a HUGE performance penalty as opposed to separate nodes for every symbol
    // The problem seems to be SoCone
    if (pShapeSep->getNumChildren() == 0) {
        // Set up the nodes
        SoMultipleCopy* cp = new SoMultipleCopy();
        cp->ref();
        cp->matrix.setNum(0);
        cp->addChild((SoNode*)createFixed(HEIGHT, WIDTH));
        pShapeSep->addChild(cp);
    }
    */

    if (strcmp(prop->getName(),"Points") == 0) {
        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();
        const std::vector<Base::Vector3d>& normals = pcConstraint->Normals.getValues();
        if (points.size() != normals.size())
            return;
        std::vector<Base::Vector3d>::const_iterator n = normals.begin();

        // Note: Points and Normals are always updated together
        pShapeSep->removeAllChildren();

        /*
        SoMultipleCopy* cp = static_cast<SoMultipleCopy*>(pShapeSep->getChild(0));
        cp->matrix.setNum(points.size());
        int idx = 0;
        */

        for (std::vector<Base::Vector3d>::const_iterator p = points.begin(); p != points.end(); p++) {
            SbVec3f base(p->x, p->y, p->z);
            SbVec3f dir(n->x, n->y, n->z);
            SbRotation rot(SbVec3f(0,-1,0), dir);
            /*
            SbMatrix m;
            m.setTransform(base, rot, SbVec3f(1,1,1));
            cp->matrix.set1Value(idx, m);
            idx++
            */
            SoSeparator* sep = new SoSeparator();
            createPlacement(sep, base, rot);
            createFixed(sep, HEIGHT, WIDTH);
            pShapeSep->addChild(sep);

            n++;
        }
    }

    ViewProviderFemConstraint::updateData(prop);
}
