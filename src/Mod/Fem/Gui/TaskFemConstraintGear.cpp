/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
#include <QMessageBox>
#include <TopoDS.hxx>
#endif

#include <App/Document.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/Fem/App/FemConstraintGear.h>
#include <Mod/Fem/App/FemTools.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintGear.h"
#include "ui_TaskFemConstraintBearing.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintGear */

TaskFemConstraintGear::TaskFemConstraintGear(ViewProviderFemConstraint* ConstraintView,
                                             QWidget* parent,
                                             const char* pixmapname)
    : TaskFemConstraintBearing(ConstraintView, parent, pixmapname)
{
    connect(ui->spinDiameter,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &TaskFemConstraintGear::onDiameterChanged);
    connect(ui->spinForce,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &TaskFemConstraintGear::onForceChanged);
    connect(ui->spinForceAngle,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &TaskFemConstraintGear::onForceAngleChanged);
    connect(ui->buttonDirection, &QPushButton::pressed, this, [=] {
        onButtonDirection(true);
    });
    connect(ui->checkReversed, &QCheckBox::toggled, this, &TaskFemConstraintGear::onCheckReversed);

    // Temporarily prevent unnecessary feature recomputes
    ui->spinDiameter->blockSignals(true);
    ui->spinForce->blockSignals(true);
    ui->spinForceAngle->blockSignals(true);
    ui->checkReversed->blockSignals(true);

    // Get the feature data
    Fem::ConstraintGear* pcConstraint =
        static_cast<Fem::ConstraintGear*>(ConstraintView->getObject());
    double dia = pcConstraint->Diameter.getValue();
    double force = pcConstraint->Force.getValue();
    double angle = pcConstraint->ForceAngle.getValue();
    std::vector<std::string> dirStrings = pcConstraint->Direction.getSubValues();
    QString dir;
    if (!dirStrings.empty()) {
        dir = makeRefText(pcConstraint->Direction.getValue(), dirStrings.front());
    }
    bool reversed = pcConstraint->Reversed.getValue();

    // Fill data into dialog elements
    ui->spinDiameter->setMinimum(0);
    ui->spinDiameter->setMaximum(FLOAT_MAX);
    ui->spinDiameter->setValue(dia);
    ui->spinForce->setMinimum(0);
    ui->spinForce->setMaximum(FLOAT_MAX);
    ui->spinForce->setValue(force);
    ui->spinForceAngle->setMinimum(-360);
    ui->spinForceAngle->setMaximum(360);
    ui->spinForceAngle->setValue(angle);
    ui->lineDirection->setText(dir);
    ui->checkReversed->setChecked(reversed);

    // Adjust ui
    ui->labelDiameter->setVisible(true);
    ui->spinDiameter->setVisible(true);
    ui->labelForce->setVisible(true);
    ui->spinForce->setVisible(true);
    ui->labelForceAngle->setVisible(true);
    ui->spinForceAngle->setVisible(true);
    ui->buttonDirection->setVisible(true);
    ui->lineDirection->setVisible(true);
    ui->checkReversed->setVisible(true);
    ui->checkAxial->setVisible(false);

    ui->spinDiameter->blockSignals(false);
    ui->spinForce->blockSignals(false);
    ui->spinForceAngle->blockSignals(false);
    ui->checkReversed->blockSignals(false);
}

void TaskFemConstraintGear::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    TaskFemConstraintBearing::onSelectionChanged(msg);

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // Don't allow selection in other document
        if (strcmp(msg.pDocName, ConstraintView->getObject()->getDocument()->getName()) != 0) {
            return;
        }

        if (!msg.pSubName || msg.pSubName[0] == '\0') {
            return;
        }
        std::string subName(msg.pSubName);

        if (selectionMode == selnone) {
            return;
        }

        std::vector<std::string> references(1, subName);
        Fem::ConstraintGear* pcConstraint =
            static_cast<Fem::ConstraintGear*>(ConstraintView->getObject());
        App::DocumentObject* obj =
            ConstraintView->getObject()->getDocument()->getObject(msg.pObjectName);
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subName.c_str());

        if (selectionMode == seldir) {
            if (subName.substr(0, 4) == "Face") {
                if (!Fem::Tools::isPlanar(TopoDS::Face(ref))) {
                    QMessageBox::warning(this,
                                         tr("Selection error"),
                                         tr("Only planar faces can be picked"));
                    return;
                }
            }
            else if (subName.substr(0, 4) == "Edge") {
                if (!Fem::Tools::isLinear(TopoDS::Edge(ref))) {
                    QMessageBox::warning(this,
                                         tr("Selection error"),
                                         tr("Only linear edges can be picked"));
                    return;
                }
            }
            else {
                QMessageBox::warning(this,
                                     tr("Selection error"),
                                     tr("Only faces and edges can be picked"));
                return;
            }
            pcConstraint->Direction.setValue(obj, references);
            ui->lineDirection->setText(makeRefText(obj, subName));

            // Turn off direction selection mode
            onButtonDirection(false);
        }

        Gui::Selection().clearSelection();
    }
}

void TaskFemConstraintGear::onDiameterChanged(double l)
{
    Fem::ConstraintGear* pcConstraint =
        static_cast<Fem::ConstraintGear*>(ConstraintView->getObject());
    pcConstraint->Diameter.setValue(l);
}

void TaskFemConstraintGear::onForceChanged(double f)
{
    Fem::ConstraintGear* pcConstraint =
        static_cast<Fem::ConstraintGear*>(ConstraintView->getObject());
    pcConstraint->Force.setValue(f);
}

void TaskFemConstraintGear::onForceAngleChanged(double a)
{
    Fem::ConstraintGear* pcConstraint =
        static_cast<Fem::ConstraintGear*>(ConstraintView->getObject());
    pcConstraint->ForceAngle.setValue(a);
}

void TaskFemConstraintGear::onButtonDirection(const bool pressed)
{
    if (pressed) {
        selectionMode = seldir;
    }
    else {
        selectionMode = selnone;
    }
    ui->buttonDirection->setChecked(pressed);
    Gui::Selection().clearSelection();
}

void TaskFemConstraintGear::onCheckReversed(const bool pressed)
{
    Fem::ConstraintGear* pcConstraint =
        static_cast<Fem::ConstraintGear*>(ConstraintView->getObject());
    pcConstraint->Reversed.setValue(pressed);
}

double TaskFemConstraintGear::getForce() const
{
    return ui->spinForce->value();
}

double TaskFemConstraintGear::getForceAngle() const
{
    return ui->spinForceAngle->value();
}

const std::string TaskFemConstraintGear::getDirectionName() const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty()) {
        return "";
    }

    int pos = dir.find_last_of(":");
    return dir.substr(0, pos).c_str();
}

const std::string TaskFemConstraintGear::getDirectionObject() const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty()) {
        return "";
    }

    int pos = dir.find_last_of(":");
    return dir.substr(pos + 1).c_str();
}

bool TaskFemConstraintGear::getReverse() const
{
    return ui->checkReversed->isChecked();
}

double TaskFemConstraintGear::getDiameter() const
{
    return ui->spinDiameter->value();
}

void TaskFemConstraintGear::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinDiameter->blockSignals(true);
        ui->spinForce->blockSignals(true);
        ui->spinForceAngle->blockSignals(true);
        ui->checkReversed->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->spinDiameter->blockSignals(false);
        ui->spinForce->blockSignals(false);
        ui->spinForceAngle->blockSignals(true);
        ui->checkReversed->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintGear::TaskDlgFemConstraintGear(ViewProviderFemConstraintGear* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintGear(ConstraintView, nullptr, "FEM_ConstraintGear");

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintGear::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintGear* parameterGear =
        static_cast<const TaskFemConstraintGear*>(parameter);

    try {
        // Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "FEM force constraint changed"));
        std::string dirname = parameterGear->getDirectionName().data();
        std::string dirobj = parameterGear->getDirectionObject().data();

        if (!dirname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(dirname));
            buf = buf.arg(QString::fromStdString(dirobj));
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.Direction = %s",
                                    name.c_str(),
                                    buf.toStdString().c_str());
        }
        else {
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.Direction = None",
                                    name.c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Reversed = %s",
                                name.c_str(),
                                parameterGear->getReverse() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Diameter = %f",
                                name.c_str(),
                                parameterGear->getDiameter());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Force = %f",
                                name.c_str(),
                                parameterGear->getForce());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.ForceAngle = %f",
                                name.c_str(),
                                parameterGear->getForceAngle());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return TaskDlgFemConstraintBearing::accept();
}

#include "moc_TaskFemConstraintGear.cpp"
