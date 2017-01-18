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
#endif

#include "ui_TaskHoleParameters.h"
#include "TaskHoleParameters.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureHole.h>
#include <QMessageBox>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskHoleParameters */

TaskHoleParameters::TaskHoleParameters(ViewProviderHole *HoleView, QWidget *parent)
    : TaskSketchBasedParameters(HoleView, parent, "PartDesign_Hole",tr("Hole parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskHoleParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    ui->threadType->addItem(tr("None"));
    ui->threadType->addItem(tr("ISO metric coarse profile"));
    ui->threadType->addItem(tr("ISO metric fine profile"));
    ui->threadType->addItem(tr("UTS coarse profile"));
    ui->threadType->addItem(tr("UTS fine profile"));
    ui->threadType->addItem(tr("UTS extra fine profile"));

    connect(ui->threaded, SIGNAL(clicked(bool)), ui->directionRightHand, SLOT(setEnabled(bool)));
    connect(ui->threaded, SIGNAL(clicked(bool)), ui->directionLeftHand, SLOT(setEnabled(bool)));
    connect(ui->threaded, SIGNAL(clicked(bool)), this, SLOT(threadedChanged()));

    connect(ui->threadType, SIGNAL(currentIndexChanged(int)), this, SLOT(threadTypeChanged(int)));
    connect(ui->threadSize, SIGNAL(currentIndexChanged(int)), this, SLOT(threadSizeChanged(int)));
    connect(ui->threadClass, SIGNAL(currentIndexChanged(int)), this, SLOT(threadClassChanged(int)));
    connect(ui->threadFit, SIGNAL(currentIndexChanged(int)), this, SLOT(threadFitChanged(int)));
    connect(ui->threadDiameter, SIGNAL(valueChanged(double)), this, SLOT(threadDiameterChanged(double)));
    connect(ui->directionRightHand, SIGNAL(clicked(bool)), this, SLOT(threadDirectionChanged()));
    connect(ui->directionLeftHand, SIGNAL(clicked(bool)), this, SLOT(threadDirectionChanged()));

    connect(ui->holeCutType, SIGNAL(currentIndexChanged(int)), this, SLOT(holeCutChanged(int)));
    connect(ui->holeCutDiameter, SIGNAL(valueChanged(double)), this, SLOT(holeCutDiameterChanged(double)));
    connect(ui->holeCutDepth, SIGNAL(valueChanged(double)), this, SLOT(holeCutDepthChanged(double)));
    connect(ui->holeCutCountersinkAngle, SIGNAL(valueChanged(int)), this, SLOT(holeCutCountersinkAngleChanged(int)));

    connect(ui->depth, SIGNAL(currentIndexChanged(int)), this, SLOT(depthChanged(int)));
    connect(ui->depthValue, SIGNAL(valueChanged(double)), this, SLOT(depthValueChanged(double)));

    connect(ui->drillPointFlat, SIGNAL(clicked(bool)), ui->drillPointAngledValue, SLOT(setDisabled(bool)));
    connect(ui->drillPointAngled, SIGNAL(clicked(bool)), ui->drillPointAngledValue, SLOT(setEnabled(bool)));
    connect(ui->drillPointFlat, SIGNAL(clicked(bool)), this, SLOT(drillPointChanged()));
    connect(ui->drillPointAngled, SIGNAL(clicked(bool)), this, SLOT(drillPointChanged()));
    connect(ui->drillPointAngledValue, SIGNAL(valueChanged(int)), this, SLOT(drillPointAngledValueChanged(int)));

    connect(ui->tapered, SIGNAL(clicked(bool)), ui->taperedAngle, SLOT(setEnabled(bool)));
    connect(ui->tapered, SIGNAL(clicked(bool)), this, SLOT(taperedChanged()));
    connect(ui->taperedAngle, SIGNAL(valueChanged(double)), this, SLOT(taperedAngleChanged(double)));

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    ui->threaded->setChecked(pcHole->Threaded.getValue());
    ui->threadType->setCurrentIndex( -1 );
    ui->threadType->setCurrentIndex( pcHole->ThreadType.getValue() );
    ui->threadSize->blockSignals(true);
    ui->threadSize->setCurrentIndex( pcHole->ThreadSize.getValue() );
    ui->threadSize->blockSignals(false);
    ui->threadClass->setCurrentIndex( pcHole->ThreadClass.getValue() );
    ui->threadDiameter->setValue( pcHole->Diameter.getValue() );
    ui->threadFit->setCurrentIndex( pcHole->ThreadFit.getValue() );

    if (pcHole->ThreadDirection.getValue() == 0)
        ui->directionRightHand->setChecked(true);
    else
        ui->directionLeftHand->setChecked(true);

    ui->holeCutType->setCurrentIndex(pcHole->HoleCutType.getValue());
    ui->holeCutDiameter->setValue(pcHole->HoleCutDiameter.getValue());
    ui->holeCutDepth->setValue(pcHole->HoleCutDepth.getValue());
    ui->holeCutCountersinkAngle->setValue(pcHole->HoleCutCountersinkAngle.getValue());

    ui->depth->setCurrentIndex(pcHole->Type.getValue());
    ui->depthValue->setValue(pcHole->Length.getValue());

    std::string drillPoint(pcHole->DrillPoint.getValueAsString());
    if (drillPoint == "Flat")
        ui->drillPointFlat->setChecked(true);
    else if (drillPoint == "Angled")
        ui->drillPointAngled->setChecked(true);

    ui->drillPointAngledValue->setValue(pcHole->DrillPointAngle.getValue());

    ui->tapered->setChecked(pcHole->Tapered.getValue());
    ui->taperedAngle->setValue(pcHole->TaperedAngle.getValue());

    updateUi();

    this->groupLayout()->addWidget(proxy);
}

TaskHoleParameters::~TaskHoleParameters()
{
    delete ui;
}

void TaskHoleParameters::updateUi()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    std::string threadType = pcHole->ThreadType.getValueAsString();
    bool threadEnable = (threadType != "None");

    ui->threaded->setEnabled(threadEnable);
    ui->threadSize->setEnabled(threadEnable);
    ui->threadClass->setEnabled(threadEnable);
    ui->threadFit->setEnabled(!ui->threaded->isChecked() && threadEnable);
    ui->directionRightHand->setEnabled(threadEnable && pcHole->Threaded.getValue());
    ui->directionLeftHand->setEnabled(threadEnable && pcHole->Threaded.getValue());
    ui->threadDiameter->setEnabled(!threadEnable);

    bool holeCutEnable = ( threadType != "ISOMetricProfile" &&
            threadType !="ISOMetricFineProfile" &&
            (std::string(pcHole->HoleCutType.getValueAsString()) != "None"));
    ui->holeCutDiameter->setEnabled(holeCutEnable);
    ui->holeCutDepth->setEnabled(holeCutEnable);
    ui->holeCutCountersinkAngle->setEnabled(holeCutEnable);

    ui->depthValue->setEnabled( (std::string(pcHole->Type.getValueAsString()) == "Dimension") );

    ui->drillPointAngledValue->setEnabled( (std::string(pcHole->DrillPoint.getValueAsString()) == "Angled") );

    ui->taperedAngle->setEnabled( pcHole->Tapered.getValue() );
}

void TaskHoleParameters::threadedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    // Set new value in feature object
    pcHole->Threaded.setValue(ui->threaded->isChecked());

    // Force recomputation of diameter
    threadSizeChanged(pcHole->ThreadSize.getValue());

    // Update ui
    updateUi();

    // Recompute feature
    recomputeFeature();
}

void TaskHoleParameters::holeCutChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutType.setValue(index);

    std::string threadType = pcHole->ThreadType.getValueAsString();
    std::string holeCutType = pcHole->HoleCutType.getValueAsString();
    bool holeCutEnable = ( threadType != "ISOMetricProfile" &&
            threadType !="ISOMetricFineProfile" &&
            (holeCutType != "None"));
    ui->holeCutDiameter->setEnabled(holeCutEnable);

    ui->holeCutDepth->setEnabled(holeCutEnable);
    if (holeCutType == "Countersink" || holeCutType == "Countersink socket screw")
        ui->holeCutDepth->setEnabled(false);

    ui->holeCutCountersinkAngle->setEnabled(holeCutEnable);
    if (holeCutType != "Countersink" && holeCutType != "Countersink socket screw")
        ui->holeCutCountersinkAngle->setEnabled(false);

    updateHoleCutParams();
    recomputeFeature();
}

void TaskHoleParameters::holeCutDiameterChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutDiameter.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::holeCutDepthChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutDepth.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::holeCutCountersinkAngleChanged(int value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutCountersinkAngle.setValue((double)value);
    recomputeFeature();
}

void TaskHoleParameters::depthChanged(int index)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Type.setValue(index);
    ui->depthValue->setEnabled( (std::string(pcHole->Type.getValueAsString()) == "Dimension") );
    recomputeFeature();
}

void TaskHoleParameters::depthValueChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Length.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::drillPointChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    if (sender() == ui->drillPointFlat)
        pcHole->DrillPoint.setValue((long)0);
    else if (sender() == ui->drillPointAngled)
        pcHole->DrillPoint.setValue((long)1);
    else
        assert( 0 );
    recomputeFeature();
}

void TaskHoleParameters::drillPointAngledValueChanged(int value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->DrillPointAngle.setValue((double)value);
    recomputeFeature();
}

void TaskHoleParameters::taperedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Tapered.setValue(ui->tapered->isChecked());
    recomputeFeature();
}

void TaskHoleParameters::taperedAngleChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->TaperedAngle.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::threadTypeChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadType.setValue(index);

    /* Update sizes */
    ui->threadSize->blockSignals(true);
    ui->threadClass->blockSignals(true);
    ui->holeCutType->blockSignals(true);
    ui->threadSize->clear();
    const char ** cursor = pcHole->ThreadSize.getEnums();
    while (*cursor) {
        ui->threadSize->addItem(tr(*cursor));
        ++cursor;
    }

    /* Update classes */
    ui->threadClass->clear();
    cursor = pcHole->ThreadClass.getEnums();
    while (*cursor) {
        ui->threadClass->addItem(tr(*cursor));
        ++cursor;
    }

    /* Update hole cut type */
    ui->holeCutType->clear();
    cursor = pcHole->HoleCutType.getEnums();
    while (*cursor) {
        ui->holeCutType->addItem(tr(*cursor));
        ++cursor;
    }

    ui->holeCutType->blockSignals(false);
    ui->threadClass->blockSignals(false);
    ui->threadSize->blockSignals(false);

    std::string threadType = pcHole->ThreadType.getValueAsString();

    if (threadType == "ISOMetricProfile" || threadType == "ISOMetricFineProfile")
        ui->holeCutCountersinkAngle->setValue(90.0);
    else if (threadType == "UNC" || threadType == "UNF" || threadType == "UNEF")
        ui->holeCutCountersinkAngle->setValue(82.0);

    // Reset thread size when thread type has changed
    threadSizeChanged(0);

    // Update the UI
    updateUi();
}

void TaskHoleParameters::updateHoleCutParams()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());
    std::string threadType = pcHole->ThreadType.getValueAsString();

    if (threadType == "ISOMetricProfile" || threadType == "ISOMetricFineProfile") {
        std::string holeCutType = pcHole->HoleCutType.getValueAsString();
        double diameter = PartDesign::Hole::threadDescription[pcHole->ThreadType.getValue()][pcHole->ThreadSize.getValue()].diameter;
        double f;
        double depth = 0;

        if (holeCutType == "Counterbore") {
            f = 2.0;
            depth = 0.6;
        }
        else if (holeCutType == "Countersink") {
            f = 2.0;
            depth = 0;
        }
        else if (holeCutType == "Cheesehead") {
            f = 1.6;
            depth = 0.6;
        }
        else if (holeCutType == "Countersink socket screw") {
            f = 2.0;
            depth = 0;
        }
        else if (holeCutType == "Cap screw") {
            f = 1.5;
            depth = 1.25;
        }
        ui->holeCutDiameter->setValue(diameter * f);
        ui->holeCutDepth->setValue(diameter * depth);
    }
}

void TaskHoleParameters::threadSizeChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadSize.setValue(index);

    int threadType = getThreadType();
    int threadSize = getThreadSize();
    double diameter = PartDesign::Hole::threadDescription[pcHole->ThreadType.getValue()][pcHole->ThreadSize.getValue()].diameter;
    double pitch = PartDesign::Hole::threadDescription[threadType][threadSize].pitch;

    if (ui->threaded->isChecked()) {
        /* Threads are always given with tap diameter. For 60 degrees threads, the formula is D' = D - pitch */

        diameter = diameter - pitch;
    }
    else {
        switch ( ui->threadFit->currentIndex() ) {
        case 0: /* standard */
            diameter = ( 5 * ( (int)( ( diameter * 110 ) / 5 ) ) ) / 100.0;
            break;
        case 1: /* close */
            diameter = ( 5 * ( (int)( ( diameter * 105 ) / 5 ) ) ) / 100.0;
            break;
        default:
            assert( 0 );
        }
    }

    ui->threadDiameter->setValue( diameter );
    updateHoleCutParams();
    recomputeFeature();
}

void TaskHoleParameters::threadClassChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadClass.setValue(index);
    recomputeFeature();
}

void TaskHoleParameters::threadDiameterChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Diameter.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::threadFitChanged(int index)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadFit.setValue(index);
    threadSizeChanged( pcHole->ThreadSize.getValue() );
    recomputeFeature();
}

void TaskHoleParameters::threadDirectionChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    if (sender() == ui->directionRightHand)
        pcHole->ThreadDirection.setValue((long)0);
    else
        pcHole->ThreadDirection.setValue((long)1);
    recomputeFeature();
}

void TaskHoleParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskHoleParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    Q_UNUSED(msg)
}

bool   TaskHoleParameters::getThreaded() const {
    return ui->threaded->isChecked();
}

long   TaskHoleParameters::getThreadType() const {
    return ui->threadType->currentIndex();
}

long   TaskHoleParameters::getThreadSize() const {
    if ( ui->threadSize->currentIndex() == -1 )
        return 0;
    else
        return ui->threadSize->currentIndex();
}

long   TaskHoleParameters::getThreadClass() const {
    if ( ui->threadSize->currentIndex() == -1 )
        return 0;
    else
        return ui->threadClass->currentIndex();
}

long TaskHoleParameters::getThreadFit() const
{
    if (ui->threaded->isChecked())
        return ui->threadFit->currentIndex();
    else
        return 0;
}

double TaskHoleParameters::getDiameter() const
{
    return ui->threadDiameter->value();
}

bool   TaskHoleParameters::getThreadDirection() const {
    return ui->directionRightHand->isChecked();
}

long   TaskHoleParameters::getHoleCutType() const {
    if (ui->holeCutType->currentIndex() == -1)
        return 0;
    else
        return ui->holeCutType->currentIndex();
}

double TaskHoleParameters::getHoleCutDiameter() const {
    return ui->holeCutDiameter->value();
}

double TaskHoleParameters::getHoleCutDepth() const {
    return ui->holeCutDepth->value();
}

double TaskHoleParameters::getHoleCutCountersinkAngle() const {
    return ui->holeCutCountersinkAngle->value();
}

long   TaskHoleParameters::getType() const {
    return ui->depth->currentIndex();
}

double TaskHoleParameters::getLength() const {
    return ui->depthValue->value();
}

long   TaskHoleParameters::getDrillPoint() const {
    if ( ui->drillPointFlat->isChecked() )
        return 0;
    if ( ui->drillPointAngled->isChecked() )
        return 1;
    assert( 0 );
}

double TaskHoleParameters::getDrillPointAngle() const {
    return ui->drillPointAngledValue->value();
}

bool   TaskHoleParameters::getTapered() const {
    return ui->tapered->isChecked();
}

double TaskHoleParameters::getTaperedAngle() const {
    return ui->taperedAngle->value();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgHoleParameters::TaskDlgHoleParameters(ViewProviderHole *HoleView)
    : TaskDlgSketchBasedParameters(HoleView)
{
    assert(HoleView);
    parameter  = new TaskHoleParameters(static_cast<ViewProviderHole*>(vp));

    Content.push_back(parameter);
}

TaskDlgHoleParameters::~TaskDlgHoleParameters()
{

}

//==== calls from the TaskView ===============================================================

bool TaskDlgHoleParameters::accept()
{
    std::string name = vp->getObject()->getNameInDocument();

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Threaded = %u",name.c_str(), parameter->getThreaded());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ThreadType = %i",name.c_str(), parameter->getThreadType());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ThreadSize = %u", name.c_str(), parameter->getThreadSize());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ThreadClass = %u", name.c_str(), parameter->getThreadClass());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ThreadFit = %u", name.c_str(), parameter->getThreadFit());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Diameter = %f", name.c_str(), parameter->getDiameter());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ThreadDirection = %u", name.c_str(), parameter->getThreadDirection());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.HoleCutType = %u", name.c_str(), parameter->getHoleCutType());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.HoleCutDiameter = %f", name.c_str(), parameter->getHoleCutDiameter());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.HoleCutDepth = %f", name.c_str(), parameter->getHoleCutDepth());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u", name.c_str(), parameter->getType());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f", name.c_str(), parameter->getLength());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.DrillPoint = %u", name.c_str(), parameter->getDrillPoint());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.DrillPointAngle = %f", name.c_str(), parameter->getDrillPointAngle());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Tapered = %u", name.c_str(), parameter->getTapered());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.TaperedAngle = %f", name.c_str(), parameter->getTaperedAngle());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!vp->getObject()->isValid())
            throw Base::Exception(vp->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;}




#include "moc_TaskHoleParameters.cpp"
