/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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

#include "ui_TaskFemConstraintBearing.h"
#include "TaskFemConstraintPulley.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyGeo.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/Fem/App/FemConstraintPulley.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>

using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintPulley */

TaskFemConstraintPulley::TaskFemConstraintPulley(ViewProviderFemConstraintPulley *ConstraintView,QWidget *parent)
    : TaskFemConstraintGear(ConstraintView, parent, "Fem_ConstraintPulley")
{
    connect(ui->spinOtherDiameter, SIGNAL(valueChanged(double)),
            this, SLOT(onOtherDiameterChanged(double)));
    connect(ui->spinCenterDistance, SIGNAL(valueChanged(double)),
            this, SLOT(onCenterDistanceChanged(double)));
    connect(ui->checkIsDriven, SIGNAL(toggled(bool)),
            this, SLOT(onCheckIsDriven(bool)));
    connect(ui->spinTensionForce, SIGNAL(valueChanged(double)),
            this, SLOT(onTensionForceChanged(double)));

    // Temporarily prevent unnecessary feature recomputes
    ui->spinOtherDiameter->blockSignals(true);
    ui->spinCenterDistance->blockSignals(true);
    ui->checkIsDriven->blockSignals(true);
    ui->spinTensionForce->blockSignals(true);

    // Get the feature data
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    double otherdia = pcConstraint->OtherDiameter.getValue();
    double centerdist = pcConstraint->CenterDistance.getValue();
    bool isdriven = pcConstraint->IsDriven.getValue();
    double tensionforce = pcConstraint->TensionForce.getValue();

    // Fill data into dialog elements
    ui->spinOtherDiameter->setMinimum(0);
    ui->spinOtherDiameter->setMaximum(FLOAT_MAX);
    ui->spinOtherDiameter->setValue(otherdia);
    ui->spinCenterDistance->setMinimum(0);
    ui->spinCenterDistance->setMaximum(FLOAT_MAX);
    ui->spinCenterDistance->setValue(centerdist);
    ui->checkIsDriven->setChecked(isdriven);
    ui->spinForce->setMinimum(-FLOAT_MAX);
    ui->spinTensionForce->setMinimum(0);
    ui->spinTensionForce->setMaximum(FLOAT_MAX);
    ui->spinTensionForce->setValue(tensionforce);

    // Adjust ui
    ui->buttonDirection->setVisible(false);
    ui->lineDirection->setVisible(false);
    ui->checkReversed->setVisible(false);
    ui->labelDiameter->setText(tr("Pulley diameter"));
    ui->labelForce->setText(tr("Torque [Nm]"));
    ui->labelOtherDiameter->setVisible(true);
    ui->spinOtherDiameter->setVisible(true);
    ui->labelCenterDistance->setVisible(true);
    ui->spinCenterDistance->setVisible(true);
    ui->checkIsDriven->setVisible(true);
    ui->labelTensionForce->setVisible(true);
    ui->spinTensionForce->setVisible(true);

    ui->spinOtherDiameter->blockSignals(false);
    ui->spinCenterDistance->blockSignals(false);
    ui->checkIsDriven->blockSignals(false);
    ui->spinTensionForce->blockSignals(false);
}

void TaskFemConstraintPulley::onOtherDiameterChanged(double l)
{
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    pcConstraint->OtherDiameter.setValue(l);
}

void TaskFemConstraintPulley::onCenterDistanceChanged(double l)
{
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    pcConstraint->CenterDistance.setValue(l);
}

void TaskFemConstraintPulley::onTensionForceChanged(double force)
{
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    pcConstraint->TensionForce.setValue(force);
}

void TaskFemConstraintPulley::onCheckIsDriven(const bool pressed)
{
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    pcConstraint->IsDriven.setValue(pressed);
}

double TaskFemConstraintPulley::getTorque(void) const
{
    return ui->spinForce->value();
}

double TaskFemConstraintPulley::getTensionForce(void) const
{
    return ui->spinTensionForce->value();
}

bool TaskFemConstraintPulley::getIsDriven() const
{
    return ui->checkIsDriven->isChecked();
}

double TaskFemConstraintPulley::getOtherDiameter(void) const
{
    return ui->spinOtherDiameter->value();
}

double TaskFemConstraintPulley::getCenterDistance(void) const
{
    return ui->spinCenterDistance->value();
}

void TaskFemConstraintPulley::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinOtherDiameter->blockSignals(true);
        ui->spinCenterDistance->blockSignals(true);
        ui->checkIsDriven->blockSignals(true);
        ui->spinTensionForce->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->spinOtherDiameter->blockSignals(false);
        ui->spinCenterDistance->blockSignals(false);
        ui->checkIsDriven->blockSignals(false);
        ui->spinTensionForce->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintPulley::TaskDlgFemConstraintPulley(ViewProviderFemConstraintPulley *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintPulley(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintPulley::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintPulley* parameterPulley = static_cast<const TaskFemConstraintPulley*>(parameter);

    try {
        //Gui::Command::openCommand("FEM pulley constraint changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.OtherDiameter = %f",name.c_str(), parameterPulley->getOtherDiameter());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.CenterDistance = %f",name.c_str(), parameterPulley->getCenterDistance());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.IsDriven = %s",name.c_str(), parameterPulley->getIsDriven() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.TensionForce = %f",name.c_str(), parameterPulley->getTensionForce());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return TaskDlgFemConstraintGear::accept();
}

#include "moc_TaskFemConstraintPulley.cpp"
