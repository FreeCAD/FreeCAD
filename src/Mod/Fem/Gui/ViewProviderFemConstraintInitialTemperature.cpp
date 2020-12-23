/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
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
# include <Standard_math.hxx>
# include <Precision.hxx>

# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoRotation.h>
# include <Inventor/nodes/SoMultipleCopy.h>
# include <Inventor/nodes/SoCylinder.h>
# include <Inventor/nodes/SoSphere.h>
# include <Inventor/nodes/SoText3.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
#endif

#include "Mod/Fem/App/FemConstraintInitialTemperature.h"
#include "TaskFemConstraintInitialTemperature.h"
#include "ViewProviderFemConstraintInitialTemperature.h"
#include <Base/Console.h>
#include <Gui/Control.h>

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintInitialTemperature, FemGui::ViewProviderFemConstraint)

ViewProviderFemConstraintInitialTemperature::ViewProviderFemConstraintInitialTemperature()
{
    sPixmap = "fem-constraint-InitialTemperature";
    ADD_PROPERTY(FaceColor,(0.2f,0.3f,0.2f));
}

ViewProviderFemConstraintInitialTemperature::~ViewProviderFemConstraintInitialTemperature()
{
}

//FIXME setEdit needs a careful review
bool ViewProviderFemConstraintInitialTemperature::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintInitialTemperature *constrDlg = qobject_cast<TaskDlgFemConstraintInitialTemperature *>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this)
            constrDlg = 0; // another constraint left open its task panel
        if (dlg && !constrDlg) {
            if (constraintDialog != NULL) {
                // Ignore the request to open another dialog
                return false;
            } else {
                constraintDialog = new TaskFemConstraintInitialTemperature(this);
                return true;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg)
            Gui::Control().showDialog(constrDlg);
        else
            Gui::Control().showDialog(new TaskDlgFemConstraintInitialTemperature(this));
        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemConstraintInitialTemperature::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    ViewProviderFemConstraint::updateData(prop);
}
