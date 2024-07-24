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
#include <sstream>
#endif

#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
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
    createDeleteAction(ui->lw_references);
    connect(deleteAction,
            &QAction::triggered,
            this,
            &TaskFemConstraintHeatflux::onReferenceDeleted);

    connect(ui->rb_convection, &QRadioButton::clicked, this, &TaskFemConstraintHeatflux::Conv);
    connect(ui->rb_radiation, &QRadioButton::clicked, this, &TaskFemConstraintHeatflux::Rad);
    connect(ui->rb_dflux, &QRadioButton::clicked, this, &TaskFemConstraintHeatflux::Flux);

    connect(ui->if_heatflux,
            qOverload<double>(&InputField::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onHeatFluxChanged);
    connect(ui->if_ambienttemp_conv,
            qOverload<double>(&InputField::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onAmbientTempChanged);
    connect(ui->if_filmcoef,
            qOverload<double>(&InputField::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onFilmCoefChanged);
    connect(ui->if_emissivity,
            qOverload<double>(&InputField::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onEmissivityChanged);
    connect(ui->if_ambienttemp_rad,
            qOverload<double>(&InputField::valueChanged),
            this,
            &TaskFemConstraintHeatflux::onAmbientTempChanged);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintHeatflux::setSelection);

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->if_ambienttemp_conv->blockSignals(true);
    // ui->if_facetemp->blockSignals(true);
    ui->if_filmcoef->blockSignals(true);
    ui->if_emissivity->blockSignals(true);
    ui->if_ambienttemp_rad->blockSignals(true);
    ui->lw_references->blockSignals(true);
    ui->btnAdd->blockSignals(true);
    ui->btnRemove->blockSignals(true);

    // Get the feature data
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->if_ambienttemp_conv->setMinimum(0);
    ui->if_ambienttemp_conv->setMaximum(FLOAT_MAX);

    ui->if_filmcoef->setMinimum(0);
    ui->if_filmcoef->setMaximum(FLOAT_MAX);

    ui->if_emissivity->setMinimum(0);
    ui->if_emissivity->setMaximum(FLOAT_MAX);

    ui->if_ambienttemp_rad->setMinimum(0);
    ui->if_ambienttemp_rad->setMaximum(FLOAT_MAX);

    std::string constraint_type = pcConstraint->ConstraintType.getValueAsString();
    if (constraint_type == "Convection") {
        ui->rb_convection->setChecked(true);
        ui->sw_heatflux->setCurrentIndex(0);
        Base::Quantity t =
            Base::Quantity(pcConstraint->AmbientTemp.getValue(), Base::Unit::Temperature);
        ui->if_ambienttemp_conv->setValue(t);
        Base::Quantity f = Base::Quantity(pcConstraint->FilmCoef.getValue(),
                                          Base::Unit::ThermalTransferCoefficient);
        ui->if_filmcoef->setValue(f);
    }
    else if (constraint_type == "Radiation") {
        ui->rb_radiation->setChecked(true);
        ui->sw_heatflux->setCurrentIndex(1);
        Base::Quantity t =
            Base::Quantity(pcConstraint->AmbientTemp.getValue(), Base::Unit::Temperature);
        ui->if_ambienttemp_rad->setValue(t);
        Base::Quantity e = Base::Quantity(pcConstraint->Emissivity.getValue(), Base::Unit());
        ui->if_emissivity->setValue(e);
    }
    else if (constraint_type == "DFlux") {
        ui->rb_dflux->setChecked(true);
        ui->sw_heatflux->setCurrentIndex(2);
        Base::Quantity c = Base::Quantity(pcConstraint->DFlux.getValue(), Base::Unit::HeatFlux);
        ui->if_heatflux->setValue(c);
    }

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

    ui->if_ambienttemp_conv->blockSignals(false);
    // ui->if_facetemp->blockSignals(false);
    ui->if_filmcoef->blockSignals(false);
    ui->if_emissivity->blockSignals(false);
    ui->if_ambienttemp_rad->blockSignals(false);
    ui->lw_references->blockSignals(false);
    ui->btnAdd->blockSignals(false);
    ui->btnRemove->blockSignals(false);

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
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
    pcConstraint->AmbientTemp.setValue(val);  //[K]
}

void TaskFemConstraintHeatflux::onFilmCoefChanged(double val)
{
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
    pcConstraint->FilmCoef.setValue(val);  // [W]/[[m^2]/[K]]
}

void TaskFemConstraintHeatflux::onEmissivityChanged(double val)
{
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
    pcConstraint->Emissivity.setValue(val);  // [-]
}

void TaskFemConstraintHeatflux::onHeatFluxChanged(double val)
{
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
    pcConstraint->DFlux.setValue(val);
}

void TaskFemConstraintHeatflux::Conv()
{
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ConstraintType = %s",
                            name.c_str(),
                            get_constraint_type().c_str());
    Base::Quantity t = Base::Quantity(300, Base::Unit::Temperature);
    ui->if_ambienttemp_conv->setValue(t);
    pcConstraint->AmbientTemp.setValue(300);
    Base::Quantity f = Base::Quantity(10, Base::Unit::ThermalTransferCoefficient);
    ui->if_filmcoef->setValue(f);
    pcConstraint->FilmCoef.setValue(10);
    ui->sw_heatflux->setCurrentIndex(0);
}

void TaskFemConstraintHeatflux::Rad()
{
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ConstraintType = %s",
                            name.c_str(),
                            get_constraint_type().c_str());
    Base::Quantity t = Base::Quantity(300, Base::Unit::Temperature);
    ui->if_ambienttemp_rad->setValue(t);
    pcConstraint->AmbientTemp.setValue(300);
    Base::Quantity e = Base::Quantity(1, Base::Unit());
    ui->if_emissivity->setValue(e);
    pcConstraint->Emissivity.setValue(1);
    ui->sw_heatflux->setCurrentIndex(1);
}

void TaskFemConstraintHeatflux::Flux()
{
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ConstraintType = %s",
                            name.c_str(),
                            get_constraint_type().c_str());
    Base::Quantity c = Base::Quantity(0, Base::Unit::HeatFlux);
    ui->if_heatflux->setValue(c);
    pcConstraint->DFlux.setValue(0);
    ui->sw_heatflux->setCurrentIndex(2);
}

void TaskFemConstraintHeatflux::addToSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
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

void TaskFemConstraintHeatflux::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    Fem::ConstraintHeatflux* pcConstraint =
        static_cast<Fem::ConstraintHeatflux*>(ConstraintView->getObject());
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

double TaskFemConstraintHeatflux::getAmbientTemp() const
{
    Base::Quantity temperature;
    if (ui->rb_convection->isChecked()) {
        temperature = ui->if_ambienttemp_conv->getQuantity();
    }
    else if (ui->rb_radiation->isChecked()) {
        temperature = ui->if_ambienttemp_rad->getQuantity();
    }
    double temperature_in_kelvin = temperature.getValueAs(Base::Quantity::Kelvin);
    return temperature_in_kelvin;
}

double TaskFemConstraintHeatflux::getFilmCoef() const
{
    Base::Quantity filmcoef = ui->if_filmcoef->getQuantity();
    double filmcoef_in_units =
        filmcoef.getValueAs(Base::Quantity(1.0, Base::Unit::ThermalTransferCoefficient));
    return filmcoef_in_units;
}

double TaskFemConstraintHeatflux::getEmissivity() const
{
    Base::Quantity emissivity = ui->if_emissivity->getQuantity();
    double emissivity_in_units = emissivity.getValueAs(Base::Quantity(1.0, Base::Unit()));
    return emissivity_in_units;
}

std::string TaskFemConstraintHeatflux::get_constraint_type() const
{
    std::string type;
    if (ui->rb_convection->isChecked()) {
        type = "\"Convection\"";
    }
    else if (ui->rb_radiation->isChecked()) {
        type = "\"Radiation\"";
    }
    else if (ui->rb_dflux->isChecked()) {
        type = "\"DFlux\"";
    }
    return type;
}

void TaskFemConstraintHeatflux::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->if_ambienttemp_conv->blockSignals(true);
        ui->if_filmcoef->blockSignals(true);
        ui->if_emissivity->blockSignals(true);
        ui->if_ambienttemp_rad->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->if_ambienttemp_conv->blockSignals(false);
        ui->if_filmcoef->blockSignals(false);
        ui->if_emissivity->blockSignals(true);
        ui->if_ambienttemp_rad->blockSignals(false);
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
    std::string scale = "1";

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.AmbientTemp = %f",
                                name.c_str(),
                                parameterHeatflux->getAmbientTemp());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.FilmCoef = %f",
                                name.c_str(),
                                parameterHeatflux->getFilmCoef());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Emissivity = %f",
                                name.c_str(),
                                parameterHeatflux->getEmissivity());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintHeatflux.cpp"
