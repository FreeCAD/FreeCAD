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
#endif

#include "ui_TaskMirroredParameters.h"
#include "TaskMirroredParameters.h"
#include "TaskMultiTransformParameters.h"
#include "Workbench.h"
#include "ReferenceSelection.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/Plane.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>
#include <Mod/Sketcher/App/SketchObject.h>

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
    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    ui->listWidgetFeatures->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onFeatureDeleted()));
    ui->listWidgetFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->comboPlane, SIGNAL(activated(int)),
            this, SLOT(onPlaneChanged(int)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    std::vector<App::DocumentObject*> originals = pcMirrored->Originals.getValues();

    // Fill data into dialog elements
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); ++i)
    {
        if ((*i) != NULL)
            ui->listWidgetFeatures->addItem(QString::fromLatin1((*i)->getNameInDocument()));
    }
    // ---------------------

    this->planeLinks.setCombo(*(ui->comboPlane));
    ui->comboPlane->setEnabled(true);

    
    App::DocumentObject* sketch = getSketchObject();
    if (!sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        sketch = 0;
    this->fillPlanesCombo(planeLinks,static_cast<Part::Part2DObject*>(sketch));

    updateUI();
    
    //show the parts coordinate system axis for selection
    App::Part* part = getPartFor(getObject(), false);
    if(part) {        
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(true, Gui::Application::Instance->activeDocument());
            origin->setTemporaryVisibilityAxis(true);
        }            
     }
}

void TaskMirroredParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

    if (planeLinks.setCurrentLink(pcMirrored->MirrorPlane) == -1){
        //failed to set current, because the link isnt in the list yet
        planeLinks.addLink(pcMirrored->MirrorPlane, getRefStr(pcMirrored->MirrorPlane.getValue(),pcMirrored->MirrorPlane.getSubValues()));
        planeLinks.setCurrentLink(pcMirrored->MirrorPlane);
    }

    blockUpdate = false;
}

void TaskMirroredParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        if (originalSelected(msg)) {
            if (selectionMode == addFeature)
                ui->listWidgetFeatures->addItem(QString::fromLatin1(msg.pObjectName));
            else
                removeItemFromListWidget(ui->listWidgetFeatures, msg.pObjectName);
            exitSelectionMode();
        } else if (selectionMode == reference) {
            // Note: ReferenceSelection has already checked the selection for validity
            exitSelectionMode();

            std::vector<std::string> mirrorPlanes;
            App::DocumentObject* selObj;
            PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
            getReferencedSelection(pcMirrored, msg, selObj, mirrorPlanes);
            pcMirrored->MirrorPlane.setValue(selObj, mirrorPlanes);

            recomputeFeature();
            updateUI();
        } else if( strstr(msg.pObjectName, App::Part::BaseplaneTypes[0]) == nullptr || 
                   strstr(msg.pObjectName, App::Part::BaseplaneTypes[1]) == nullptr ||
                   strstr(msg.pObjectName, App::Part::BaseplaneTypes[2]) == nullptr) {
           
            std::vector<std::string> planes;
            App::DocumentObject* selObj;
            PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
            getReferencedSelection(pcMirrored, msg, selObj, planes);
            pcMirrored->MirrorPlane.setValue(selObj, planes);

            recomputeFeature();
            updateUI();
        }
    }
}

void TaskMirroredParameters::clearButtons()
{
    ui->buttonAddFeature->setChecked(false);
    ui->buttonRemoveFeature->setChecked(false);
}

void TaskMirroredParameters::onPlaneChanged(int num) {
    if (blockUpdate)
        return;
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
        QMessageBox::warning(0,tr("Error"),QString::fromAscii(e.what()));
    }

    recomputeFeature();
}

void TaskMirroredParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgMirroredParameters::accept() but without doCommand
        PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
        std::vector<std::string> mirrorPlanes;
        App::DocumentObject* obj;

        getMirrorPlane(obj, mirrorPlanes);
        pcMirrored->MirrorPlane.setValue(obj,mirrorPlanes);

        recomputeFeature();
    }
}

void TaskMirroredParameters::onFeatureDeleted(void)
{
    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    originals.erase(originals.begin() + ui->listWidgetFeatures->currentRow());
    pcTransformed->Originals.setValues(originals);
    ui->listWidgetFeatures->model()->removeRow(ui->listWidgetFeatures->currentRow());
    recomputeFeature();
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
    App::Part* part = getPartFor(getObject(), false);
    if(part) {
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(false);
        }            
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
    : TaskDlgTransformedParameters(MirroredView)
{
    parameter = new TaskMirroredParameters(MirroredView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgMirroredParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
            // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskMirroredParameters* mirrorParameter = static_cast<TaskMirroredParameters*>(parameter);
        std::vector<std::string> mirrorPlanes;
        App::DocumentObject* obj;
        mirrorParameter->getMirrorPlane(obj, mirrorPlanes);
        std::string mirrorPlane = getPythonStr(obj, mirrorPlanes);
        if (!mirrorPlane.empty() && obj) {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MirrorPlane = %s", name.c_str(), mirrorPlane.c_str());
        } else {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MirrorPlane = None", name.c_str());
        }
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!TransformedView->getObject()->isValid())
            throw Base::Exception(TransformedView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

#include "moc_TaskMirroredParameters.cpp"
