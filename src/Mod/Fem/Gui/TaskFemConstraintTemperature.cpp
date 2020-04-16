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
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <gp_Ax1.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>

# include <sstream>
# include <QAction>
# include <QMessageBox>
# include <QKeyEvent>
# include <QRegExp>
# include <QTextStream>
#endif

#include "Mod/Fem/App/FemConstraintTemperature.h"
#include "TaskFemConstraintTemperature.h"
#include "ui_TaskFemConstraintTemperature.h"
#include <App/Application.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Mod/Part/App/PartFeature.h>


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintTemperature */

TaskFemConstraintTemperature::TaskFemConstraintTemperature(ViewProviderFemConstraintTemperature *ConstraintView,QWidget *parent)
  : TaskFemConstraint(ConstraintView, parent, "FEM_ConstraintTemperature")
{
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintTemperature();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_references);
    deleteAction->connect(deleteAction, SIGNAL(triggered()), this, SLOT(onReferenceDeleted()));

    connect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->lw_references, SIGNAL(itemClicked(QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->rb_temperature, SIGNAL(clicked(bool)),  this, SLOT(Temp()));
    connect(ui->rb_cflux, SIGNAL(clicked(bool)),  this, SLOT(Flux()));

    connect(ui->if_temperature, SIGNAL(valueChanged(double)),
            this, SLOT(onTempCfluxChanged(double)));

    this->groupLayout()->addWidget(proxy);

    // Get the feature data
    Fem::ConstraintTemperature* pcConstraint = static_cast<Fem::ConstraintTemperature*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->if_temperature->setMinimum(0);
    ui->if_temperature->setMaximum(FLOAT_MAX);

    std::string constraint_type = pcConstraint->ConstraintType.getValueAsString();
    if (constraint_type == "Temperature") {
        ui->rb_temperature->setChecked(1);
        std::string str = "Temperature";
        QString qstr = QString::fromStdString(str);
        ui->lbl_type->setText(qstr);
        Base::Quantity t = Base::Quantity(pcConstraint->Temperature.getValue(), Base::Unit::Temperature);
        ui->if_temperature->setValue(t);
    } else if (constraint_type == "CFlux") {
        ui->rb_cflux->setChecked(1);
        std::string str = "Concentrated heat flux";
        QString qstr = QString::fromStdString(str);
        ui->lbl_type->setText(qstr);
        Base::Quantity c = Base::Quantity(pcConstraint->CFlux.getValue(), Base::Unit::Power);
        ui->if_temperature->setValue(c);
    }

    ui->lw_references->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->lw_references->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (Objects.size() > 0) {
        ui->lw_references->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }

    //Selection buttons
    connect(ui->btnAdd, SIGNAL(clicked()),  this, SLOT(addToSelection()));
    connect(ui->btnRemove, SIGNAL(clicked()),  this, SLOT(removeFromSelection()));

    updateUI();
}

TaskFemConstraintTemperature::~TaskFemConstraintTemperature()
{
    delete ui;
}

void TaskFemConstraintTemperature::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintTemperature::onTempCfluxChanged(double val)
{
    Fem::ConstraintTemperature* pcConstraint = static_cast<Fem::ConstraintTemperature*>(ConstraintView->getObject());
    if (ui->rb_temperature->isChecked()) {
        pcConstraint->Temperature.setValue(val);
    } else {
        pcConstraint->CFlux.setValue(val);
    }
}

void TaskFemConstraintTemperature::Temp()
{
    Fem::ConstraintTemperature* pcConstraint = static_cast<Fem::ConstraintTemperature*>(ConstraintView->getObject());
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ConstraintType = %s",name.c_str(), get_constraint_type().c_str());
    std::string str = "Temperature";
    QString qstr = QString::fromStdString(str);
    ui->lbl_type->setText(qstr);
    Base::Quantity t = Base::Quantity(300, Base::Unit::Temperature);
    ui->if_temperature->setValue(t);
    pcConstraint->Temperature.setValue(300);
}

void TaskFemConstraintTemperature::Flux()
{
    Fem::ConstraintTemperature* pcConstraint = static_cast<Fem::ConstraintTemperature*>(ConstraintView->getObject());
    std::string name = ConstraintView->getObject()->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ConstraintType = %s",name.c_str(), get_constraint_type().c_str());
    std::string str = "Concentrated heat flux";
    QString qstr = QString::fromStdString(str);
    ui->lbl_type->setText(qstr);
    Base::Quantity c = Base::Quantity(0, Base::Unit::Power);
    ui->if_temperature->setValue(c);
    pcConstraint->CFlux.setValue(0);
}

void TaskFemConstraintTemperature::addToSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size() == 0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintTemperature* pcConstraint = static_cast<Fem::ConstraintTemperature*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end(); ++it){//for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        std::vector<std::string> subNames=it->getSubNames();
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(it->getFeatName());
        for (size_t subIt = 0; subIt < (subNames.size()); ++subIt){// for every selected sub element
            bool addMe = true;
            for (std::vector<std::string>::iterator itr = std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
                   itr != SubElements.end();
                   itr = std::find(++itr,SubElements.end(), subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj == Objects[std::distance(SubElements.begin(), itr)]){//if selected sub element's object equals the one in old list then it was added before so don't add
                    addMe = false;
                }
            }
            if (addMe){
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

void TaskFemConstraintTemperature::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size() == 0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintTemperature* pcConstraint = static_cast<Fem::ConstraintTemperature*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<size_t> itemsToDel;
    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end(); ++it){//for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"),tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames=it->getSubNames();
        App::DocumentObject* obj = it->getObject();

        for (size_t subIt = 0; subIt < (subNames.size()); ++subIt){// for every selected sub element
            for (std::vector<std::string>::iterator itr = std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
                itr != SubElements.end();
                itr = std::find(++itr, SubElements.end(), subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj == Objects[std::distance(SubElements.begin(), itr)]){//if selected sub element's object equals the one in old list then it was added before so mark for deletion
                    itemsToDel.push_back(std::distance(SubElements.begin(), itr));
                }
            }
        }
    }
    std::sort(itemsToDel.begin(), itemsToDel.end());
    while (itemsToDel.size() > 0){
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

void TaskFemConstraintTemperature::onReferenceDeleted() {
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

double TaskFemConstraintTemperature::get_temperature() const{
    Base::Quantity temperature =  ui->if_temperature->getQuantity();
    double temperature_in_kelvin = temperature.getValueAs(Base::Quantity::Kelvin);
    return temperature_in_kelvin;
}

double TaskFemConstraintTemperature::get_cflux() const{
    Base::Quantity cflux =  ui->if_temperature->getQuantity();
    double cflux_in_watt = cflux.getValueAs(Base::Quantity::Watt);
    return cflux_in_watt;
}

std::string TaskFemConstraintTemperature::get_constraint_type(void) const {
    std::string type;
    if (ui->rb_temperature->isChecked()) {
        type = "\"Temperature\"";
    } else if (ui->rb_cflux->isChecked()) {
        type = "\"CFlux\"";
    }
    return type;
}

bool TaskFemConstraintTemperature::event(QEvent *e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintTemperature::changeEvent(QEvent *)
{
//    TaskBox::changeEvent(e);
//    if (e->type() == QEvent::LanguageChange) {
//        ui->if_pressure->blockSignals(true);
//        ui->retranslateUi(proxy);
//        ui->if_pressure->blockSignals(false);
//    }
}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintTemperature::TaskDlgFemConstraintTemperature(ViewProviderFemConstraintTemperature *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintTemperature(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintTemperature::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint temperature");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintTemperature::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintTemperature* parameterTemperature = static_cast<const TaskFemConstraintTemperature*>(parameter);

    try {
        std::string scale = parameterTemperature->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintTemperature::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintTemperature.cpp"
