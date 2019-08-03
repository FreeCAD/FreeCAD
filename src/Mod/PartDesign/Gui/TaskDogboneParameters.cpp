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

#include "ui_TaskDogboneParameters.h"
#include "TaskDogboneParameters.h"
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
#include <Mod/PartDesign/App/FeatureDogbone.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDogboneParameters */

TaskDogboneParameters::TaskDogboneParameters(ViewProviderDressUp *DressUpView,QWidget *parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskDogboneParameters();
    ui->setupUi(proxy);

    this->groupLayout()->addWidget(proxy);

    PartDesign::Dogbone* pcDogbone = static_cast<PartDesign::Dogbone*>(DressUpView->getObject());
    double r = pcDogbone->Radius.getValue();

    ui->dogboneRadius->setUnit(Base::Unit::Length);
    ui->dogboneRadius->setValue(r);
    ui->dogboneRadius->setMinimum(0);
    ui->dogboneRadius->selectNumber();
    ui->dogboneRadius->bind(pcDogbone->Radius);
    QMetaObject::invokeMethod(ui->dogboneRadius, "setFocus", Qt::QueuedConnection);
    std::vector<std::string> strings = pcDogbone->Base.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
    {
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
    }

    QMetaObject::connectSlotsByName(this);

    connect(ui->dogboneRadius, SIGNAL(valueChanged(double)),
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
}

void TaskDogboneParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
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

void TaskDogboneParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refAdd) ui->buttonRefAdd->setChecked(false);
    if (notThis != refRemove) ui->buttonRefRemove->setChecked(false);
    DressUpView->highlightReferences(false);
}

void TaskDogboneParameters::onRefDeleted(void)
{
    PartDesign::Dogbone* pcDogbone = static_cast<PartDesign::Dogbone*>(DressUpView->getObject());
    App::DocumentObject* base = pcDogbone->Base.getValue();
    std::vector<std::string> refs = pcDogbone->Base.getSubValues();
    refs.erase(refs.begin() + ui->listWidgetReferences->currentRow());
    pcDogbone->Base.setValue(base, refs);
    ui->listWidgetReferences->model()->removeRow(ui->listWidgetReferences->currentRow());
    pcDogbone->getDocument()->recomputeFeature(pcDogbone);
}

void TaskDogboneParameters::onLengthChanged(double len)
{
    clearButtons(none);
    PartDesign::Dogbone* pcDogbone = static_cast<PartDesign::Dogbone*>(DressUpView->getObject());
    pcDogbone->Radius.setValue(len);
    pcDogbone->getDocument()->recomputeFeature(pcDogbone);
}

double TaskDogboneParameters::getRadius(void) const
{
    return ui->dogboneRadius->value().getValue();
}

TaskDogboneParameters::~TaskDogboneParameters()
{
    Gui::Selection().rmvSelectionGate();
    delete ui;
}

void TaskDogboneParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskDogboneParameters::apply()
{
    std::string name = getDressUpView()->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Dogbone changed");
    ui->dogboneRadius->apply();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDogboneParameters::TaskDlgDogboneParameters(ViewProviderDogbone *DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter  = new TaskDogboneParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgDogboneParameters::~TaskDlgDogboneParameters()
{

}

//==== calls from the TaskView ===============================================================


//void TaskDlgDogboneParameters::open()
//{
//    // a transaction is already open at creation time of the Dogbone
//    if (!Gui::Command::hasPendingCommand()) {
//        QString msg = tr("Edit Dogbone");
//        Gui::Command::openCommand((const char*)msg.toUtf8());
//    }
//}
bool TaskDlgDogboneParameters::accept()
{
    parameter->showObject();
    parameter->apply();

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskDogboneParameters.cpp"
