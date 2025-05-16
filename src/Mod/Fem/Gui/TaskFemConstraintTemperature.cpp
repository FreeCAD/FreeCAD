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
#include <QAction>
#include <QMessageBox>
#include <limits>
#include <sstream>
#endif

#include <App/Document.h>
#include <Gui/Command.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Fem/App/FemConstraintTemperature.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintTemperature.h"
#include "ui_TaskFemConstraintTemperature.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintTemperature */

TaskFemConstraintTemperature::TaskFemConstraintTemperature(
    ViewProviderFemConstraintTemperature* ConstraintView,
    QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintTemperature")
    , ui(new Ui_TaskFemConstraintTemperature)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // Get the feature data
    Fem::ConstraintTemperature* pcConstraint =
        ConstraintView->getObject<Fem::ConstraintTemperature>();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->qsb_temperature->setMinimum(0);
    ui->qsb_temperature->setMaximum(std::numeric_limits<float>::max());
    ui->qsb_cflux->setMinimum(-std::numeric_limits<float>::max());
    ui->qsb_cflux->setMaximum(std::numeric_limits<float>::max());

    App::PropertyEnumeration* constrType = &pcConstraint->ConstraintType;
    QStringList qTypeList;
    for (auto item : constrType->getEnumVector()) {
        qTypeList << QString::fromUtf8(item.c_str());
    }

    ui->cb_constr_type->addItems(qTypeList);
    ui->cb_constr_type->setCurrentIndex(constrType->getValue());
    onConstrTypeChanged(constrType->getValue());

    ui->qsb_temperature->setValue(pcConstraint->Temperature.getQuantityValue());
    ui->qsb_temperature->bind(pcConstraint->Temperature);
    ui->qsb_temperature->setUnit(pcConstraint->Temperature.getUnit());

    ui->qsb_cflux->setValue(pcConstraint->CFlux.getQuantityValue());
    ui->qsb_cflux->bind(pcConstraint->CFlux);
    ui->qsb_cflux->setUnit(pcConstraint->CFlux.getUnit());

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
            &TaskFemConstraintTemperature::onReferenceDeleted);

    connect(ui->lw_references,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintTemperature::setSelection);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintTemperature::setSelection);
    connect(ui->cb_constr_type,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskFemConstraintTemperature::onConstrTypeChanged);
    connect(ui->qsb_temperature,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintTemperature::onTempChanged);
    connect(ui->qsb_cflux,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintTemperature::onCFluxChanged);

    // Selection buttons
    buttonGroup->addButton(ui->btnAdd, static_cast<int>(SelectionChangeModes::refAdd));
    buttonGroup->addButton(ui->btnRemove, static_cast<int>(SelectionChangeModes::refRemove));

    updateUI();
}

TaskFemConstraintTemperature::~TaskFemConstraintTemperature() = default;

void TaskFemConstraintTemperature::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintTemperature::onTempChanged(double)
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.Temperature = \"%s\"",
                            name.c_str(),
                            get_temperature().c_str());
}

void TaskFemConstraintTemperature::onCFluxChanged(double)
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.CFlux = \"%s\"",
                            name.c_str(),
                            get_cflux().c_str());
}

void TaskFemConstraintTemperature::onConstrTypeChanged(int item)
{
    auto obj = ConstraintView->getObject<Fem::ConstraintTemperature>();
    obj->ConstraintType.setValue(item);
    const char* type = obj->ConstraintType.getValueAsString();
    if (strcmp(type, "Temperature") == 0) {
        ui->qsb_temperature->setVisible(true);
        ui->qsb_cflux->setVisible(false);
        ui->lbl_temperature->setVisible(true);
        ui->lbl_cflux->setVisible(false);
    }
    else if (strcmp(type, "CFlux") == 0) {
        ui->qsb_cflux->setVisible(true);
        ui->qsb_temperature->setVisible(false);
        ui->lbl_cflux->setVisible(true);
        ui->lbl_temperature->setVisible(false);
    }
}

void TaskFemConstraintTemperature::addToSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintTemperature* pcConstraint =
        ConstraintView->getObject<Fem::ConstraintTemperature>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        std::vector<std::string> subNames = it.getSubNames();
        App::DocumentObject* obj =
            ConstraintView->getObject()->getDocument()->getObject(it.getFeatName());
        for (const auto& subName : subNames) {  // for every selected sub element
            bool addMe = true;
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

void TaskFemConstraintTemperature::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintTemperature* pcConstraint =
        ConstraintView->getObject<Fem::ConstraintTemperature>();
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

void TaskFemConstraintTemperature::onReferenceDeleted()
{
    TaskFemConstraintTemperature::removeFromSelection();
}

const std::string TaskFemConstraintTemperature::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

std::string TaskFemConstraintTemperature::get_temperature() const
{
    return ui->qsb_temperature->value().getSafeUserString();
}

std::string TaskFemConstraintTemperature::get_cflux() const
{
    return ui->qsb_cflux->value().getSafeUserString();
}

std::string TaskFemConstraintTemperature::get_constraint_type() const
{
    return ui->cb_constr_type->currentText().toStdString();
}

void TaskFemConstraintTemperature::changeEvent(QEvent*)
{
    //    TaskBox::changeEvent(e);
    //    if (e->type() == QEvent::LanguageChange) {
    //        ui->if_pressure->blockSignals(true);
    //        ui->retranslateUi(proxy);
    //        ui->if_pressure->blockSignals(false);
    //    }
}

void TaskFemConstraintTemperature::clearButtons(const SelectionChangeModes notThis)
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

TaskDlgFemConstraintTemperature::TaskDlgFemConstraintTemperature(
    ViewProviderFemConstraintTemperature* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintTemperature(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintTemperature::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintTemperature* parameterTemperature =
        static_cast<const TaskFemConstraintTemperature*>(parameter);

    auto type = parameterTemperature->get_constraint_type();

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.ConstraintType = \"%s\"",
                                name.c_str(),
                                parameterTemperature->get_constraint_type().c_str());
        if (type == "Temperature") {
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.Temperature = \"%s\"",
                                    name.c_str(),
                                    parameterTemperature->get_temperature().c_str());
        }
        else if (type == "CFlux") {
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.CFlux = \"%s\"",
                                    name.c_str(),
                                    parameterTemperature->get_cflux().c_str());
        }
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintTemperature.cpp"
