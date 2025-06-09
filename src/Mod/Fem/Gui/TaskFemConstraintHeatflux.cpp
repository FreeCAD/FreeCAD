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

#include <Gui/Command.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Fem/App/FemConstraintHeatflux.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintHeatflux.h"
#include "ui_TaskFemConstraintHeatflux.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintHeatflux */

TaskFemConstraintHeatflux::TaskFemConstraintHeatflux(
    ViewProviderFemConstraintHeatflux* ConstraintView,
    QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintHeatflux")
    , ui(new Ui_TaskFemConstraintHeatflux)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createActions(ui->lw_references);
    connect(deleteAction,
            &QAction::triggered,
            this,
            &TaskFemConstraintHeatflux::onReferenceDeleted);
    connect(ui->cb_constr_type,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskFemConstraintHeatflux::onConstrTypeChanged);
    connect(ui->qsb_heat_flux,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onHeatFluxChanged);
    connect(ui->qsb_ambienttemp_conv,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onAmbientTempChanged);
    connect(ui->qsb_film_coef,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onFilmCoefChanged);
    connect(ui->dsb_emissivity,
            qOverload<double>(&DoubleSpinBox::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onEmissivityChanged);
    connect(ui->qsb_ambienttemp_rad,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onAmbientTempChanged);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintHeatflux::setSelection);

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->qsb_ambienttemp_conv->blockSignals(true);
    // ui->if_facetemp->blockSignals(true);
    ui->qsb_film_coef->blockSignals(true);
    ui->dsb_emissivity->blockSignals(true);
    ui->qsb_ambienttemp_rad->blockSignals(true);
    ui->qsb_heat_flux->blockSignals(true);
    ui->lw_references->blockSignals(true);
    ui->btnAdd->blockSignals(true);
    ui->btnRemove->blockSignals(true);

    // Get the feature data
    auto pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    App::PropertyEnumeration* constrType = &pcConstraint->ConstraintType;
    QStringList qTypeList;
    for (auto item : constrType->getEnumVector()) {
        qTypeList << QString::fromUtf8(item.c_str());
    }
    ui->cb_constr_type->addItems(qTypeList);
    ui->cb_constr_type->setCurrentIndex(constrType->getValue());
    ui->sw_heatflux->setCurrentIndex(constrType->getValue());

    ui->qsb_ambienttemp_conv->setMinimum(0);
    ui->qsb_ambienttemp_conv->setMaximum(std::numeric_limits<float>::max());

    ui->qsb_film_coef->setMinimum(0);
    ui->qsb_film_coef->setMaximum(std::numeric_limits<float>::max());

    ui->dsb_emissivity->setMinimum(0);
    ui->dsb_emissivity->setMaximum(std::numeric_limits<float>::max());

    ui->qsb_ambienttemp_rad->setMinimum(0);
    ui->qsb_ambienttemp_rad->setMaximum(std::numeric_limits<float>::max());

    ui->qsb_ambienttemp_conv->setValue(pcConstraint->AmbientTemp.getQuantityValue());
    ui->qsb_film_coef->setValue(pcConstraint->FilmCoef.getQuantityValue());

    ui->qsb_ambienttemp_rad->setValue(pcConstraint->AmbientTemp.getQuantityValue());
    ui->dsb_emissivity->setValue(pcConstraint->Emissivity.getValue());

    ui->qsb_heat_flux->setValue(pcConstraint->DFlux.getQuantityValue());

    ui->lw_references->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->lw_references->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (!Objects.empty()) {
        ui->lw_references->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }

    // Selection buttons
    buttonGroup->addButton(ui->btnAdd, static_cast<int>(SelectionChangeModes::refAdd));
    buttonGroup->addButton(ui->btnRemove, static_cast<int>(SelectionChangeModes::refRemove));

    ui->qsb_ambienttemp_conv->blockSignals(false);
    // ui->if_facetemp->blockSignals(false);
    ui->qsb_film_coef->blockSignals(false);
    ui->dsb_emissivity->blockSignals(false);
    ui->qsb_ambienttemp_rad->blockSignals(false);
    ui->qsb_heat_flux->blockSignals(false);
    ui->lw_references->blockSignals(false);
    ui->btnAdd->blockSignals(false);
    ui->btnRemove->blockSignals(false);

    ui->qsb_film_coef->bind(pcConstraint->FilmCoef);
    ui->qsb_ambienttemp_conv->bind(pcConstraint->AmbientTemp);
    ui->qsb_ambienttemp_rad->bind(pcConstraint->AmbientTemp);
    ui->dsb_emissivity->bind(pcConstraint->Emissivity);
    ui->qsb_heat_flux->bind(pcConstraint->DFlux);

    updateUI();
}

TaskFemConstraintHeatflux::~TaskFemConstraintHeatflux() = default;

void TaskFemConstraintHeatflux::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintHeatflux::onAmbientTempChanged(double val)
{
    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    pcConstraint->AmbientTemp.setValue(val);
}

void TaskFemConstraintHeatflux::onFilmCoefChanged(double val)
{
    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    pcConstraint->FilmCoef.setValue(val);
}

void TaskFemConstraintHeatflux::onEmissivityChanged(double val)
{
    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    pcConstraint->Emissivity.setValue(val);
}

void TaskFemConstraintHeatflux::onHeatFluxChanged(double val)
{
    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    pcConstraint->DFlux.setValue(val);
}

void TaskFemConstraintHeatflux::Conv()
{
    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ConstraintType = \"%s\"",
                            name.c_str(),
                            getConstraintType().c_str());
    ui->qsb_ambienttemp_conv->setValue(pcConstraint->AmbientTemp.getQuantityValue());
    ui->qsb_film_coef->setValue(pcConstraint->FilmCoef.getQuantityValue());
    ui->sw_heatflux->setCurrentIndex(1);
}

void TaskFemConstraintHeatflux::Rad()
{
    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ConstraintType = \"%s\"",
                            name.c_str(),
                            getConstraintType().c_str());
    ui->qsb_ambienttemp_rad->setValue(pcConstraint->AmbientTemp.getQuantityValue());
    ui->dsb_emissivity->setValue(pcConstraint->Emissivity.getValue());
    ui->sw_heatflux->setCurrentIndex(2);
}

void TaskFemConstraintHeatflux::Flux()
{
    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ConstraintType = \"%s\"",
                            name.c_str(),
                            getConstraintType().c_str());
    ui->qsb_heat_flux->setValue(pcConstraint->DFlux.getQuantityValue());
    ui->sw_heatflux->setCurrentIndex(0);
}

void TaskFemConstraintHeatflux::onConstrTypeChanged(int item)
{
    auto obj = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    obj->ConstraintType.setValue(item);
    const char* type = obj->ConstraintType.getValueAsString();
    if (strcmp(type, "DFlux") == 0) {
        this->Flux();
    }
    else if (strcmp(type, "Convection") == 0) {
        this->Conv();
    }
    else if (strcmp(type, "Radiation") == 0) {
        this->Rad();
    }
}

void TaskFemConstraintHeatflux::addToSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        App::DocumentObject* obj = it.getObject();

        if (!subNames.empty()) {
            for (const auto& subName : subNames) {
                if (subName.substr(0, 4) != "Face") {
                    QMessageBox::warning(this,
                                         tr("Selection error"),
                                         tr("Selection must only consist of faces!"));
                    return;
                }
            }
        }
        else {
            // fix me, if an object is selected completely, getSelectionEx does not return any
            // SubElements
        }
        for (const auto& subName : subNames) {  // for every selected sub element
            bool addMe = true;
            for (auto itr = std::ranges::find(SubElements.begin(), SubElements.end(), subName);
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

void TaskFemConstraintHeatflux::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    Fem::ConstraintHeatflux* pcConstraint = ConstraintView->getObject<Fem::ConstraintHeatflux>();
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

        if (!subNames.empty()) {
            for (const auto& subName : subNames) {
                if (subName.substr(0, 4) != "Face") {
                    QMessageBox::warning(this,
                                         tr("Selection error"),
                                         tr("Selection must only consist of faces!"));
                    return;
                }
            }
        }
        else {
            // fix me, if an object is selected completely, getSelectionEx does not return any
            // SubElements
        }
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

void TaskFemConstraintHeatflux::onReferenceDeleted()
{
    TaskFemConstraintHeatflux::removeFromSelection();
}

const std::string TaskFemConstraintHeatflux::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

std::string TaskFemConstraintHeatflux::getAmbientTemp() const
{
    std::string type = this->getConstraintType();
    if (type == "Convection") {
        return ui->qsb_ambienttemp_conv->value().getSafeUserString();
    }
    else if (type == "Convection") {
        return ui->qsb_ambienttemp_rad->value().getSafeUserString();
    }
    else {
        auto obj = ConstraintView->getObject<Fem::ConstraintHeatflux>();
        return obj->AmbientTemp.getQuantityValue().getSafeUserString();
    }
}

std::string TaskFemConstraintHeatflux::getFilmCoef() const
{
    return ui->qsb_film_coef->value().getSafeUserString();
}

std::string TaskFemConstraintHeatflux::getDFlux() const
{
    return ui->qsb_heat_flux->value().getSafeUserString();
}

double TaskFemConstraintHeatflux::getEmissivity() const
{
    return ui->dsb_emissivity->value();
}

std::string TaskFemConstraintHeatflux::getConstraintType() const
{
    return ui->cb_constr_type->currentText().toStdString();
}

void TaskFemConstraintHeatflux::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->qsb_ambienttemp_conv->blockSignals(true);
        ui->qsb_film_coef->blockSignals(true);
        ui->dsb_emissivity->blockSignals(true);
        ui->qsb_ambienttemp_rad->blockSignals(true);
        ui->qsb_heat_flux->blockSignals(true);

        ui->retranslateUi(proxy);

        ui->qsb_ambienttemp_conv->blockSignals(false);
        ui->qsb_film_coef->blockSignals(false);
        ui->dsb_emissivity->blockSignals(false);
        ui->qsb_ambienttemp_rad->blockSignals(false);
        ui->qsb_heat_flux->blockSignals(false);
    }
}

void TaskFemConstraintHeatflux::clearButtons(const SelectionChangeModes notThis)
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

TaskDlgFemConstraintHeatflux::TaskDlgFemConstraintHeatflux(
    ViewProviderFemConstraintHeatflux* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintHeatflux(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintHeatflux::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintHeatflux* parameterHeatflux =
        static_cast<const TaskFemConstraintHeatflux*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.AmbientTemp = \"%s\"",
                                name.c_str(),
                                parameterHeatflux->getAmbientTemp().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.FilmCoef = \"%s\"",
                                name.c_str(),
                                parameterHeatflux->getFilmCoef().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Emissivity = %f",
                                name.c_str(),
                                parameterHeatflux->getEmissivity());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.DFlux = \"%s\"",
                                name.c_str(),
                                parameterHeatflux->getDFlux().c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintHeatflux.cpp"
