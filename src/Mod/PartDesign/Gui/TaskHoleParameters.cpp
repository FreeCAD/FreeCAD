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
# include <QMessageBox>
# include <boost_bind_bind.hpp>
#endif

#include "ui_TaskHoleParameters.h"
#include "TaskHoleParameters.h"
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/PartDesign/App/FeatureHole.h>

using namespace PartDesignGui;
using namespace Gui;
namespace bp = boost::placeholders;

/* TRANSLATOR PartDesignGui::TaskHoleParameters */

// See Hole::HoleCutType_ISOmetric_Enums
// and Hole::HoleCutType_ISOmetricfine_Enums
#if 0 // needed for Qt's lupdate utility
    qApp->translate("PartDesignGui::TaskHoleParameters", "Counterbore");
    qApp->translate("PartDesignGui::TaskHoleParameters", "Countersink");
    qApp->translate("PartDesignGui::TaskHoleParameters", "Cheesehead (deprecated)");
    qApp->translate("PartDesignGui::TaskHoleParameters", "Countersink socket screw (deprecated)");
    qApp->translate("PartDesignGui::TaskHoleParameters", "Cap screw (deprecated)");
#endif

TaskHoleParameters::TaskHoleParameters(ViewProviderHole *HoleView, QWidget *parent)
    : TaskSketchBasedParameters(HoleView, parent, "PartDesign_Hole",tr("Hole parameters"))
    , observer(new Observer(this, static_cast<PartDesign::Hole*>(vp->getObject())))
    , isApplying(false)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskHoleParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    /* Remove actual threading parameters for now */
    ui->ModelActualThread->setVisible(false);
    ui->ThreadPitch->setVisible(false);
    ui->ThreadCutOffInner->setVisible(false);
    ui->ThreadCutOffOuter->setVisible(false);
    ui->ThreadAngle->setVisible(false);
    ui->label_Pitch->setVisible(false);
    ui->label_CutoffInner->setVisible(false);
    ui->label_CutoffOuter->setVisible(false);
    ui->label_Angle->setVisible(false);

    ui->ThreadType->addItem(tr("None"), QByteArray("None"));
    ui->ThreadType->addItem(tr("ISO metric regular profile"), QByteArray("ISO"));
    ui->ThreadType->addItem(tr("ISO metric fine profile"), QByteArray("ISO"));
    ui->ThreadType->addItem(tr("UTS coarse profile"), QByteArray("UTS"));
    ui->ThreadType->addItem(tr("UTS fine profile"), QByteArray("UTS"));
    ui->ThreadType->addItem(tr("UTS extra fine profile"), QByteArray("UTS"));

    // read values from the hole properties
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());
    ui->Threaded->setChecked(pcHole->Threaded.getValue());
    ui->ModelActualThread->setChecked(pcHole->ModelActualThread.getValue());
    ui->ThreadPitch->setValue(pcHole->ThreadPitch.getValue());
    ui->ThreadAngle->setValue(pcHole->ThreadAngle.getValue());
    ui->ThreadCutOffInner->setValue(pcHole->ThreadCutOffInner.getValue());
    ui->ThreadCutOffOuter->setValue(pcHole->ThreadCutOffOuter.getValue());
    ui->ThreadType->setCurrentIndex(pcHole->ThreadType.getValue());
    ui->ThreadSize->clear();
    const char** cursor = pcHole->ThreadSize.getEnums();
    while (*cursor) {
        ui->ThreadSize->addItem(tr(*cursor));
        ++cursor;
    }
    ui->ThreadSize->setCurrentIndex(pcHole->ThreadSize.getValue());
    ui->ThreadClass->clear();
    cursor = pcHole->ThreadClass.getEnums();
    while (*cursor) {
        ui->ThreadClass->addItem(tr(*cursor));
        ++cursor;
    }
    ui->ThreadClass->setCurrentIndex(pcHole->ThreadClass.getValue());
    // Class is only enabled (sensible) if threaded
    ui->ThreadClass->setEnabled(pcHole->Threaded.getValue());
    ui->ThreadFit->setCurrentIndex(pcHole->ThreadFit.getValue());
    // Fit is only enabled (sensible) if not threaded
    ui->ThreadFit->setEnabled(!pcHole->Threaded.getValue());
    ui->Diameter->setValue(pcHole->Diameter.getValue());
    // Diameter is only enabled if ThreadType is None
    if (pcHole->ThreadType.getValue() != 0L)
        ui->Diameter->setEnabled(false);
    if (pcHole->ThreadDirection.getValue() == 0L)
        ui->directionRightHand->setChecked(true);
    else
        ui->directionLeftHand->setChecked(true);
    // ThreadDirection is only sensible if there is a thread
    ui->directionRightHand->setEnabled(pcHole->Threaded.getValue());
    ui->directionLeftHand->setEnabled(pcHole->Threaded.getValue());
    ui->HoleCutType->clear();
    cursor = pcHole->HoleCutType.getEnums();
    while (*cursor) {
        ui->HoleCutType->addItem(tr(*cursor));
        ++cursor;
    }
    ui->HoleCutType->setCurrentIndex(pcHole->HoleCutType.getValue());
    ui->HoleCutDiameter->setValue(pcHole->HoleCutDiameter.getValue());
    ui->HoleCutDepth->setValue(pcHole->HoleCutDepth.getValue());
    ui->HoleCutCountersinkAngle->setValue(pcHole->HoleCutCountersinkAngle.getValue());

    std::string holeCutType;
    if (pcHole->HoleCutType.isValid())
        holeCutType = pcHole->HoleCutType.getValueAsString();

    if (holeCutType == "None") {
        ui->HoleCutDiameter->setEnabled(false);
        ui->HoleCutDepth->setEnabled(false);
        ui->HoleCutCountersinkAngle->setEnabled(false);
    }
    else if (holeCutType == "Counterbore") {
        ui->HoleCutDiameter->setEnabled(true);
        ui->HoleCutDepth->setEnabled(true);
        ui->HoleCutCountersinkAngle->setEnabled(false);
    }
    else if (holeCutType == "Countersink") {
        ui->HoleCutDiameter->setEnabled(true);
        ui->HoleCutDepth->setEnabled(true);
        ui->HoleCutCountersinkAngle->setEnabled(true);
    }
    else { // screw definition
        ui->HoleCutDiameter->setEnabled(true);
        ui->HoleCutDepth->setEnabled(true);
        ui->HoleCutCountersinkAngle->setEnabled(true);
    }

    ui->DepthType->setCurrentIndex(pcHole->DepthType.getValue());
    ui->Depth->setValue(pcHole->Depth.getValue());
    if (pcHole->DrillPoint.getValue() == 0L)
        ui->drillPointFlat->setChecked(true);
    else
        ui->drillPointAngled->setChecked(true);
    ui->DrillPointAngle->setValue(pcHole->DrillPointAngle.getValue());
    ui->Tapered->setChecked(pcHole->Tapered.getValue());
    // Angle is only enabled (sensible) if tapered
    ui->TaperedAngle->setEnabled(pcHole->Tapered.getValue());
    ui->TaperedAngle->setValue(pcHole->TaperedAngle.getValue());
    ui->Reversed->setChecked(pcHole->Reversed.getValue());

    connect(ui->Threaded, SIGNAL(clicked(bool)), this, SLOT(threadedChanged()));
    connect(ui->ThreadType, SIGNAL(currentIndexChanged(int)), this, SLOT(threadTypeChanged(int)));
    connect(ui->ModelActualThread, SIGNAL(clicked(bool)), this, SLOT(modelActualThreadChanged()));
    connect(ui->ThreadPitch, SIGNAL(valueChanged(double)), this, SLOT(threadPitchChanged(double)));
    connect(ui->ThreadAngle, SIGNAL(valueChanged(double)), this, SLOT(threadAngleChanged(double)));
    connect(ui->ThreadCutOffInner, SIGNAL(valueChanged(double)), this, SLOT(threadCutOffInnerChanged(double)));
    connect(ui->ThreadCutOffOuter, SIGNAL(valueChanged(double)), this, SLOT(threadCutOffOuterChanged(double)));
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
    connect(ui->Reversed, SIGNAL(clicked(bool)), this, SLOT(reversedChanged()));
    connect(ui->TaperedAngle, SIGNAL(valueChanged(double)), this, SLOT(taperedAngleChanged(double)));

    vp->show();

    ui->ThreadPitch->bind(pcHole->ThreadPitch);
    ui->ThreadAngle->bind(pcHole->ThreadAngle);
    ui->ThreadCutOffInner->bind(pcHole->ThreadCutOffInner);
    ui->ThreadCutOffOuter->bind(pcHole->ThreadCutOffOuter);
    ui->Diameter->bind(pcHole->Diameter);
    ui->HoleCutDiameter->bind(pcHole->HoleCutDiameter);
    ui->HoleCutDepth->bind(pcHole->HoleCutDepth);
    ui->HoleCutCountersinkAngle->bind(pcHole->HoleCutCountersinkAngle);
    ui->Depth->bind(pcHole->Depth);
    ui->DrillPointAngle->bind(pcHole->DrillPointAngle);
    ui->TaperedAngle->bind(pcHole->TaperedAngle);

    connectPropChanged = App::GetApplication().signalChangePropertyEditor.connect(
            boost::bind(&TaskHoleParameters::changedObject, this, bp::_1, bp::_2));

    this->groupLayout()->addWidget(proxy);
}

TaskHoleParameters::~TaskHoleParameters()
{
    delete ui;
}

void TaskHoleParameters::threadedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Threaded.setValue(ui->Threaded->isChecked());
    recomputeFeature();
}

void TaskHoleParameters::modelActualThreadChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ModelActualThread.setValue(ui->ModelActualThread->isChecked());
    recomputeFeature();
}

void TaskHoleParameters::threadPitchChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadPitch.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::threadAngleChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadAngle.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::threadCutOffInnerChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadCutOffInner.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::threadCutOffOuterChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadCutOffOuter.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::holeCutChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    // the HoleCutDepth is something different for countersinks and counterbores
    // therefore reset it, it will be reset to sensible values by setting the new HoleCutType 
    pcHole->HoleCutDepth.setValue(0.0);

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

    if (ui->HoleCutCountersinkAngle->isEnabled()){
        // we have a countersink and recalculate the HoleCutDiameter

        // store current depth
        double DepthDifference = value - pcHole->HoleCutDepth.getValue();
        // new diameter is the old one + 2*tan(angle/2)*DepthDifference
        double newDiameter = pcHole->HoleCutDiameter.getValue()
            + 2 * tan(Base::toRadians(pcHole->HoleCutCountersinkAngle.getValue() / 2)) * DepthDifference;
        // only apply if the result is not smaller than the hole diameter
        if (newDiameter > pcHole->Diameter.getValue()) {
            pcHole->HoleCutDiameter.setValue(newDiameter);
            pcHole->HoleCutDepth.setValue(value);
        }
    }
    else {
        pcHole->HoleCutDepth.setValue(value);
    }

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

void TaskHoleParameters::reversedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Reversed.setValue(ui->Reversed->isChecked());
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

    // A typical case is that users change from an ISO profile to another one.
    // When they had e.g. the size "M3" in one profile they expect
    // the same size in the other profile if it exists there.
    // Besides the size also the thread class" and hole cut type are affected.

    // at first check what type class is used
    QByteArray TypeClass = ui->ThreadType->itemData(index).toByteArray();

    // store the current size
    QString ThreadSizeString = ui->ThreadSize->currentText();
    // store the current class
    QString ThreadClassString = ui->ThreadClass->currentText();
    // store the current type
    QString CutTypeString = ui->HoleCutType->currentText();

    // now set the new type, this will reset the comboboxes to item 0
    pcHole->ThreadType.setValue(index);

    // Size
    // the size for ISO type has either the form "M3x0.35" or just "M3"
    // so we need to check if the size contains a 'x'. If yes, check if the string
    // up to the 'x' is exists in the new list
    if (TypeClass == QByteArray("ISO")) {
        if (ThreadSizeString.indexOf(QString::fromLatin1("x")) > -1) {
            // we have an ISO fine size
            // cut of the part behind the 'x'
            ThreadSizeString = ThreadSizeString.left(ThreadSizeString.indexOf(QString::fromLatin1("x")));
        }

        // search if the string exists in the combobox
        int threadSizeIndex = ui->ThreadSize->findText(ThreadSizeString, Qt::MatchContains);
        if (threadSizeIndex > -1) {
            // we can set it
            ui->ThreadSize->setCurrentIndex(threadSizeIndex);
        }
    }

    // for the UTS types the entries are the same
    if (TypeClass == QByteArray("UTS")) {
        int threadSizeIndex = ui->ThreadSize->findText(ThreadSizeString, Qt::MatchContains);
        if (threadSizeIndex > -1) {
            ui->ThreadSize->setCurrentIndex(threadSizeIndex);
        }
    }

    // Class and cut type
    // the class and cut types are the same for both TypeClass so we don't need to distinguish between ISO and UTS
    int threadClassIndex = ui->ThreadClass->findText(ThreadClassString, Qt::MatchContains);
    if (threadClassIndex > -1)
        ui->ThreadClass->setCurrentIndex(threadClassIndex);
    int holeCutIndex = ui->HoleCutType->findText(CutTypeString, Qt::MatchContains);
    if (holeCutIndex > -1)
        ui->HoleCutType->setCurrentIndex(holeCutIndex);
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

void TaskHoleParameters::changedObject(const App::Document&, const App::Property &Prop)
{
    // happens when aborting the command
    if (vp == nullptr)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());
    bool ro = Prop.isReadOnly();

    Base::Console().Log("Parameter %s was updated\n", Prop.getName());

    if (&Prop == &pcHole->Threaded) {
        ui->Threaded->setEnabled(true);
        if (ui->Threaded->isChecked() ^ pcHole->Threaded.getValue()) {
            ui->Threaded->blockSignals(true);
            ui->Threaded->setChecked(pcHole->Threaded.getValue());
            ui->Threaded->blockSignals(false);
        }
        ui->Threaded->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ModelActualThread) {
        if (ui->ModelActualThread->isChecked() ^ pcHole->ModelActualThread.getValue()) {
            ui->ModelActualThread->blockSignals(true);
            ui->ModelActualThread->setChecked(pcHole->ModelActualThread.getValue());
            ui->ModelActualThread->blockSignals(false);
        }
        ui->ModelActualThread->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadPitch) {
        if (ui->ThreadPitch->value().getValue() != pcHole->ThreadPitch.getValue()) {
            ui->ThreadPitch->blockSignals(true);
            ui->ThreadPitch->setValue(pcHole->ThreadPitch.getValue());
            ui->ThreadPitch->blockSignals(false);
        }
        ui->ThreadPitch->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadAngle) {
        if (ui->ThreadAngle->value().getValue() != pcHole->ThreadAngle.getValue()) {
            ui->ThreadAngle->blockSignals(true);
            ui->ThreadAngle->setValue(pcHole->ThreadAngle.getValue());
            ui->ThreadAngle->blockSignals(false);
        }
        ui->ThreadAngle->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadCutOffInner) {
        if (ui->ThreadCutOffInner->value().getValue() != pcHole->ThreadCutOffInner.getValue()) {
            ui->ThreadCutOffInner->blockSignals(true);
            ui->ThreadCutOffInner->setValue(pcHole->ThreadCutOffInner.getValue());
            ui->ThreadCutOffInner->blockSignals(false);
        }
        ui->ThreadCutOffInner->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadCutOffOuter) {
        if (ui->ThreadCutOffOuter->value().getValue() != pcHole->ThreadCutOffOuter.getValue()) {
            ui->ThreadCutOffOuter->blockSignals(true);
            ui->ThreadCutOffOuter->setValue(pcHole->ThreadCutOffOuter.getValue());
            ui->ThreadCutOffOuter->blockSignals(false);
        }
        ui->ThreadCutOffOuter->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadType) {
        ui->ThreadType->setEnabled(true);

        ui->ThreadSize->blockSignals(true);
        ui->ThreadSize->clear();
        const char** cursor = pcHole->ThreadSize.getEnums();
        while (*cursor) {
            ui->ThreadSize->addItem(QString::fromLatin1(*cursor));
            ++cursor;
        }
        ui->ThreadSize->setCurrentIndex(pcHole->ThreadSize.getValue());
        ui->ThreadSize->blockSignals(false);

        // Thread type also updates HoleCutType and ThreadClass
        ui->HoleCutType->blockSignals(true);
        ui->HoleCutType->clear();
        cursor = pcHole->HoleCutType.getEnums();
        while (*cursor) {
            ui->HoleCutType->addItem(QString::fromLatin1(*cursor));
            ++cursor;
        }
        ui->HoleCutType->setCurrentIndex(pcHole->HoleCutType.getValue());
        ui->HoleCutType->blockSignals(false);

        ui->ThreadClass->blockSignals(true);
        ui->ThreadClass->clear();
        cursor = pcHole->ThreadClass.getEnums();
        while (*cursor) {
            ui->ThreadClass->addItem(QString::fromLatin1(*cursor));
            ++cursor;
        }
        ui->ThreadClass->setCurrentIndex(pcHole->ThreadClass.getValue());
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
        if (ui->Tapered->isChecked() ^ pcHole->Tapered.getValue()) {
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

bool TaskHoleParameters::getThreaded() const
{
    return ui->Threaded->isChecked();
}

long TaskHoleParameters::getThreadType() const
{
    return ui->ThreadType->currentIndex();
}

long TaskHoleParameters::getThreadSize() const
{
    if ( ui->ThreadSize->currentIndex() == -1 )
        return 0;
    else
        return ui->ThreadSize->currentIndex();
}

long TaskHoleParameters::getThreadClass() const
{
    if ( ui->ThreadSize->currentIndex() == -1 )
        return 0;
    else
        return ui->ThreadClass->currentIndex();
}

long TaskHoleParameters::getThreadFit() const
{
    // the fit (clearance) is independent if the hole is threaded or not
    // since an unthreaded hole for a screw can also have a close fit
    return ui->ThreadFit->currentIndex();
}

Base::Quantity TaskHoleParameters::getDiameter() const
{
    return ui->Diameter->value();
}

long TaskHoleParameters::getThreadDirection() const
{
    if (ui->directionRightHand->isChecked())
        return 0;
    else
        return 1;
}

long TaskHoleParameters::getHoleCutType() const
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

long TaskHoleParameters::getDepthType() const
{
    return ui->DepthType->currentIndex();
}

Base::Quantity TaskHoleParameters::getDepth() const
{
    return ui->Depth->value();
}

long TaskHoleParameters::getDrillPoint() const
{
    if ( ui->drillPointFlat->isChecked() )
        return 0;
    if ( ui->drillPointAngled->isChecked() )
        return 1;
    assert( 0 );
    return -1; // to avoid a compiler warning
}

Base::Quantity TaskHoleParameters::getDrillPointAngle() const
{
    return ui->DrillPointAngle->value();
}

bool TaskHoleParameters::getTapered() const
{
    return ui->Tapered->isChecked();
}

Base::Quantity TaskHoleParameters::getTaperedAngle() const
{
    return ui->TaperedAngle->value();
}

void TaskHoleParameters::apply()
{
    auto obj = vp->getObject();
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    isApplying = true;

    ui->ThreadPitch->apply();
    ui->ThreadAngle->apply();
    ui->ThreadCutOffInner->apply();
    ui->ThreadCutOffOuter->apply();
    ui->Diameter->apply();
    ui->HoleCutDiameter->apply();
    ui->HoleCutDepth->apply();
    ui->HoleCutCountersinkAngle->apply();
    ui->Depth->apply();
    ui->DrillPointAngle->apply();
    ui->TaperedAngle->apply();

    if (!pcHole->Threaded.isReadOnly())
        FCMD_OBJ_CMD(obj,"Threaded = " << (getThreaded() ? 1 : 0));
    if (!pcHole->ModelActualThread.isReadOnly())
        FCMD_OBJ_CMD(obj,"ModelActualThread = " << (getThreaded() ? 1 : 0));
    if (!pcHole->ThreadType.isReadOnly())
        FCMD_OBJ_CMD(obj,"ThreadType = " << getThreadType());
    if (!pcHole->ThreadSize.isReadOnly())
        FCMD_OBJ_CMD(obj,"ThreadSize = " << getThreadSize());
    if (!pcHole->ThreadClass.isReadOnly())
        FCMD_OBJ_CMD(obj,"ThreadClass = " << getThreadClass());
    if (!pcHole->ThreadFit.isReadOnly())    
        FCMD_OBJ_CMD(obj,"ThreadFit = " << getThreadFit());
    if (!pcHole->ThreadDirection.isReadOnly())
        FCMD_OBJ_CMD(obj,"ThreadDirection = " << getThreadDirection());
    if (!pcHole->HoleCutType.isReadOnly())
        FCMD_OBJ_CMD(obj,"HoleCutType = " << getHoleCutType());
    if (!pcHole->DepthType.isReadOnly())
        FCMD_OBJ_CMD(obj,"DepthType = " << getDepthType());
    if (!pcHole->DrillPoint.isReadOnly())
        FCMD_OBJ_CMD(obj,"DrillPoint = " << getDrillPoint());
    if (!pcHole->Tapered.isReadOnly())
        FCMD_OBJ_CMD(obj,"Tapered = " << getTapered());

    isApplying = false;
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
        if(Obj.getDocument())
            owner->changedObject(*Obj.getDocument(),Prop);
    }
}
