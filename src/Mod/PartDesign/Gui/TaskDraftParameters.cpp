/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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
#endif

#include "ui_TaskDraftParameters.h"
#include "TaskDraftParameters.h"
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
#include <Gui/MainWindow.h>
#include <Mod/PartDesign/App/FeatureDraft.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDraftParameters */

TaskDraftParameters::TaskDraftParameters(ViewProviderDraft *DraftView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Draft"),tr("Draft parameters"),true, parent),DraftView(DraftView)
{
    selectionMode = none;

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskDraftParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->doubleSpinBox, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onReversedChanged(bool)));
    connect(ui->buttonFaceAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonFaceAdd(bool)));
    connect(ui->buttonFaceRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonFaceRemove(bool)));
    connect(ui->buttonPlane, SIGNAL(toggled(bool)),
            this, SLOT(onButtonPlane(bool)));
    connect(ui->buttonLine, SIGNAL(toggled(bool)),
            this, SLOT(onButtonLine(bool)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DraftView->getObject());
    double a = pcDraft->Angle.getValue();

    ui->doubleSpinBox->setMinimum(0.0);
    ui->doubleSpinBox->setMaximum(89.99);
    ui->doubleSpinBox->setValue(a);
    ui->doubleSpinBox->selectAll();
    QMetaObject::invokeMethod(ui->doubleSpinBox, "setFocus", Qt::QueuedConnection);

    bool r = pcDraft->Reversed.getValue();
    ui->checkReverse->setChecked(r);

    std::vector<std::string> strings = pcDraft->Base.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
    {
        ui->listWidgetFaces->insertItem(0, QString::fromStdString(*i));
    }
    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    ui->listWidgetFaces->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onFaceDeleted()));
    ui->listWidgetFaces->setContextMenuPolicy(Qt::ActionsContextMenu);

    strings = pcDraft->NeutralPlane.getSubValues();
    std::string neutralPlane = (strings.empty() ? "" : strings[0]);
    ui->linePlane->setText(QString::fromStdString(neutralPlane));

    strings = pcDraft->PullDirection.getSubValues();
    std::string pullDirection = (strings.empty() ? "" : strings[0]);
    ui->lineLine->setText(QString::fromStdString(pullDirection));
}

void TaskDraftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        if (strcmp(msg.pDocName, DraftView->getObject()->getDocument()->getName()) != 0)
            return;

        PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DraftView->getObject());
        App::DocumentObject* base = this->getBase();
        // TODO: Must we make a copy here instead of assigning to const char* ?
        const char* fname = base->getNameInDocument();
        std::string subName(msg.pSubName);

        if ((selectionMode == faceAdd) && (subName.size() > 4 && subName.substr(0,4) == "Face")) {

            if (strcmp(msg.pObjectName, fname) != 0)
                return;

            std::vector<std::string> faces = pcDraft->Base.getSubValues();
            if (std::find(faces.begin(), faces.end(), subName) == faces.end()) {
                faces.push_back(subName);
                pcDraft->Base.setValue(base, faces);
                ui->listWidgetFaces->insertItem(0, QString::fromStdString(subName));

                pcDraft->getDocument()->recomputeFeature(pcDraft);
                ui->buttonFaceAdd->setChecked(false);
                exitSelectionMode();
            }
        } else if ((selectionMode == faceRemove) && (subName.size() > 4 && subName.substr(0,4) == "Face")) {

            if (strcmp(msg.pObjectName, fname) != 0)
                return;

            std::vector<std::string> faces = pcDraft->Base.getSubValues();
            std::vector<std::string>::iterator f = std::find(faces.begin(), faces.end(), subName);
            if (f != faces.end()) {
                faces.erase(f);
                pcDraft->Base.setValue(base, faces);
                QList<QListWidgetItem*> items = ui->listWidgetFaces->findItems(QString::fromStdString(subName), Qt::MatchExactly);
                if (!items.empty()) {
                    for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
                        QListWidgetItem* it = ui->listWidgetFaces->takeItem(ui->listWidgetFaces->row(*i));
                        delete it;
                    }
                }
                pcDraft->getDocument()->recomputeFeature(pcDraft);
                ui->buttonFaceRemove->setChecked(false);
                exitSelectionMode();
            }
        } else if ((selectionMode == plane) && (subName.size() > 4) &&
                   ((subName.substr(0,4) == "Face") || (subName.substr(0,4) == "Edge"))) {

            if (strcmp(msg.pObjectName, fname) != 0)
                return;

            std::vector<std::string> planes(1,subName);
            pcDraft->NeutralPlane.setValue(base, planes);
            ui->linePlane->setText(QString::fromStdString(subName));

            pcDraft->getDocument()->recomputeFeature(pcDraft);
            ui->buttonPlane->setChecked(false);
            exitSelectionMode();
        } else if ((selectionMode == line) && (subName.size() > 4 && subName.substr(0,4) == "Edge")) {

            if (strcmp(msg.pObjectName, fname) != 0)
                return;

            std::vector<std::string> edges(1,subName);
            pcDraft->PullDirection.setValue(base, edges);
            ui->lineLine->setText(QString::fromStdString(subName));

            pcDraft->getDocument()->recomputeFeature(pcDraft);
            ui->buttonLine->setChecked(false);
            exitSelectionMode();
        }
    }
}

void TaskDraftParameters::onButtonFaceAdd(bool checked)
{
    if (checked) {
        hideObject();
        selectionMode = faceAdd;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), false, true, false));
    } else {
        exitSelectionMode();
    }
}

void TaskDraftParameters::onButtonFaceRemove(bool checked)
{
    if (checked) {
        hideObject();
        selectionMode = faceRemove;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), false, true, false));
    } else {
        exitSelectionMode();
    }
}

void TaskDraftParameters::onButtonPlane(bool checked)
{
    if (checked) {
        hideObject();
        selectionMode = plane;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), true, true, true));
    } else {
        exitSelectionMode();
    }
}

void TaskDraftParameters::onButtonLine(bool checked)
{
    if (checked) {
        hideObject();
        selectionMode = line;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), true, false, true));
    } else {
        exitSelectionMode();
    }
}

const std::vector<std::string> TaskDraftParameters::getFaces(void) const
{
    std::vector<std::string> result;
    for (int i = 0; i < ui->listWidgetFaces->count(); i++)
        result.push_back(ui->listWidgetFaces->item(i)->text().toStdString());
    return result;
}

void TaskDraftParameters::onFaceDeleted(void)
{
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DraftView->getObject());
    App::DocumentObject* base = pcDraft->Base.getValue();
    std::vector<std::string> faces = pcDraft->Base.getSubValues();
    faces.erase(faces.begin() + ui->listWidgetFaces->currentRow());
    pcDraft->Base.setValue(base, faces);
    ui->listWidgetFaces->model()->removeRow(ui->listWidgetFaces->currentRow());
    pcDraft->getDocument()->recomputeFeature(pcDraft);
}

const std::string TaskDraftParameters::getPlane(void) const
{
    return ui->linePlane->text().toStdString();
}

const std::string TaskDraftParameters::getLine(void) const
{
    return ui->lineLine->text().toStdString();
}

void TaskDraftParameters::hideObject()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    App::DocumentObject* base = getBase();
    if (doc != NULL && base != NULL) {
        doc->setHide(DraftView->getObject()->getNameInDocument());
        doc->setShow(base->getNameInDocument());
    }
}

void TaskDraftParameters::showObject()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    App::DocumentObject* base = getBase();
    if (doc != NULL && base != NULL) {
        doc->setShow(DraftView->getObject()->getNameInDocument());
        doc->setHide(base->getNameInDocument());
    }
}

void TaskDraftParameters::onAngleChanged(double angle)
{
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DraftView->getObject());
    pcDraft->Angle.setValue(angle);
    pcDraft->getDocument()->recomputeFeature(pcDraft);
}

const double TaskDraftParameters::getAngle(void) const
{
    return ui->doubleSpinBox->value();
}

void TaskDraftParameters::onReversedChanged(const bool on) {
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DraftView->getObject());
    pcDraft->Reversed.setValue(on);
    pcDraft->getDocument()->recomputeFeature(pcDraft);
}

const bool TaskDraftParameters::getReversed(void) const
{
    return ui->checkReverse->isChecked();
}

App::DocumentObject* TaskDraftParameters::getBase(void) const
{
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DraftView->getObject());
    return pcDraft->Base.getValue();
}

TaskDraftParameters::~TaskDraftParameters()
{
    Gui::Selection().rmvSelectionGate();
    delete ui;
}

void TaskDraftParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskDraftParameters::exitSelectionMode()
{
    selectionMode = none;
    Gui::Selection().rmvSelectionGate();
    showObject();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDraftParameters::TaskDlgDraftParameters(ViewProviderDraft *DraftView)
    : TaskDialog(),DraftView(DraftView)
{
    assert(DraftView);
    parameter  = new TaskDraftParameters(DraftView);

    Content.push_back(parameter);
}

TaskDlgDraftParameters::~TaskDlgDraftParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgDraftParameters::open()
{

}

void TaskDlgDraftParameters::clicked(int)
{

}

bool TaskDlgDraftParameters::accept()
{
    parameter->showObject();

    // Force the user to select a neutral plane
    if (parameter->getPlane().empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Missing neutral plane"),
            QObject::tr("Please select a plane or an edge plus a pull direction"));
        return false;
    }

    std::string name = DraftView->getObject()->getNameInDocument();

    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Angle = %f",name.c_str(),parameter->getAngle());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),parameter->getReversed());
    try {
        std::vector<std::string> faces = parameter->getFaces();
        std::stringstream str;
        str << "App.ActiveDocument." << name.c_str() << ".Base = (App.ActiveDocument."
            << parameter->getBase()->getNameInDocument() << ",[";
        for (std::vector<std::string>::const_iterator it = faces.begin(); it != faces.end(); ++it)
            str << "\"" << *it << "\",";
        str << "])";
        Gui::Command::doCommand(Gui::Command::Doc,str.str().c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }
    std::string neutralPlane = parameter->getPlane();
    if (!neutralPlane.empty()) {
        QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
        buf = buf.arg(QString::fromUtf8(parameter->getBase()->getNameInDocument()));
        buf = buf.arg(QString::fromUtf8(neutralPlane.c_str()));
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.NeutralPlane = %s", name.c_str(), buf.toStdString().c_str());
    } else
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.NeutralPlane = None", name.c_str());
    std::string pullDirection = parameter->getLine();
    if (!pullDirection.empty()) {
        QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
        buf = buf.arg(QString::fromUtf8(parameter->getBase()->getNameInDocument()));
        buf = buf.arg(QString::fromUtf8(pullDirection.c_str()));
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.PullDirection = %s", name.c_str(), buf.toStdString().c_str());
    } else
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.PullDirection = None", name.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();

    return true;
}

bool TaskDlgDraftParameters::reject()
{
    // get the support
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DraftView->getObject());
    App::DocumentObject    *pcSupport;
    pcSupport = pcDraft->Base.getValue();

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcDraft)) {
        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
            Gui::Application::Instance->getViewProvider(pcSupport)->show();
    }

    return true;
}



#include "moc_TaskDraftParameters.cpp"
