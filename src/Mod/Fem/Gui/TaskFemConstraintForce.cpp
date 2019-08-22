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
# include <sstream>

# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <QAction>

# include <Precision.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Plane.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <Geom_Line.hxx>
# include <gp_Lin.hxx>
#endif

#include "ui_TaskFemConstraintForce.h"
#include "TaskFemConstraintForce.h"
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
#include <Mod/Fem/App/FemConstraintForce.h>
#include <Mod/Fem/App/FemTools.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>

using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintForce */

TaskFemConstraintForce::TaskFemConstraintForce(ViewProviderFemConstraintForce *ConstraintView,QWidget *parent)
    : TaskFemConstraint(ConstraintView, parent, "fem-constraint-force")
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintForce();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // Create a context menu for the listview of the references
    QAction* action = new QAction(tr("Delete"), ui->listReferences);
    action->connect(action, SIGNAL(triggered()),
                    this, SLOT(onReferenceDeleted()));
    ui->listReferences->addAction(action);
    ui->listReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->spinForce, SIGNAL(valueChanged(double)),
            this, SLOT(onForceChanged(double)));
    connect(ui->buttonReference, SIGNAL(pressed()),
            this, SLOT(onButtonReference()));
    connect(ui->buttonDirection, SIGNAL(pressed()),
            this, SLOT(onButtonDirection()));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->spinForce->blockSignals(true);
    ui->listReferences->blockSignals(true);
    ui->buttonReference->blockSignals(true);
    ui->buttonDirection->blockSignals(true);
    ui->checkReverse->blockSignals(true);

    // Get the feature data
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    double f = pcConstraint->Force.getValue();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<std::string> dirStrings = pcConstraint->Direction.getSubValues();
    QString dir;
    if (!dirStrings.empty())
        dir = makeRefText(pcConstraint->Direction.getValue(), dirStrings.front());
    bool reversed = pcConstraint->Reversed.getValue();

    // Fill data into dialog elements
    ui->spinForce->setMinimum(0);
    ui->spinForce->setMaximum(FLOAT_MAX);
    ui->spinForce->setValue(f);
    ui->listReferences->clear();
    for (std::size_t i = 0; i < Objects.size(); i++)
        ui->listReferences->addItem(makeRefText(Objects[i], SubElements[i]));
    if (Objects.size() > 0)
        ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    ui->lineDirection->setText(dir.isEmpty() ? QString() : dir);
    ui->checkReverse->setChecked(reversed);

    ui->spinForce->blockSignals(false);
    ui->listReferences->blockSignals(false);
    ui->buttonReference->blockSignals(false);
    ui->buttonDirection->blockSignals(false);
    ui->checkReverse->blockSignals(false);

    updateUI();
}

void TaskFemConstraintForce::updateUI()
{
    if (ui->listReferences->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }

    std::string ref = ui->listReferences->item(0)->text().toStdString();
    int pos = ref.find_last_of(":");
    if (ref.substr(pos+1, 6) == "Vertex")
        ui->labelForce->setText(tr("Point load"));
    else if (ref.substr(pos+1, 4) == "Edge")
        ui->labelForce->setText(tr("Line load"));
    else if (ref.substr(pos+1, 4) == "Face")
        ui->labelForce->setText(tr("Area load"));
}

void TaskFemConstraintForce::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // Don't allow selection in other document
        if (strcmp(msg.pDocName, ConstraintView->getObject()->getDocument()->getName()) != 0)
            return;

        if (!msg.pSubName || msg.pSubName[0] == '\0')
            return;
        std::string subName(msg.pSubName);

        if (selectionMode == selnone)
            return;

        std::vector<std::string> references(1,subName);
        Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(msg.pObjectName);
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subName.c_str());

        if (selectionMode == selref) {
            std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
            std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

            // Ensure we don't have mixed reference types
            if (SubElements.size() > 0) {
                if (subName.substr(0,4) != SubElements.front().substr(0,4)) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Mixed shape types are not possible. Use a second constraint instead"));
                    return;
                }
            }
            else {
                if ((subName.substr(0,4) != "Face") && (subName.substr(0,4) != "Edge") && (subName.substr(0,6) != "Vertex")) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only faces, edges and vertices can be picked"));
                    return;
                }
            }

            // Avoid duplicates
            std::size_t pos = 0;
            for (; pos < Objects.size(); pos++) {
                if (obj == Objects[pos]) {
                    break;
                }
            }

            if (pos != Objects.size()) {
                if (subName == SubElements[pos]) {
                    return;
                }
            }

            // add the new reference
            Objects.push_back(obj);
            SubElements.push_back(subName);
            pcConstraint->References.setValues(Objects,SubElements);
            ui->listReferences->addItem(makeRefText(obj, subName));

            // Turn off reference selection mode
            onButtonReference(false);
        }
        else if (selectionMode == seldir) {
            if (subName.substr(0,4) == "Face") {
                if (!Fem::Tools::isPlanar(TopoDS::Face(ref))) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only planar faces can be picked"));
                    return;
                }
            }
            else if (subName.substr(0,4) == "Edge") {
                if (!Fem::Tools::isLinear(TopoDS::Edge(ref))) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only linear edges can be picked"));
                    return;
                }
            }
            else {
                QMessageBox::warning(this, tr("Selection error"), tr("Only faces and edges can be picked"));
                return;
            }
            pcConstraint->Direction.setValue(obj, references);
            ui->lineDirection->setText(makeRefText(obj, subName));

            // Turn off direction selection mode
            onButtonDirection(false);
        }

        Gui::Selection().clearSelection();
        updateUI();
    }
}

void TaskFemConstraintForce::onForceChanged(double f)
{
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    pcConstraint->Force.setValue(f);
}

void TaskFemConstraintForce::onReferenceDeleted() {
    int row = ui->listReferences->currentIndex().row();
    TaskFemConstraint::onReferenceDeleted(row);
    ui->listReferences->model()->removeRow(row);
    ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
}

void TaskFemConstraintForce::onButtonDirection(const bool pressed) {
    if (pressed) {
        selectionMode = seldir;
    } else {
        selectionMode = selnone;
    }
    ui->buttonDirection->setChecked(pressed);
    Gui::Selection().clearSelection();
}

void TaskFemConstraintForce::onCheckReverse(const bool pressed)
{
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    pcConstraint->Reversed.setValue(pressed);
}

double TaskFemConstraintForce::getForce(void) const
{
    return ui->spinForce->value().getValue();
}

const std::string TaskFemConstraintForce::getReferences() const
{
    int rows = ui->listReferences->model()->rowCount();

    std::vector<std::string> items;
    for (int r = 0; r < rows; r++)
        items.push_back(ui->listReferences->item(r)->text().toStdString());
    return TaskFemConstraint::getReferences(items);
}

const std::string TaskFemConstraintForce::getDirectionName(void) const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty())
        return "";

    int pos = dir.find_last_of(":");
    return dir.substr(0, pos).c_str();
}

const std::string TaskFemConstraintForce::getDirectionObject(void) const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty())
        return "";

    int pos = dir.find_last_of(":");
    return dir.substr(pos+1).c_str();
}

bool TaskFemConstraintForce::getReverse() const
{
    return ui->checkReverse->isChecked();
}

TaskFemConstraintForce::~TaskFemConstraintForce()
{
    delete ui;
}

void TaskFemConstraintForce::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinForce->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->spinForce->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintForce::TaskDlgFemConstraintForce(ViewProviderFemConstraintForce *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintForce(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintForce::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint force");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintForce::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintForce* parameterForce = static_cast<const TaskFemConstraintForce*>(parameter);

    try {
        //Gui::Command::openCommand("FEM force constraint changed");

        if (parameterForce->getForce()<=0)
        {
            QMessageBox::warning(parameter, tr("Input error"), tr("Please specify a force greater than 0"));
            return false;
        }
        else
        {
            QByteArray num = QByteArray::number(parameterForce->getForce());
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Force = %s",name.c_str(), num.data());
        }

        std::string dirname = parameterForce->getDirectionName().data();
        std::string dirobj = parameterForce->getDirectionObject().data();
        std::string scale = "1";

        if (!dirname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(dirname));
            buf = buf.arg(QString::fromStdString(dirobj));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = %s", name.c_str(), buf.toStdString().c_str());
        } else {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = None", name.c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %s", name.c_str(), parameterForce->getReverse() ? "True" : "False");

        scale = parameterForce->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintForce::reject()
{
    // roll back the changes
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintForce.cpp"
