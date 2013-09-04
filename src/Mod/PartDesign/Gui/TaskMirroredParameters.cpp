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
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
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

    referenceSelectionMode = false;

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
    ui->labelOriginal->hide();
    ui->lineOriginal->hide();
    ui->checkBoxUpdateView->hide();

    referenceSelectionMode = false;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskMirroredParameters::setupUI()
{
    connect(ui->comboPlane, SIGNAL(activated(int)),
            this, SLOT(onPlaneChanged(int)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    std::vector<App::DocumentObject*> originals = pcMirrored->Originals.getValues();

    // Fill data into dialog elements
    ui->lineOriginal->setEnabled(false);
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); i++)
    {
        if ((*i) != NULL) { // find the first valid original
            ui->lineOriginal->setText(QString::fromAscii((*i)->getNameInDocument()));
            break;
        }
    }
    // ---------------------

    ui->comboPlane->setEnabled(true);
    updateUI();
}

void TaskMirroredParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

    App::DocumentObject* mirrorPlaneFeature = pcMirrored->MirrorPlane.getValue();
    std::vector<std::string> mirrorPlanes = pcMirrored->MirrorPlane.getSubValues();

    App::DocumentObject* sketch = getSketchObject();
    int maxcount=2;
    if (sketch)
        maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();

    for (int i=ui->comboPlane->count()-1; i >= 2; i--)
        ui->comboPlane->removeItem(i);
    for (int i=ui->comboPlane->count(); i < maxcount; i++)
        ui->comboPlane->addItem(QString::fromAscii("Sketch axis %1").arg(i-2));

    bool undefined = false;
    if (mirrorPlaneFeature != NULL && !mirrorPlanes.empty()) {
        if (mirrorPlanes.front() == "H_Axis")
            ui->comboPlane->setCurrentIndex(0);
        else if (mirrorPlanes.front() == "V_Axis")
            ui->comboPlane->setCurrentIndex(1);
        else if (mirrorPlanes.front().size() > 4 && mirrorPlanes.front().substr(0,4) == "Axis") {
            int pos = 2 + std::atoi(mirrorPlanes.front().substr(4,4000).c_str());
            if (pos <= maxcount)
                ui->comboPlane->setCurrentIndex(pos);
            else
                undefined = true;
        } else if (mirrorPlaneFeature != NULL && !mirrorPlanes.empty()) {
            ui->comboPlane->addItem(QString::fromAscii(mirrorPlanes.front().c_str()));
            ui->comboPlane->setCurrentIndex(maxcount);
        }
    } else {
        undefined = true;
    }

    if (referenceSelectionMode) {
        ui->comboPlane->addItem(tr("Select a face"));
        ui->comboPlane->setCurrentIndex(ui->comboPlane->count() - 1);
    } else if (undefined) {
        ui->comboPlane->addItem(tr("Undefined"));
        ui->comboPlane->setCurrentIndex(ui->comboPlane->count() - 1);
    } else
        ui->comboPlane->addItem(tr("Select reference..."));

    blockUpdate = false;
}

void TaskMirroredParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        if (strcmp(msg.pDocName, getObject()->getDocument()->getName()) != 0)
            return;

        std::string subName(msg.pSubName);
        if (originalSelected(msg)) {
            ui->lineOriginal->setText(QString::fromAscii(msg.pObjectName));
        } else if (referenceSelectionMode &&
                   (subName.size() > 4 && subName.substr(0,4) == "Face")) {

            if (strcmp(msg.pObjectName, getSupportObject()->getNameInDocument()) != 0)
                return;

            exitSelectionMode();
            if (!blockUpdate) {
                PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
                std::vector<std::string> mirrorPlanes(1,subName);
                pcMirrored->MirrorPlane.setValue(getSupportObject(), mirrorPlanes);

                recomputeFeature();
                updateUI();
            }
            else {
                App::DocumentObject* sketch = getSketchObject();
                int maxcount=2;
                if (sketch)
                    maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();
                for (int i=ui->comboPlane->count()-1; i >= maxcount; i--)
                    ui->comboPlane->removeItem(i);

                ui->comboPlane->addItem(QString::fromAscii(subName.c_str()));
                ui->comboPlane->setCurrentIndex(maxcount);
                ui->comboPlane->addItem(tr("Select reference..."));
            }
        }
    }
}

void TaskMirroredParameters::onPlaneChanged(int num) {
    if (blockUpdate)
        return;
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

    App::DocumentObject* pcSketch = getSketchObject();
    int maxcount=2;
    if (pcSketch)
        maxcount += static_cast<Part::Part2DObject*>(pcSketch)->getAxisCount();

    if (num == 0) {
        pcMirrored->MirrorPlane.setValue(pcSketch, std::vector<std::string>(1,"H_Axis"));
        exitSelectionMode();
    }
    else if (num == 1) {
        pcMirrored->MirrorPlane.setValue(pcSketch, std::vector<std::string>(1,"V_Axis"));
        exitSelectionMode();
    }
    else if (num >= 2 && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-2);
        std::string str = buf.toStdString();
        pcMirrored->MirrorPlane.setValue(pcSketch, std::vector<std::string>(1,str));
    }
    else if (num == ui->comboPlane->count() - 1) {
        // enter reference selection mode
        hideObject();
        showOriginals();
        referenceSelectionMode = true;
        Gui::Selection().clearSelection();
        addReferenceSelectionGate(false, true);
    }
    else if (num == maxcount)
        exitSelectionMode();

    recomputeFeature();
}

void TaskMirroredParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgMirroredParameters::accept() but without doCommand
        PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

        std::string mirrorPlane = getMirrorPlane();
        if (!mirrorPlane.empty()) {
            std::vector<std::string> planes(1,mirrorPlane);
            if (mirrorPlane == "H_Axis" || mirrorPlane == "V_Axis" ||
                (mirrorPlane.size() > 4 && mirrorPlane.substr(0,4) == "Axis"))
                pcMirrored->MirrorPlane.setValue(getSketchObject(),planes);
            else
                pcMirrored->MirrorPlane.setValue(getSupportObject(),planes);
        } else
            pcMirrored->MirrorPlane.setValue(NULL);

        recomputeFeature();
    }
}

const std::string TaskMirroredParameters::getMirrorPlane(void) const
{
    App::DocumentObject* pcSketch = getSketchObject();
    int maxcount=2;
    if (pcSketch)
        maxcount += static_cast<Part::Part2DObject*>(pcSketch)->getAxisCount();

    int num = ui->comboPlane->currentIndex();
    if (num == 0)
        return "H_Axis";
    else if (num == 1)
        return "V_Axis";
    else if (num >= 2 && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-2);
        return buf.toStdString();
    } else if (num == maxcount &&
               ui->comboPlane->count() == maxcount + 2)
        return ui->comboPlane->currentText().toStdString();
    return std::string("");
}

TaskMirroredParameters::~TaskMirroredParameters()
{
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
        //Gui::Command::openCommand("Mirrored changed");
        // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskMirroredParameters* mirrorParameter = static_cast<TaskMirroredParameters*>(parameter);
        std::string mirrorPlane = mirrorParameter->getMirrorPlane();
        if (!mirrorPlane.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            if (mirrorPlane == "H_Axis" || mirrorPlane == "V_Axis" ||
                (mirrorPlane.size() > 4 && mirrorPlane.substr(0,4) == "Axis"))
                buf = buf.arg(QString::fromUtf8(mirrorParameter->getSketchObject()->getNameInDocument()));
            else
                buf = buf.arg(QString::fromUtf8(mirrorParameter->getSupportObject()->getNameInDocument()));
            buf = buf.arg(QString::fromUtf8(mirrorPlane.c_str()));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MirrorPlane = %s", name.c_str(), buf.toStdString().c_str());
        } else
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MirrorPlane = None", name.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!TransformedView->getObject()->isValid())
            throw Base::Exception(TransformedView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;
}

#include "moc_TaskMirroredParameters.cpp"
