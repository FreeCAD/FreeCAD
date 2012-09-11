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

#include "ui_TaskPolarPatternParameters.h"
#include "TaskPolarPatternParameters.h"
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
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "TaskMultiTransformParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPolarPatternParameters */

TaskPolarPatternParameters::TaskPolarPatternParameters(ViewProviderTransformed *TransformedView,QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPolarPatternParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    referenceSelectionMode = false;

    updateUIinProgress = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskPolarPatternParameters::TaskPolarPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskPolarPatternParameters();
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

    updateUIinProgress = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskPolarPatternParameters::setupUI()
{
    connect(ui->buttonX, SIGNAL(pressed()),
            this, SLOT(onButtonX()));
    connect(ui->buttonY, SIGNAL(pressed()),
            this, SLOT(onButtonY()));
    connect(ui->buttonZ, SIGNAL(pressed()),
            this, SLOT(onButtonZ()));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));
    connect(ui->spinAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngle(double)));
    connect(ui->spinOccurrences, SIGNAL(valueChanged(int)),
            this, SLOT(onOccurrences(int)));
    connect(ui->buttonReference, SIGNAL(toggled(bool)),
            this, SLOT(onButtonReference(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    std::vector<App::DocumentObject*> originals = pcPolarPattern->Originals.getValues();

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

    ui->buttonX->setEnabled(true);
    ui->buttonY->setEnabled(true);
    ui->buttonZ->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->spinAngle->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);
    ui->buttonReference->setEnabled(true);
    ui->lineReference->setEnabled(false); // This is never enabled since it is for optical feed-back only
    updateUI();
}

void TaskPolarPatternParameters::updateUI()
{
    if (updateUIinProgress)
        return;
    updateUIinProgress = true;

    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());

    App::DocumentObject* axisFeature = pcPolarPattern->Axis.getValue();
    std::vector<std::string> axes = pcPolarPattern->Axis.getSubValues();
    std::string stdAxis = pcPolarPattern->StdAxis.getValue();
    bool reverse = pcPolarPattern->Reversed.getValue();
    double angle = pcPolarPattern->Angle.getValue();
    unsigned occurrences = pcPolarPattern->Occurrences.getValue();

    ui->buttonReference->setChecked(referenceSelectionMode);
    if (!stdAxis.empty())
    {
        ui->buttonX->setAutoExclusive(true);
        ui->buttonY->setAutoExclusive(true);
        ui->buttonZ->setAutoExclusive(true);
        ui->buttonX->setChecked(stdAxis == "X");
        ui->buttonY->setChecked(stdAxis == "Y");
        ui->buttonZ->setChecked(stdAxis == "Z");
        ui->lineReference->setText(tr(""));
    } else if (axisFeature != NULL && !axes.empty()) {
        ui->buttonX->setAutoExclusive(false);
        ui->buttonY->setAutoExclusive(false);
        ui->buttonZ->setAutoExclusive(false);
        ui->buttonX->setChecked(false);
        ui->buttonY->setChecked(false);
        ui->buttonZ->setChecked(false);
        ui->lineReference->setText(QString::fromAscii(axes.front().c_str()));
    } else {
        // Error message?
        ui->lineReference->setText(tr(""));
    }
    if (referenceSelectionMode)
        ui->lineReference->setText(tr("Select an edge"));

    ui->checkReverse->setChecked(reverse);
    ui->spinAngle->setValue(angle);
    ui->spinOccurrences->setValue(occurrences);

    updateUIinProgress = false;
}

void TaskPolarPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        std::string subName(msg.pSubName);
        PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
        if (originalSelectionMode) {
            App::DocumentObject* selectedObject = pcPolarPattern->getDocument()->getActiveObject();
            if ((selectedObject == NULL) || !selectedObject->isDerivedFrom(Part::Feature::getClassTypeId()))
                return;
            if (originalSelected(msg))
                ui->lineOriginal->setText(QString::fromAscii(selectedObject->getNameInDocument()));
        } else if (referenceSelectionMode &&
                   (subName.size() > 4 && subName.substr(0,4) == "Edge")) {

            std::vector<std::string> axes;
            axes.push_back(subName.c_str());
            pcPolarPattern->Axis.setValue(getOriginalObject(), axes);
            pcPolarPattern->StdAxis.setValue("");

            if (updateView())
                recomputeFeature();

            exitSelectionMode();
            updateUI();
        }
    }
}

void TaskPolarPatternParameters::onButtonX() {
    onStdAxis("X");
}

void TaskPolarPatternParameters::onButtonY() {
    onStdAxis("Y");
}

void TaskPolarPatternParameters::onButtonZ() {
    onStdAxis("Z");
}

void TaskPolarPatternParameters::onCheckReverse(const bool on) {
    if (updateUIinProgress)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Reversed.setValue(on);

    exitSelectionMode();
    updateUI();
    if (updateView())
        recomputeFeature();
}

void TaskPolarPatternParameters::onAngle(const double a) {
    if (updateUIinProgress)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Angle.setValue(a);

    exitSelectionMode();
    updateUI();
    if (updateView())
        recomputeFeature();
}

void TaskPolarPatternParameters::onOccurrences(const int n) {
    if (updateUIinProgress)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Occurrences.setValue(n);

    exitSelectionMode();
    updateUI();
    if (updateView())
        recomputeFeature();
}

void TaskPolarPatternParameters::onStdAxis(const std::string& axis) {
    if (updateUIinProgress)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->StdAxis.setValue(axis.c_str());
    pcPolarPattern->Axis.setValue(NULL);

    exitSelectionMode();
    updateUI();
    if (updateView())
        recomputeFeature();
}

void TaskPolarPatternParameters::onButtonReference(bool checked)
{
    if (updateUIinProgress)
        return;
    if (checked ) {
        hideObject();
        showOriginals();
        referenceSelectionMode = true;
        Gui::Selection().clearSelection();
    } else {
        exitSelectionMode();
    }
    updateUI();
}

void TaskPolarPatternParameters::onUpdateView(bool on)
{
    ui->buttonX->blockSignals(!on);
    ui->buttonY->blockSignals(!on);
    ui->buttonZ->blockSignals(!on);
    ui->checkReverse->blockSignals(!on);
    ui->spinAngle->blockSignals(!on);
    ui->spinOccurrences->blockSignals(!on);
}

const std::string TaskPolarPatternParameters::getStdAxis(void) const
{
    std::string stdAxis;

    if (ui->buttonX->isChecked())
        stdAxis = "X";
    else if (ui->buttonY->isChecked())
        stdAxis = "Y";
    else if (ui->buttonZ->isChecked())
        stdAxis = "Z";

    if (!stdAxis.empty())
        return std::string("\"") + stdAxis + "\"";
    else
        return std::string("");
}

const QString TaskPolarPatternParameters::getAxis(void) const
{
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    App::DocumentObject* feature = pcPolarPattern->Axis.getValue();
    if (feature == NULL)
        return QString::fromUtf8("");
    std::vector<std::string> axes = pcPolarPattern->Axis.getSubValues();
    QString buf;

    if ((feature != NULL) && !axes.empty()) {
        buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
        buf = buf.arg(QString::fromUtf8(feature->getNameInDocument()));
        buf = buf.arg(QString::fromUtf8(axes.front().c_str()));
    }
    else
        buf = QString::fromUtf8("");

    return buf;
}

const bool TaskPolarPatternParameters::getReverse(void) const
{
    return ui->checkReverse->isChecked();
}

const double TaskPolarPatternParameters::getAngle(void) const
{
    return ui->spinAngle->value();
}

const unsigned TaskPolarPatternParameters::getOccurrences(void) const
{
    return ui->spinOccurrences->value();
}

const bool TaskPolarPatternParameters::updateView() const
{
    if (insideMultiTransform)
        return parentTask->updateView();
    else
        return ui->checkBoxUpdateView->isChecked();
}

void TaskPolarPatternParameters::exitSelectionMode()
{
    originalSelectionMode = false;
    referenceSelectionMode = false;
    showObject();
    hideOriginals();
}


TaskPolarPatternParameters::~TaskPolarPatternParameters()
{
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskPolarPatternParameters::changeEvent(QEvent *e)
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

TaskDlgPolarPatternParameters::TaskDlgPolarPatternParameters(ViewProviderPolarPattern *PolarPatternView)
    : TaskDlgTransformedParameters(PolarPatternView)
{
    parameter = new TaskPolarPatternParameters(PolarPatternView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgPolarPatternParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand("PolarPattern changed");
        // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskPolarPatternParameters* polarpatternParameter = static_cast<TaskPolarPatternParameters*>(parameter);
        std::string axis = polarpatternParameter->getAxis().toStdString();
        if (!axis.empty())
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Axis = %s", name.c_str(), axis.c_str());
        std::string stdAxis = polarpatternParameter->getStdAxis();
        if (!stdAxis.empty())
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.StdAxis = %s",name.c_str(),stdAxis.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),polarpatternParameter->getReverse());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Angle = %f",name.c_str(),polarpatternParameter->getAngle());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Occurrences = %u",name.c_str(),polarpatternParameter->getOccurrences());
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

#include "moc_TaskPolarPatternParameters.cpp"
