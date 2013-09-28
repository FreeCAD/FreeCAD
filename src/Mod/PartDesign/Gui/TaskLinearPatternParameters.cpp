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
#include "Workbench.h"
#include "ReferenceSelection.h"
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
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumLine.h>
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

    selectionMode = none;

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
    ui->buttonAddFeature->hide();
    ui->buttonRemoveFeature->hide();
    ui->listWidgetFeatures->hide();
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskLinearPatternParameters::setupUI()
{
    connect(ui->buttonAddFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonAddFeature(bool)));
    connect(ui->buttonRemoveFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonRemoveFeature(bool)));
    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    ui->listWidgetFeatures->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onFeatureDeleted()));
    ui->listWidgetFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);

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
    connect(ui->spinOccurrences, SIGNAL(valueChanged(uint)),
            this, SLOT(onOccurrences(uint)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    std::vector<App::DocumentObject*> originals = pcLinearPattern->Originals.getValues();

    // Fill data into dialog elements
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); ++i)
    {
        if ((*i) != NULL)
            ui->listWidgetFeatures->insertItem(0, QString::fromLatin1((*i)->getNameInDocument()));
    }
    // ---------------------

    ui->spinLength->bind(pcLinearPattern->Length);
    ui->spinOccurrences->setMaximum(INT_MAX);
    ui->spinOccurrences->bind(pcLinearPattern->Occurrences);

    ui->comboDirection->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->spinLength->blockSignals(true);
    ui->spinLength->setEnabled(true);
    ui->spinLength->setUnit(Base::Unit::Length);
    ui->spinLength->blockSignals(false);
    ui->spinOccurrences->setEnabled(true);
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

    // Add user-defined sketch axes to the reference selection combo box
    App::DocumentObject* sketch = getSketchObject();
    int maxcount=5;
    if (sketch)
        maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();

    for (int i=ui->comboDirection->count()-1; i >= 5; i--)
        ui->comboDirection->removeItem(i);
    for (int i=ui->comboDirection->count(); i < maxcount; i++)
        ui->comboDirection->addItem(QString::fromLatin1("Sketch axis %1").arg(i-5));

    bool undefined = false;
    if (directionFeature != NULL && !directions.empty()) {
        if (directions.front() == "H_Axis")
            ui->comboDirection->setCurrentIndex(0);
        else if (directions.front() == "V_Axis")
            ui->comboDirection->setCurrentIndex(1);
        else if (strcmp(directionFeature->getNameInDocument(), PartDesignGui::BaseplaneNames[0]) == 0)
            ui->comboDirection->setCurrentIndex(2);
        else if (strcmp(directionFeature->getNameInDocument(), PartDesignGui::BaseplaneNames[1]) == 0)
            ui->comboDirection->setCurrentIndex(3);
        else if (strcmp(directionFeature->getNameInDocument(), PartDesignGui::BaseplaneNames[2]) == 0)
            ui->comboDirection->setCurrentIndex(4);
        else if (directions.front().size() > 4 && directions.front().substr(0,4) == "Axis") {
            int pos = 5 + std::atoi(directions.front().substr(4,4000).c_str());
            if (pos <= maxcount)
                ui->comboDirection->setCurrentIndex(pos);
            else
                undefined = true;
        } else {
            ui->comboDirection->addItem(getRefStr(directionFeature, directions));
            ui->comboDirection->setCurrentIndex(maxcount);
        }
    } else {
        undefined = true;
    }

    if (selectionMode == reference) {
        ui->comboDirection->addItem(tr("Select an edge/face or datum line/plane"));
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
        if (originalSelected(msg)) {
            if (selectionMode == addFeature)
                ui->listWidgetFeatures->insertItem(0, QString::fromLatin1(msg.pObjectName));
            else
                removeItemFromListWidget(ui->listWidgetFeatures, msg.pObjectName);

            exitSelectionMode();
        } else if (selectionMode == reference) {
            // Note: ReferenceSelection has already checked the selection for validity
            exitSelectionMode();
            if (!blockUpdate) {
                std::vector<std::string> directions;
                App::DocumentObject* selObj;
                PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
                getReferencedSelection(pcLinearPattern, msg, selObj, directions);
                pcLinearPattern->Direction.setValue(selObj, directions);

                recomputeFeature();
                updateUI();
            }
            else {
                App::DocumentObject* sketch = getSketchObject();
                int maxcount=5;
                if (sketch)
                    maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();
                for (int i=ui->comboDirection->count()-1; i >= maxcount; i--)
                    ui->comboDirection->removeItem(i);

                std::vector<std::string> directions;
                App::DocumentObject* selObj;
                PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
                getReferencedSelection(pcLinearPattern, msg, selObj, directions);
                ui->comboDirection->addItem(getRefStr(selObj, directions));
                ui->comboDirection->setCurrentIndex(maxcount);
                ui->comboDirection->addItem(tr("Select reference..."));
            }
        }
    }
}

void TaskLinearPatternParameters::clearButtons()
{
    ui->buttonAddFeature->setChecked(false);
    ui->buttonRemoveFeature->setChecked(false);
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

void TaskLinearPatternParameters::onOccurrences(const uint n) {
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
    int maxcount=5;
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
    else if (num == 2) {
        pcLinearPattern->Direction.setValue(getObject()->getDocument()->getObject(PartDesignGui::BaseplaneNames[0]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num == 3) {
        pcLinearPattern->Direction.setValue(getObject()->getDocument()->getObject(PartDesignGui::BaseplaneNames[1]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num == 4) {
        pcLinearPattern->Direction.setValue(getObject()->getDocument()->getObject(PartDesignGui::BaseplaneNames[2]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num >= 5 && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-5);
        std::string str = buf.toStdString();
        pcLinearPattern->Direction.setValue(pcSketch, std::vector<std::string>(1,str));
        exitSelectionMode();
    }
    else if (num == ui->comboDirection->count() - 1) {
        // enter reference selection mode
        hideObject();
        showBase();
        selectionMode = reference;
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
        std::vector<std::string> directions;
        App::DocumentObject* obj;

        getDirection(obj, directions);
        pcLinearPattern->Direction.setValue(obj,directions);
        pcLinearPattern->Reversed.setValue(getReverse());
        pcLinearPattern->Length.setValue(getLength());
        pcLinearPattern->Occurrences.setValue(getOccurrences());

        recomputeFeature();
    }
}

void TaskLinearPatternParameters::onFeatureDeleted(void)
{
    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    originals.erase(originals.begin() + ui->listWidgetFeatures->currentRow());
    pcTransformed->Originals.setValues(originals);
    ui->listWidgetFeatures->model()->removeRow(ui->listWidgetFeatures->currentRow());
    recomputeFeature();
}

void TaskLinearPatternParameters::getDirection(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    obj = getSketchObject();
    sub = std::vector<std::string>(1,"");
    int maxcount=5;
    if (obj)
        maxcount += static_cast<Part::Part2DObject*>(obj)->getAxisCount();

    int num = ui->comboDirection->currentIndex();
    if (num == 0)
        sub[0] = "H_Axis";
    else if (num == 1)
        sub[0] = "V_Axis";
    else if (num == 2)
        obj = getObject()->getDocument()->getObject(PartDesignGui::BaseplaneNames[0]);
    else if (num == 3)
        obj = getObject()->getDocument()->getObject(PartDesignGui::BaseplaneNames[1]);
    else if (num == 4)
        obj = getObject()->getDocument()->getObject(PartDesignGui::BaseplaneNames[2]);
    else if (num >= 5 && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-5);
        sub[0] = buf.toStdString();
    } else if (num == maxcount && ui->comboDirection->count() == maxcount + 2) {
        QStringList parts = ui->comboDirection->currentText().split(QChar::fromAscii(':'));
        obj = getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
        if (parts.size() > 1)
            sub[0] = parts[1].toStdString();
    } else {
        obj = NULL;
    }
}

const bool TaskLinearPatternParameters::getReverse(void) const
{
    return ui->checkReverse->isChecked();
}

const double TaskLinearPatternParameters::getLength(void) const
{
    return ui->spinLength->value().getValue();
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

void TaskLinearPatternParameters::apply()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    std::vector<std::string> directions;
    App::DocumentObject* obj;
    getDirection(obj, directions);
    std::string direction = getPythonStr(obj, directions);
    if (!direction.empty()) {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = %s", name.c_str(), direction.c_str());
    } else
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = None", name.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),getReverse());

    ui->spinLength->apply();
    ui->spinOccurrences->apply();

    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    if (!TransformedView->getObject()->isValid())
        throw Base::Exception(TransformedView->getObject()->getStatusString());
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();
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
    try {
        // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        parameter->apply();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

#include "moc_TaskLinearPatternParameters.cpp"
