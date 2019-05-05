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
# include <QAction>
# include <QTimer>
#endif

#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
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
#include <Mod/PartDesign/App/Body.h>

#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"
#include "Utils.h"

#include "ui_TaskPolarPatternParameters.h"
#include "TaskPolarPatternParameters.h"

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

    setupListWidget(ui->listWidgetFeatures);

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
    // ---------------------

    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    ui->polarAngle->bind(pcPolarPattern->Angle);
    ui->spinOccurrences->setMaximum(INT_MAX);
    ui->spinOccurrences->bind(pcPolarPattern->Occurrences);

    ui->comboAxis->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->polarAngle->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);

    this->axesLinks.setCombo(*(ui->comboAxis));
    App::DocumentObject* sketch = getSketchObject();
    if (sketch && sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        this->fillAxisCombo(axesLinks, static_cast<Part::Part2DObject*>(sketch));
    }
    else {
        this->fillAxisCombo(axesLinks, NULL);
    }

    //show the parts coordinate system axis for selection
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( getObject() );

    if(body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, false);
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what () );
        }
    }

    updateUI();
}

void TaskPolarPatternParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());

    bool reverse = pcPolarPattern->Reversed.getValue();
    double angle = pcPolarPattern->Angle.getValue();
    unsigned occurrences = pcPolarPattern->Occurrences.getValue();

    if (axesLinks.setCurrentLink(pcPolarPattern->Axis) == -1){
        //failed to set current, because the link isn't in the list yet
        axesLinks.addLink(pcPolarPattern->Axis, getRefStr(pcPolarPattern->Axis.getValue(),pcPolarPattern->Axis.getSubValues()));
        axesLinks.setCurrentLink(pcPolarPattern->Axis);
    }

    // Note: These three lines would trigger onLength(), on Occurrences() and another updateUI() if we
    // didn't check for blockUpdate
    ui->checkReverse->setChecked(reverse);
    ui->polarAngle->setValue(angle);
    ui->spinOccurrences->setValue(occurrences);

    blockUpdate = false;
}

void TaskPolarPatternParameters::onUpdateViewTimer()
{
    setupTransaction();
    recomputeFeature();
}

void TaskPolarPatternParameters::kickUpdateViewTimer() const
{
    updateViewTimer->start();
}

void TaskPolarPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode!=none && msg.Type == Gui::SelectionChanges::AddSelection) {
        
        if (originalSelected(msg)) {
            exitSelectionMode();
        }
        else {
            std::vector<std::string> axes;
            App::DocumentObject* selObj;
            PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
            getReferencedSelection(pcPolarPattern, msg, selObj, axes);
            if(!selObj)
                    return;
            
            if (selectionMode == reference || selObj->isDerivedFrom ( App::Line::getClassTypeId () ) ) {
                setupTransaction();
                pcPolarPattern->Axis.setValue(selObj, axes);
                recomputeFeature();
                updateUI();
            }
            exitSelectionMode();
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

void TaskPolarPatternParameters::onAxisChanged(int /*num*/)
{
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());

    try{
        if(axesLinks.getCurrentLink().getValue() == 0){
            // enter reference selection mode
            hideObject();
            showBase();
            selectionMode = reference;
            Gui::Selection().clearSelection();
            addReferenceSelectionGate(true, false);
        } else {
            exitSelectionMode();
            pcPolarPattern->Axis.Paste(axesLinks.getCurrentLink());
        }
    } catch (Base::Exception &e) {
        QMessageBox::warning(0,tr("Error"),QString::fromLatin1(e.what()));
    }

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

        setupTransaction();
        getAxis(obj, axes);
        pcPolarPattern->Axis.setValue(obj,axes);
        pcPolarPattern->Reversed.setValue(getReverse());
        pcPolarPattern->Angle.setValue(getAngle());
        pcPolarPattern->Occurrences.setValue(getOccurrences());

        recomputeFeature();
    }
}

void TaskPolarPatternParameters::getAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    const App::PropertyLinkSub &lnk = axesLinks.getCurrentLink();
    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

bool TaskPolarPatternParameters::getReverse(void) const
{
    return ui->checkReverse->isChecked();
}

double TaskPolarPatternParameters::getAngle(void) const
{
    return ui->polarAngle->value().getValue();
}

unsigned TaskPolarPatternParameters::getOccurrences(void) const
{
    return ui->spinOccurrences->value();
}


TaskPolarPatternParameters::~TaskPolarPatternParameters()
{
    //hide the parts coordinate system axis for selection
    try {
        PartDesign::Body * body = PartDesign::Body::findBodyOf ( getObject() );
        if ( body ) {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility ();
        }
    } catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what () );
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
    auto tobj = TransformedView->getObject();
    std::vector<std::string> axes;
    App::DocumentObject* obj;
    getAxis(obj, axes);
    std::string axis = buildLinkSingleSubPythonStr(obj, axes);

    FCMD_OBJ_CMD(tobj,"Axis = " << axis.c_str());
    FCMD_OBJ_CMD(tobj,"Reversed = " << getReverse());
    ui->polarAngle->apply();
    ui->spinOccurrences->apply();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPolarPatternParameters::TaskDlgPolarPatternParameters(ViewProviderPolarPattern *PolarPatternView)
    : TaskDlgTransformedParameters(PolarPatternView, new TaskPolarPatternParameters(PolarPatternView))
{
}

#include "moc_TaskPolarPatternParameters.cpp"
