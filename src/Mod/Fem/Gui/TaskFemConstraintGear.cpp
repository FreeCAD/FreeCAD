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
/*
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Plane.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <Geom_Line.hxx>
# include <gp_Lin.hxx>
*/
#endif

#include "ui_TaskFemConstraintCylindrical.h"
#include "TaskFemConstraintGear.h"
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
#include <Mod/Fem/App/FemConstraintGear.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>

using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintGear */

TaskFemConstraintGear::TaskFemConstraintGear(ViewProviderFemConstraintGear *ConstraintView,QWidget *parent)
    : TaskFemConstraintBearing(ConstraintView, parent, "Fem_ConstraintGear")
{
    // we need a separate container widget to add all controls to
    connect(ui->spinDiameter, SIGNAL(valueChanged(double)),
            this, SLOT(onDiameterChanged(double)));

    // Temporarily prevent unnecessary feature recomputes
    ui->spinDiameter->blockSignals(true);

    // Get the feature data
    Fem::ConstraintGear* pcConstraint = static_cast<Fem::ConstraintGear*>(ConstraintView->getObject());
    double dia = pcConstraint->Diameter.getValue();

    // Fill data into dialog elements
    ui->spinDiameter->setMinimum(0);
    ui->spinDiameter->setMaximum(INT_MAX);
    ui->spinDiameter->setValue(dia);

    // Adjust ui to specific constraint type
    ui->checkAxial->setVisible(false);
    ui->spinDiameter->setVisible(true);
    ui->labelDiameter->setVisible(true);

    ui->spinDiameter->blockSignals(false);

    onButtonReference(true);
}
void TaskFemConstraintGear::onDiameterChanged(double l)
{
    Fem::ConstraintGear* pcConstraint = static_cast<Fem::ConstraintGear*>(ConstraintView->getObject());
    pcConstraint->Diameter.setValue((float)l);
}

double TaskFemConstraintGear::getDiameter(void) const
{
    return ui->spinDiameter->value();
}

void TaskFemConstraintGear::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinDiameter->blockSignals(true);
        ui->spinDistance->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->spinDiameter->blockSignals(false);
        ui->spinDistance->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintGear::TaskDlgFemConstraintGear(ViewProviderFemConstraintGear *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintGear(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintGear::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintGear* parameterGear = static_cast<const TaskFemConstraintGear*>(parameter);

    try {
        //Gui::Command::openCommand("FEM force constraint changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Diameter = %f",name.c_str(), parameterGear->getDiameter());
           }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return TaskDlgFemConstraintBearing::accept();
}

#include "moc_TaskFemConstraintGear.cpp"
