/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
 *   Based on Force constraint by Jan Rheinländer                          *
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
#include <QMessageBox>
#include <sstream>
#endif

#include <Gui/Command.h>
#include <Mod/Fem/App/FemConstraintInitialTemperature.h>

#include "TaskFemConstraintInitialTemperature.h"
#include "ui_TaskFemConstraintInitialTemperature.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintInitialTemperature */

TaskFemConstraintInitialTemperature::TaskFemConstraintInitialTemperature(
    ViewProviderFemConstraintInitialTemperature* ConstraintView,
    QWidget* parent)
    : TaskFemConstraint(ConstraintView, parent, "FEM_ConstraintInitialTemperature")
    , ui(new Ui_TaskFemConstraintInitialTemperature)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // Get the feature data
    Fem::ConstraintInitialTemperature* pcConstraint =
        ConstraintView->getObject<Fem::ConstraintInitialTemperature>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->if_temperature->setValue(pcConstraint->initialTemperature.getQuantityValue());

    ui->if_temperature->bind(pcConstraint->initialTemperature);
}

TaskFemConstraintInitialTemperature::~TaskFemConstraintInitialTemperature() = default;

std::string TaskFemConstraintInitialTemperature::get_temperature() const
{
    return ui->if_temperature->value().getSafeUserString().toStdString();
}

void TaskFemConstraintInitialTemperature::changeEvent(QEvent*)
{}


//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintInitialTemperature::TaskDlgFemConstraintInitialTemperature(
    ViewProviderFemConstraintInitialTemperature* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintInitialTemperature(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintInitialTemperature::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintInitialTemperature* parameterTemperature =
        static_cast<const TaskFemConstraintInitialTemperature*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.initialTemperature = \"%s\"",
                                name.c_str(),
                                parameterTemperature->get_temperature().c_str());
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        if (!ConstraintView->getObject()->isValid()) {
            throw Base::RuntimeError(ConstraintView->getObject()->getStatusString());
        }
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

#include "moc_TaskFemConstraintInitialTemperature.cpp"
