// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <QMessageBox>


#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProviderCoordinateSystem.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>

#include "ui_TaskMirroredParameters.h"
#include "TaskMirroredParameters.h"
#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskMirroredParameters */

TaskMirroredParameters::TaskMirroredParameters(ViewProviderTransformed* TransformedView, QWidget* parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskMirroredParameters)
{
    setupUI();
}

TaskMirroredParameters::TaskMirroredParameters(
    TaskMultiTransformParameters* parentTask,
    QWidget* parameterWidget
)
    : TaskTransformedParameters(parentTask)
    , ui(new Ui_TaskMirroredParameters)
{
    setupParameterUI(parameterWidget);
}

void TaskMirroredParameters::setupParameterUI(QWidget* widget)
{
    ui->setupUi(widget);
    QMetaObject::connectSlotsByName(this);

    connect(
        ui->comboPlane,
        qOverload<int>(&QComboBox::activated),
        this,
        &TaskMirroredParameters::onPlaneChanged
    );

    this->planeLinks.setCombo(ui->comboPlane);
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
            auto vpOrigin = static_cast<ViewProviderCoordinateSystem*>(
                Gui::Application::Instance->getViewProvider(origin)
            );
            vpOrigin->setTemporaryVisibility(Gui::DatumElement::Planes);
        }
        catch (const Base::Exception& ex) {
            Base::Console().error("%s\n", ex.what());
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

    syncStagedMirrorPlaneFromObject();
    syncPlaneComboToStagedMirrorPlane();

    blockUpdate = false;
}

void TaskMirroredParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // Handle selection ONLY when in reference selection mode
    if (selectionMode == SelectionMode::None || msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }

    if (originalSelected(msg)) {
        exitSelectionMode();
        return;
    }

    auto pcMirrored = getObject<PartDesign::Mirrored>();

    std::vector<std::string> mirrorPlanes;
    App::DocumentObject* selObj = nullptr;
    getReferencedSelection(pcMirrored, msg, selObj, mirrorPlanes);
    if (!selObj) {
        return;
    }

    if (selectionMode == SelectionMode::Reference || selObj->isDerivedFrom<App::Plane>()) {
        App::PropertyLinkSub mirrorPlane;
        mirrorPlane.setValue(selObj, mirrorPlanes);
        setStagedMirrorPlane(mirrorPlane);
        requestStagedPreviewUpdate();
    }
    exitSelectionMode();
}

void TaskMirroredParameters::onPlaneChanged(int /*num*/)
{
    try {
        const auto& currentLink = planeLinks.getCurrentLink();
        if (!currentLink.getValue()) {
            syncPlaneComboToStagedMirrorPlane();
            // enter reference selection mode
            hideObject();
            showBase();
            selectionMode = SelectionMode::Reference;
            Gui::Selection().clearSelection();
            addReferenceSelectionGate(AllowSelection::FACE | AllowSelection::PLANAR);
        }
        else {
            exitSelectionMode();
            setStagedMirrorPlane(currentLink);
            requestStagedPreviewUpdate();
        }
    }
    catch (Base::Exception& e) {
        QMessageBox::warning(nullptr, tr("Error"), QApplication::translate("Exception", e.what()));
    }
}

void TaskMirroredParameters::onUpdateView(bool on)
{
    setUpdateViewEnabled(on);
    if (on) {
        requestStagedPreviewUpdate();
    }
}

void TaskMirroredParameters::getMirrorPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    const App::PropertyLinkSub& lnk = stagedMirrorPlane;
    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

void TaskMirroredParameters::syncStagedMirrorPlaneFromObject()
{
    auto pcMirrored = getObject<PartDesign::Mirrored>();
    stagedMirrorPlane.Paste(pcMirrored->MirrorPlane);
}

void TaskMirroredParameters::setStagedMirrorPlane(const App::PropertyLinkSub& lnk)
{
    stagedMirrorPlane.Paste(lnk);
    syncPlaneComboToStagedMirrorPlane();
}

void TaskMirroredParameters::syncPlaneComboToStagedMirrorPlane()
{
    if (planeLinks.setCurrentLink(stagedMirrorPlane) != -1) {
        return;
    }

    auto* object = stagedMirrorPlane.getValue();
    if (!object) {
        return;
    }

    QString label = getRefStr(object, stagedMirrorPlane.getSubValues());
    if (label.isEmpty()) {
        label = QString::fromLatin1(object->getNameInDocument());
    }

    for (int i = 0; i < planeLinks.count(); ++i) {
        if (!planeLinks.getLink(i).getValue()) {
            planeLinks.addLinkBefore(stagedMirrorPlane, label, planeLinks.getUserData(i));
            planeLinks.setCurrentLink(stagedMirrorPlane);
            return;
        }
    }

    planeLinks.addLink(stagedMirrorPlane, label);
    planeLinks.setCurrentLink(stagedMirrorPlane);
}

void TaskMirroredParameters::applyStagedPreviewStateToObject()
{
    auto pcMirrored = getObject<PartDesign::Mirrored>();
    pcMirrored->MirrorPlane.Paste(stagedMirrorPlane);
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
            auto vpOrigin = static_cast<ViewProviderCoordinateSystem*>(
                Gui::Application::Instance->getViewProvider(origin)
            );
            vpOrigin->resetTemporaryVisibility();
        }
    }
    catch (const Base::Exception& ex) {
        Base::Console().error("%s\n", ex.what());
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
    Content.push_back(preview);
}

#include "moc_TaskMirroredParameters.cpp"
