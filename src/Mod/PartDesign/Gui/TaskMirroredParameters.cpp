/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#include <QMessageBox>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>

#include "ui_TaskMirroredParameters.h"
#include "TaskMirroredParameters.h"
#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskMirroredParameters */

TaskMirroredParameters::TaskMirroredParameters(ViewProviderTransformed* TransformedView,
                                               QWidget* parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskMirroredParameters)
{
    setupUI();
}

TaskMirroredParameters::TaskMirroredParameters(TaskMultiTransformParameters* parentTask,
                                               QWidget* parameterWidget)
    : TaskTransformedParameters(parentTask)
    , ui(new Ui_TaskMirroredParameters)
{
    setupParameterUI(parameterWidget);
}

void TaskMirroredParameters::setupParameterUI(QWidget* widget)
{
    ui->setupUi(widget);
    QMetaObject::connectSlotsByName(this);

    connect(ui->comboPlane,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskMirroredParameters::onPlaneChanged);

    this->planeLinks.setCombo(*(ui->comboPlane));
    ui->comboPlane->setEnabled(true);

    App::DocumentObject* sketch = getSketchObject();
    if (sketch && sketch->isDerivedFrom<Part::Part2DObject>()) {
        this->fillPlanesCombo(planeLinks, static_cast<Part::Part2DObject*>(sketch));
    }
    else {
        this->fillPlanesCombo(planeLinks, nullptr);
    }

    // show the parts coordinate system planes for selection
    PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
    if (body) {
        try {
            App::Origin* origin = body->getOrigin();
            auto vpOrigin = static_cast<ViewProviderOrigin*>(
                Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(false, true);
        }
        catch (const Base::Exception& ex) {
            Base::Console().Error("%s\n", ex.what());
        }
    }

    updateUI();
}

void TaskMirroredParameters::retranslateParameterUI(QWidget* widget)
{
    ui->retranslateUi(widget);
}

void TaskMirroredParameters::updateUI()
{
    if (blockUpdate) {
        return;
    }
    blockUpdate = true;

    auto pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

    if (planeLinks.setCurrentLink(pcMirrored->MirrorPlane) == -1) {
        // failed to set current, because the link isn't in the list yet
        planeLinks.addLink(
            pcMirrored->MirrorPlane,
            getRefStr(pcMirrored->MirrorPlane.getValue(), pcMirrored->MirrorPlane.getSubValues()));
        planeLinks.setCurrentLink(pcMirrored->MirrorPlane);
    }

    blockUpdate = false;
}

void TaskMirroredParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode != SelectionMode::None && msg.Type == Gui::SelectionChanges::AddSelection) {

        if (originalSelected(msg)) {
            exitSelectionMode();
        }
        else {
            auto pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

            std::vector<std::string> mirrorPlanes;
            App::DocumentObject* selObj = nullptr;
            getReferencedSelection(pcMirrored, msg, selObj, mirrorPlanes);
            if (!selObj) {
                return;
            }

            if (selectionMode == SelectionMode::Reference || selObj->isDerivedFrom<App::Plane>()) {
                setupTransaction();
                pcMirrored->MirrorPlane.setValue(selObj, mirrorPlanes);
                recomputeFeature();
                updateUI();
            }
            exitSelectionMode();
        }
    }
}

void TaskMirroredParameters::onPlaneChanged(int /*num*/)
{
    if (blockUpdate) {
        return;
    }
    setupTransaction();
    auto pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    try {
        if (!planeLinks.getCurrentLink().getValue()) {
            // enter reference selection mode
            hideObject();
            showBase();
            selectionMode = SelectionMode::Reference;
            Gui::Selection().clearSelection();
            addReferenceSelectionGate(AllowSelection::FACE | AllowSelection::PLANAR);
        }
        else {
            exitSelectionMode();
            pcMirrored->MirrorPlane.Paste(planeLinks.getCurrentLink());
        }
    }
    catch (Base::Exception& e) {
        QMessageBox::warning(nullptr, tr("Error"), QApplication::translate("Exception", e.what()));
    }

    recomputeFeature();
}

void TaskMirroredParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        setupTransaction();
        // Do the same like in TaskDlgMirroredParameters::accept() but without doCommand
        auto pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
        std::vector<std::string> mirrorPlanes;
        App::DocumentObject* obj = nullptr;

        getMirrorPlane(obj, mirrorPlanes);
        pcMirrored->MirrorPlane.setValue(obj, mirrorPlanes);

        recomputeFeature();
    }
}

void TaskMirroredParameters::getMirrorPlane(App::DocumentObject*& obj,
                                            std::vector<std::string>& sub) const
{
    const App::PropertyLinkSub& lnk = planeLinks.getCurrentLink();
    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

void TaskMirroredParameters::apply()
{
    std::vector<std::string> mirrorPlanes;
    App::DocumentObject* obj = nullptr;
    getMirrorPlane(obj, mirrorPlanes);
    std::string mirrorPlane = buildLinkSingleSubPythonStr(obj, mirrorPlanes);

    FCMD_OBJ_CMD(getObject(), "MirrorPlane = " << mirrorPlane);
}

TaskMirroredParameters::~TaskMirroredParameters()
{
    // hide the parts coordinate system axis for selection
    try {
        PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
        if (body) {
            App::Origin* origin = body->getOrigin();
            auto vpOrigin = static_cast<ViewProviderOrigin*>(
                Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    }
    catch (const Base::Exception& ex) {
        Base::Console().Error("%s\n", ex.what());
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgMirroredParameters::TaskDlgMirroredParameters(ViewProviderMirrored* MirroredView)
    : TaskDlgTransformedParameters(MirroredView)
{
    parameter = new TaskMirroredParameters(MirroredView);

    Content.push_back(parameter);
}

#include "moc_TaskMirroredParameters.cpp"
