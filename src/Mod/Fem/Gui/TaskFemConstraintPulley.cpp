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

#include "ui_TaskFemConstraintCylindrical.h"
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
    : TaskFemConstraintBearing(ConstraintView, parent, "Fem_ConstraintPulley")
{
    // we need a separate container widget to add all controls to
    connect(ui->spinDiameter, SIGNAL(valueChanged(double)),
            this, SLOT(onDiameterChanged(double)));
    connect(ui->spinOtherDiameter, SIGNAL(valueChanged(double)),
            this, SLOT(onOtherDiameterChanged(double)));
    connect(ui->spinCenterDistance, SIGNAL(valueChanged(double)),
            this, SLOT(onCenterDistanceChanged(double)));

    // Temporarily prevent unnecessary feature recomputes
    ui->spinDiameter->blockSignals(true);
    ui->spinOtherDiameter->blockSignals(true);
    ui->spinCenterDistance->blockSignals(true);

    // Get the feature data
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    double dia = pcConstraint->Diameter.getValue();
    double otherdia = pcConstraint->OtherDiameter.getValue();
    double centerdist = pcConstraint->CenterDistance.getValue();

    // Fill data into dialog elements
    ui->spinDiameter->setMinimum(0);
    ui->spinDiameter->setMaximum(INT_MAX);
    ui->spinDiameter->setValue(dia);
    ui->spinOtherDiameter->setMinimum(0);
    ui->spinOtherDiameter->setMaximum(INT_MAX);
    ui->spinOtherDiameter->setValue(otherdia);
    ui->spinCenterDistance->setMinimum(INT_MIN);
    ui->spinCenterDistance->setMaximum(INT_MAX);
    ui->spinCenterDistance->setValue(centerdist);

    // Adjust ui to specific constraint type
    ui->checkAxial->setVisible(false);
    ui->spinDiameter->setVisible(true);
    ui->labelDiameter->setVisible(true);
    ui->labelDiameter->setText(tr("Pulley diameter"));
    ui->labelOtherDiameter->setVisible(true);
    ui->spinOtherDiameter->setVisible(true);
    ui->labelCenterDistance->setVisible(true);
    ui->spinCenterDistance->setVisible(true);

    ui->spinDiameter->blockSignals(false);
    ui->spinOtherDiameter->blockSignals(false);
    ui->spinCenterDistance->blockSignals(false);

    onButtonReference(true);
}

void TaskFemConstraintPulley::onDiameterChanged(double l)
{
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    pcConstraint->Diameter.setValue((float)l);
}

void TaskFemConstraintPulley::onOtherDiameterChanged(double l)
{
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    pcConstraint->OtherDiameter.setValue((float)l);
}

void TaskFemConstraintPulley::onCenterDistanceChanged(double l)
{
    Fem::ConstraintPulley* pcConstraint = static_cast<Fem::ConstraintPulley*>(ConstraintView->getObject());
    pcConstraint->CenterDistance.setValue((float)l);
}

double TaskFemConstraintPulley::getDiameter(void) const
{
    return ui->spinDiameter->value();
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
        ui->spinDiameter->blockSignals(true);
        ui->spinDistance->blockSignals(true);
        ui->spinOtherDiameter->blockSignals(true);
        ui->spinCenterDistance->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->spinDiameter->blockSignals(false);
        ui->spinDistance->blockSignals(false);
        ui->spinOtherDiameter->blockSignals(false);
        ui->spinCenterDistance->blockSignals(false);
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
        //Gui::Command::openCommand("FEM force constraint changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Diameter = %f",name.c_str(), parameterPulley->getDiameter());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.OtherDiameter = %f",name.c_str(), parameterPulley->getOtherDiameter());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.CenterDistance = %f",name.c_str(), parameterPulley->getCenterDistance());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return TaskDlgFemConstraintBearing::accept();
}

#include "moc_TaskFemConstraintPulley.cpp"
