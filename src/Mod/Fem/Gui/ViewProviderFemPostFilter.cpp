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

#include <Mod/Fem/App/FemPostFilter.h>

#include "TaskPostBoxes.h"
#include "ViewProviderFemPostFilter.h"


using namespace FemGui;

// ***************************************************************************
// in the following, the different filters sorted alphabetically
// ***************************************************************************


// ***************************************************************************
// data along line filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostDataAlongLine, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostDataAlongLine::ViewProviderFemPostDataAlongLine()
{
    sPixmap = "FEM_PostFilterDataAlongLine";
}

ViewProviderFemPostDataAlongLine::~ViewProviderFemPostDataAlongLine() = default;

void ViewProviderFemPostDataAlongLine::setupTaskDialog(TaskDlgPost* dlg)
{
    // add the function box
    dlg->appendBox(new TaskPostDataAlongLine(dlg->getView()));
}


// ***************************************************************************
// data at point filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostDataAtPoint, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostDataAtPoint::ViewProviderFemPostDataAtPoint()
{
    sPixmap = "FEM_PostFilterDataAtPoint";
}

void ViewProviderFemPostDataAtPoint::show()
{
    Gui::ViewProviderDocumentObject::show();
}

void ViewProviderFemPostDataAtPoint::onSelectionChanged(const Gui::SelectionChanges&)
{
    // do not do anything here
    // For DataAtPoint the color bar must not be refreshed when it is selected
    // because a single point does not make sense with a color range.
}

ViewProviderFemPostDataAtPoint::~ViewProviderFemPostDataAtPoint() = default;

void ViewProviderFemPostDataAtPoint::setupTaskDialog(TaskDlgPost* dlg)
{
    // add the function box
    dlg->appendBox(new TaskPostDataAtPoint(dlg->getView()));
}


// ***************************************************************************
// clip filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostClip, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostClip::ViewProviderFemPostClip()
{

    sPixmap = "FEM_PostFilterClipRegion";
}

ViewProviderFemPostClip::~ViewProviderFemPostClip() = default;

void ViewProviderFemPostClip::setupTaskDialog(TaskDlgPost* dlg)
{

    // add the function box
    dlg->appendBox(new TaskPostClip(
        dlg->getView(),
        &static_cast<Fem::FemPostClipFilter*>(dlg->getView()->getObject())->Function));

    // add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}


// ***************************************************************************
// contours filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostContours, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostContours::ViewProviderFemPostContours()
{
    sPixmap = "FEM_PostFilterContours";
}

ViewProviderFemPostContours::~ViewProviderFemPostContours() = default;

void ViewProviderFemPostContours::setupTaskDialog(TaskDlgPost* dlg)
{
    // the filter-specific task panel
    dlg->appendBox(new TaskPostContours(dlg->getView()));
}


// ***************************************************************************
// cut filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostCut, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostCut::ViewProviderFemPostCut()
{
    sPixmap = "FEM_PostFilterCutFunction";
}

ViewProviderFemPostCut::~ViewProviderFemPostCut() = default;

void ViewProviderFemPostCut::setupTaskDialog(TaskDlgPost* dlg)
{
    // add the function box
    dlg->appendBox(new TaskPostCut(
        dlg->getView(),
        &static_cast<Fem::FemPostCutFilter*>(dlg->getView()->getObject())->Function));

    // add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}


// ***************************************************************************
// scalar clip filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostScalarClip, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostScalarClip::ViewProviderFemPostScalarClip()
{
    sPixmap = "FEM_PostFilterClipScalar";
}

ViewProviderFemPostScalarClip::~ViewProviderFemPostScalarClip() = default;

void ViewProviderFemPostScalarClip::setupTaskDialog(TaskDlgPost* dlg)
{
    // add the function box
    dlg->appendBox(new TaskPostScalarClip(dlg->getView()));

    // add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}


// ***************************************************************************
// warp vector filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostWarpVector, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostWarpVector::ViewProviderFemPostWarpVector()
{
    sPixmap = "FEM_PostFilterWarp";
}

ViewProviderFemPostWarpVector::~ViewProviderFemPostWarpVector() = default;

void ViewProviderFemPostWarpVector::setupTaskDialog(TaskDlgPost* dlg)
{
    // add the function box
    dlg->appendBox(new TaskPostWarpVector(dlg->getView()));

    // add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}
