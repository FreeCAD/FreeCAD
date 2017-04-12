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
    , observer(new Observer(this, static_cast<PartDesign::Hole*>(vp->getObject())))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskHoleParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    ui->ThreadType->addItem(tr("None"));
    ui->ThreadType->addItem(tr("ISO metric coarse profile"));
    ui->ThreadType->addItem(tr("ISO metric fine profile"));
    ui->ThreadType->addItem(tr("UTS coarse profile"));
    ui->ThreadType->addItem(tr("UTS fine profile"));
    ui->ThreadType->addItem(tr("UTS extra fine profile"));

    connect(ui->Threaded, SIGNAL(clicked(bool)), this, SLOT(threadedChanged()));
    connect(ui->ThreadType, SIGNAL(currentIndexChanged(int)), this, SLOT(threadTypeChanged(int)));
    connect(ui->ThreadSize, SIGNAL(currentIndexChanged(int)), this, SLOT(threadSizeChanged(int)));
    connect(ui->ThreadClass, SIGNAL(currentIndexChanged(int)), this, SLOT(threadClassChanged(int)));
    connect(ui->ThreadFit, SIGNAL(currentIndexChanged(int)), this, SLOT(threadFitChanged(int)));
    connect(ui->Diameter, SIGNAL(valueChanged(double)), this, SLOT(threadDiameterChanged(double)));
    connect(ui->directionRightHand, SIGNAL(clicked(bool)), this, SLOT(threadDirectionChanged()));
    connect(ui->directionLeftHand, SIGNAL(clicked(bool)), this, SLOT(threadDirectionChanged()));
    connect(ui->HoleCutType, SIGNAL(currentIndexChanged(int)), this, SLOT(holeCutChanged(int)));
    connect(ui->HoleCutDiameter, SIGNAL(valueChanged(double)), this, SLOT(holeCutDiameterChanged(double)));
    connect(ui->HoleCutDepth, SIGNAL(valueChanged(double)), this, SLOT(holeCutDepthChanged(double)));
    connect(ui->HoleCutCountersinkAngle, SIGNAL(valueChanged(double)), this, SLOT(holeCutCountersinkAngleChanged(double)));
    connect(ui->DepthType, SIGNAL(currentIndexChanged(int)), this, SLOT(depthChanged(int)));
    connect(ui->Depth, SIGNAL(valueChanged(double)), this, SLOT(depthValueChanged(double)));
    connect(ui->drillPointFlat, SIGNAL(clicked(bool)), this, SLOT(drillPointChanged()));
    connect(ui->drillPointAngled, SIGNAL(clicked(bool)), this, SLOT(drillPointChanged()));
    connect(ui->DrillPointAngle, SIGNAL(valueChanged(double)), this, SLOT(drillPointAngledValueChanged(double)));
    connect(ui->Tapered, SIGNAL(clicked(bool)), this, SLOT(taperedChanged()));
    connect(ui->TaperedAngle, SIGNAL(valueChanged(double)), this, SLOT(taperedAngleChanged(double)));

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    changedObject(pcHole->Threaded);
    changedObject(pcHole->ThreadType);
    changedObject(pcHole->ThreadSize);
    changedObject(pcHole->ThreadClass);
    changedObject(pcHole->ThreadFit);
    changedObject(pcHole->Diameter);
    changedObject(pcHole->ThreadDirection);
    changedObject(pcHole->HoleCutType);
    changedObject(pcHole->HoleCutDiameter);
    changedObject(pcHole->HoleCutDepth);
    changedObject(pcHole->HoleCutCountersinkAngle);
    changedObject(pcHole->DepthType);
    changedObject(pcHole->Depth);
    changedObject(pcHole->DrillPoint);
    changedObject(pcHole->DrillPointAngle);
    changedObject(pcHole->Tapered);
    changedObject(pcHole->TaperedAngle);

//    updateUi();

    connectPropChanged = App::GetApplication().signalChangePropertyEditor.connect(boost::bind(&TaskHoleParameters::changedObject, this, _1));

    this->groupLayout()->addWidget(proxy);
}

TaskHoleParameters::~TaskHoleParameters()
{
    delete ui;
}

void TaskHoleParameters::updateUi()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    ui->Threaded->setDisabled(pcHole->ThreadType.isReadOnly());
    ui->ThreadSize->setDisabled(pcHole->ThreadSize.isReadOnly());
    ui->ThreadClass->setDisabled(pcHole->ThreadClass.isReadOnly());
    ui->ThreadFit->setDisabled(pcHole->ThreadFit.isReadOnly());
    ui->directionRightHand->setDisabled(pcHole->ThreadDirection.isReadOnly());
    ui->directionLeftHand->setDisabled(pcHole->ThreadDirection.isReadOnly());
    ui->Diameter->setDisabled(pcHole->Diameter.isReadOnly());
    ui->HoleCutDiameter->setDisabled(pcHole->HoleCutDiameter.isReadOnly());
    ui->HoleCutDepth->setDisabled(pcHole->HoleCutDepth.isReadOnly());
    ui->HoleCutCountersinkAngle->setDisabled(pcHole->HoleCutCountersinkAngle.isReadOnly());
    ui->Depth->setDisabled(pcHole->Depth.isReadOnly() );
    ui->DrillPointAngle->setDisabled(pcHole->DrillPointAngle.isReadOnly());
    ui->TaperedAngle->setDisabled( pcHole->TaperedAngle.isReadOnly() );
}

void TaskHoleParameters::threadedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Threaded.setValue(ui->Threaded->isChecked());
    recomputeFeature();
}

void TaskHoleParameters::holeCutChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutType.setValue(index);
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

void TaskHoleParameters::holeCutCountersinkAngleChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutCountersinkAngle.setValue((double)value);
    recomputeFeature();
}

void TaskHoleParameters::depthChanged(int index)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->DepthType.setValue(index);
    recomputeFeature();
}

void TaskHoleParameters::depthValueChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Depth.setValue(value);
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

void TaskHoleParameters::drillPointAngledValueChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->DrillPointAngle.setValue((double)value);
    recomputeFeature();
}

void TaskHoleParameters::taperedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Tapered.setValue(ui->Tapered->isChecked());
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
}

void TaskHoleParameters::threadSizeChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadSize.setValue(index);
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

void TaskHoleParameters::changedObject(const App::Property &Prop)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());
    bool ro = Prop.isReadOnly();

    Base::Console().Log("Parameter %s was updated\n", Prop.getName());

    if (&Prop == &pcHole->Threaded) {
        ui->Threaded->setEnabled(true);
        if (ui->Threaded->isChecked() && !pcHole->Threaded.getValue()) {
            ui->Threaded->blockSignals(true);
            ui->Threaded->setChecked(pcHole->Threaded.getValue());
            ui->Threaded->blockSignals(false);
        }
        ui->Threaded->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadType) {
        ui->ThreadType->setEnabled(true);

        ui->ThreadSize->blockSignals(true);
        ui->ThreadSize->clear();
        const char ** cursor = pcHole->ThreadSize.getEnums();
        while (*cursor) {
            ui->ThreadSize->addItem(tr(*cursor));
            ++cursor;
        }
        ui->ThreadSize->blockSignals(false);

        // Thread type also updates HoleCutType and ThreadClass
        ui->HoleCutType->blockSignals(true);
        ui->HoleCutType->clear();
        cursor = pcHole->HoleCutType.getEnums();
        while (*cursor) {
            ui->HoleCutType->addItem(tr(*cursor));
            ++cursor;
        }
        ui->HoleCutType->blockSignals(false);

        ui->ThreadClass->blockSignals(true);
        ui->ThreadClass->clear();
        cursor = pcHole->ThreadClass.getEnums();
        while (*cursor) {
            ui->ThreadClass->addItem(tr(*cursor));
            ++cursor;
        }
        ui->ThreadClass->blockSignals(false);

        if (ui->ThreadType->currentIndex() != pcHole->ThreadType.getValue()) {
            ui->ThreadType->blockSignals(true);
            ui->ThreadType->setCurrentIndex(pcHole->ThreadType.getValue());
            ui->ThreadType->blockSignals(false);
        }
        ui->ThreadType->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadSize) {
        ui->ThreadSize->setEnabled(true);
        if (ui->ThreadSize->currentIndex() != pcHole->ThreadSize.getValue()) {
            ui->ThreadSize->blockSignals(true);
            ui->ThreadSize->setCurrentIndex(pcHole->ThreadSize.getValue());
            ui->ThreadSize->blockSignals(false);
        }
        ui->ThreadSize->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadClass) {
        ui->ThreadClass->setEnabled(true);
        if (ui->ThreadClass->currentIndex() != pcHole->ThreadClass.getValue()) {
            ui->ThreadClass->blockSignals(true);
            ui->ThreadClass->setCurrentIndex(pcHole->ThreadClass.getValue());
            ui->ThreadClass->blockSignals(false);
        }
        ui->ThreadClass->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadFit) {
        ui->ThreadFit->setEnabled(true);
        if (ui->ThreadFit->currentIndex() != pcHole->ThreadFit.getValue()) {
            ui->ThreadFit->blockSignals(true);
            ui->ThreadFit->setCurrentIndex(pcHole->ThreadFit.getValue());
            ui->ThreadFit->blockSignals(false);
        }
        ui->ThreadFit->setDisabled(ro);
    }
    else if (&Prop == &pcHole->Diameter) {
        ui->Diameter->setEnabled(true);
        if (ui->Diameter->value().getValue() != pcHole->Diameter.getValue()) {
            ui->Diameter->blockSignals(true);
            ui->Diameter->setValue(pcHole->Diameter.getValue());
            ui->Diameter->blockSignals(false);
        }
        ui->Diameter->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadDirection) {
        ui->directionRightHand->setEnabled(true);
        ui->directionLeftHand->setEnabled(true);
        std::string direction(pcHole->ThreadDirection.getValueAsString());
        if (direction == "Right" && !ui->directionRightHand->isChecked()) {
            ui->directionRightHand->blockSignals(true);
            ui->directionRightHand->setChecked(true);
            ui->directionRightHand->blockSignals(false);
        }
        if (direction == "Left"  && !ui->directionLeftHand->isChecked()) {
            ui->directionLeftHand->blockSignals(true);
            ui->directionLeftHand->setChecked(true);
            ui->directionLeftHand->blockSignals(false);
        }
        ui->directionRightHand->setDisabled(ro);
        ui->directionLeftHand->setDisabled(ro);
    }
    else if (&Prop == &pcHole->HoleCutType) {
        ui->HoleCutType->setEnabled(true);
        if (ui->HoleCutType->currentIndex() != pcHole->HoleCutType.getValue()) {
            ui->HoleCutType->blockSignals(true);
            ui->HoleCutType->setCurrentIndex(pcHole->HoleCutType.getValue());
            ui->HoleCutType->blockSignals(false);
        }
        ui->HoleCutType->setDisabled(ro);
    }
    else if (&Prop == &pcHole->HoleCutDiameter) {
        ui->HoleCutDiameter->setEnabled(true);
        if (ui->HoleCutDiameter->value().getValue() != pcHole->HoleCutDiameter.getValue()) {
            ui->HoleCutDiameter->blockSignals(true);
            ui->HoleCutDiameter->setValue(pcHole->HoleCutDiameter.getValue());
            ui->HoleCutDiameter->blockSignals(false);
        }
        ui->HoleCutDiameter->setDisabled(ro);
    }
    else if (&Prop == &pcHole->HoleCutDepth) {
        ui->HoleCutDepth->setEnabled(true);
        if (ui->HoleCutDepth->value().getValue() != pcHole->HoleCutDepth.getValue()) {
            ui->HoleCutDepth->blockSignals(true);
            ui->HoleCutDepth->setValue(pcHole->HoleCutDepth.getValue());
            ui->HoleCutDepth->blockSignals(false);
        }
        ui->HoleCutDepth->setDisabled(ro);
    }
    else if (&Prop == &pcHole->HoleCutCountersinkAngle) {
        ui->HoleCutCountersinkAngle->setEnabled(true);
        if (ui->HoleCutCountersinkAngle->value().getValue() != pcHole->HoleCutCountersinkAngle.getValue()) {
            ui->HoleCutCountersinkAngle->blockSignals(true);
            ui->HoleCutCountersinkAngle->setValue(pcHole->HoleCutCountersinkAngle.getValue());
            ui->HoleCutCountersinkAngle->blockSignals(false);
        }
        ui->HoleCutCountersinkAngle->setDisabled(ro);
    }
    else if (&Prop == &pcHole->DepthType) {
        ui->DepthType->setEnabled(true);
        if (ui->DepthType->currentIndex() != pcHole->DepthType.getValue()) {
            ui->DepthType->blockSignals(true);
            ui->DepthType->setCurrentIndex(pcHole->DepthType.getValue());
            ui->DepthType->blockSignals(false);
        }
        ui->DepthType->setDisabled(ro);
    }
    else if (&Prop == &pcHole->Depth) {
        ui->Depth->setEnabled(true);
        if (ui->Depth->value().getValue() != pcHole->Depth.getValue()) {
            ui->Depth->blockSignals(true);
            ui->Depth->setValue(pcHole->Depth.getValue());
            ui->Depth->blockSignals(false);
        }
        ui->Depth->setDisabled(ro);
    }
    else if (&Prop == &pcHole->DrillPoint) {
        ui->drillPointFlat->setEnabled(true);
        ui->drillPointAngled->setEnabled(true);
        std::string drillPoint(pcHole->DrillPoint.getValueAsString());
        if (drillPoint == "Flat" && !ui->drillPointFlat->isChecked()) {
            ui->drillPointFlat->blockSignals(true);
            ui->drillPointFlat->setChecked(true);
            ui->drillPointFlat->blockSignals(false);
        }
        if (drillPoint == "Angled" && !ui->drillPointAngled->isChecked()) {
            ui->drillPointAngled->blockSignals(true);
            ui->drillPointAngled->setChecked(true);
            ui->drillPointAngled->blockSignals(false);
        }
        ui->drillPointFlat->setDisabled(ro);
        ui->drillPointAngled->setDisabled(ro);
    }
    else if (&Prop == &pcHole->DrillPointAngle) {
        ui->DrillPointAngle->setEnabled(true);
        if (ui->DrillPointAngle->value().getValue() != pcHole->DrillPointAngle.getValue()) {
            ui->DrillPointAngle->blockSignals(true);
            ui->DrillPointAngle->setValue(pcHole->DrillPointAngle.getValue());
            ui->DrillPointAngle->blockSignals(false);
        }
        ui->DrillPointAngle->setDisabled(ro);
    }
    else if (&Prop == &pcHole->Tapered) {
        ui->Tapered->setEnabled(true);
        if (ui->Tapered->isChecked() && !pcHole->Tapered.getValue()) {
            ui->Tapered->blockSignals(true);
            ui->Tapered->setChecked(pcHole->Tapered.getValue());
            ui->Tapered->blockSignals(false);
        }
        ui->Tapered->setDisabled(ro);
    }
    else if (&Prop == &pcHole->TaperedAngle) {
        ui->TaperedAngle->setEnabled(true);
        if (ui->TaperedAngle->value().getValue() != pcHole->TaperedAngle.getValue()) {
            ui->TaperedAngle->blockSignals(true);
            ui->TaperedAngle->setValue(pcHole->TaperedAngle.getValue());
            ui->TaperedAngle->blockSignals(false);
        }
        ui->TaperedAngle->setDisabled(ro);
    }
}

void TaskHoleParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    Q_UNUSED(msg)
}

bool   TaskHoleParameters::getThreaded() const
{
    return ui->Threaded->isChecked();
}

long   TaskHoleParameters::getThreadType() const
{
    return ui->ThreadType->currentIndex();
}

long   TaskHoleParameters::getThreadSize() const
{
    if ( ui->ThreadSize->currentIndex() == -1 )
        return 0;
    else
        return ui->ThreadSize->currentIndex();
}

long   TaskHoleParameters::getThreadClass() const
{
    if ( ui->ThreadSize->currentIndex() == -1 )
        return 0;
    else
        return ui->ThreadClass->currentIndex();
}

long TaskHoleParameters::getThreadFit() const
{
    if (ui->Threaded->isChecked())
        return ui->ThreadFit->currentIndex();
    else
        return 0;
}

Base::Quantity TaskHoleParameters::getDiameter() const
{
    return ui->Diameter->value();
}

bool   TaskHoleParameters::getThreadDirection() const
{
    return ui->directionRightHand->isChecked();
}

long   TaskHoleParameters::getHoleCutType() const
{
    if (ui->HoleCutType->currentIndex() == -1)
        return 0;
    else
        return ui->HoleCutType->currentIndex();
}

Base::Quantity TaskHoleParameters::getHoleCutDiameter() const
{
    return ui->HoleCutDiameter->value();
}

Base::Quantity TaskHoleParameters::getHoleCutDepth() const
{
    return ui->HoleCutDepth->value();
}

Base::Quantity TaskHoleParameters::getHoleCutCountersinkAngle() const
{
    return ui->HoleCutCountersinkAngle->value();
}

long   TaskHoleParameters::getType() const
{
    return ui->DepthType->currentIndex();
}

Base::Quantity TaskHoleParameters::getLength() const
{
    return ui->Depth->value();
}

long   TaskHoleParameters::getDrillPoint() const
{
    if ( ui->drillPointFlat->isChecked() )
        return 0;
    if ( ui->drillPointAngled->isChecked() )
        return 1;
    assert( 0 );
}

Base::Quantity TaskHoleParameters::getDrillPointAngle() const
{
    return ui->DrillPointAngle->value();
}

bool   TaskHoleParameters::getTapered() const
{
    return ui->Tapered->isChecked();
}

Base::Quantity TaskHoleParameters::getTaperedAngle() const
{
    return ui->TaperedAngle->value();
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
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.DepthType = %u", name.c_str(), parameter->getType());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Depth = %f", name.c_str(), parameter->getLength());
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
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

#include "moc_TaskHoleParameters.cpp"

TaskHoleParameters::Observer::Observer(TaskHoleParameters *_owner, PartDesign::Hole * _hole)
    : DocumentObserver(_hole->getDocument())
    , owner(_owner)
    , hole(_hole)
{
}

void TaskHoleParameters::Observer::slotChangedObject(const App::DocumentObject &Obj, const App::Property &Prop)
{
    if (&Obj == hole) {
        Base::Console().Log("Parameter %s was updated with a new value\n", Prop.getName());
        owner->changedObject(Prop);
    }
}
