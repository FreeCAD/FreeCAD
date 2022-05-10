/***************************************************************************
 *   Copyright (c) 2021 FreeCAD Developers                                 *
 *   Author: Preslav Aleksandrov <preslav.aleksandrov@protonmail>          *
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
# include <QAction>
# include <QMessageBox>
# include <sstream>
#endif

#include <Gui/Command.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Mod/Fem/App/FemConstraintSpring.h>

#include "TaskFemConstraintSpring.h"
#include "ui_TaskFemConstraintSpring.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintSpring */

TaskFemConstraintSpring::TaskFemConstraintSpring(ViewProviderFemConstraintSpring* ConstraintView, QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintSpring")
{
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintSpring();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_references);
    deleteAction->connect(deleteAction, SIGNAL(triggered()), this, SLOT(onReferenceDeleted()));

    connect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->lw_references, SIGNAL(itemClicked(QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    this->groupLayout()->addWidget(proxy);

    /* Note: */
    // Get the feature data
    Fem::ConstraintSpring* pcConstraint = static_cast<Fem::ConstraintSpring*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->if_norm->setMinimum(0); // TODO fix this -------------------------------------------------------------------
    ui->if_norm->setMaximum(FLOAT_MAX);
    Base::Quantity ns = Base::Quantity((pcConstraint->normalStiffness.getValue()), Base::Unit::Stiffness);
    ui->if_norm->setValue(ns);

    // Fill data into dialog elements
    ui->if_tan->setMinimum(0); // TODO fix this -------------------------------------------------------------------
    ui->if_tan->setMaximum(FLOAT_MAX);
    Base::Quantity ts = Base::Quantity((pcConstraint->tangentialStiffness.getValue()), Base::Unit::Stiffness);
    ui->if_tan->setValue(ts);

    /* */

    ui->lw_references->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->lw_references->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (Objects.size() > 0) {
        ui->lw_references->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }

    //Selection buttons
    buttonGroup->addButton(ui->btnAdd, (int)SelectionChangeModes::refAdd);
    buttonGroup->addButton(ui->btnRemove, (int)SelectionChangeModes::refRemove);

    updateUI();
}

TaskFemConstraintSpring::~TaskFemConstraintSpring()
{
    delete ui;
}

void TaskFemConstraintSpring::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}


void TaskFemConstraintSpring::addToSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size() == 0) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintSpring* pcConstraint = static_cast<Fem::ConstraintSpring*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end(); ++it) {//for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it->getSubNames();
        App::DocumentObject* obj = it->getObject();

        for (size_t subIt = 0; subIt < (subNames.size()); ++subIt) {// for every selected sub element
            bool addMe = true;
            if (subNames[subIt].substr(0, 4) != "Face") {
                QMessageBox::warning(this, tr("Selection error"), tr("Only faces can be picked"));
                return;
            }
            for (std::vector<std::string>::iterator itr = std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
                itr != SubElements.end();
                itr = std::find(++itr, SubElements.end(), subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj == Objects[std::distance(SubElements.begin(), itr)]) {//if selected sub element's object equals the one in old list then it was added before so don't add
                    addMe = false;
                }
            }
            if (addMe) {
                QSignalBlocker block(ui->lw_references);
                Objects.push_back(obj);
                SubElements.push_back(subNames[subIt]);
                ui->lw_references->addItem(makeRefText(obj, subNames[subIt]));
            }
        }
    }
    //Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintSpring::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size() == 0) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintSpring* pcConstraint = static_cast<Fem::ConstraintSpring*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<size_t> itemsToDel;
    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end(); ++it) {//for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it->getSubNames();
        App::DocumentObject* obj = it->getObject();

        for (size_t subIt = 0; subIt < (subNames.size()); ++subIt) {// for every selected sub element
            for (std::vector<std::string>::iterator itr = std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
                itr != SubElements.end();
                itr = std::find(++itr, SubElements.end(), subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj == Objects[std::distance(SubElements.begin(), itr)]) {//if selected sub element's object equals the one in old list then it was added before so mark for deletion
                    itemsToDel.push_back(std::distance(SubElements.begin(), itr));
                }
            }
        }
    }
    std::sort(itemsToDel.begin(), itemsToDel.end());
    while (itemsToDel.size() > 0) {
        Objects.erase(Objects.begin() + itemsToDel.back());
        SubElements.erase(SubElements.begin() + itemsToDel.back());
        itemsToDel.pop_back();
    }
    //Update UI
    {
        QSignalBlocker block(ui->lw_references);
        ui->lw_references->clear();
        for (unsigned int j = 0; j < Objects.size(); j++) {
            ui->lw_references->addItem(makeRefText(Objects[j], SubElements[j]));
        }
    }
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintSpring::onReferenceDeleted() {
    TaskFemConstraintSpring::removeFromSelection();
}

const std::string TaskFemConstraintSpring::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

/* Note: */
double TaskFemConstraintSpring::get_normalStiffness() const
{
    Base::Quantity stiffness = ui->if_norm->getQuantity();
    double stiffness_double = stiffness.getValueAs(Base::Quantity::NewtonPerMeter);
    return stiffness_double;
}

double TaskFemConstraintSpring::get_tangentialStiffness() const
{
    Base::Quantity stiffness = ui->if_tan->getQuantity();
    double stiffness_double = stiffness.getValueAs(Base::Quantity::NewtonPerMeter);
    return stiffness_double;
}


bool TaskFemConstraintSpring::event(QEvent* e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintSpring::changeEvent(QEvent*)
{
}

void TaskFemConstraintSpring::clearButtons(const SelectionChangeModes notThis)
{
    if (notThis != SelectionChangeModes::refAdd)
        ui->btnAdd->setChecked(false);
    if (notThis != SelectionChangeModes::refRemove)
        ui->btnRemove->setChecked(false);
}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintSpring::TaskDlgFemConstraintSpring(ViewProviderFemConstraintSpring* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintSpring(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintSpring::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint spring");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(Gui::Command::Doc, ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintSpring::accept()
{
    /* Note: */
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintSpring* parameterStiffness = static_cast<const TaskFemConstraintSpring*>(parameter);
    //const TaskFemConstraintSpring* parameterTan = static_cast<const TaskFemConstraintSpring>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.normalStiffness = %f",
            name.c_str(), parameterStiffness->get_normalStiffness());
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.tangentialStiffness = %f",
            name.c_str(), parameterStiffness->get_tangentialStiffness());
        std::string scale = parameterStiffness->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
    /* */
    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintSpring::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintSpring.cpp"
