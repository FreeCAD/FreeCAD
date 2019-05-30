/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
 *            Ofentse Kgoa <kgoaot@eskom.co.za>                            *
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
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoText3.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoScale.h>

# include <math.h>
#endif

#include <Gui/Command.h>

#include "Mod/Fem/App/FemConstraintTransform.h"
#include "TaskFemConstraintTransform.h"
#include "ViewProviderFemConstraintTransform.h"
#include <Base/Console.h>
#include <Gui/Control.h>


#define PI (3.141592653589793238462643383279502884L)

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintTransform, FemGui::ViewProviderFemConstraint)

ViewProviderFemConstraintTransform::ViewProviderFemConstraintTransform()
{
    sPixmap = "fem-constraint-transform";
    //
    //ADD_PROPERTY(FaceColor,(0.0f,0.2f,0.8f));
}

ViewProviderFemConstraintTransform::~ViewProviderFemConstraintTransform()
{
}

//FIXME setEdit needs a careful review
bool ViewProviderFemConstraintTransform::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintTransform *constrDlg = qobject_cast<TaskDlgFemConstraintTransform *>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this)
            constrDlg = 0; // another constraint left open its task panel
        if (dlg && !constrDlg) {
            if (constraintDialog != NULL) {
                // Ignore the request to open another dialog
                return false;
            } else {
                constraintDialog = new TaskFemConstraintTransform(this);
                return true;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg)
            Gui::Control().showDialog(constrDlg);
        else
            Gui::Control().showDialog(new TaskDlgFemConstraintTransform(this));
        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

#define HEIGHTAXIS (20)
#define RADIUSAXIS (0.8)
#define ARROWLENGTH (3)
#define ARROWHEADRADIUS (ARROWLENGTH/3)
#define LENGTHDISC (0.25)
#define RADIUSDISC (0.8)

void ViewProviderFemConstraintTransform::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    Fem::ConstraintTransform* pcConstraint = static_cast<Fem::ConstraintTransform*>(this->getObject());
    float scaledradiusaxis = RADIUSAXIS * pcConstraint->Scale.getValue(); //OvG: Calculate scaled values once only
    float scaledheightaxis = HEIGHTAXIS * pcConstraint->Scale.getValue();
    float scaledheadradiusA = ARROWHEADRADIUS * pcConstraint->Scale.getValue(); //OvG: Calculate scaled values once only
    float scaledlengthA = ARROWLENGTH * pcConstraint->Scale.getValue();
    std::string transform_type = pcConstraint->TransformType.getValueAsString();
    if (transform_type == "Rectangular") {

    if (strcmp(prop->getName(),"Points") == 0) {
        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();
        const std::vector<Base::Vector3d>& normals = pcConstraint->Normals.getValues();
        if (points.size() != normals.size())
            return;
        std::vector<Base::Vector3d>::const_iterator n = normals.begin();

        // Points and Normals are always updated together
        Gui::coinRemoveAllChildren(pShapeSep);

        for (std::vector<Base::Vector3d>::const_iterator p = points.begin(); p != points.end(); p++) {
            SbVec3f base(p->x, p->y, p->z);
            SbVec3f basex(p->x, p->y, p->z);
            SbVec3f basey(p->x, p->y, p->z);

            double x_axis_x = 1;
            double x_axis_y = 0;
            double x_axis_z = 0;

            double y_axis_x = 0;
            double y_axis_y = 1;
            double y_axis_z = 0;

            double z_axis_x = 0;
            double z_axis_y = 0;
            double z_axis_z = 1;

            double rot_x = (pcConstraint->X_rot.getValue() * (PI/180));
            double rot_y = (pcConstraint->Y_rot.getValue() * (PI/180));
            double rot_z = (pcConstraint->Z_rot.getValue() * (PI/180));

            double x_axis_x_p;
            double x_axis_y_p;
            double x_axis_z_p;

            double y_axis_x_p;
            double y_axis_y_p;
            double y_axis_z_p;

            double z_axis_x_p;
            double z_axis_y_p;
            double z_axis_z_p;

            if (rot_x!=0){
                x_axis_z_p = x_axis_z*cos(rot_x) - x_axis_y*sin(rot_x);
                x_axis_y_p = x_axis_y*cos(rot_x) + x_axis_z*sin(rot_x);
                x_axis_z = x_axis_z_p;
                x_axis_y = x_axis_y_p;

                y_axis_z_p = y_axis_z*cos(rot_x) - y_axis_y*sin(rot_x);
                y_axis_y_p = y_axis_y*cos(rot_x) + y_axis_z*sin(rot_x);
                y_axis_z = y_axis_z_p;
                y_axis_y = y_axis_y_p;

                z_axis_z_p = z_axis_z*cos(rot_x) - z_axis_y*sin(rot_x);
                z_axis_y_p = z_axis_y*cos(rot_x) + z_axis_z*sin(rot_x);
                z_axis_z = z_axis_z_p;
                z_axis_y = z_axis_y_p;
            }
            if (rot_y != 0){
                x_axis_z_p = x_axis_z*cos(rot_y) + x_axis_x*sin(rot_y);
                x_axis_x_p = x_axis_x*cos(rot_y) - x_axis_z*sin(rot_y);
                x_axis_z = x_axis_z_p;
                x_axis_x = x_axis_x_p;

                y_axis_z_p = y_axis_z*cos(rot_y) + y_axis_x*sin(rot_y);
                y_axis_x_p = y_axis_x*cos(rot_y) - y_axis_z*sin(rot_y);
                y_axis_z = y_axis_z_p;
                y_axis_x = y_axis_x_p;

                z_axis_z_p = z_axis_z*cos(rot_y) + z_axis_x*sin(rot_y);
                z_axis_x_p = z_axis_x*cos(rot_y) - z_axis_z*sin(rot_y);
                z_axis_z = z_axis_z_p;
                z_axis_x = z_axis_x_p;
            }
            if (rot_z !=0){
                x_axis_x_p = x_axis_x*cos(rot_z) + x_axis_y*sin(rot_z);
                x_axis_y_p = x_axis_y*cos(rot_z) - x_axis_x*sin(rot_z);
                x_axis_x = x_axis_x_p;
                x_axis_y = x_axis_y_p;

                y_axis_x_p = y_axis_x*cos(rot_z) + y_axis_y*sin(rot_z);
                y_axis_y_p = y_axis_y*cos(rot_z) - y_axis_x*sin(rot_z);
                y_axis_x = y_axis_x_p;
                y_axis_y = y_axis_y_p;

                z_axis_x_p = z_axis_x*cos(rot_z) + z_axis_y*sin(rot_z);
                z_axis_y_p = z_axis_y*cos(rot_z) - z_axis_x*sin(rot_z);
                z_axis_x = z_axis_x_p;
                z_axis_y = z_axis_y_p;
            }

            SbVec3f dirz(z_axis_x, z_axis_y ,z_axis_z);
            SbRotation rot(SbVec3f(0, 1, 0), dirz);

            SbVec3f dirx(x_axis_x, x_axis_y ,x_axis_z);
            SbRotation rotx(SbVec3f(0, 1, 0), dirx);

            SbVec3f diry(y_axis_x, y_axis_y ,y_axis_z);
            SbRotation roty(SbVec3f(0, 1, 0), diry);

            base = base + dirz * scaledlengthA * 0.75f;
            basex = basex + dirx * scaledlengthA * 0.65f;
            basey = basey + diry * scaledlengthA * 0.65f;

            SoSeparator* sep = new SoSeparator();

            SoMaterial* myMaterial = new SoMaterial;
            myMaterial->diffuseColor.set1Value(0,SbColor(0,0,1));//RGB
            sep->addChild(myMaterial);

            createPlacement(sep, base, rot);
            createArrow(sep, scaledlengthA*0.75 , scaledheadradiusA*0.9); //OvG: Scaling
            pShapeSep->addChild(sep);

            SoSeparator* sepx = new SoSeparator();

            SoMaterial* myMaterialx = new SoMaterial;
            myMaterialx->diffuseColor.set1Value(0,SbColor(1,0,0));//RGB
            sepx->addChild(myMaterialx);

            createPlacement(sepx, basex, rotx);
            createArrow(sepx, scaledlengthA*0.65 , scaledheadradiusA*0.65); //OvG: Scaling
            pShapeSep->addChild(sepx);

            SoSeparator* sepy = new SoSeparator();

            SoMaterial* myMaterialy = new SoMaterial;
            myMaterialy->diffuseColor.set1Value(0,SbColor(0,1,0));//RGB
            sepy->addChild(myMaterialy);

            createPlacement(sepy, basey, roty);
            createArrow(sepy, scaledlengthA*0.65 , scaledheadradiusA*0.65); //OvG: Scaling
            pShapeSep->addChild(sepy);

            n++;
        }

    }
    } else if (transform_type == "Cylindrical") {

        // Points and Normals are always updated together
        Gui::coinRemoveAllChildren(pShapeSep);

        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();
        const std::vector<Base::Vector3d>& normals = pcConstraint->Normals.getValues();

        if (points.size() != normals.size())
            return;
        std::vector<Base::Vector3d>::const_iterator n = normals.begin();

        if (points.size() > 0) {
            Base::Vector3d base = pcConstraint->BasePoint.getValue();
            Base::Vector3d axis = pcConstraint->Axis.getValue();

            SbVec3f b(base.x, base.y, base.z);
            SbVec3f ax(axis.x, axis.y, axis.z);
            SbRotation rots(SbVec3f(0,-1,0), ax);

            b = b - ax * scaledheightaxis/2;
            SoSeparator* sepAx = new SoSeparator();
            SoMaterial* myMaterial = new SoMaterial;
            myMaterial->diffuseColor.set1Value(0,SbColor(0,0,1));//RGB
            sepAx->addChild(myMaterial);
            createPlacement(sepAx, b, rots);
            createArrow(sepAx, scaledheightaxis, scaledradiusaxis);
            pShapeSep->addChild(sepAx);
        }

        for (std::vector<Base::Vector3d>::const_iterator p = points.begin(); p != points.end(); p++) {
            SbVec3f base(p->x, p->y, p->z);
            SbVec3f dir(n->x, n->y, n->z);
            base = base + dir * scaledlengthA; //OvG: Scaling
            SbRotation rot(SbVec3f(0, 1, 0), dir);

            SoSeparator* sep = new SoSeparator();
            SoMaterial* myMaterials = new SoMaterial;
            myMaterials->diffuseColor.set1Value(0,SbColor(1,0,0));//RGB
            sep->addChild(myMaterials);
            createPlacement(sep, base, rot);
            createArrow(sep, scaledlengthA , scaledheadradiusA); //OvG: Scaling
            pShapeSep->addChild(sep);
            n++;
        }
    }

    // Gets called whenever a property of the attached object changes
    ViewProviderFemConstraint::updateData(prop);
}
