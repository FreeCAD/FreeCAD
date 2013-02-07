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

#include "ui_TaskFemConstraint.h"
#include "TaskFemConstraint.h"
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
#include <Mod/Fem/App/FemConstraint.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>

using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraint */

const QString makeRefText(const App::DocumentObject* obj, const std::string& subName)
{
    return QString::fromUtf8((std::string(obj->getNameInDocument()) + ":" + subName).c_str());
}

TaskFemConstraint::TaskFemConstraint(ViewProviderFemConstraint *ConstraintView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Fem_Constraint"),tr("FEM constraint parameters"),true, parent),ConstraintView(ConstraintView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraint();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // Create a context menu for the listview of the references
    QAction* action = new QAction(tr("Delete"), ui->listReferences);
    action->connect(action, SIGNAL(triggered()),
                    this, SLOT(onReferenceDeleted()));
    ui->listReferences->addAction(action);
    ui->listReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->comboType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTypeChanged(int)));
    connect(ui->spinForce, SIGNAL(valueChanged(double)),
            this, SLOT(onForceChanged(double)));
    connect(ui->buttonReference, SIGNAL(pressed()),
            this, SLOT(onButtonReference()));
    connect(ui->buttonDirection, SIGNAL(pressed()),
            this, SLOT(onButtonDirection()));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));
    connect(ui->buttonLocation, SIGNAL(pressed()),
            this, SLOT(onButtonLocation()));
    connect(ui->spinDistance, SIGNAL(valueChanged(double)),
            this, SLOT(onDistanceChanged(double)));
    connect(ui->spinDiameter, SIGNAL(valueChanged(double)),
            this, SLOT(onDiameterChanged(double)));
    connect(ui->spinOtherDia, SIGNAL(valueChanged(double)),
            this, SLOT(onOtherDiameterChanged(double)));
    connect(ui->spinCenterDistance, SIGNAL(valueChanged(double)),
            this, SLOT(onCenterDistanceChanged(double)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->comboType->blockSignals(true);
    ui->spinForce->blockSignals(true);
    ui->listReferences->blockSignals(true);
    ui->buttonReference->blockSignals(true);
    ui->buttonDirection->blockSignals(true);
    ui->checkReverse->blockSignals(true);
    ui->buttonLocation->blockSignals(true);
    ui->spinDistance->blockSignals(true);
    ui->spinDiameter->blockSignals(true);
    ui->spinOtherDia->blockSignals(true);
    ui->spinCenterDistance->blockSignals(true);

    // Get the feature data
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    int index = pcConstraint->Type.getValue();
    double f = pcConstraint->Force.getValue();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<std::string> dirStrings = pcConstraint->Direction.getSubValues();
    QString dir;
    if (!dirStrings.empty())
        dir = makeRefText(pcConstraint->Direction.getValue(), dirStrings.front());
    bool reversed = pcConstraint->Reversed.getValue();
    std::vector<std::string> locStrings = pcConstraint->Location.getSubValues();
    QString loc;
    if (!locStrings.empty())
        loc = makeRefText(pcConstraint->Location.getValue(), locStrings.front());
    double d = pcConstraint->Distance.getValue();
    double dia = pcConstraint->Diameter.getValue();
    double otherdia = pcConstraint->OtherDiameter.getValue();
    double centerdist = pcConstraint->CenterDistance.getValue();

    // Fill data into dialog elements
    ui->comboType->clear();
    ui->comboType->insertItem(0, tr("Force on geometry"));
    ui->comboType->insertItem(1, tr("Fixed"));
    ui->comboType->insertItem(2, tr("Bearing (axial free)"));
    ui->comboType->insertItem(3, tr("Bearing (axial fixed)"));
    ui->comboType->insertItem(4, tr("Pulley"));
    ui->comboType->insertItem(5, tr("Gear (straight toothed)"));
    ui->comboType->setCurrentIndex(index);
    ui->spinForce->setMinimum(0);
    ui->spinForce->setMaximum(INT_MAX);
    ui->spinForce->setValue(f);
    ui->listReferences->clear();

    for (int i = 0; i < Objects.size(); i++)
        ui->listReferences->addItem(makeRefText(Objects[i], SubElements[i]));
    if (Objects.size() > 0)
        ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    ui->lineDirection->setText(dir.isEmpty() ? tr("") : dir);
    ui->checkReverse->setChecked(reversed);
    ui->lineDirection->setText(loc.isEmpty() ? tr("") : loc);
    ui->spinDistance->setMinimum(INT_MIN);
    ui->spinDistance->setMaximum(INT_MAX);
    ui->spinDistance->setValue(d);
    ui->spinDiameter->setMinimum(0);
    ui->spinDiameter->setMaximum(INT_MAX);
    ui->spinDiameter->setValue(dia);
    ui->spinOtherDia->setMinimum(0);
    ui->spinOtherDia->setMaximum(INT_MAX);
    ui->spinOtherDia->setValue(otherdia);
    ui->spinCenterDistance->setMinimum(0);
    ui->spinCenterDistance->setMaximum(INT_MAX);
    ui->spinCenterDistance->setValue(centerdist);

    // activate and de-activate dialog elements as appropriate
    ui->comboType->blockSignals(false);
    ui->spinForce->blockSignals(false);
    ui->listReferences->blockSignals(false);
    ui->buttonReference->blockSignals(false);
    ui->buttonDirection->blockSignals(false);
    ui->checkReverse->blockSignals(false);
    ui->buttonLocation->blockSignals(false);
    ui->spinDistance->blockSignals(false);
    ui->spinDiameter->blockSignals(false);
    ui->spinOtherDia->blockSignals(false);
    ui->spinCenterDistance->blockSignals(false);

    selectionMode = selref;
    updateUI();
}

void TaskFemConstraint::updateUI()
{
    if (ui->comboType->currentIndex() == 0) {
        ui->labelForce->setVisible(true);
        ui->spinForce->setVisible(true);
        ui->buttonDirection->setVisible(true);
        ui->lineDirection->setVisible(true);
        ui->checkReverse->setVisible(true);
        ui->buttonLocation->setVisible(false);
        ui->lineLocation->setVisible(false);
        ui->labelDistance->setVisible(false);
        ui->spinDistance->setVisible(false);
        ui->labelDiameter->setVisible(false);
        ui->spinDiameter->setVisible(false);
        ui->labelOtherDia->setVisible(false);
        ui->spinOtherDia->setVisible(false);
        ui->labelCenterDistance->setVisible(false);
        ui->spinCenterDistance->setVisible(false);
    } else if (ui->comboType->currentIndex() == 1) {
        ui->labelForce->setVisible(false);
        ui->spinForce->setVisible(false);
        ui->buttonDirection->setVisible(false);
        ui->lineDirection->setVisible(false);
        ui->checkReverse->setVisible(false);
        ui->buttonLocation->setVisible(false);
        ui->lineLocation->setVisible(false);
        ui->labelDistance->setVisible(false);
        ui->spinDistance->setVisible(false);
        ui->labelDiameter->setVisible(false);
        ui->spinDiameter->setVisible(false);
        ui->labelOtherDia->setVisible(false);
        ui->spinOtherDia->setVisible(false);
        ui->labelCenterDistance->setVisible(false);
        ui->spinCenterDistance->setVisible(false);
    } else if ((ui->comboType->currentIndex() == 2) || (ui->comboType->currentIndex() == 3)) {
        ui->labelForce->setVisible(false);
        ui->spinForce->setVisible(false);
        ui->buttonDirection->setVisible(false);
        ui->lineDirection->setVisible(false);
        ui->checkReverse->setVisible(false);
        ui->buttonLocation->setVisible(true);
        ui->lineLocation->setVisible(true);
        ui->labelDistance->setVisible(true);
        ui->spinDistance->setVisible(true);
        ui->labelDiameter->setVisible(false);
        ui->spinDiameter->setVisible(false);
        ui->labelOtherDia->setVisible(false);
        ui->spinOtherDia->setVisible(false);
        ui->labelCenterDistance->setVisible(false);
        ui->spinCenterDistance->setVisible(false);
    } else if (ui->comboType->currentIndex() == 4) {
        ui->labelForce->setVisible(false);
        ui->spinForce->setVisible(false);
        ui->buttonDirection->setVisible(false);
        ui->lineDirection->setVisible(false);
        ui->checkReverse->setVisible(false);
        ui->buttonLocation->setVisible(true);
        ui->lineLocation->setVisible(true);
        ui->labelDistance->setVisible(true);
        ui->spinDistance->setVisible(true);
        ui->labelDiameter->setVisible(true);
        ui->spinDiameter->setVisible(true);
        ui->labelOtherDia->setVisible(true);
        ui->spinOtherDia->setVisible(true);
        ui->labelCenterDistance->setVisible(true);
        ui->spinCenterDistance->setVisible(true);
    } else if (ui->comboType->currentIndex() == 5) {
        ui->labelForce->setVisible(false);
        ui->spinForce->setVisible(false);
        ui->buttonDirection->setVisible(false);
        ui->lineDirection->setVisible(false);
        ui->checkReverse->setVisible(false);
        ui->buttonLocation->setVisible(true);
        ui->lineLocation->setVisible(true);
        ui->labelDistance->setVisible(true);
        ui->spinDistance->setVisible(true);
        ui->labelDiameter->setVisible(true);
        ui->spinDiameter->setVisible(true);
        ui->labelOtherDia->setVisible(true);
        ui->spinOtherDia->setVisible(true);
        ui->labelCenterDistance->setVisible(false);
        ui->spinCenterDistance->setVisible(false);
    }

    if (ui->listReferences->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }

    if (ui->comboType->currentIndex() == 0) {
        std::string ref = ui->listReferences->item(0)->text().toStdString();
        int pos = ref.find_last_of(":");
        if (ref.substr(pos+1, 6) == "Vertex")
            ui->labelForce->setText(tr("Force [N]"));
        else if (ref.substr(pos+1, 4) == "Edge")
            ui->labelForce->setText(tr("Force [N/mm]"));
        else if (ref.substr(pos+1, 4) == "Face")
            ui->labelForce->setText(tr("Force [N/mm²]"));
    }
}

void TaskFemConstraint::onSelectionChanged(const Gui::SelectionChanges& msg)
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
        Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(msg.pObjectName);
        //if (!obj->getClassTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        //    return;
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subName.c_str());

        if (selectionMode == selref) {
            std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
            std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

            if (pcConstraint->Type.getValue() == 0) {
                // Force on geometry elements:
                // Ensure we don't have mixed reference types
                if (SubElements.size() > 0) {
                    if (subName.substr(0,4) != SubElements.front().substr(0,4)) {
                        QMessageBox::warning(this, tr("Selection error"), tr("Mixed shape types are not possible. Use a second constraint instead"));
                        return;
                    }
                } else {
                    if ((subName.substr(0,4) != "Face") && (subName.substr(0,4) != "Edge") && (subName.substr(0,6) != "Vertex")) {
                        QMessageBox::warning(this, tr("Selection error"), tr("Only faces, edges and vertices can be picked"));
                        return;
                    }
                }

                // Avoid duplicates
                int pos = 0;
                for (; pos < Objects.size(); pos++)
                    if (obj == Objects[pos])
                        break;

                if (pos != Objects.size())
                    if (subName == SubElements[pos])
                        return;
            } else if (pcConstraint->Type.getValue() == 1) {
                // Fixed
                if ((subName.substr(0,4) != "Face") && (subName.substr(0,4) != "Edge") && (subName.substr(0,6) != "Vertex")) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Mixed shape types are not possible. Use a second constraint instead"));
                    return;
                }

                // Avoid duplicates
                int pos = 0;
                for (; pos < Objects.size(); pos++)
                    if (obj == Objects[pos])
                        break;

                if (pos != Objects.size())
                    if (subName == SubElements[pos])
                        return;
            } else if ((pcConstraint->Type.getValue() >= 2) && (pcConstraint->Type.getValue() <= 5)) {
                // Bearing, pulley, gear
                if (Objects.size() > 0) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Please use only a single reference for bearing constraint"));
                    return;
                }
                // Only cylindrical faces allowed
                if (subName.substr(0,4) != "Face") {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only faces can be picked"));
                    return;
                }

                BRepAdaptor_Surface surface(TopoDS::Face(ref));
                if (surface.GetType() != GeomAbs_Cylinder) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only cylindrical faces can be picked"));
                    return;
                }
            } else {
                return;
            }

            // add the new reference
            Objects.push_back(obj);
            SubElements.push_back(subName);
            pcConstraint->References.setValues(Objects,SubElements);
            ui->listReferences->addItem(makeRefText(obj, subName));

            // Turn off reference selection mode
            onButtonReference(false);
        } else if ((selectionMode == seldir) || (selectionMode == selloc)) {
            if (subName.substr(0,4) == "Face") {
                BRepAdaptor_Surface surface(TopoDS::Face(ref));
                if (surface.GetType() != GeomAbs_Plane) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only planar faces can be picked"));
                    return;
                }
            } else if (subName.substr(0,4) == "Edge") {
                BRepAdaptor_Curve line(TopoDS::Edge(ref));
                if (line.GetType() != GeomAbs_Line) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only linear edges can be picked"));
                    return;
                }
            } else {
                QMessageBox::warning(this, tr("Selection error"), tr("Only faces and edges can be picked"));
                return;
            }
            if (selectionMode == seldir) {
                pcConstraint->Direction.setValue(obj, references);
                ui->lineDirection->setText(makeRefText(obj, subName));

                // Turn off direction selection mode
                onButtonDirection(false);
            } else {
                pcConstraint->Location.setValue(obj, references);
                ui->lineLocation->setText(makeRefText(obj, subName));

                // Turn off direction selection mode
                onButtonLocation(false);
            }
        }
        updateUI();
    }
}

void TaskFemConstraint::onTypeChanged(int index)
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    int oldType = pcConstraint->Type.getValue();
    pcConstraint->Type.setValue(index);

    if (((oldType == 2) && (index == 3)) || ((oldType == 3) && (index == 2))) {
        pcConstraint->References.touch(); // Update visual
        updateUI();
    } else {
        // Clear all references if the old and new type mismatch
        std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
        std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

        Objects.clear();
        SubElements.clear();
        pcConstraint->References.setValues(Objects, SubElements);

        ui->listReferences->clear(); //model()->removeRows(0, ui->listReferences->model()->rowCount());
        updateUI();
    }
}

void TaskFemConstraint::onForceChanged(double f)
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    pcConstraint->Force.setValue((float)f);
}

void TaskFemConstraint::onDistanceChanged(double f)
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    pcConstraint->Distance.setValue((float)f);
}

void TaskFemConstraint::onDiameterChanged(double f)
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    pcConstraint->Diameter.setValue((float)f);
}

void TaskFemConstraint::onOtherDiameterChanged(double f)
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    pcConstraint->OtherDiameter.setValue((float)f);
}

void TaskFemConstraint::onCenterDistanceChanged(double d)
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    pcConstraint->CenterDistance.setValue((float)d);
}

void TaskFemConstraint::onButtonReference(const bool pressed) {
    if (pressed)
        selectionMode = selref;
    else
        selectionMode = selnone;
    ui->buttonReference->setChecked(pressed);
    Gui::Selection().clearSelection();
}

void TaskFemConstraint::onReferenceDeleted() {
    int row = ui->listReferences->currentIndex().row();
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    Objects.erase(Objects.begin() + row);
    SubElements.erase(SubElements.begin() + row);
    pcConstraint->References.setValues(Objects, SubElements);

    ui->listReferences->model()->removeRow(row);
    ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
}

void TaskFemConstraint::onButtonDirection(const bool pressed) {
    if (pressed) {
        selectionMode = seldir;
    } else {
        selectionMode = selnone;
    }
    ui->buttonDirection->setChecked(pressed);
    Gui::Selection().clearSelection();
}

void TaskFemConstraint::onButtonLocation(const bool pressed) {
    if (pressed) {
        selectionMode = selloc;
    } else {
        selectionMode = selnone;
    }
    ui->buttonLocation->setChecked(pressed);
    Gui::Selection().clearSelection();
}

void TaskFemConstraint::onCheckReverse(const bool pressed)
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    pcConstraint->Reversed.setValue(pressed);
}

int TaskFemConstraint::getType(void) const
{
    return ui->comboType->currentIndex();
}

double TaskFemConstraint::getForce(void) const
{
    return ui->spinForce->value();
}

double TaskFemConstraint::getDistance(void) const
{
    return ui->spinDistance->value();
}

double TaskFemConstraint::getDiameter(void) const
{
    return ui->spinDiameter->value();
}

double TaskFemConstraint::getOtherDiameter(void) const
{
    return ui->spinOtherDia->value();
}

double TaskFemConstraint::getCenterDistance(void) const
{
    return ui->spinCenterDistance->value();
}

const std::string TaskFemConstraint::getReferences(void) const
{
    int rows = ui->listReferences->model()->rowCount();
    if (rows == 0)
        return "";

    std::string result;
    for (int r = 0; r < rows; r++) {
        std::string item = ui->listReferences->item(r)->text().toStdString();
        int pos = item.find_last_of(":");
        std::string objStr = "App.ActiveDocument." + item.substr(0, pos);
        std::string refStr = "\"" + item.substr(pos+1) + "\"";
        result = result + (r > 0 ? ", " : "") + "(" + objStr + "," + refStr + ")";
    }

    return result;
}

const std::string TaskFemConstraint::getDirectionName(void) const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty())
        return "";

    int pos = dir.find_last_of(":");
    return dir.substr(0, pos).c_str();
}

const std::string TaskFemConstraint::getDirectionObject(void) const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty())
        return "";

    int pos = dir.find_last_of(":");
    return dir.substr(pos+1).c_str();
}

const std::string TaskFemConstraint::getLocationName(void) const
{
    std::string loc = ui->lineLocation->text().toStdString();
    if (loc.empty())
        return "";

    int pos = loc.find_last_of(":");
    return loc.substr(0, pos).c_str();
}

const std::string TaskFemConstraint::getLocationObject(void) const
{
    std::string loc = ui->lineLocation->text().toStdString();
    if (loc.empty())
        return "";

    int pos = loc.find_last_of(":");
    return loc.substr(pos+1).c_str();
}

bool TaskFemConstraint::getReverse() const
{
    return ui->checkReverse->isChecked();
}

TaskFemConstraint::~TaskFemConstraint()
{
    delete ui;
}

void TaskFemConstraint::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->comboType->blockSignals(true);
        ui->spinForce->blockSignals(true);
        ui->spinDistance->blockSignals(true);
        int index = ui->comboType->currentIndex();
        ui->comboType->clear();
        ui->comboType->insertItem(0, tr("Force on geometry"));
        ui->comboType->insertItem(1, tr("Fixed"));
        ui->comboType->insertItem(2, tr("Bearing (axial free)"));
        ui->comboType->insertItem(3, tr("Bearing (axial fixed)"));
        ui->comboType->insertItem(4, tr("Pulley"));
        ui->comboType->insertItem(5, tr("Gear (straight toothed)"));
        ui->comboType->setCurrentIndex(index);
        ui->retranslateUi(proxy);
        ui->comboType->blockSignals(false);
        ui->spinForce->blockSignals(false);
        ui->spinDistance->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraint::TaskDlgFemConstraint(ViewProviderFemConstraint *ConstraintView)
    : TaskDialog(),ConstraintView(ConstraintView)
{
    assert(ConstraintView);
    parameter  = new TaskFemConstraint(ConstraintView);

    Content.push_back(parameter);
}

TaskDlgFemConstraint::~TaskDlgFemConstraint()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgFemConstraint::open()
{
    
}

void TaskDlgFemConstraint::clicked(int)
{
    
}

bool TaskDlgFemConstraint::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand("FEM constraint changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",name.c_str(),parameter->getType());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Force = %f",name.c_str(),parameter->getForce());
        std::string refs = parameter->getReferences();

        if (!refs.empty()) {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.References = [%s]", name.c_str(), refs.c_str());
        } else {
            QMessageBox::warning(parameter, tr("Input error"), tr("You must specify at least one reference"));
            return false;
        }

        std::string dirname = parameter->getDirectionName().data();
        std::string dirobj = parameter->getDirectionObject().data();

        if (!dirname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(dirname));
            buf = buf.arg(QString::fromStdString(dirobj));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = %s", name.c_str(), buf.toStdString().c_str());
        } else {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = None", name.c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %s", name.c_str(), parameter->getReverse() ? "True" : "False");

        std::string locname = parameter->getLocationName().data();
        std::string locobj = parameter->getLocationObject().data();

        if (!locname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(locname));
            buf = buf.arg(QString::fromStdString(locobj));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Location = %s", name.c_str(), buf.toStdString().c_str());
        } else {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = None", name.c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Distance = %f",name.c_str(),parameter->getDistance());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Diameter = %f",name.c_str(),parameter->getDiameter());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.OtherDiameter = %f",name.c_str(),parameter->getOtherDiameter());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.CenterDistance = %f",name.c_str(),parameter->getCenterDistance());

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!ConstraintView->getObject()->isValid())
            throw Base::Exception(ConstraintView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgFemConstraint::reject()
{
    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    return true;
}



#include "moc_TaskFemConstraint.cpp"
