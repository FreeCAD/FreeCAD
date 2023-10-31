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
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoTranslation.h>
#endif

#include "Mod/Fem/App/FemConstraintTemperature.h"
#include "TaskFemConstraintTemperature.h"
#include "ViewProviderFemConstraintTemperature.h"
#include <Gui/Control.h>


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintTemperature,
                FemGui::ViewProviderFemConstraintOnBoundary)

ViewProviderFemConstraintTemperature::ViewProviderFemConstraintTemperature()
{
    sPixmap = "FEM_ConstraintTemperature";
    ADD_PROPERTY(FaceColor, (0.2f, 0.3f, 0.2f));
}

ViewProviderFemConstraintTemperature::~ViewProviderFemConstraintTemperature() = default;

// FIXME setEdit needs a careful review
bool ViewProviderFemConstraintTemperature::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintTemperature* constrDlg =
            qobject_cast<TaskDlgFemConstraintTemperature*>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this) {
            constrDlg = nullptr;  // another constraint left open its task panel
        }
        if (dlg && !constrDlg) {
            if (constraintDialog) {
                // Ignore the request to open another dialog
                return false;
            }
            else {
                constraintDialog = new TaskFemConstraintTemperature(this);
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
            Gui::Control().showDialog(new TaskDlgFemConstraintTemperature(this));
        }
        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);  // clazy:exclude=skipped-base-method
    }
}

#define HEIGHT (1.5)
#define RADIUS (0.3)
// #define USE_MULTIPLE_COPY  //OvG: MULTICOPY fails to update scaled display on initial drawing -
// so disable

void ViewProviderFemConstraintTemperature::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    Fem::ConstraintTemperature* pcConstraint =
        static_cast<Fem::ConstraintTemperature*>(this->getObject());
    float scaledradius =
        RADIUS * pcConstraint->Scale.getValue();  // OvG: Calculate scaled values once only
    float scaledheight = HEIGHT * pcConstraint->Scale.getValue();
    // float temperature = pcConstraint->temperature.getValue();

    if (prop == &pcConstraint->Points) {
        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();
        const std::vector<Base::Vector3d>& normals = pcConstraint->Normals.getValues();
        if (points.size() != normals.size()) {
            return;
        }
        std::vector<Base::Vector3d>::const_iterator n = normals.begin();

        // Note: Points and Normals are always updated together
        Gui::coinRemoveAllChildren(pShapeSep);

        for (const auto& point : points) {
            // Define base and normal directions
            SbVec3f base(point.x, point.y, point.z);
            SbVec3f dir(n->x, n->y, n->z);  // normal

            /// Temperature indication
            // define separator
            SoSeparator* sep = new SoSeparator();

            /// draw a temp gauge,with sphere and a cylinder
            // first move to correct position
            SoTranslation* trans = new SoTranslation();
            SbVec3f newPos = base + scaledradius * dir * 0.7f;
            trans->translation.setValue(newPos);
            sep->addChild(trans);

            // adjust orientation
            SoRotation* rot = new SoRotation();
            rot->rotation.setValue(SbRotation(SbVec3f(0, 1, 0), dir));
            sep->addChild(rot);

            // define color of shape
            SoMaterial* myMaterial = new SoMaterial;
            myMaterial->diffuseColor.set1Value(0, SbColor(1, 0, 0));  // RGB
            // myMaterial->diffuseColor.set1Value(1,SbColor(.1,.1,.1));//possible to adjust sides
            // separately
            sep->addChild(myMaterial);

            // draw a sphere
            SoSphere* sph = new SoSphere();
            sph->radius.setValue(scaledradius * 0.75);
            sep->addChild(sph);
            // translate position
            SoTranslation* trans2 = new SoTranslation();
            trans2->translation.setValue(SbVec3f(0, scaledheight * 0.375, 0));
            sep->addChild(trans2);
            // draw a cylinder
            SoCylinder* cyl = new SoCylinder();
            cyl->height.setValue(scaledheight * 0.5);
            cyl->radius.setValue(scaledradius * 0.375);
            sep->addChild(cyl);
            // translate position
            SoTranslation* trans3 = new SoTranslation();
            trans3->translation.setValue(SbVec3f(0, scaledheight * 0.375, 0));
            sep->addChild(trans3);
            // define color of shape
            SoMaterial* myMaterial2 = new SoMaterial;
            myMaterial2->diffuseColor.set1Value(0, SbColor(1, 1, 1));  // RGB
            sep->addChild(myMaterial2);
            // draw a cylinder
            SoCylinder* cyl2 = new SoCylinder();
            cyl2->height.setValue(scaledheight * 0.25);
            cyl2->radius.setValue(scaledradius * 0.375);
            sep->addChild(cyl2);

            pShapeSep->addChild(sep);

            n++;
        }
    }
    // Gets called whenever a property of the attached object changes
    ViewProviderFemConstraint::updateData(prop);
}
