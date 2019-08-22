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

#include "ui_TaskFemConstraintBearing.h"
#include "TaskFemConstraintBearing.h"
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
#include <Mod/Fem/App/FemConstraintBearing.h>
#include <Mod/Fem/App/FemTools.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>

using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintBearing */

TaskFemConstraintBearing::TaskFemConstraintBearing(ViewProviderFemConstraint *ConstraintView,QWidget *parent,
                                                   const char *pixmapname)
    : TaskFemConstraint(ConstraintView, parent, pixmapname)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintBearing();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // Create a context menu for the listview of the references
    QAction* action = new QAction(tr("Delete"), ui->listReferences);
    action->connect(action, SIGNAL(triggered()),
                    this, SLOT(onReferenceDeleted()));
    ui->listReferences->addAction(action);
    ui->listReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->spinDistance, SIGNAL(valueChanged(double)),
            this, SLOT(onDistanceChanged(double)));
    connect(ui->buttonReference, SIGNAL(pressed()),
            this, SLOT(onButtonReference()));
    connect(ui->buttonLocation, SIGNAL(pressed()),
            this, SLOT(onButtonLocation()));
    connect(ui->checkAxial, SIGNAL(toggled(bool)),
            this, SLOT(onCheckAxial(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->spinDistance->blockSignals(true);
    ui->listReferences->blockSignals(true);
    ui->buttonReference->blockSignals(true);
    ui->buttonLocation->blockSignals(true);
    ui->checkAxial->blockSignals(true);

    // Get the feature data
    Fem::ConstraintBearing* pcConstraint = static_cast<Fem::ConstraintBearing*>(ConstraintView->getObject());
    double d = pcConstraint->Dist.getValue();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<std::string> locStrings = pcConstraint->Location.getSubValues();
    QString loc;
    if (!locStrings.empty())
        loc = makeRefText(pcConstraint->Location.getValue(), locStrings.front());
    bool axialfree = pcConstraint->AxialFree.getValue();

    // Fill data into dialog elements
    ui->spinDistance->setMinimum(-FLOAT_MAX);
    ui->spinDistance->setMaximum(FLOAT_MAX);
    ui->spinDistance->setValue(d);
    ui->listReferences->clear();
    for (std::size_t i = 0; i < Objects.size(); i++)
        ui->listReferences->addItem(makeRefText(Objects[i], SubElements[i]));
    if (Objects.size() > 0)
        ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    ui->lineLocation->setText(loc);
    ui->checkAxial->setChecked(axialfree);

    // Hide unwanted ui elements
    ui->labelDiameter->setVisible(false);
    ui->spinDiameter->setVisible(false);
    ui->labelOtherDiameter->setVisible(false);
    ui->spinOtherDiameter->setVisible(false);
    ui->labelCenterDistance->setVisible(false);
    ui->spinCenterDistance->setVisible(false);
    ui->checkIsDriven->setVisible(false);
    ui->labelForce->setVisible(false);
    ui->spinForce->setVisible(false);
    ui->labelTensionForce->setVisible(false);
    ui->spinTensionForce->setVisible(false);
    ui->labelForceAngle->setVisible(false);
    ui->spinForceAngle->setVisible(false);
    ui->buttonDirection->setVisible(false);
    ui->lineDirection->setVisible(false);
    ui->checkReversed->setVisible(false);

    ui->spinDistance->blockSignals(false);
    ui->listReferences->blockSignals(false);
    ui->buttonReference->blockSignals(false);
    ui->buttonLocation->blockSignals(false);
    ui->checkAxial->blockSignals(false);

    onButtonReference(true);
}

void TaskFemConstraintBearing::onSelectionChanged(const Gui::SelectionChanges& msg)
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

        Fem::ConstraintBearing* pcConstraint = static_cast<Fem::ConstraintBearing*>(ConstraintView->getObject());
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(msg.pObjectName);
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subName.c_str());

        if (selectionMode == selref) {
            std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
            std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

            if (Objects.size() > 0) {
                QMessageBox::warning(this, tr("Selection error"), tr("Please use only a single reference for bearing constraint"));
                return;
            }
            if (subName.substr(0,4) != "Face") {
                QMessageBox::warning(this, tr("Selection error"), tr("Only faces can be picked"));
                return;
            }

            // Only cylindrical faces allowed
            BRepAdaptor_Surface surface(TopoDS::Face(ref));
            if (surface.GetType() != GeomAbs_Cylinder) {
                QMessageBox::warning(this, tr("Selection error"), tr("Only cylindrical faces can be picked"));
                return;
            }

            // add the new reference
            Objects.push_back(obj);
            SubElements.push_back(subName);
            pcConstraint->References.setValues(Objects,SubElements);
            ui->listReferences->addItem(makeRefText(obj, subName));

            // Turn off reference selection mode
            onButtonReference(false);
        }
        else if (selectionMode == selloc) {
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
            std::vector<std::string> references(1,subName);
            pcConstraint->Location.setValue(obj, references);
            ui->lineLocation->setText(makeRefText(obj, subName));

            // Turn off location selection mode
            onButtonLocation(false);
        }

        Gui::Selection().clearSelection();
    }
}

void TaskFemConstraintBearing::onDistanceChanged(double l)
{
    Fem::ConstraintBearing* pcConstraint = static_cast<Fem::ConstraintBearing*>(ConstraintView->getObject());
    pcConstraint->Dist.setValue(l);
}

void TaskFemConstraintBearing::onReferenceDeleted() {
    int row = ui->listReferences->currentIndex().row();
    TaskFemConstraint::onReferenceDeleted(row);
    ui->listReferences->model()->removeRow(row);
    ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
}

void TaskFemConstraintBearing::onButtonLocation(const bool pressed) {
    if (pressed) {
        selectionMode = selloc;
    } else {
        selectionMode = selnone;
    }
    ui->buttonLocation->setChecked(pressed);
    Gui::Selection().clearSelection();
}

void TaskFemConstraintBearing::onCheckAxial(const bool pressed)
{
    Fem::ConstraintBearing* pcConstraint = static_cast<Fem::ConstraintBearing*>(ConstraintView->getObject());
    pcConstraint->AxialFree.setValue(pressed);
}

double TaskFemConstraintBearing::getDistance(void) const
{
    return ui->spinDistance->value();
}

const std::string TaskFemConstraintBearing::getReferences() const
{
    int rows = ui->listReferences->model()->rowCount();

    std::vector<std::string> items;
    for (int r = 0; r < rows; r++)
        items.push_back(ui->listReferences->item(r)->text().toStdString());
    return TaskFemConstraint::getReferences(items);
}

const std::string TaskFemConstraintBearing::getLocationName(void) const
{
    std::string loc = ui->lineLocation->text().toStdString();
    if (loc.empty())
        return "";

    int pos = loc.find_last_of(":");
    return loc.substr(0, pos).c_str();
}

const std::string TaskFemConstraintBearing::getLocationObject(void) const
{
    std::string loc = ui->lineLocation->text().toStdString();
    if (loc.empty())
        return "";

    int pos = loc.find_last_of(":");
    return loc.substr(pos+1).c_str();
}

bool TaskFemConstraintBearing::getAxial() const
{
    return ui->checkAxial->isChecked();
}

TaskFemConstraintBearing::~TaskFemConstraintBearing()
{
    delete ui;
}

void TaskFemConstraintBearing::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinDistance->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->spinDistance->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintBearing::TaskDlgFemConstraintBearing(ViewProviderFemConstraintBearing *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintBearing(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintBearing::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintBearing* parameterBearing = static_cast<const TaskFemConstraintBearing*>(parameter);

    try {
        //Gui::Command::openCommand("FEM force constraint changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Dist = %f",name.c_str(), parameterBearing->getDistance());

        std::string locname = parameterBearing->getLocationName().data();
        std::string locobj = parameterBearing->getLocationObject().data();

        if (!locname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(locname));
            buf = buf.arg(QString::fromStdString(locobj));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Location = %s", name.c_str(), buf.toStdString().c_str());
        } else {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Location = None", name.c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.AxialFree = %s", name.c_str(), parameterBearing->getAxial() ? "True" : "False");
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintBearing.cpp"
