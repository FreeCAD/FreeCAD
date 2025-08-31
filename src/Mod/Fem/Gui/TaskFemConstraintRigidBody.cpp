/***************************************************************************
 *   Copyright (c) 2022 Ajinkya Dahale <dahale.a.p@gmail.com>              *
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
#include <QAction>
#include <QMessageBox>
#include <limits>
#include <sstream>
#endif

#include <App/Document.h>
#include <Gui/Command.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Fem/App/FemConstraintRigidBody.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintRigidBody.h"
#include "ui_TaskFemConstraintRigidBody.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintRigidBody */

TaskFemConstraintRigidBody::TaskFemConstraintRigidBody(
    ViewProviderFemConstraintRigidBody* ConstraintView,
    QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintRigidBody")
{  // Note change "RigidBody" in line above to new constraint name
    constexpr float floatMax = std::numeric_limits<float>::max();
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintRigidBody();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createActions(ui->lw_references);
    deleteAction->connect(deleteAction,
                          &QAction::triggered,
                          this,
                          &TaskFemConstraintRigidBody::onReferenceDeleted);

    connect(ui->lw_references,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintRigidBody::setSelection);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintRigidBody::setSelection);
    connect(ui->cb_x_trans_mode,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskFemConstraintRigidBody::onTransModeXChanged);
    connect(ui->cb_y_trans_mode,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskFemConstraintRigidBody::onTransModeYChanged);
    connect(ui->cb_z_trans_mode,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskFemConstraintRigidBody::onTransModeZChanged);
    connect(ui->cb_x_rot_mode,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskFemConstraintRigidBody::onRotModeXChanged);
    connect(ui->cb_y_rot_mode,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskFemConstraintRigidBody::onRotModeYChanged);
    connect(ui->cb_z_rot_mode,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskFemConstraintRigidBody::onRotModeZChanged);
    connect(ui->qsb_ref_node_x,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintRigidBody::onRefNodeXChanged);
    connect(ui->qsb_ref_node_y,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintRigidBody::onRefNodeYChanged);
    connect(ui->qsb_ref_node_z,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskFemConstraintRigidBody::onRefNodeZChanged);

    this->groupLayout()->addWidget(proxy);

    /* Note: */
    // Get the feature data
    auto pcConstraint = ConstraintView->getObject<Fem::ConstraintRigidBody>();

    const Base::Vector3d& refNode = pcConstraint->ReferenceNode.getValue();
    const Base::Vector3d& disp = pcConstraint->Displacement.getValue();
    Base::Vector3d rotDir;
    double rotAngleRad;
    pcConstraint->Rotation.getValue().getValue(rotDir, rotAngleRad);
    Base::Quantity rotAngle(rotAngleRad, "rad");
    Base::Quantity forceX = pcConstraint->ForceX.getQuantityValue();
    Base::Quantity forceY = pcConstraint->ForceY.getQuantityValue();
    Base::Quantity forceZ = pcConstraint->ForceZ.getQuantityValue();
    Base::Quantity momentX = pcConstraint->MomentX.getQuantityValue();
    Base::Quantity momentY = pcConstraint->MomentY.getQuantityValue();
    Base::Quantity momentZ = pcConstraint->MomentZ.getQuantityValue();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->qsb_ref_node_x->setValue(refNode.x);
    ui->qsb_ref_node_y->setValue(refNode.y);
    ui->qsb_ref_node_z->setValue(refNode.z);
    ui->qsb_ref_node_x->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("ReferenceNode.x")));
    ui->qsb_ref_node_y->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("ReferenceNode.y")));
    ui->qsb_ref_node_z->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("ReferenceNode.z")));
    ui->qsb_ref_node_x->setMinimum(-floatMax);
    ui->qsb_ref_node_x->setMaximum(floatMax);
    ui->qsb_ref_node_y->setMinimum(-floatMax);
    ui->qsb_ref_node_y->setMaximum(floatMax);
    ui->qsb_ref_node_z->setMinimum(-floatMax);
    ui->qsb_ref_node_z->setMaximum(floatMax);

    ui->qsb_disp_x->setValue(disp.x);
    ui->qsb_disp_y->setValue(disp.y);
    ui->qsb_disp_z->setValue(disp.z);
    ui->qsb_disp_x->bind(App::ObjectIdentifier::parse(pcConstraint, std::string("Displacement.x")));
    ui->qsb_disp_y->bind(App::ObjectIdentifier::parse(pcConstraint, std::string("Displacement.y")));
    ui->qsb_disp_z->bind(App::ObjectIdentifier::parse(pcConstraint, std::string("Displacement.z")));
    ui->qsb_disp_x->setMinimum(-floatMax);
    ui->qsb_disp_x->setMaximum(floatMax);
    ui->qsb_disp_y->setMinimum(-floatMax);
    ui->qsb_disp_y->setMaximum(floatMax);
    ui->qsb_disp_z->setMinimum(-floatMax);
    ui->qsb_disp_z->setMaximum(floatMax);

    ui->spb_rot_axis_x->setValue(rotDir.x);
    ui->spb_rot_axis_y->setValue(rotDir.y);
    ui->spb_rot_axis_z->setValue(rotDir.z);
    ui->qsb_rot_angle->setValue(rotAngle.getValueAs(Base::Quantity::Degree));
    ui->spb_rot_axis_x->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("Rotation.Axis.x")));
    ui->spb_rot_axis_y->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("Rotation.Axis.y")));
    ui->spb_rot_axis_z->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("Rotation.Axis.z")));
    ui->qsb_rot_angle->bind(
        App::ObjectIdentifier::parse(pcConstraint, std::string("Rotation.Angle")));
    ui->spb_rot_axis_x->setMinimum(-floatMax);
    ui->spb_rot_axis_x->setMaximum(floatMax);
    ui->spb_rot_axis_y->setMinimum(-floatMax);
    ui->spb_rot_axis_y->setMaximum(floatMax);
    ui->spb_rot_axis_z->setMinimum(-floatMax);
    ui->spb_rot_axis_z->setMaximum(floatMax);
    ui->qsb_rot_angle->setMinimum(-floatMax);
    ui->qsb_rot_angle->setMaximum(floatMax);

    ui->qsb_force_x->setValue(forceX);
    ui->qsb_force_y->setValue(forceY);
    ui->qsb_force_z->setValue(forceZ);
    ui->qsb_force_x->bind(pcConstraint->ForceX);
    ui->qsb_force_y->bind(pcConstraint->ForceY);
    ui->qsb_force_z->bind(pcConstraint->ForceZ);
    ui->qsb_force_x->setMinimum(-floatMax);
    ui->qsb_force_x->setMaximum(floatMax);
    ui->qsb_force_y->setMinimum(-floatMax);
    ui->qsb_force_y->setMaximum(floatMax);
    ui->qsb_force_z->setMinimum(-floatMax);
    ui->qsb_force_z->setMaximum(floatMax);

    ui->qsb_moment_x->setValue(momentX);
    ui->qsb_moment_y->setValue(momentY);
    ui->qsb_moment_z->setValue(momentZ);
    ui->qsb_moment_x->bind(pcConstraint->MomentX);
    ui->qsb_moment_y->bind(pcConstraint->MomentY);
    ui->qsb_moment_z->bind(pcConstraint->MomentZ);
    ui->qsb_moment_x->setMinimum(-floatMax);
    ui->qsb_moment_x->setMaximum(floatMax);
    ui->qsb_moment_y->setMinimum(-floatMax);
    ui->qsb_moment_y->setMaximum(floatMax);
    ui->qsb_moment_z->setMinimum(-floatMax);
    ui->qsb_moment_z->setMaximum(floatMax);

    QStringList modeList;

    App::PropertyEnumeration* transMode = &pcConstraint->TranslationalModeX;
    for (auto item : transMode->getEnumVector()) {
        modeList << QString::fromUtf8(item.c_str());
    }
    ui->cb_x_trans_mode->addItems(modeList);
    ui->cb_y_trans_mode->addItems(modeList);
    ui->cb_z_trans_mode->addItems(modeList);
    ui->cb_x_trans_mode->setCurrentIndex(pcConstraint->TranslationalModeX.getValue());
    ui->cb_y_trans_mode->setCurrentIndex(pcConstraint->TranslationalModeY.getValue());
    ui->cb_z_trans_mode->setCurrentIndex(pcConstraint->TranslationalModeZ.getValue());

    modeList.clear();
    App::PropertyEnumeration* rotMode = &pcConstraint->RotationalModeX;
    for (auto item : rotMode->getEnumVector()) {
        modeList << QString::fromUtf8(item.c_str());
    }
    ui->cb_x_rot_mode->addItems(modeList);
    ui->cb_y_rot_mode->addItems(modeList);
    ui->cb_z_rot_mode->addItems(modeList);
    ui->cb_x_rot_mode->setCurrentIndex(pcConstraint->RotationalModeX.getValue());
    ui->cb_y_rot_mode->setCurrentIndex(pcConstraint->RotationalModeY.getValue());
    ui->cb_z_rot_mode->setCurrentIndex(pcConstraint->RotationalModeZ.getValue());

    onTransModeXChanged(pcConstraint->TranslationalModeX.getValue());
    onTransModeYChanged(pcConstraint->TranslationalModeY.getValue());
    onTransModeZChanged(pcConstraint->TranslationalModeZ.getValue());
    onRotModeXChanged(pcConstraint->RotationalModeX.getValue());
    onRotModeYChanged(pcConstraint->RotationalModeY.getValue());
    onRotModeZChanged(pcConstraint->RotationalModeZ.getValue());

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

    updateUI();
}

TaskFemConstraintRigidBody::~TaskFemConstraintRigidBody()
{
    delete ui;
}

void TaskFemConstraintRigidBody::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintRigidBody::addToSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintRigidBody* pcConstraint = ConstraintView->getObject<Fem::ConstraintRigidBody>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end();
         ++it) {  // for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        std::vector<std::string> subNames = it->getSubNames();
        App::DocumentObject* obj =
            ConstraintView->getObject()->getDocument()->getObject(it->getFeatName());
        for (size_t subIt = 0; subIt < (subNames.size());
             ++subIt) {  // for every selected sub element
            bool addMe = true;
            for (auto itr = std::ranges::find(SubElements, subNames[subIt]);
                 itr != SubElements.end();
                 itr = std::find(++itr,
                                 SubElements.end(),
                                 subNames[subIt])) {  // for every sub element in selection that
                                                      // matches one in old list
                if (obj
                    == Objects[std::distance(
                        SubElements.begin(),
                        itr)]) {  // if selected sub element's object equals the one in old list
                                  // then it was added before so don't add
                    addMe = false;
                }
            }
            // limit constraint such that only vertexes or faces or edges can be used depending on
            // what was selected first
            std::string searchStr;
            if (subNames[subIt].find("Vertex") != std::string::npos) {
                searchStr = "Vertex";
            }
            else if (subNames[subIt].find("Edge") != std::string::npos) {
                searchStr = "Edge";
            }
            else {
                searchStr = "Face";
            }
            for (size_t iStr = 0; iStr < (SubElements.size()); ++iStr) {
                if (SubElements[iStr].find(searchStr) == std::string::npos) {
                    QString msg = tr("Only one type of selection (vertex, face or edge) per "
                                     "constraint allowed!");
                    QMessageBox::warning(this, tr("Selection error"), msg);
                    addMe = false;
                    break;
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
    // Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintRigidBody::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintRigidBody* pcConstraint = ConstraintView->getObject<Fem::ConstraintRigidBody>();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<size_t> itemsToDel;
    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end();
         ++it) {  // for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it->getSubNames();
        App::DocumentObject* obj = it->getObject();

        for (size_t subIt = 0; subIt < (subNames.size());
             ++subIt) {  // for every selected sub element
            for (auto itr = std::ranges::find(SubElements, subNames[subIt]);
                 itr != SubElements.end();
                 itr = std::find(++itr,
                                 SubElements.end(),
                                 subNames[subIt])) {  // for every sub element in selection that
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

void TaskFemConstraintRigidBody::onReferenceDeleted()
{
    TaskFemConstraintRigidBody::removeFromSelection();
}

void TaskFemConstraintRigidBody::onRotModeXChanged(int item)
{
    const char* val = ConstraintView->getObject<Fem::ConstraintRigidBody>()
                          ->RotationalModeX.getEnumVector()[item]
                          .c_str();

    if (strcmp(val, "Free") == 0) {
        ui->spb_rot_axis_x->setEnabled(false);
        ui->qsb_moment_x->setEnabled(false);
    }
    else if (strcmp(val, "Constraint") == 0) {
        ui->spb_rot_axis_x->setEnabled(true);
        ui->qsb_moment_x->setEnabled(false);
    }
    else if (strcmp(val, "Load") == 0) {
        ui->spb_rot_axis_x->setEnabled(false);
        ui->qsb_moment_x->setEnabled(true);
    }
}
void TaskFemConstraintRigidBody::onRotModeYChanged(int item)
{
    const char* val = ConstraintView->getObject<Fem::ConstraintRigidBody>()
                          ->RotationalModeY.getEnumVector()[item]
                          .c_str();

    if (strcmp(val, "Free") == 0) {
        ui->spb_rot_axis_y->setEnabled(false);
        ui->qsb_moment_y->setEnabled(false);
    }
    else if (strcmp(val, "Constraint") == 0) {
        ui->spb_rot_axis_y->setEnabled(true);
        ui->qsb_moment_y->setEnabled(false);
    }
    else if (strcmp(val, "Load") == 0) {
        ui->spb_rot_axis_y->setEnabled(false);
        ui->qsb_moment_y->setEnabled(true);
    }
}
void TaskFemConstraintRigidBody::onRotModeZChanged(int item)
{
    const char* val = ConstraintView->getObject<Fem::ConstraintRigidBody>()
                          ->RotationalModeZ.getEnumVector()[item]
                          .c_str();

    if (strcmp(val, "Free") == 0) {
        ui->spb_rot_axis_z->setEnabled(false);
        ui->qsb_moment_z->setEnabled(false);
    }
    else if (strcmp(val, "Constraint") == 0) {
        ui->spb_rot_axis_z->setEnabled(true);
        ui->qsb_moment_z->setEnabled(false);
    }
    else if (strcmp(val, "Load") == 0) {
        ui->spb_rot_axis_z->setEnabled(false);
        ui->qsb_moment_z->setEnabled(true);
    }
}

void TaskFemConstraintRigidBody::onTransModeXChanged(int item)
{
    const char* val = ConstraintView->getObject<Fem::ConstraintRigidBody>()
                          ->TranslationalModeX.getEnumVector()[item]
                          .c_str();

    if (strcmp(val, "Free") == 0) {
        ui->qsb_disp_x->setEnabled(false);
        ui->qsb_force_x->setEnabled(false);
    }
    else if (strcmp(val, "Constraint") == 0) {
        ui->qsb_disp_x->setEnabled(true);
        ui->qsb_force_x->setEnabled(false);
    }
    else if (strcmp(val, "Load") == 0) {
        ui->qsb_disp_x->setEnabled(false);
        ui->qsb_force_x->setEnabled(true);
    }
}
void TaskFemConstraintRigidBody::onTransModeYChanged(int item)
{
    const char* val = ConstraintView->getObject<Fem::ConstraintRigidBody>()
                          ->TranslationalModeY.getEnumVector()[item]
                          .c_str();

    if (strcmp(val, "Free") == 0) {
        ui->qsb_disp_y->setEnabled(false);
        ui->qsb_force_y->setEnabled(false);
    }
    else if (strcmp(val, "Constraint") == 0) {
        ui->qsb_disp_y->setEnabled(true);
        ui->qsb_force_y->setEnabled(false);
    }
    else if (strcmp(val, "Load") == 0) {
        ui->qsb_disp_y->setEnabled(false);
        ui->qsb_force_y->setEnabled(true);
    }
}
void TaskFemConstraintRigidBody::onTransModeZChanged(int item)
{
    const char* val = ConstraintView->getObject<Fem::ConstraintRigidBody>()
                          ->TranslationalModeZ.getEnumVector()[item]
                          .c_str();

    if (strcmp(val, "Free") == 0) {
        ui->qsb_disp_z->setEnabled(false);
        ui->qsb_force_z->setEnabled(false);
    }
    else if (strcmp(val, "Constraint") == 0) {
        ui->qsb_disp_z->setEnabled(true);
        ui->qsb_force_z->setEnabled(false);
    }
    else if (strcmp(val, "Load") == 0) {
        ui->qsb_disp_z->setEnabled(false);
        ui->qsb_force_z->setEnabled(true);
    }
}

void TaskFemConstraintRigidBody::onRefNodeXChanged(double value)
{
    auto obj = ConstraintView->getObject<Fem::ConstraintRigidBody>();
    Base::Vector3d refNode = obj->ReferenceNode.getValue();
    refNode.x = value;
    obj->ReferenceNode.setValue(refNode);
}

void TaskFemConstraintRigidBody::onRefNodeYChanged(double value)
{
    auto obj = ConstraintView->getObject<Fem::ConstraintRigidBody>();
    Base::Vector3d refNode = obj->ReferenceNode.getValue();
    refNode.y = value;
    obj->ReferenceNode.setValue(refNode);
}

void TaskFemConstraintRigidBody::onRefNodeZChanged(double value)
{
    auto obj = ConstraintView->getObject<Fem::ConstraintRigidBody>();
    Base::Vector3d refNode = obj->ReferenceNode.getValue();
    refNode.z = value;
    obj->ReferenceNode.setValue(refNode);
}

const std::string TaskFemConstraintRigidBody::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

Base::Vector3d TaskFemConstraintRigidBody::getReferenceNode() const
{
    double x = ui->qsb_ref_node_x->rawValue();
    double y = ui->qsb_ref_node_y->rawValue();
    double z = ui->qsb_ref_node_z->rawValue();

    return Base::Vector3d(x, y, z);
}

Base::Vector3d TaskFemConstraintRigidBody::getDisplacement() const
{
    double x = ui->qsb_disp_x->rawValue();
    double y = ui->qsb_disp_y->rawValue();
    double z = ui->qsb_disp_z->rawValue();

    return Base::Vector3d(x, y, z);
}

Base::Rotation TaskFemConstraintRigidBody::getRotation() const
{
    double x = ui->spb_rot_axis_x->value();
    double y = ui->spb_rot_axis_y->value();
    double z = ui->spb_rot_axis_z->value();
    double angle = ui->qsb_rot_angle->value().getValueAs(Base::Quantity::Radian);

    return Base::Rotation(Base::Vector3d(x, y, z), angle);
}

std::vector<std::string> TaskFemConstraintRigidBody::getForce() const
{
    std::string x = ui->qsb_force_x->value().getSafeUserString();
    std::string y = ui->qsb_force_y->value().getSafeUserString();
    std::string z = ui->qsb_force_z->value().getSafeUserString();

    return {x, y, z};
}

std::vector<std::string> TaskFemConstraintRigidBody::getMoment() const
{
    std::string x = ui->qsb_moment_x->value().getSafeUserString();
    std::string y = ui->qsb_moment_y->value().getSafeUserString();
    std::string z = ui->qsb_moment_z->value().getSafeUserString();

    return std::vector<std::string>({x, y, z});
}

std::vector<std::string> TaskFemConstraintRigidBody::getTranslationalMode() const
{
    std::vector<std::string> transModes(3);
    transModes[0] = ui->cb_x_trans_mode->currentText().toStdString();
    transModes[1] = ui->cb_y_trans_mode->currentText().toStdString();
    transModes[2] = ui->cb_z_trans_mode->currentText().toStdString();

    return transModes;
}

std::vector<std::string> TaskFemConstraintRigidBody::getRotationalMode() const
{
    std::vector<std::string> rotModes(3);
    rotModes[0] = ui->cb_x_rot_mode->currentText().toStdString();
    rotModes[1] = ui->cb_y_rot_mode->currentText().toStdString();
    rotModes[2] = ui->cb_z_rot_mode->currentText().toStdString();

    return rotModes;
}

void TaskFemConstraintRigidBody::changeEvent(QEvent*)
{}

void TaskFemConstraintRigidBody::clearButtons(const SelectionChangeModes notThis)
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

TaskDlgFemConstraintRigidBody::TaskDlgFemConstraintRigidBody(
    ViewProviderFemConstraintRigidBody* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintRigidBody(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintRigidBody::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintRigidBody* parameters =
        static_cast<const TaskFemConstraintRigidBody*>(parameter);
    try {
        Base::Vector3d ref = parameters->getReferenceNode();
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.ReferenceNode = App.Vector(%f, %f, %f)",
                                name.c_str(),
                                ref.x,
                                ref.y,
                                ref.z);

        Base::Vector3d disp = parameters->getDisplacement();
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Displacement = App.Vector(%f, %f, %f)",
                                name.c_str(),
                                disp.x,
                                disp.y,
                                disp.z);

        Base::Rotation rot = parameters->getRotation();
        Base::Vector3d axis;
        double angle;
        rot.getValue(axis, angle);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.ActiveDocument.%s.Rotation = App.Rotation(App.Vector(%f,% f, %f), Radian=%f)",
            name.c_str(),
            axis.x,
            axis.y,
            axis.z,
            angle);

        auto force = parameters->getForce();
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.ForceX = \"%s\"",
                                name.c_str(),
                                force[0].c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.ForceY = \"%s\"",
                                name.c_str(),
                                force[1].c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.ForceZ = \"%s\"",
                                name.c_str(),
                                force[2].c_str());

        auto moment = parameters->getMoment();
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.MomentX = \"%s\"",
                                name.c_str(),
                                moment[0].c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.MomentY = \"%s\"",
                                name.c_str(),
                                moment[1].c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.MomentZ = \"%s\"",
                                name.c_str(),
                                moment[2].c_str());

        auto transModes = parameters->getTranslationalMode();
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.TranslationalModeX = \"%s\"",
                                name.c_str(),
                                transModes[0].c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.TranslationalModeY = \"%s\"",
                                name.c_str(),
                                transModes[1].c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.TranslationalModeZ = \"%s\"",
                                name.c_str(),
                                transModes[2].c_str());

        auto rotModes = parameters->getRotationalMode();
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.RotationalModeX = \"%s\"",
                                name.c_str(),
                                rotModes[0].c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.RotationalModeY = \"%s\"",
                                name.c_str(),
                                rotModes[1].c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.RotationalModeZ = \"%s\"",
                                name.c_str(),
                                rotModes[2].c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintRigidBody.cpp"
