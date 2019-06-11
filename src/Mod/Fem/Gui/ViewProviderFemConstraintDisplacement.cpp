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
#endif

#include "Mod/Fem/App/FemConstraintDisplacement.h"
#include "TaskFemConstraintDisplacement.h"
#include "ViewProviderFemConstraintDisplacement.h"
#include <Base/Console.h>
#include <Gui/Control.h>

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintDisplacement, FemGui::ViewProviderFemConstraint)

ViewProviderFemConstraintDisplacement::ViewProviderFemConstraintDisplacement()
{
    sPixmap = "fem-constraint-displacement";
    ADD_PROPERTY(FaceColor,(0.2f,0.3f,0.2f));
}

ViewProviderFemConstraintDisplacement::~ViewProviderFemConstraintDisplacement()
{
}

//FIXME setEdit needs a careful review
bool ViewProviderFemConstraintDisplacement::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraintDisplacement *constrDlg = qobject_cast<TaskDlgFemConstraintDisplacement *>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this)
            constrDlg = 0; // another constraint left open its task panel
        if (dlg && !constrDlg) {
            if (constraintDialog != NULL) {
                // Ignore the request to open another dialog
                return false;
            } else {
                constraintDialog = new TaskFemConstraintDisplacement(this);
                return true;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg)
            Gui::Control().showDialog(constrDlg);
        else
            Gui::Control().showDialog(new TaskDlgFemConstraintDisplacement(this));
        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

#define HEIGHT (4)
#define WIDTH (0.3)
//#define USE_MULTIPLE_COPY  //OvG: MULTICOPY fails to update scaled display on initial drawing - so disable

void ViewProviderFemConstraintDisplacement::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    Fem::ConstraintDisplacement* pcConstraint = static_cast<Fem::ConstraintDisplacement*>(this->getObject());
    float scaledwidth = WIDTH * pcConstraint->Scale.getValue(); //OvG: Calculate scaled values once only
    float scaledheight = HEIGHT * pcConstraint->Scale.getValue();
    bool xFree = pcConstraint->xFree.getValue();
    bool yFree = pcConstraint->yFree.getValue();
    bool zFree = pcConstraint->zFree.getValue();
    bool rotxFree = pcConstraint->rotxFree.getValue();
    bool rotyFree = pcConstraint->rotyFree.getValue();
    bool rotzFree = pcConstraint->rotzFree.getValue();

#ifdef USE_MULTIPLE_COPY
    //OvG: always need access to cp for scaling
    SoMultipleCopy* cpx = new SoMultipleCopy();
    SoMultipleCopy* cpy = new SoMultipleCopy();
    SoMultipleCopy* cpz = new SoMultipleCopy();
    SoMultipleCopy* cprotx = new SoMultipleCopy();
    SoMultipleCopy* cproty = new SoMultipleCopy();
    SoMultipleCopy* cprotz = new SoMultipleCopy();
    if (pShapeSep->getNumChildren() == 0) {
        // Set up the nodes
        cpx->matrix.setNum(0);
        cpx->addChild((SoNode*)createDisplacement(scaledheight, scaledwidth)); //OvG: Scaling

        cpy->matrix.setNum(0);
        cpy->addChild((SoNode*)createDisplacement(scaledheight, scaledwidth)); //OvG: Scaling

        cpz->matrix.setNum(0);
        cpz->addChild((SoNode*)createDisplacement(scaledheight, scaledwidth)); //OvG: Scaling

        cprotx->matrix.setNum(0);
        cprotx->addChild((SoNode*)createRotation(scaledheight, scaledwidth)); //OvG: Scaling

        cproty->matrix.setNum(0);
        cproty->addChild((SoNode*)createRotation(scaledheight, scaledwidth)); //OvG: Scaling

        cprotz->matrix.setNum(0);
        cprotz->addChild((SoNode*)createRotation(scaledheight, scaledwidth)); //OvG: Scaling

        pShapeSep->addChild(cpx);
        pShapeSep->addChild(cpy);
        pShapeSep->addChild(cpz);
        pShapeSep->addChild(cprotx);
        pShapeSep->addChild(cproty);
        pShapeSep->addChild(cprotz;
    }
#endif

    if (strcmp(prop->getName(),"Points") == 0) {
        const std::vector<Base::Vector3d>& points = pcConstraint->Points.getValues();
        const std::vector<Base::Vector3d>& normals = pcConstraint->Normals.getValues();
        if (points.size() != normals.size())
            return;
        std::vector<Base::Vector3d>::const_iterator n = normals.begin();

#ifdef USE_MULTIPLE_COPY
        cpx = static_cast<SoMultipleCopy*>(pShapeSep->getChild(0));
        cpx->matrix.setNum(points.size());
        SbMatrix* matricesx = cpx->matrix.startEditing();

        cpy = static_cast<SoMultipleCopy*>(pShapeSep->getChild(1));
        cpy->matrix.setNum(points.size());
        SbMatrix* matricesy = cpy->matrix.startEditing();

        cpz = static_cast<SoMultipleCopy*>(pShapeSep->getChild(2));
        cpz->matrix.setNum(points.size());
        SbMatrix* matricesz = cpz->matrix.startEditing();

        cprotx = static_cast<SoMultipleCopy*>(pShapeSep->getChild(3));
        cprotx->matrix.setNum(points.size());
        SbMatrix* matricesrotx = cprotx->matrix.startEditing();

        cproty = static_cast<SoMultipleCopy*>(pShapeSep->getChild(4));
        cproty->matrix.setNum(points.size());
        SbMatrix* matricesroty = cproty->matrix.startEditing();

        cprotz = static_cast<SoMultipleCopy*>(pShapeSep->getChild(5));
        cprotz->matrix.setNum(points.size());
        SbMatrix* matricesrotz = cprotz->matrix.startEditing();

        int idx = 0;
        int idy = 0;
        int idz = 0;
        int idrotx = 0;
        int idroty = 0;
        int idrotz = 0;
#else
        // Note: Points and Normals are always updated together
        pShapeSep->removeAllChildren();
#endif

        for (std::vector<Base::Vector3d>::const_iterator p = points.begin(); p != points.end(); p++) {
            SbVec3f base(p->x, p->y, p->z);
            SbVec3f dirx(1,0,0); //OvG: Make relevant to global axes
            SbVec3f diry(0,1,0); //OvG: Make relevant to global axes
            SbVec3f dirz(0,0,1); //OvG: Make relevant to global axes
            SbRotation rotx(SbVec3f(0,-1,0), dirx); //OvG Tri-cones
            SbRotation roty(SbVec3f(0,-1,0), diry);
            SbRotation rotz(SbVec3f(0,-1,0), dirz);
#ifdef USE_MULTIPLE_COPY
            SbMatrix mx;
            SbMatrix my;
            SbMatrix mz;
            //OvG: Translation indication
            if(!xFree)
            {
                SbMatrix mx;
                mx.setTransform(base, rotx, SbVec3f(1,1,1));
                matricesx[idx] = mx;
                idx++;
            }
            if(!yFree)
            {
                SbMatrix my;
                my.setTransform(base, roty, SbVec3f(1,1,1));
                matricesy[idy] = my;
                idy++;
            }
            if(!zFree)
            {
                SbMatrix mz;
                mz.setTransform(base, rotz, SbVec3f(1,1,1));
                matricesz[idz] = mz;
                idz++;
            }

            //OvG: Rotation indication
            if(!rotxFree)
            {
                SbMatrix mrotx;
                mrotx.setTransform(base, rotx, SbVec3f(1,1,1));
                matricesrotx[idrotx] = mrotx;
                idrotx++;
            }
            if(!rotyFree)
            {
                SbMatrix mroty;
                mroty.setTransform(base, roty, SbVec3f(1,1,1));
                matricesroty[idroty] = mroty;
                idroty++;
            }
            if(!rotzFree)
            {
                SbMatrix mrotz;
                mrotz.setTransform(base, rotz, SbVec3f(1,1,1));
                matricesrotz[idrotz] = mrotz;
                idrotz++;
            }
#else
            //OvG: Translation indication
            if(!xFree)
            {
                SoSeparator* sepx = new SoSeparator();
                createPlacement(sepx, base, rotx);
                createDisplacement(sepx, scaledheight, scaledwidth); //OvG: Scaling
                pShapeSep->addChild(sepx);
            }
            if(!yFree)
            {
                SoSeparator* sepy = new SoSeparator();
                createPlacement(sepy, base, roty);
                createDisplacement(sepy, scaledheight, scaledwidth); //OvG: Scaling
                pShapeSep->addChild(sepy);
            }
            if(!zFree)
            {
                SoSeparator* sepz = new SoSeparator();
                createPlacement(sepz, base, rotz);
                createDisplacement(sepz, scaledheight, scaledwidth); //OvG: Scaling
                pShapeSep->addChild(sepz);
            }

            //OvG: Rotation indication
            if(!rotxFree)
            {
                SoSeparator* sepx = new SoSeparator();
                createPlacement(sepx, base, rotx);
                createRotation(sepx, scaledheight, scaledwidth); //OvG: Scaling
                pShapeSep->addChild(sepx);
            }
            if(!rotyFree)
            {
                SoSeparator* sepy = new SoSeparator();
                createPlacement(sepy, base, roty);
                createRotation(sepy, scaledheight, scaledwidth); //OvG: Scaling
                pShapeSep->addChild(sepy);
            }
            if(!rotzFree)
            {
                SoSeparator* sepz = new SoSeparator();
                createPlacement(sepz, base, rotz);
                createRotation(sepz, scaledheight, scaledwidth); //OvG: Scaling
                pShapeSep->addChild(sepz);
            }
#endif
            n++;
        }
#ifdef USE_MULTIPLE_COPY
        cpx->matrix.finishEditing();
        cpy->matrix.finishEditing();
        cpz->matrix.finishEditing();
        cprotx->matrix.finishEditing();
        cproty->matrix.finishEditing();
        cprotz->matrix.finishEditing();
#endif
    }

    // Gets called whenever a property of the attached object changes
    ViewProviderFemConstraint::updateData(prop);
}
