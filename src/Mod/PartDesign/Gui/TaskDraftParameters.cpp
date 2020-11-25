/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <QAction>
#endif

#include "ui_TaskDraftParameters.h"
#include "TaskDraftParameters.h"
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Base/Tools.h>
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

    this->groupLayout()->addWidget(proxy);

    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    double a = pcDraft->Angle.getValue();

    ui->draftAngle->setMinimum(0.0);
    ui->draftAngle->setMaximum(89.99);
    ui->draftAngle->setValue(a);
    ui->draftAngle->selectAll();
    QMetaObject::invokeMethod(ui->draftAngle, "setFocus", Qt::QueuedConnection);

    // Bind input fields to properties
    ui->draftAngle->bind(pcDraft->Angle);

    bool r = pcDraft->Reversed.getValue();
    ui->checkReverse->setChecked(r);

    QMetaObject::connectSlotsByName(this);

    connect(ui->draftAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onReversedChanged(bool)));
    connect(ui->buttonPlane, SIGNAL(toggled(bool)),
            this, SLOT(onButtonPlane(bool)));
    connect(ui->btnClearPlane, SIGNAL(clicked()), this, SLOT(onClearPlane()));
    connect(ui->buttonLine, SIGNAL(toggled(bool)),
            this, SLOT(onButtonLine(bool)));
    connect(ui->btnClearLine, SIGNAL(clicked()), this, SLOT(onClearLine()));

    setupTransaction();
    bool touched = populateRefElement(&pcDraft->NeutralPlane, ui->linePlane, true);
    touched |= populateRefElement(&pcDraft->PullDirection, ui->lineLine, true);

    ui->linePlane->installEventFilter(this);
    ui->lineLine->installEventFilter(this);

    setup(ui->message, ui->listWidgetReferences, ui->buttonRefAdd, touched);
}

void TaskDraftParameters::refresh() {
    if(!DressUpView)
        return;

    TaskDressUpParameters::refresh();

    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    double a = pcDraft->Angle.getValue();
    {
        QSignalBlocker blocker(ui->draftAngle);
        ui->draftAngle->setValue(a);
    }
    bool r = pcDraft->Reversed.getValue();
    {
        QSignalBlocker blocker(ui->checkReverse);
        ui->checkReverse->setChecked(r);
    }

    populateRefElement(&pcDraft->NeutralPlane, ui->linePlane, false);
    populateRefElement(&pcDraft->PullDirection, ui->lineLine, false);
}

void TaskDraftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if(!DressUpView)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionMode == plane || selectionMode == line) {
            PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
            std::vector<std::string> elements;
            App::DocumentObject* selObj;
            getReferencedSelection(pcDraft, msg, selObj, elements);
            if(!selObj)
                return;
            setupTransaction();
            if(selectionMode == plane) {
                pcDraft->NeutralPlane.setValue(selObj, elements);
                ui->linePlane->setText(getRefStr(selObj, elements));
            } else {
                pcDraft->PullDirection.setValue(selObj, elements);
                ui->lineLine->setText(getRefStr(selObj, elements));
            }
            recompute();
            clearButtons(none);
            return;
        }
    }

    TaskDressUpParameters::onSelectionChanged(msg);
}

void TaskDraftParameters::onClear(selectionModes mode)
{
    if(!DressUpView)
        return;

    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    setupTransaction();
    if(mode == plane) {
        ui->linePlane->setText(QString());
        if(!pcDraft->NeutralPlane.getValue())
            return;
        pcDraft->NeutralPlane.setValue(0);
    } else {
        ui->lineLine->setText(QString());
        if(!pcDraft->PullDirection.getValue())
            return;
        pcDraft->PullDirection.setValue(0);
    }
    recompute();
}

void TaskDraftParameters::onClearPlane()
{
    onClear(plane);
}

void TaskDraftParameters::onClearLine()
{
    onClear(line);
}

void TaskDraftParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != line) {
        QSignalBlocker blocker(ui->buttonLine);
        ui->buttonLine->setChecked(false);
    }
    if (notThis != plane) {
        QSignalBlocker blocker(ui->buttonPlane);
        ui->buttonPlane->setChecked(false);
    }
    TaskDressUpParameters::clearButtons(notThis);
}

void TaskDraftParameters::onButton(selectionModes mode, bool checked)
{
    if(!DressUpView)
        return;

    if(!checked) {
        clearButtons(none);
        return;
    }

    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    auto &prop = mode==plane ? pcDraft->NeutralPlane : pcDraft->PullDirection;
    clearButtons(mode);
    selectionMode = mode;
    Gui::Selection().clearSelection();

    std::unique_ptr<Gui::SelectionFilterGate> gateRefPtr(
            new ReferenceSelection(getBase(), true, mode==plane, true));
    std::unique_ptr<Gui::SelectionFilterGate> gateDepPtr(new NoDependentsSelection(pcDraft));
    Gui::Selection().addSelectionGate(new CombineSelectionFilterGates(gateRefPtr, gateDepPtr));

    std::vector<App::SubObjectT> objs;
    auto base = prop.getValue();
    auto subs = prop.getSubValues();
    if(base && subs.size()) {
        objs.push_back(getInEdit(base,subs.front().c_str()));
        if(base != getBase())
            objs.push_back(getInEdit());
    }
    showOnTop(true,std::move(objs));
}

void TaskDraftParameters::onButtonPlane(bool checked)
{
    onButton(plane,checked);
}

void TaskDraftParameters::onButtonLine(bool checked)
{
    onButton(line,checked);
}

void TaskDraftParameters::getPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if(!DressUpView)
        return;

    sub = std::vector<std::string>(1,"");
    QStringList parts = ui->linePlane->text().split(QChar::fromLatin1(':'));
    obj = DressUpView->getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
    if (parts.size() > 1)
        sub[0] = parts[1].toStdString();
}

void TaskDraftParameters::getLine(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if(!DressUpView)
        return;

    sub = std::vector<std::string>(1,"");
    QStringList parts = ui->lineLine->text().split(QChar::fromLatin1(':'));
    obj = DressUpView->getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
    if (parts.size() > 1)
        sub[0] = parts[1].toStdString();
}

void TaskDraftParameters::onAngleChanged(double angle)
{
    if(!DressUpView)
        return;

    clearButtons(none);
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    setupTransaction();
    pcDraft->Angle.setValue(angle);
    recompute();
}

double TaskDraftParameters::getAngle(void) const
{
    return ui->draftAngle->value().getValue();
}

void TaskDraftParameters::onReversedChanged(const bool on) {
    if(!DressUpView)
        return;

    clearButtons(none);
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    setupTransaction();
    pcDraft->Reversed.setValue(on);
    recompute();
}

bool TaskDraftParameters::getReversed(void) const
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

bool TaskDraftParameters::eventFilter(QObject *o, QEvent *e)
{
    if(o == ui->linePlane || o == ui->lineLine) {
        if(e->type() == QEvent::Leave) {
            enteredObject = nullptr;
            timer->stop();
            Gui::Selection().rmvPreselect();
        } else if (e->type() == QEvent::Enter) {
            enteredObject = o;
            timer->start(150);
        }
    }
    return TaskDressUpParameters::eventFilter(o,e);
}

void TaskDraftParameters::onTimer()
{
    if(!enteredObject || !DressUpView)
        return;

    App::DocumentObject *base = nullptr;
    std::vector<std::string> subs;
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    if(enteredObject == ui->linePlane) {
        base = pcDraft->NeutralPlane.getValue();
        subs = pcDraft->NeutralPlane.getSubValues();
    } else if (enteredObject == ui->lineLine) {
        base = pcDraft->PullDirection.getValue();
        subs = pcDraft->PullDirection.getSubValues();
    } else {
        TaskDressUpParameters::onTimer();
        return;
    }
    if(base && subs.size()) {
        std::string subname;
        auto obj = getInEdit(subname, base);
        if(obj) {
            subname += subs.front();
            Gui::Selection().setPreselect(obj->getDocument()->getName(),
                    obj->getNameInDocument(), subname.c_str(),0,0,0,2);
        }
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
    std::vector<std::string> strings;
    App::DocumentObject* obj;
    TaskDraftParameters* draftparameter = static_cast<TaskDraftParameters*>(parameter);

    draftparameter->getPlane(obj, strings);
    std::string neutralPlane = buildLinkSingleSubPythonStr(obj, strings);

    draftparameter->getLine(obj, strings);
    std::string pullDirection = buildLinkSingleSubPythonStr(obj, strings);

    // Force the user to select a neutral plane
    // if (neutralPlane.empty() || neutralPlane == "None") {
    //     QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Missing neutral plane"),
    //         QObject::tr("Please select a plane or an edge plus a pull direction"));
    //     return false;
    // }

    auto tobj = vp->getObject();
    FCMD_OBJ_CMD(tobj,"Angle = " << draftparameter->getAngle());
    FCMD_OBJ_CMD(tobj,"Reversed = " << draftparameter->getReversed());
    if(neutralPlane.empty())
        neutralPlane = "None";
    FCMD_OBJ_CMD(tobj,"NeutralPlane = " << neutralPlane);
    if(pullDirection.empty())
        pullDirection = "None";
    FCMD_OBJ_CMD(tobj,"PullDirection = " << pullDirection);

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskDraftParameters.cpp"
