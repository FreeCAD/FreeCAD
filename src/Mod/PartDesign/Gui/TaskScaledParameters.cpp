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

#include "ui_TaskScaledParameters.h"
#include "TaskScaledParameters.h"
#include "TaskMultiTransformParameters.h"
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
#include <Mod/PartDesign/App/FeatureScaled.h>
#include <Mod/Sketcher/App/SketchObject.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskScaledParameters */

TaskScaledParameters::TaskScaledParameters(ViewProviderTransformed *TransformedView,QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskScaledParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskScaledParameters::TaskScaledParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskScaledParameters();
    ui->setupUi(proxy);
    connect(ui->buttonOK, SIGNAL(pressed()),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->labelOriginal->hide();
    ui->lineOriginal->hide();
    ui->checkBoxUpdateView->hide();

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskScaledParameters::setupUI()
{
    connect(ui->spinFactor, SIGNAL(valueChanged(double)),
            this, SLOT(onFactor(double)));
    connect(ui->spinOccurrences, SIGNAL(valueChanged(int)),
            this, SLOT(onOccurrences(int)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());
    std::vector<App::DocumentObject*> originals = pcScaled->Originals.getValues();

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

    ui->spinFactor->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);
    ui->spinFactor->setDecimals(Base::UnitsApi::getDecimals());

    updateUI();
}

void TaskScaledParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());

    double factor = pcScaled->Factor.getValue();
    unsigned occurrences = pcScaled->Occurrences.getValue();

    ui->spinFactor->setValue(factor);
    ui->spinOccurrences->setValue(occurrences);

    blockUpdate = false;
}

void TaskScaledParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (originalSelected(msg)) {
        App::DocumentObject* selectedObject = TransformedView->getObject()->getDocument()->getActiveObject();
        ui->lineOriginal->setText(QString::fromAscii(selectedObject->getNameInDocument()));
    }
}

void TaskScaledParameters::onFactor(const double f) {
    if (blockUpdate)
        return;
    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());
    pcScaled->Factor.setValue(f);
    recomputeFeature();
}

void TaskScaledParameters::onOccurrences(const int n) {
    if (blockUpdate)
        return;
    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());
    pcScaled->Occurrences.setValue(n);
    recomputeFeature();
}

void TaskScaledParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgScaledParameters::accept() but without doCommand
        PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());
        pcScaled->Factor.setValue(getFactor());
        pcScaled->Occurrences.setValue(getOccurrences());
        recomputeFeature();
    }
}

const double TaskScaledParameters::getFactor(void) const
{
    return ui->spinFactor->value();
}

const unsigned TaskScaledParameters::getOccurrences(void) const
{
    return ui->spinOccurrences->value();
}


TaskScaledParameters::~TaskScaledParameters()
{
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskScaledParameters::changeEvent(QEvent *e)
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

TaskDlgScaledParameters::TaskDlgScaledParameters(ViewProviderScaled *ScaledView)
    : TaskDlgTransformedParameters(ScaledView)
{
    parameter = new TaskScaledParameters(ScaledView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgScaledParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand("Scaled changed");
        // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskScaledParameters* scaledParameter = static_cast<TaskScaledParameters*>(parameter);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Factor = %f",name.c_str(),scaledParameter->getFactor());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Occurrences = %u",name.c_str(),scaledParameter->getOccurrences());
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

#include "moc_TaskScaledParameters.cpp"
