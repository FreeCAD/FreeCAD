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
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <gp_Ax1.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>

# include <QMessageBox>
# include <QRegExp>
# include <QTextStream>

# include <sstream>
#endif

#include "Mod/Fem/App/FemConstraintInitialTemperature.h"
#include "TaskFemConstraintInitialTemperature.h"
#include "ui_TaskFemConstraintInitialTemperature.h"
#include <App/Application.h>
#include <Gui/Command.h>



#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintInitialTemperature */

TaskFemConstraintInitialTemperature::TaskFemConstraintInitialTemperature(ViewProviderFemConstraintInitialTemperature *ConstraintView,QWidget *parent)
  : TaskFemConstraint(ConstraintView, parent, "fem-constraint-InitialTemperature")
{
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintInitialTemperature();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // Get the feature data
    Fem::ConstraintInitialTemperature* pcConstraint = static_cast<Fem::ConstraintInitialTemperature*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->if_temperature->setMinimum(0);
    ui->if_temperature->setMaximum(FLOAT_MAX);
    Base::Quantity t = Base::Quantity(pcConstraint->initialTemperature.getValue(), Base::Unit::Temperature);
    ui->if_temperature->setValue(t);
}

TaskFemConstraintInitialTemperature::~TaskFemConstraintInitialTemperature()
{
    delete ui;
}

double TaskFemConstraintInitialTemperature::get_temperature() const
{
    Base::Quantity temperature =  ui->if_temperature->getQuantity();
    double temperature_in_kelvin = temperature.getValueAs(Base::Quantity::Kelvin);
    return temperature_in_kelvin;
}

void TaskFemConstraintInitialTemperature::changeEvent(QEvent *)
{
}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintInitialTemperature::TaskDlgFemConstraintInitialTemperature(ViewProviderFemConstraintInitialTemperature *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintInitialTemperature(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================
void TaskDlgFemConstraintInitialTemperature::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint initial temperature");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintInitialTemperature::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintInitialTemperature* parameterTemperature = static_cast<const TaskFemConstraintInitialTemperature*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.initialTemperature = %f",
            name.c_str(), parameterTemperature->get_temperature());

        std::string scale = parameterTemperature->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!ConstraintView->getObject()->isValid())
            throw Base::RuntimeError(ConstraintView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgFemConstraintInitialTemperature::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintInitialTemperature.cpp"
