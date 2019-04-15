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
# include <QAction>
#endif

#include "ui_TaskThicknessParameters.h"
#include "TaskThicknessParameters.h"
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
#include <Mod/PartDesign/App/FeatureThickness.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskThicknessParameters */

TaskThicknessParameters::TaskThicknessParameters(ViewProviderDressUp *DressUpView,QWidget *parent)
    : TaskDressUpParameters(DressUpView, false, true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskThicknessParameters();
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    double a = pcThickness->Value.getValue();

    ui->Value->setMinimum(0.0);
    ui->Value->setMaximum(89.99);
    ui->Value->setValue(a);
    ui->Value->selectAll();
    QMetaObject::invokeMethod(ui->Value, "setFocus", Qt::QueuedConnection);

    // Bind input fields to properties
    ui->Value->bind(pcThickness->Value);

    bool r = pcThickness->Reversed.getValue();
    ui->checkReverse->setChecked(r);

    std::vector<std::string> strings = pcThickness->Base.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
    {
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
    }

    QMetaObject::connectSlotsByName(this);

    connect(ui->Value, SIGNAL(valueChanged(double)),
            this, SLOT(onValueChanged(double)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onReversedChanged(bool)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->modeComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->joinComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onJoinTypeChanged(int)));

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(QString::fromLatin1("Del"));
    ui->listWidgetReferences->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onRefDeleted()));
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    int mode = pcThickness->Mode.getValue();
    ui->modeComboBox->setCurrentIndex(mode);

    int join = pcThickness->Join.getValue();
    ui->joinComboBox->setCurrentIndex(join);
}

void TaskThicknessParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
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
        } 
    }
}

void TaskThicknessParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refAdd) ui->buttonRefAdd->setChecked(false);
    if (notThis != refRemove) ui->buttonRefRemove->setChecked(false);
    DressUpView->highlightReferences(false);
}

void TaskThicknessParameters::onRefDeleted(void)
{
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    App::DocumentObject* base = pcThickness->Base.getValue();
    std::vector<std::string> faces = pcThickness->Base.getSubValues();
    faces.erase(faces.begin() + ui->listWidgetReferences->currentRow());
    pcThickness->Base.setValue(base, faces);
    ui->listWidgetReferences->model()->removeRow(ui->listWidgetReferences->currentRow());
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    clearButtons(none);
    exitSelectionMode();
}

void TaskThicknessParameters::onValueChanged(double angle)
{
    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    pcThickness->Value.setValue(angle);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
}

void TaskThicknessParameters::onJoinTypeChanged(int join) {

    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    pcThickness->Join.setValue(join);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
}

void TaskThicknessParameters::onModeChanged(int mode) {

    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    pcThickness->Mode.setValue(mode);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
}


double TaskThicknessParameters::getValue(void) const
{
    return ui->Value->value().getValue();
}

void TaskThicknessParameters::onReversedChanged(const bool on) {
    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    pcThickness->Reversed.setValue(on);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
}

bool TaskThicknessParameters::getReversed(void) const
{
    return ui->checkReverse->isChecked();
}

int TaskThicknessParameters::getJoinType(void) const {
    
    return ui->joinComboBox->currentIndex();
}

int TaskThicknessParameters::getMode(void) const {

    return ui->modeComboBox->currentIndex();
}


TaskThicknessParameters::~TaskThicknessParameters()
{
    Gui::Selection().rmvSelectionGate();
    delete ui;
}

void TaskThicknessParameters::changeEvent(QEvent *e)
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

TaskDlgThicknessParameters::TaskDlgThicknessParameters(ViewProviderThickness *DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter  = new TaskThicknessParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgThicknessParameters::~TaskDlgThicknessParameters()
{

}

//==== calls from the TaskView ===============================================================


//void TaskDlgThicknessParameters::open()
//{
//    // a transaction is already open at creation time of the draft
//    if (!Gui::Command::hasPendingCommand()) {
//        QString msg = QObject::tr("Edit draft");
//        Gui::Command::openCommand((const char*)msg.toUtf8());
//    }
//}
//
//void TaskDlgThicknessParameters::clicked(int)
//{
//
//}

bool TaskDlgThicknessParameters::accept()
{
    parameter->showObject();

    TaskThicknessParameters* draftparameter = static_cast<TaskThicknessParameters*>(parameter);

    std::string name = vp->getObject()->getNameInDocument();

    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Value = %f",name.c_str(),draftparameter->getValue());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),draftparameter->getReversed());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Mode = %u",name.c_str(),draftparameter->getMode());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Join = %u",name.c_str(),draftparameter->getJoinType());

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskThicknessParameters.cpp"
