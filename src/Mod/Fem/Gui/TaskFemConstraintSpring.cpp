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
#include <QAction>
#include <QMessageBox>
#include <sstream>
#endif

#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
#include <Mod/Fem/App/FemConstraintSpring.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintSpring.h"
#include "ui_TaskFemConstraintSpring.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintSpring */

TaskFemConstraintSpring::TaskFemConstraintSpring(ViewProviderFemConstraintSpring* ConstraintView,
                                                 QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintSpring")
    , ui(new Ui_TaskFemConstraintSpring)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_references);
    connect(deleteAction, &QAction::triggered, this, &TaskFemConstraintSpring::onReferenceDeleted);

    connect(ui->lw_references,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintSpring::setSelection);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintSpring::setSelection);

    this->groupLayout()->addWidget(proxy);

    /* Note: */
    // Get the feature data
    Fem::ConstraintSpring* pcConstraint =
        static_cast<Fem::ConstraintSpring*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->qsb_norm->setUnit(pcConstraint->NormalStiffness.getUnit());
    ui->qsb_norm->setMaximum(FLOAT_MAX);
    ui->qsb_norm->setValue(pcConstraint->NormalStiffness.getQuantityValue());

    ui->qsb_tan->setUnit(pcConstraint->TangentialStiffness.getUnit());
    ui->qsb_tan->setMaximum(FLOAT_MAX);
    ui->qsb_tan->setValue(pcConstraint->TangentialStiffness.getQuantityValue());

    ui->cb_elmer_stiffness->clear();
    auto stiffnesses = pcConstraint->ElmerStiffness.getEnumVector();
    QStringList stiffnessesList;
    for (auto item : stiffnesses) {
        stiffnessesList << QLatin1String(item.c_str());
    }
    ui->cb_elmer_stiffness->addItems(stiffnessesList);
    ui->cb_elmer_stiffness->setCurrentIndex(pcConstraint->ElmerStiffness.getValue());

    ui->lw_references->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->lw_references->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (!Objects.empty()) {
        ui->lw_references->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }

    // Selection buttons
    buttonGroup->addButton(ui->btnAdd, (int)SelectionChangeModes::refAdd);
    buttonGroup->addButton(ui->btnRemove, (int)SelectionChangeModes::refRemove);

    ui->qsb_norm->bind(pcConstraint->NormalStiffness);
    ui->qsb_tan->bind(pcConstraint->TangentialStiffness);

    updateUI();
}

TaskFemConstraintSpring::~TaskFemConstraintSpring() = default;

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
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintSpring* pcConstraint =
        static_cast<Fem::ConstraintSpring*>(ConstraintView->getObject());
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
            for (std::vector<std::string>::iterator itr =
                     std::find(SubElements.begin(), SubElements.end(), subName);
                 itr != SubElements.end();
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

void TaskFemConstraintSpring::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintSpring* pcConstraint =
        static_cast<Fem::ConstraintSpring*>(ConstraintView->getObject());
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
            for (std::vector<std::string>::iterator itr =
                     std::find(SubElements.begin(), SubElements.end(), subName);
                 itr != SubElements.end();
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
    std::sort(itemsToDel.begin(), itemsToDel.end());
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

void TaskFemConstraintSpring::onReferenceDeleted()
{
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

std::string TaskFemConstraintSpring::getNormalStiffness() const
{
    return ui->qsb_norm->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintSpring::getTangentialStiffness() const
{
    return ui->qsb_tan->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintSpring::getElmerStiffness() const
{
    return Base::Tools::toStdString(ui->cb_elmer_stiffness->currentText());
}

void TaskFemConstraintSpring::changeEvent(QEvent*)
{}

void TaskFemConstraintSpring::clearButtons(const SelectionChangeModes notThis)
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

TaskDlgFemConstraintSpring::TaskDlgFemConstraintSpring(
    ViewProviderFemConstraintSpring* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintSpring(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintSpring::accept()
{
    /* Note: */
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintSpring* parameterStiffness =
        static_cast<const TaskFemConstraintSpring*>(parameter);
    // const TaskFemConstraintSpring* parameterTan = static_cast<const
    // TaskFemConstraintSpring>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.NormalStiffness = \"%s\"",
                                name.c_str(),
                                parameterStiffness->getNormalStiffness().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.TangentialStiffness = \"%s\"",
                                name.c_str(),
                                parameterStiffness->getTangentialStiffness().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.ElmerStiffness = '%s'",
                                name.c_str(),
                                parameterStiffness->getElmerStiffness().c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
    /* */
    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintSpring.cpp"
