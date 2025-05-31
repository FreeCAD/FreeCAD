/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
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
#include <QAction>
#include <QMessageBox>
#include <limits>
#include <sstream>
#endif

#include <Gui/Command.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Fem/App/FemConstraintPressure.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintPressure.h"
#include "ui_TaskFemConstraintPressure.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintPressure */

TaskFemConstraintPressure::TaskFemConstraintPressure(
    ViewProviderFemConstraintPressure* ConstraintView,
    QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintPressure")
    , ui(new Ui_TaskFemConstraintPressure)
{  // Note change "pressure" in line above to new constraint name
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // Get the feature data
    Fem::ConstraintPressure* pcConstraint = ConstraintView->getObject<Fem::ConstraintPressure>();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->if_pressure->setUnit(pcConstraint->Pressure.getUnit());
    ui->if_pressure->setMinimum(0);
    ui->if_pressure->setMaximum(std::numeric_limits<float>::max());
    ui->if_pressure->setValue(pcConstraint->Pressure.getQuantityValue());
    ui->if_pressure->bind(pcConstraint->Pressure);

    bool reversed = pcConstraint->Reversed.getValue();
    ui->checkBoxReverse->setChecked(reversed);

    ui->lw_references->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->lw_references->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (!Objects.empty()) {
        ui->lw_references->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }

    // create a context menu for the listview of the references
    createActions(ui->lw_references);
    connect(deleteAction,
            &QAction::triggered,
            this,
            &TaskFemConstraintPressure::onReferenceDeleted);
    connect(ui->lw_references,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintPressure::setSelection);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintPressure::setSelection);

    connect(ui->checkBoxReverse,
            &QCheckBox::toggled,
            this,
            &TaskFemConstraintPressure::onCheckReverse);

    // Selection buttons
    buttonGroup->addButton(ui->btnAdd, static_cast<int>(SelectionChangeModes::refAdd));
    buttonGroup->addButton(ui->btnRemove, static_cast<int>(SelectionChangeModes::refRemove));

    updateUI();
}

TaskFemConstraintPressure::~TaskFemConstraintPressure() = default;

void TaskFemConstraintPressure::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintPressure::onCheckReverse(const bool pressed)
{
    Fem::ConstraintPressure* pcConstraint = ConstraintView->getObject<Fem::ConstraintPressure>();
    pcConstraint->Reversed.setValue(pressed);
}

void TaskFemConstraintPressure::addToSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintPressure* pcConstraint = ConstraintView->getObject<Fem::ConstraintPressure>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        App::DocumentObject* obj = it.getObject();

        for (const auto& subName : subNames) {  // for every selected sub element
            bool addMe = true;
            if (subName.substr(0, 4) != "Face") {
                QMessageBox::warning(this, tr("Selection error"), tr("Only faces can be picked"));
                return;
            }
            for (auto itr = std::ranges::find(SubElements, subName); itr != SubElements.end();
                 itr = std::find(++itr,
                                 SubElements.end(),
                                 subName)) {  // for every sub element in selection that
                                              // matches one in old list
                if (obj
                    == Objects[std::distance(
                        SubElements.begin(),
                        itr)]) {  // if selected sub element's object equals the one in old list
                                  // then it was added before so don't add
                    addMe = false;
                }
            }
            if (addMe) {
                QSignalBlocker block(ui->lw_references);
                Objects.push_back(obj);
                SubElements.push_back(subName);
                ui->lw_references->addItem(makeRefText(obj, subName));
            }
        }
    }
    // Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintPressure::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintPressure* pcConstraint = ConstraintView->getObject<Fem::ConstraintPressure>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<size_t> itemsToDel;
    for (const auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        const App::DocumentObject* obj = it.getObject();

        for (const auto& subName : subNames) {  // for every selected sub element
            for (auto itr = std::ranges::find(SubElements, subName); itr != SubElements.end();
                 itr = std::find(++itr,
                                 SubElements.end(),
                                 subName)) {  // for every sub element in selection that
                                              // matches one in old list
                if (obj
                    == Objects[std::distance(
                        SubElements.begin(),
                        itr)]) {  // if selected sub element's object equals the one in old list
                                  // then it was added before so mark for deletion
                    itemsToDel.push_back(std::distance(SubElements.begin(), itr));
                }
            }
        }
    }
    std::ranges::sort(itemsToDel);
    while (!itemsToDel.empty()) {
        Objects.erase(Objects.begin() + itemsToDel.back());
        SubElements.erase(SubElements.begin() + itemsToDel.back());
        itemsToDel.pop_back();
    }
    // Update UI
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

void TaskFemConstraintPressure::onReferenceDeleted()
{
    TaskFemConstraintPressure::removeFromSelection();
}

const std::string TaskFemConstraintPressure::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

std::string TaskFemConstraintPressure::getPressure() const
{
    return ui->if_pressure->value().getSafeUserString();
}

bool TaskFemConstraintPressure::getReverse() const
{
    return ui->checkBoxReverse->isChecked();
}

void TaskFemConstraintPressure::changeEvent(QEvent*)
{}

void TaskFemConstraintPressure::clearButtons(const SelectionChangeModes notThis)
{
    if (notThis != SelectionChangeModes::refAdd) {
        ui->btnAdd->setChecked(false);
    }
    if (notThis != SelectionChangeModes::refRemove) {
        ui->btnRemove->setChecked(false);
    }
}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintPressure::TaskDlgFemConstraintPressure(
    ViewProviderFemConstraintPressure* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintPressure(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintPressure::accept()
{
    /* Note: */
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintPressure* parameterPressure =
        static_cast<const TaskFemConstraintPressure*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Pressure = \"%s\"",
                                name.c_str(),
                                parameterPressure->getPressure().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Reversed = %s",
                                name.c_str(),
                                parameterPressure->getReverse() ? "True" : "False");
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
    /* */
    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintPressure.cpp"
