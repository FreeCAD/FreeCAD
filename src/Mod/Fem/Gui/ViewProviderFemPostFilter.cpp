/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include "ViewProviderFemPostFilter.h"
#include "TaskPostBoxes.h"
#include <Mod/Fem/App/FemPostFilter.h>
#include <Base/Console.h>

using namespace FemGui;


PROPERTY_SOURCE(FemGui::ViewProviderFemPostClip, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostClip::ViewProviderFemPostClip() {

    sPixmap = "FEM_PostFilterClipRegion";
}

ViewProviderFemPostClip::~ViewProviderFemPostClip() {

}

void ViewProviderFemPostClip::setupTaskDialog(TaskDlgPost* dlg) {

    //add the function box
    dlg->appendBox(new TaskPostClip(dlg->getView(),
                                    &static_cast<Fem::FemPostClipFilter*>(dlg->getView()->getObject())->Function));

    //add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}

PROPERTY_SOURCE(FemGui::ViewProviderFemPostDataAlongLine, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostDataAlongLine::ViewProviderFemPostDataAlongLine() {

    sPixmap = "FEM_PostFilterDataAlongLine";
}

ViewProviderFemPostDataAlongLine::~ViewProviderFemPostDataAlongLine() {

}

void ViewProviderFemPostDataAlongLine::setupTaskDialog(TaskDlgPost* dlg) {

    //add the function box
    dlg->appendBox(new TaskPostDataAlongLine(dlg->getView()));

}

PROPERTY_SOURCE(FemGui::ViewProviderFemPostDataAtPoint, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostDataAtPoint::ViewProviderFemPostDataAtPoint() {

    sPixmap = "FEM_PostFilterDataAtPoint";
}

ViewProviderFemPostDataAtPoint::~ViewProviderFemPostDataAtPoint() {

}

void ViewProviderFemPostDataAtPoint::setupTaskDialog(TaskDlgPost* dlg) {

    //add the function box
    dlg->appendBox(new TaskPostDataAtPoint(dlg->getView()));

}


PROPERTY_SOURCE(FemGui::ViewProviderFemPostScalarClip, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostScalarClip::ViewProviderFemPostScalarClip() {

    sPixmap = "FEM_PostFilterClipScalar";
}

ViewProviderFemPostScalarClip::~ViewProviderFemPostScalarClip() {

}

void ViewProviderFemPostScalarClip::setupTaskDialog(TaskDlgPost* dlg) {

    //add the function box
    dlg->appendBox(new TaskPostScalarClip(dlg->getView()));

    //add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}

PROPERTY_SOURCE(FemGui::ViewProviderFemPostWarpVector, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostWarpVector::ViewProviderFemPostWarpVector() {

    sPixmap = "FEM_PostFilterWarp";
}

ViewProviderFemPostWarpVector::~ViewProviderFemPostWarpVector() {

}

void ViewProviderFemPostWarpVector::setupTaskDialog(TaskDlgPost* dlg) {

    //add the function box
    dlg->appendBox(new TaskPostWarpVector(dlg->getView()));

    //add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}


PROPERTY_SOURCE(FemGui::ViewProviderFemPostCut, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostCut::ViewProviderFemPostCut() {

    sPixmap = "FEM_PostFilterCutFunction";
}

ViewProviderFemPostCut::~ViewProviderFemPostCut() {

}

void ViewProviderFemPostCut::setupTaskDialog(TaskDlgPost* dlg) {

    //add the function box
    dlg->appendBox(new TaskPostCut(dlg->getView(),
                                    &static_cast<Fem::FemPostCutFilter*>(dlg->getView()->getObject())->Function));

    //add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}
