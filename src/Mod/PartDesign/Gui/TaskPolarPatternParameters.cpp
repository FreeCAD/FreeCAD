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

#include "ui_TaskPolarPatternParameters.h"
#include "TaskPolarPatternParameters.h"
#include "TaskMultiTransformParameters.h"
#include "Workbench.h"
#include "ReferenceSelection.h"
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/Line.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/DatumLine.h>

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

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
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
    ui->buttonAddFeature->hide();
    ui->buttonRemoveFeature->hide();
    ui->listWidgetFeatures->hide();
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskPolarPatternParameters::setupUI()
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
    connect(ui->comboAxis, SIGNAL(activated(int)),
            this, SLOT(onAxisChanged(int)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));
    connect(ui->polarAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngle(double)));
    connect(ui->spinOccurrences, SIGNAL(valueChanged(uint)),
            this, SLOT(onOccurrences(uint)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    std::vector<App::DocumentObject*> originals = pcPolarPattern->Originals.getValues();

    // Fill data into dialog elements
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); ++i)
    {
        if ((*i) != NULL)
            ui->listWidgetFeatures->addItem(QString::fromLatin1((*i)->getNameInDocument()));
    }
    // ---------------------

    ui->polarAngle->bind(pcPolarPattern->Angle);
    ui->spinOccurrences->setMaximum(INT_MAX);
    ui->spinOccurrences->bind(pcPolarPattern->Occurrences);

    ui->comboAxis->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->polarAngle->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);
    
    App::DocumentObject* sketch = getSketchObject();
    if (sketch) {
        ui->comboAxis->addItem(QString::fromAscii("Normal sketch axis"));
    }
    //add the base axes to the selection combo box
    ui->comboAxis->addItem(QString::fromAscii("Base X axis"));
    ui->comboAxis->addItem(QString::fromAscii("Base Y axis"));    
    ui->comboAxis->addItem(QString::fromAscii("Base Z axis")); 
    ui->comboAxis->addItem(QString::fromAscii("Select reference..."));
    updateUI();
    
    //show the parts coordinate system axis for selection
    App::Part* part = getPartFor(getObject(), false);
    if(part) {        
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(true, Gui::Application::Instance->activeDocument());
            origin->setTemporaryVisibilityAxis(true);
        }            
     }
}

void TaskPolarPatternParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());

    App::DocumentObject* axisFeature = pcPolarPattern->Axis.getValue();
    std::vector<std::string> axes = pcPolarPattern->Axis.getSubValues();
    bool reverse = pcPolarPattern->Reversed.getValue();
    double angle = pcPolarPattern->Angle.getValue();
    unsigned occurrences = pcPolarPattern->Occurrences.getValue();

    // Add user-defined sketch axes to the reference selection combo box
    App::DocumentObject* sketch = getSketchObject();
    int maxcount=3;
    if (sketch) {
        maxcount = 4;
        maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();
    }

    for (int i=ui->comboAxis->count()-1; i >= (sketch ? 4 : 3); i--)
        ui->comboAxis->removeItem(i);
    for (int i=ui->comboAxis->count(); i < maxcount; i++)
        ui->comboAxis->addItem(QString::fromAscii("Sketch axis %1").arg(i-1));

    bool undefined = false;
    if (axisFeature != NULL && !axes.empty()) {
        bool is_base_line = axisFeature->isDerivedFrom(App::Line::getClassTypeId());
        
        if (axes.front() == "N_Axis") {
            ui->comboAxis->setCurrentIndex(0);
        } else if (is_base_line && strcmp(static_cast<App::Line*>(axisFeature)->LineType.getValue(), App::Part::BaselineTypes[0]) == 0) {
            ui->comboAxis->setCurrentIndex((sketch ? 1 : 0));
        } else if (is_base_line && strcmp(static_cast<App::Line*>(axisFeature)->LineType.getValue(), App::Part::BaselineTypes[1]) == 0) {
            ui->comboAxis->setCurrentIndex((sketch ? 2 : 1));
        } else if (is_base_line && strcmp(static_cast<App::Line*>(axisFeature)->LineType.getValue(), App::Part::BaselineTypes[2]) == 0) {
            ui->comboAxis->setCurrentIndex((sketch ? 3 : 2));
        } else if (axes.front().size() > (sketch ? 4 : 3) && axes.front().substr(0,4) == "Axis") {
            int pos = (sketch ? 4 : 3) + std::atoi(axes.front().substr(4,4000).c_str());
            if (pos <= maxcount)
                ui->comboAxis->setCurrentIndex(pos);
            else
                undefined = true;
        } else {
            ui->comboAxis->addItem(getRefStr(axisFeature, axes));
            ui->comboAxis->setCurrentIndex(maxcount);
        }
    } else {
        undefined = true;
    }

    if (selectionMode == reference) {
        ui->comboAxis->addItem(tr("Select an edge or datum line"));
        ui->comboAxis->setCurrentIndex(ui->comboAxis->count() - 1);
    } else if (undefined) {
        ui->comboAxis->addItem(tr("Undefined"));
        ui->comboAxis->setCurrentIndex(ui->comboAxis->count() - 1);
    } else
        ui->comboAxis->addItem(tr("Select reference..."));

    // Note: These three lines would trigger onLength(), on Occurrences() and another updateUI() if we
    // didn't check for blockUpdate
    ui->checkReverse->setChecked(reverse);
    ui->polarAngle->setValue(angle);
    ui->spinOccurrences->setValue(occurrences);

    blockUpdate = false;
}

void TaskPolarPatternParameters::onUpdateViewTimer()
{
    recomputeFeature();
}

void TaskPolarPatternParameters::kickUpdateViewTimer() const
{
    updateViewTimer->start();
}

void TaskPolarPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        if (originalSelected(msg)) {
            if (selectionMode == addFeature)
                ui->listWidgetFeatures->addItem(QString::fromLatin1(msg.pObjectName));
            else
                removeItemFromListWidget(ui->listWidgetFeatures, msg.pObjectName);
            exitSelectionMode();
        } else if (selectionMode == reference) {
            // Note: ReferenceSelection has already checked the selection for validity
            exitSelectionMode();
            if (!blockUpdate) {
                std::vector<std::string> axes;
                App::DocumentObject* selObj;
                PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
                getReferencedSelection(pcPolarPattern, msg, selObj, axes);
                pcPolarPattern->Axis.setValue(selObj, axes);

                recomputeFeature();
                updateUI();
            }
            else {
                App::DocumentObject* sketch = getSketchObject();
                int maxcount=1;
                if (sketch)
                    maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();
                for (int i=ui->comboAxis->count()-1; i >= maxcount; i--)
                    ui->comboAxis->removeItem(i);

                std::vector<std::string> axes;
                App::DocumentObject* selObj;
                PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
                getReferencedSelection(pcPolarPattern, msg, selObj, axes);
                ui->comboAxis->addItem(getRefStr(selObj, axes));
                ui->comboAxis->setCurrentIndex(1);
                ui->comboAxis->addItem(tr("Select reference..."));
            }
        } else if( strstr(msg.pObjectName, App::Part::BaselineTypes[0]) == nullptr || 
                   strstr(msg.pObjectName, App::Part::BaselineTypes[1]) == nullptr ||
                   strstr(msg.pObjectName, App::Part::BaselineTypes[2]) == nullptr) {
           
            std::vector<std::string> axes;
            App::DocumentObject* selObj;
            PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
            getReferencedSelection(pcPolarPattern, msg, selObj, axes);
            pcPolarPattern->Axis.setValue(selObj, axes);

            recomputeFeature();
            updateUI();
        }
    }
}

void TaskPolarPatternParameters::clearButtons()
{
    ui->buttonAddFeature->setChecked(false);
    ui->buttonRemoveFeature->setChecked(false);
}

void TaskPolarPatternParameters::onCheckReverse(const bool on) {
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Reversed.setValue(on);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskPolarPatternParameters::onAngle(const double a) {
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Angle.setValue(a);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskPolarPatternParameters::onOccurrences(const uint n) {
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Occurrences.setValue(n);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskPolarPatternParameters::onAxisChanged(int num) {
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());

    App::DocumentObject* pcSketch = getSketchObject();
    int maxcount=3;
    if (pcSketch) {
        maxcount = 4;
        maxcount += static_cast<Part::Part2DObject*>(pcSketch)->getAxisCount();
    }

    if(pcSketch) {
        if (num == 0) {
            pcPolarPattern->Axis.setValue(getSketchObject(), std::vector<std::string>(1,"N_Axis"));
            exitSelectionMode();
        }
    }
    if (num == (pcSketch ? 1 : 0)) {
        pcPolarPattern->Axis.setValue(getPartLines(App::Part::BaselineTypes[0]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num == (pcSketch ? 2 : 1)) {
        pcPolarPattern->Axis.setValue(getPartLines(App::Part::BaselineTypes[1]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num == (pcSketch ? 3 : 2)) {
        pcPolarPattern->Axis.setValue(getPartLines(App::Part::BaselineTypes[2]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num >= (pcSketch ? 4 : 3) && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-(pcSketch ? 4 : 3));
        std::string str = buf.toStdString();
        pcPolarPattern->Axis.setValue(pcSketch, std::vector<std::string>(1,str));
        exitSelectionMode();
    }
    else if (num == ui->comboAxis->count() - 1) {
        // enter reference selection mode
        hideObject();
        showBase();
        selectionMode = reference;
        Gui::Selection().clearSelection();
        addReferenceSelectionGate(true, false);
    }
    else if (num == maxcount)
        exitSelectionMode();

    kickUpdateViewTimer();
}

void TaskPolarPatternParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgPolarPatternParameters::accept() but without doCommand
        PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
        std::vector<std::string> axes;
        App::DocumentObject* obj;

        getAxis(obj, axes);
        pcPolarPattern->Axis.setValue(obj,axes);
        pcPolarPattern->Reversed.setValue(getReverse());
        pcPolarPattern->Angle.setValue(getAngle());
        pcPolarPattern->Occurrences.setValue(getOccurrences());

        recomputeFeature();
    }
}

void TaskPolarPatternParameters::onFeatureDeleted(void)
{
    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    originals.erase(originals.begin() + ui->listWidgetFeatures->currentRow());
    pcTransformed->Originals.setValues(originals);
    ui->listWidgetFeatures->model()->removeRow(ui->listWidgetFeatures->currentRow());
    recomputeFeature();
}

void TaskPolarPatternParameters::getAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{    
    obj = getSketchObject();
    sub = std::vector<std::string>(1,"");
    int maxcount=1;
    if (obj) {
        maxcount = 4;
        maxcount += static_cast<Part::Part2DObject*>(obj)->getAxisCount();
    }

    int num = ui->comboAxis->currentIndex();
    if(obj) {
        if (ui->comboAxis->currentIndex() == 0) {
            sub[0] = "N_Axis";
        }
    }
    if (num == (obj ? 1 : 0))
        obj = getPartLines(App::Part::BaselineTypes[0]);
    else if (num == (obj ? 2 : 1))
        obj = getPartLines(App::Part::BaselineTypes[1]);
    else if (num == (obj ? 3 : 2))
        obj = getPartLines(App::Part::BaselineTypes[2]);
    else if (num >= (obj ? 4 : 3) && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-(obj ? 4 : 3));
        sub[0] = buf.toStdString();
    }
    else if (num == maxcount && ui->comboAxis->count() == maxcount + 2) {
        QStringList parts = ui->comboAxis->currentText().split(QChar::fromAscii(':'));
        obj = getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
        if (parts.size() > 1)
            sub[0] = parts[1].toStdString();
    } else {
        obj = NULL;
    }
}

const bool TaskPolarPatternParameters::getReverse(void) const
{
    return ui->checkReverse->isChecked();
}

const double TaskPolarPatternParameters::getAngle(void) const
{
    return ui->polarAngle->value().getValue();
}

const unsigned TaskPolarPatternParameters::getOccurrences(void) const
{
    return ui->spinOccurrences->value();
}


TaskPolarPatternParameters::~TaskPolarPatternParameters()
{
    //hide the parts coordinate system axis for selection
    App::Part* part = getPartFor(getObject(), false);
    if(part) {
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(false);
        }            
    }
    
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

void TaskPolarPatternParameters::apply()
{
    std::string name = TransformedView->getObject()->getNameInDocument();
    std::vector<std::string> axes;
    App::DocumentObject* obj;
    getAxis(obj, axes);
    std::string axis = getPythonStr(obj, axes);
    if (!axis.empty() && obj) {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Axis = %s", name.c_str(), axis.c_str());
    } else {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Axis = None", name.c_str());
    }
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),getReverse());
    ui->polarAngle->apply();
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

TaskDlgPolarPatternParameters::TaskDlgPolarPatternParameters(ViewProviderPolarPattern *PolarPatternView)
    : TaskDlgTransformedParameters(PolarPatternView)
{
    parameter = new TaskPolarPatternParameters(PolarPatternView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgPolarPatternParameters::accept()
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

#include "moc_TaskPolarPatternParameters.cpp"
