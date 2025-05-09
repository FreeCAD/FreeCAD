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

#ifndef _PreComp_
#include <Inventor/nodes/SoDrawStyle.h>
#endif

#include <Mod/Fem/App/FemPostFilter.h>

#include "TaskPostBoxes.h"
#include "ViewProviderFemPostFilter.h"
#include "ViewProviderFemPostFilterPy.h"

#ifdef FC_USE_VTK_PYTHON
#include "TaskPostExtraction.h"
#endif

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemPostFilterPythonBase, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostFilterPythonBase::ViewProviderFemPostFilterPythonBase()
{}

ViewProviderFemPostFilterPythonBase::~ViewProviderFemPostFilterPythonBase() = default;

std::vector<std::string> ViewProviderFemPostFilterPythonBase::getDisplayModes() const
{
    return std::vector<std::string>();
}

namespace Gui
{
PROPERTY_SOURCE_TEMPLATE(FemGui::ViewProviderPostFilterPython,
                         FemGui::ViewProviderFemPostFilterPythonBase)

template<>
PyObject* FemGui::ViewProviderPostFilterPython::getPyObject()
{
    if (!pyViewObject) {
        pyViewObject = new ViewProviderFemPostFilterPy(this);
    }
    pyViewObject->IncRef();
    return pyViewObject;
}

// explicit template instantiation
template class FemGuiExport ViewProviderFeaturePythonT<FemGui::ViewProviderFemPostFilterPythonBase>;

}  // namespace Gui

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
    assert(dlg->getView() == this);
    auto panel = new TaskPostDataAlongLine(this);
    dlg->addTaskBox(panel->getIcon(), panel);

#ifdef FC_USE_VTK_PYTHON
    // and the extraction
    auto extr_panel = new TaskPostExtraction(this);
    dlg->addTaskBox(extr_panel->windowIcon().pixmap(32), extr_panel);
#endif
}


// ***************************************************************************
// data at point filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostDataAtPoint, FemGui::ViewProviderFemPostObject)

App::PropertyFloatConstraint::Constraints ViewProviderFemPostDataAtPoint::sizeRange = {1.0,
                                                                                       64.0,
                                                                                       1.0};

ViewProviderFemPostDataAtPoint::ViewProviderFemPostDataAtPoint()
{
    float pSize = m_drawStyle->pointSize.getValue();
    ADD_PROPERTY_TYPE(PointSize, (pSize), "Object Style", App::Prop_None, "Set point size");
    PointSize.setConstraints(&sizeRange);

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

void ViewProviderFemPostDataAtPoint::onChanged(const App::Property* prop)
{
    if (prop == &PointSize) {
        m_drawStyle->pointSize.setValue(PointSize.getValue());
    }

    ViewProviderFemPostObject::onChanged(prop);
}

ViewProviderFemPostDataAtPoint::~ViewProviderFemPostDataAtPoint() = default;

void ViewProviderFemPostDataAtPoint::setupTaskDialog(TaskDlgPost* dlg)
{
    // add the function box
    assert(dlg->getView() == this);
    auto panel = new TaskPostDataAtPoint(this);
    dlg->addTaskBox(panel->getIcon(), panel);

#ifdef FC_USE_VTK_PYTHON
    // and the extraction
    auto extr_panel = new TaskPostExtraction(this);
    dlg->addTaskBox(extr_panel->windowIcon().pixmap(32), extr_panel);
#endif
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
    assert(dlg->getView() == this);
    auto panel =
        new TaskPostClip(this, &dlg->getView()->getObject<Fem::FemPostClipFilter>()->Function);
    dlg->addTaskBox(panel->getIcon(), panel);

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
    assert(dlg->getView() == this);
    auto panel = new TaskPostContours(this);
    dlg->addTaskBox(panel->getIcon(), panel);
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
    assert(dlg->getView() == this);
    auto panel =
        new TaskPostCut(this, &dlg->getView()->getObject<Fem::FemPostCutFilter>()->Function);
    dlg->addTaskBox(panel->getIcon(), panel);

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
    assert(dlg->getView() == this);
    auto panel = new TaskPostScalarClip(this);
    dlg->addTaskBox(panel->getIcon(), panel);

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
    assert(dlg->getView() == this);
    auto panel = new TaskPostWarpVector(this);
    dlg->addTaskBox(panel->getIcon(), panel);

    // add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}


// ***************************************************************************
// calculator filter
PROPERTY_SOURCE(FemGui::ViewProviderFemPostCalculator, FemGui::ViewProviderFemPostObject)

ViewProviderFemPostCalculator::ViewProviderFemPostCalculator()
{
    sPixmap = "FEM_PostFilterCalculator";
}

ViewProviderFemPostCalculator::~ViewProviderFemPostCalculator() = default;

void ViewProviderFemPostCalculator::updateData(const App::Property* prop)
{
    auto obj = getObject<Fem::FemPostCalculatorFilter>();
    if (prop == &obj->Data) {
        // update color bar
        ViewProviderFemPostObject::updateData(prop);
        updateMaterial();
    }
    else {
        return ViewProviderFemPostObject::updateData(prop);
    }
}

void ViewProviderFemPostCalculator::setupTaskDialog(TaskDlgPost* dlg)
{
    // add the function box
    assert(dlg->getView() == this);
    auto panel = new TaskPostCalculator(this);
    dlg->addTaskBox(panel->getIcon(), panel);

    // add the display options
    FemGui::ViewProviderFemPostObject::setupTaskDialog(dlg);
}
