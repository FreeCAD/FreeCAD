/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "ui_TaskFilletParameters.h"
#include "TaskFilletParameters.h"
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
#include <Mod/PartDesign/App/FeatureFillet.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskFilletParameters */

TaskFilletParameters::TaskFilletParameters(ViewProviderDressUp *DressUpView,QWidget *parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskFilletParameters();
    ui->setupUi(proxy);

    this->groupLayout()->addWidget(proxy);

    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(DressUpView->getObject());
    double r = pcFillet->Radius.getValue();

    ui->filletRadius->setUnit(Base::Unit::Length);
    ui->filletRadius->setValue(r);
    ui->filletRadius->setMinimum(0);
    ui->filletRadius->selectNumber();
    ui->filletRadius->bind(pcFillet->Radius);
    QMetaObject::invokeMethod(ui->filletRadius, "setFocus", Qt::QueuedConnection);

    QMetaObject::connectSlotsByName(this);

    connect(ui->filletRadius, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(QString::fromLatin1("Del"));
    ui->listWidgetReferences->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onRefDeleted()));
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    setup(ui->listWidgetReferences);

}

void TaskFilletParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
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

void TaskFilletParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refAdd) ui->buttonRefAdd->setChecked(false);
    if (notThis != refRemove) ui->buttonRefRemove->setChecked(false);
    DressUpView->highlightReferences(false);
}

void TaskFilletParameters::onRefDeleted(void)
{
    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(DressUpView->getObject());
    App::DocumentObject* base = pcFillet->Base.getValue();
    std::vector<std::string> refs = pcFillet->Base.getSubValues();
    refs.erase(refs.begin() + ui->listWidgetReferences->currentRow());
    setupTransaction();
    pcFillet->Base.setValue(base, refs);
    ui->listWidgetReferences->model()->removeRow(ui->listWidgetReferences->currentRow());
    pcFillet->getDocument()->recomputeFeature(pcFillet);
}

void TaskFilletParameters::onLengthChanged(double len)
{
    clearButtons(none);
    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(DressUpView->getObject());
    setupTransaction();
    pcFillet->Radius.setValue(len);
    pcFillet->getDocument()->recomputeFeature(pcFillet);
}

double TaskFilletParameters::getLength(void) const
{
    return ui->filletRadius->value().getValue();
}

TaskFilletParameters::~TaskFilletParameters()
{
    Gui::Selection().rmvSelectionGate();
    delete ui;
}

void TaskFilletParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskFilletParameters::apply()
{
    std::string name = getDressUpView()->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Fillet changed");
    ui->filletRadius->apply();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFilletParameters::TaskDlgFilletParameters(ViewProviderFillet *DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter  = new TaskFilletParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgFilletParameters::~TaskDlgFilletParameters()
{

}

//==== calls from the TaskView ===============================================================


//void TaskDlgFilletParameters::open()
//{
//    // a transaction is already open at creation time of the fillet
//    if (!Gui::Command::hasPendingCommand()) {
//        QString msg = tr("Edit fillet");
//        Gui::Command::openCommand((const char*)msg.toUtf8());
//    }
//}
bool TaskDlgFilletParameters::accept()
{
    parameter->showObject();
    parameter->apply();

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskFilletParameters.cpp"
