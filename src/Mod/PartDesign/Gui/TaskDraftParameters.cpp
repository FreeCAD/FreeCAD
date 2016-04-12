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
# include <QMessageBox>
#endif

#include "ui_TaskDraftParameters.h"
#include "TaskDraftParameters.h"
#include "Workbench.h"
#include <Base/UnitsApi.h>
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
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDraftParameters */

TaskDraftParameters::TaskDraftParameters(ViewProviderDressUp *DressUpView,QWidget *parent)
    : TaskDressUpParameters(DressUpView, false, true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskDraftParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->draftAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onReversedChanged(bool)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->buttonPlane, SIGNAL(toggled(bool)),
            this, SLOT(onButtonPlane(bool)));
    connect(ui->buttonLine, SIGNAL(toggled(bool)),
            this, SLOT(onButtonLine(bool)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    double a = pcDraft->Angle.getValue();

    ui->draftAngle->setMinimum(0.0);
    ui->draftAngle->setMaximum(89.99);
    ui->draftAngle->setValue(a);
    ui->draftAngle->selectAll();
    QMetaObject::invokeMethod(ui->draftAngle, "setFocus", Qt::QueuedConnection);

    bool r = pcDraft->Reversed.getValue();
    ui->checkReverse->setChecked(r);

    std::vector<std::string> strings = pcDraft->Base.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); ++i)
    {
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
    }
    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    ui->listWidgetReferences->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onRefDeleted()));
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    App::DocumentObject* ref = pcDraft->NeutralPlane.getValue();
    strings = pcDraft->NeutralPlane.getSubValues();
    ui->linePlane->setText(getRefStr(ref, strings));

    ref = pcDraft->PullDirection.getValue();
    strings = pcDraft->PullDirection.getSubValues();
    ui->lineLine->setText(getRefStr(ref, strings));
}

void TaskDraftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd)
                ui->listWidgetReferences->addItem(QString::fromStdString(msg.pSubName));
            else
                removeItemFromListWidget(ui->listWidgetReferences, msg.pSubName);
            clearButtons(none);
            exitSelectionMode();
        } else if (selectionMode == plane) {
            PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
            std::vector<std::string> planes;
            App::DocumentObject* selObj;
            getReferencedSelection(pcDraft, msg, selObj, planes);
            pcDraft->NeutralPlane.setValue(selObj, planes);
            ui->linePlane->setText(getRefStr(selObj, planes));

            pcDraft->getDocument()->recomputeFeature(pcDraft);
            clearButtons(none);
            exitSelectionMode();
        } else if (selectionMode == line) {
            PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
            std::vector<std::string> edges;
            App::DocumentObject* selObj;
            getReferencedSelection(pcDraft, msg, selObj, edges);
            pcDraft->PullDirection.setValue(selObj, edges);
            ui->lineLine->setText(getRefStr(selObj, edges));

            pcDraft->getDocument()->recomputeFeature(pcDraft);
            clearButtons(none);
            exitSelectionMode();
        }
    }
}

void TaskDraftParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refAdd) ui->buttonRefAdd->setChecked(false);
    if (notThis != refRemove) ui->buttonRefRemove->setChecked(false);
    if (notThis != line) ui->buttonLine->setChecked(false);
    if (notThis != plane) ui->buttonPlane->setChecked(false);
    DressUpView->highlightReferences(false);
}

void TaskDraftParameters::onButtonPlane(bool checked)
{
    if (checked) {
        clearButtons(plane);
        hideObject();
        selectionMode = plane;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), true, true, true));
    }
}

void TaskDraftParameters::onButtonLine(bool checked)
{
    if (checked) {
        clearButtons(line);
        hideObject();
        selectionMode = line;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), true, false, true));
    }
}

void TaskDraftParameters::onRefDeleted(void)
{
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    App::DocumentObject* base = pcDraft->Base.getValue();
    std::vector<std::string> faces = pcDraft->Base.getSubValues();
    faces.erase(faces.begin() + ui->listWidgetReferences->currentRow());
    pcDraft->Base.setValue(base, faces);
    ui->listWidgetReferences->model()->removeRow(ui->listWidgetReferences->currentRow());
    pcDraft->getDocument()->recomputeFeature(pcDraft);
}

void TaskDraftParameters::getPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    sub = std::vector<std::string>(1,"");
    QStringList parts = ui->linePlane->text().split(QChar::fromAscii(':'));
    obj = DressUpView->getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
    if (parts.size() > 1)
        sub[0] = parts[1].toStdString();
}

void TaskDraftParameters::getLine(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    sub = std::vector<std::string>(1,"");
    QStringList parts = ui->lineLine->text().split(QChar::fromAscii(':'));
    obj = DressUpView->getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
    if (parts.size() > 1)
        sub[0] = parts[1].toStdString();
}

void TaskDraftParameters::onAngleChanged(double angle)
{
    clearButtons(none);
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    pcDraft->Angle.setValue(angle);
    pcDraft->getDocument()->recomputeFeature(pcDraft);
}

const double TaskDraftParameters::getAngle(void) const
{
    return ui->draftAngle->value().getValue();
}

void TaskDraftParameters::onReversedChanged(const bool on) {
    clearButtons(none);
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    pcDraft->Reversed.setValue(on);
    pcDraft->getDocument()->recomputeFeature(pcDraft);
}

const bool TaskDraftParameters::getReversed(void) const
{
    return ui->checkReverse->isChecked();
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


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDraftParameters::TaskDlgDraftParameters(ViewProviderDraft *DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter  = new TaskDraftParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgDraftParameters::~TaskDlgDraftParameters()
{

}

//==== calls from the TaskView ===============================================================


//void TaskDlgDraftParameters::open()
//{
//    // a transaction is already open at creation time of the draft
//    if (!Gui::Command::hasPendingCommand()) {
//        QString msg = QObject::tr("Edit draft");
//        Gui::Command::openCommand((const char*)msg.toUtf8());
//    }
//}

bool TaskDlgDraftParameters::accept()
{
    parameter->showObject();

    // Force the user to select a neutral plane
    std::vector<std::string> strings;
    App::DocumentObject* obj;
    TaskDraftParameters* draftparameter = static_cast<TaskDraftParameters*>(parameter);
    draftparameter->getPlane(obj, strings);
    std::string neutralPlane = getPythonStr(obj, strings);
    if (neutralPlane.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Missing neutral plane"),
            QObject::tr("Please select a plane or an edge plus a pull direction"));
        return false;
    }

    std::string name = vp->getObject()->getNameInDocument();

    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Angle = %f",name.c_str(),draftparameter->getAngle());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),draftparameter->getReversed());
    if (!neutralPlane.empty()) {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.NeutralPlane = %s", name.c_str(), neutralPlane.c_str());
    } else
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.NeutralPlane = None", name.c_str());
    draftparameter->getLine(obj, strings);
    std::string pullDirection = getPythonStr(obj, strings);
    if (!pullDirection.empty()) {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.PullDirection = %s", name.c_str(), pullDirection.c_str());
    } else
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.PullDirection = None", name.c_str());

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskDraftParameters.cpp"
