/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
# include <QMessageBox>
# include <QAction>
#endif

#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"
#include "Utils.h"

#include "ui_TaskMirroredParameters.h"
#include "TaskMirroredParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskMirroredParameters */

TaskMirroredParameters::TaskMirroredParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskMirroredParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskMirroredParameters::TaskMirroredParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskMirroredParameters();
    ui->setupUi(proxy);
    connect(ui->buttonOK, SIGNAL(pressed()),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->buttonAddFeature->hide();
    ui->buttonRemoveFeature->hide();
    ui->listWidgetFeatures->hide();
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskMirroredParameters::setupUI()
{
    connect(ui->buttonAddFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonAddFeature(bool)));
    connect(ui->buttonRemoveFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonRemoveFeature(bool)));

    setupListWidget(ui->listWidgetFeatures);

    connect(ui->comboPlane, SIGNAL(activated(int)),
            this, SLOT(onPlaneChanged(int)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    this->planeLinks.setCombo(*(ui->comboPlane));
    ui->comboPlane->setEnabled(true);

    App::DocumentObject* sketch = getSketchObject();
    if (sketch && sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        this->fillPlanesCombo(planeLinks,static_cast<Part::Part2DObject*>(sketch));
    }
    else {
        this->fillPlanesCombo(planeLinks, NULL);
    }

    //show the parts coordinate system planes for selection
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( getObject() );
    if(body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(false, true);
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what () );
        }
    }

    updateUI();
}

void TaskMirroredParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

    if (planeLinks.setCurrentLink(pcMirrored->MirrorPlane) == -1){
        //failed to set current, because the link isn't in the list yet
        planeLinks.addLink(pcMirrored->MirrorPlane, getRefStr(pcMirrored->MirrorPlane.getValue(),pcMirrored->MirrorPlane.getSubValues()));
        planeLinks.setCurrentLink(pcMirrored->MirrorPlane);
    }

    blockUpdate = false;
}

void TaskMirroredParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode!=none && msg.Type == Gui::SelectionChanges::AddSelection) {

        if (originalSelected(msg)) {
            exitSelectionMode();
        } else {
            std::vector<std::string> mirrorPlanes;
            App::DocumentObject* selObj;
            PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
            getReferencedSelection(pcMirrored, msg, selObj, mirrorPlanes);
            if (!selObj)
                    return;
            
            if ( selectionMode == reference || selObj->isDerivedFrom ( App::Plane::getClassTypeId () ) ) {
                setupTransaction();
                pcMirrored->MirrorPlane.setValue(selObj, mirrorPlanes);
                recomputeFeature();
                updateUI();
            }
            exitSelectionMode();
        }
    }
}

void TaskMirroredParameters::clearButtons()
{
    ui->buttonAddFeature->setChecked(false);
    ui->buttonRemoveFeature->setChecked(false);
}

void TaskMirroredParameters::onPlaneChanged(int /*num*/)
{
    if (blockUpdate)
        return;
    setupTransaction();
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    try{
        if(planeLinks.getCurrentLink().getValue() == 0){
            // enter reference selection mode
            hideObject();
            showBase();
            selectionMode = reference;
            Gui::Selection().clearSelection();
            addReferenceSelectionGate(false, true);
        } else {
            exitSelectionMode();
            pcMirrored->MirrorPlane.Paste(planeLinks.getCurrentLink());
        }
    } catch (Base::Exception &e) {
        QMessageBox::warning(0,tr("Error"),QString::fromLatin1(e.what()));
    }

    recomputeFeature();
}

void TaskMirroredParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        setupTransaction();
        // Do the same like in TaskDlgMirroredParameters::accept() but without doCommand
        PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
        std::vector<std::string> mirrorPlanes;
        App::DocumentObject* obj;

        getMirrorPlane(obj, mirrorPlanes);
        pcMirrored->MirrorPlane.setValue(obj,mirrorPlanes);

        recomputeFeature();
    }
}

void TaskMirroredParameters::getMirrorPlane(App::DocumentObject*& obj, std::vector<std::string> &sub) const
{
    const App::PropertyLinkSub &lnk = planeLinks.getCurrentLink();
    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

void TaskMirroredParameters::apply()
{
}

TaskMirroredParameters::~TaskMirroredParameters()
{
    //hide the parts coordinate system axis for selection
    try {
        PartDesign::Body * body = PartDesign::Body::findBodyOf ( getObject() );
        if ( body ) {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    } catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what () );
    }

    delete ui;
    if (proxy)
        delete proxy;
}

void TaskMirroredParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgMirroredParameters::TaskDlgMirroredParameters(ViewProviderMirrored *MirroredView)
    : TaskDlgTransformedParameters(MirroredView, new TaskMirroredParameters(MirroredView))
{
}
//==== calls from the TaskView ===============================================================

bool TaskDlgMirroredParameters::accept()
{
    TaskMirroredParameters* mirrorParameter = static_cast<TaskMirroredParameters*>(parameter);
    std::vector<std::string> mirrorPlanes;
    App::DocumentObject* obj;
    mirrorParameter->getMirrorPlane(obj, mirrorPlanes);
    std::string mirrorPlane = buildLinkSingleSubPythonStr(obj, mirrorPlanes);

    FCMD_OBJ_CMD(vp->getObject(),"MirrorPlane = " << mirrorPlane);

    return TaskDlgTransformedParameters::accept();
}

#include "moc_TaskMirroredParameters.cpp"
