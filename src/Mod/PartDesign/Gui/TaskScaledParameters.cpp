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
#include "TaskMultiTransformParameters.h"

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

    updateUIinProgress = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskScaledParameters::TaskScaledParameters(QWidget *parent, TaskMultiTransformParameters *parentTask)
        : TaskTransformedParameters(parent, parentTask)
{
    ui = new Ui_TaskScaledParameters();
    ui->setupUi(parent);
    connect(ui->buttonOK, SIGNAL(pressed()),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    ui->buttonOK->setEnabled(true);
    ui->listFeatures->hide();
    ui->checkBoxUpdateView->hide();

    updateUIinProgress = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
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

    // TODO: The following code could be generic in TaskTransformedParameters
    // if it were possible to make ui_TaskScaledParameters a subclass of
    // ui_TaskTransformedParameters
    // ---------------------
    // Add a context menu to the listview of the originals to delete items
    QAction* action = new QAction(tr("Delete"), ui->listFeatures);
    action->connect(action, SIGNAL(triggered()),
                    this, SLOT(onOriginalDeleted()));
    ui->listFeatures->addAction(action);
    ui->listFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);

    // Get the feature data
    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());
    std::vector<App::DocumentObject*> originals = pcScaled->Originals.getValues();

    // Fill data into dialog elements
    ui->listFeatures->setEnabled(true);
    ui->listFeatures->clear();
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); i++)
    {
        if ((*i) != NULL)
            ui->listFeatures->addItem(QString::fromAscii((*i)->getNameInDocument()));
    }
    QMetaObject::invokeMethod(ui->listFeatures, "setFocus", Qt::QueuedConnection);
    // ---------------------

    ui->spinFactor->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);

    updateUI();
}

void TaskScaledParameters::updateUI()
{
    if (updateUIinProgress) return;
    updateUIinProgress = true;

    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());

    double factor = pcScaled->Factor.getValue();
    unsigned occurrences = pcScaled->Occurrences.getValue();

    ui->spinFactor->setValue(factor);
    ui->spinOccurrences->setValue(occurrences);

    updateUIinProgress = false;
}

void TaskScaledParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());
    App::DocumentObject* selectedObject = pcScaled->getDocument()->getActiveObject();
    if ((selectedObject == NULL) || !selectedObject->isDerivedFrom(Part::Feature::getClassTypeId()))
        return;

    if (featureSelectionMode) {
        if (originalSelected(msg))
            ui->listFeatures->addItem(QString::fromAscii(selectedObject->getNameInDocument()));
    } else {
        return;
    }
}

void TaskScaledParameters::onOriginalDeleted()
{
    int row = ui->listFeatures->currentIndex().row();
    TaskTransformedParameters::onOriginalDeleted(row);
    ui->listFeatures->model()->removeRow(row);
}

void TaskScaledParameters::onFactor(const double f) {
    if (updateUIinProgress) return;
    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());
    pcScaled->Factor.setValue(f);
    updateUI();
    if (insideMultiTransform && !parentTask->updateView())
        return;
    recomputeFeature();
}

void TaskScaledParameters::onOccurrences(const int n) {
    if (updateUIinProgress) return;
    PartDesign::Scaled* pcScaled = static_cast<PartDesign::Scaled*>(getObject());
    pcScaled->Occurrences.setValue(n);
    updateUI();
    if (insideMultiTransform && !parentTask->updateView())
        return;
    recomputeFeature();
}

void TaskScaledParameters::onUpdateView(bool on)
{
    ui->listFeatures->blockSignals(!on);
    ui->spinFactor->blockSignals(!on);
    ui->spinOccurrences->blockSignals(!on);
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
