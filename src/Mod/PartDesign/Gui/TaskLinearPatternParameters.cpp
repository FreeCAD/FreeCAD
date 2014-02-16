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
# include <QTimer>
#endif

#include "ui_TaskLinearPatternParameters.h"
#include "TaskLinearPatternParameters.h"
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
#include <Mod/PartDesign/App/FeatureLinearPattern.h>
#include <Mod/Sketcher/App/SketchObject.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLinearPatternParameters */

TaskLinearPatternParameters::TaskLinearPatternParameters(ViewProviderTransformed *TransformedView,QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskLinearPatternParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    referenceSelectionMode = false;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskLinearPatternParameters::TaskLinearPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskLinearPatternParameters();
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

void TaskLinearPatternParameters::setupUI()
{
    updateViewTimer = new QTimer(this);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(getUpdateViewTimeout());

    connect(updateViewTimer, SIGNAL(timeout()),
            this, SLOT(onUpdateViewTimer()));
    connect(ui->comboDirection, SIGNAL(activated(int)),
            this, SLOT(onDirectionChanged(int)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));
    connect(ui->spinLength, SIGNAL(valueChanged(double)),
            this, SLOT(onLength(double)));
    connect(ui->spinOccurrences, SIGNAL(valueChanged(int)),
            this, SLOT(onOccurrences(int)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    std::vector<App::DocumentObject*> originals = pcLinearPattern->Originals.getValues();

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

    ui->comboDirection->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->spinLength->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);
    ui->spinLength->setDecimals(Base::UnitsApi::getDecimals());
    updateUI();
}

void TaskLinearPatternParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());

    App::DocumentObject* directionFeature = pcLinearPattern->Direction.getValue();
    std::vector<std::string> directions = pcLinearPattern->Direction.getSubValues();
    bool reverse = pcLinearPattern->Reversed.getValue();
    double length = pcLinearPattern->Length.getValue();
    unsigned occurrences = pcLinearPattern->Occurrences.getValue();

    App::DocumentObject* sketch = getSketchObject();
    int maxcount=2;
    if (sketch)
        maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();

    for (int i=ui->comboDirection->count()-1; i >= 2; i--)
        ui->comboDirection->removeItem(i);
    for (int i=ui->comboDirection->count(); i < maxcount; i++)
        ui->comboDirection->addItem(QString::fromAscii("Sketch axis %1").arg(i-2));

    bool undefined = false;
    if (directionFeature != NULL && !directions.empty()) {
        if (directions.front() == "H_Axis")
            ui->comboDirection->setCurrentIndex(0);
        else if (directions.front() == "V_Axis")
            ui->comboDirection->setCurrentIndex(1);
        else if (directions.front().size() > 4 && directions.front().substr(0,4) == "Axis") {
            int pos = 2 + std::atoi(directions.front().substr(4,4000).c_str());
            if (pos <= maxcount)
                ui->comboDirection->setCurrentIndex(pos);
            else
                undefined = true;
        } else if (directionFeature != NULL && !directions.empty()) {
            ui->comboDirection->addItem(QString::fromAscii(directions.front().c_str()));
            ui->comboDirection->setCurrentIndex(maxcount);
        }
    } else {
        undefined = true;
    }

    if (referenceSelectionMode) {
        ui->comboDirection->addItem(tr("Select an edge or a face"));
        ui->comboDirection->setCurrentIndex(ui->comboDirection->count() - 1);
    } else if (undefined) {
        ui->comboDirection->addItem(tr("Undefined"));
        ui->comboDirection->setCurrentIndex(ui->comboDirection->count() - 1);
    } else
        ui->comboDirection->addItem(tr("Select reference..."));

    // Note: These three lines would trigger onLength(), on Occurrences() and another updateUI() if we
    // didn't check for blockUpdate
    ui->checkReverse->setChecked(reverse);
    ui->spinLength->setValue(length);
    ui->spinOccurrences->setValue(occurrences);

    blockUpdate = false;
}

void TaskLinearPatternParameters::onUpdateViewTimer()
{
    recomputeFeature();
}

void TaskLinearPatternParameters::kickUpdateViewTimer() const
{
    updateViewTimer->start();
}

void TaskLinearPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        if (strcmp(msg.pDocName, getObject()->getDocument()->getName()) != 0)
            return;

        std::string subName(msg.pSubName);
        if (originalSelected(msg)) {
            ui->lineOriginal->setText(QString::fromAscii(msg.pObjectName));
        } else if (referenceSelectionMode &&
                   ((subName.size() > 4 && subName.substr(0,4) == "Edge") ||
                    (subName.size() > 4 && subName.substr(0,4) == "Face"))) {

            if (strcmp(msg.pObjectName, getSupportObject()->getNameInDocument()) != 0)
                return;

            exitSelectionMode();
            if (!blockUpdate) {
                PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
                std::vector<std::string> directions(1,subName);
                pcLinearPattern->Direction.setValue(getSupportObject(), directions);

                recomputeFeature();
                updateUI();
            }
            else {
                App::DocumentObject* sketch = getSketchObject();
                int maxcount=2;
                if (sketch)
                    maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();
                for (int i=ui->comboDirection->count()-1; i >= maxcount; i--)
                    ui->comboDirection->removeItem(i);

                ui->comboDirection->addItem(QString::fromAscii(subName.c_str()));
                ui->comboDirection->setCurrentIndex(maxcount);
                ui->comboDirection->addItem(tr("Select reference..."));
            }
        }
    }
}

void TaskLinearPatternParameters::onCheckReverse(const bool on) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Reversed.setValue(on);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onLength(const double l) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Length.setValue(l);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onOccurrences(const int n) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Occurrences.setValue(n);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onDirectionChanged(int num) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());

    App::DocumentObject* pcSketch = getSketchObject();
    int maxcount=2;
    if (pcSketch)
        maxcount += static_cast<Part::Part2DObject*>(pcSketch)->getAxisCount();

    if (num == 0) {
        pcLinearPattern->Direction.setValue(pcSketch, std::vector<std::string>(1,"H_Axis"));
        exitSelectionMode();
    }
    else if (num == 1) {
        pcLinearPattern->Direction.setValue(pcSketch, std::vector<std::string>(1,"V_Axis"));
        exitSelectionMode();
    }
    else if (num >= 2 && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-2);
        std::string str = buf.toStdString();
        pcLinearPattern->Direction.setValue(pcSketch, std::vector<std::string>(1,str));
    }
    else if (num == ui->comboDirection->count() - 1) {
        // enter reference selection mode
        hideObject();
        showOriginals();
        referenceSelectionMode = true;
        Gui::Selection().clearSelection();
        addReferenceSelectionGate(true, true);
    }
    else if (num == maxcount)
        exitSelectionMode();

    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgLinearPatternParameters::accept() but without doCommand
        PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());

        std::string direction = getDirection();
        if (!direction.empty()) {
            std::vector<std::string> directions(1,direction);
            if (direction == "H_Axis" || direction == "V_Axis" ||
                (direction.size() > 4 && direction.substr(0,4) == "Axis"))
                pcLinearPattern->Direction.setValue(getSketchObject(), directions);
            else
                pcLinearPattern->Direction.setValue(getSupportObject(), directions);
        } else
            pcLinearPattern->Direction.setValue(NULL);

        pcLinearPattern->Reversed.setValue(getReverse());
        pcLinearPattern->Length.setValue(getLength());
        pcLinearPattern->Occurrences.setValue(getOccurrences());

        recomputeFeature();
    }
}

const std::string TaskLinearPatternParameters::getDirection(void) const
{
    App::DocumentObject* pcSketch = getSketchObject();
    int maxcount=2;
    if (pcSketch)
        maxcount += static_cast<Part::Part2DObject*>(pcSketch)->getAxisCount();

    int num = ui->comboDirection->currentIndex();
    if (num == 0)
        return "H_Axis";
    else if (num == 1)
        return "V_Axis";
    else if (num >= 2 && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-2);
        return buf.toStdString();
    } else if (num == maxcount &&
               ui->comboDirection->count() == maxcount + 2)
        return ui->comboDirection->currentText().toStdString();
    return std::string("");
}

const bool TaskLinearPatternParameters::getReverse(void) const
{
    return ui->checkReverse->isChecked();
}

const double TaskLinearPatternParameters::getLength(void) const
{
    return ui->spinLength->value();
}

const unsigned TaskLinearPatternParameters::getOccurrences(void) const
{
    return ui->spinOccurrences->value();
}


TaskLinearPatternParameters::~TaskLinearPatternParameters()
{
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskLinearPatternParameters::changeEvent(QEvent *e)
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

TaskDlgLinearPatternParameters::TaskDlgLinearPatternParameters(ViewProviderLinearPattern *LinearPatternView)
    : TaskDlgTransformedParameters(LinearPatternView)
{
    parameter = new TaskLinearPatternParameters(LinearPatternView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgLinearPatternParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand("LinearPattern changed");
        // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskLinearPatternParameters* linearpatternParameter = static_cast<TaskLinearPatternParameters*>(parameter);
        std::string direction = linearpatternParameter->getDirection();
        if (!direction.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            if (direction == "H_Axis" || direction == "V_Axis" ||
                (direction.size() > 4 && direction.substr(0,4) == "Axis"))
                buf = buf.arg(QString::fromUtf8(linearpatternParameter->getSketchObject()->getNameInDocument()));
            else
                buf = buf.arg(QString::fromUtf8(linearpatternParameter->getSupportObject()->getNameInDocument()));
            buf = buf.arg(QString::fromUtf8(direction.c_str()));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = %s", name.c_str(), buf.toStdString().c_str());
        } else
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = None", name.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),linearpatternParameter->getReverse());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f",name.c_str(),linearpatternParameter->getLength());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Occurrences = %u",name.c_str(),linearpatternParameter->getOccurrences());
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

#include "moc_TaskLinearPatternParameters.cpp"
