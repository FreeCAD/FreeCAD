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
#include <sstream>
#endif

#include <App/Document.h>
#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
#include <Mod/Fem/App/FemConstraintRigidBody.h>

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
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintRigidBody();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_references);
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

    // TODO: Relate inputs to property

    this->groupLayout()->addWidget(proxy);

    /* Note: */
    // Get the feature data
    Fem::ConstraintRigidBody* pcConstraint =
        static_cast<Fem::ConstraintRigidBody*>(ConstraintView->getObject());
    double fStates[15];
    fStates[0] = pcConstraint->xRefNode.getValue();
    fStates[1] = pcConstraint->yRefNode.getValue();
    fStates[2] = pcConstraint->zRefNode.getValue();
    fStates[3] = pcConstraint->xDisplacement.getValue();
    fStates[4] = pcConstraint->yDisplacement.getValue();
    fStates[5] = pcConstraint->zDisplacement.getValue();
    fStates[6] = pcConstraint->xRotation.getValue();
    fStates[7] = pcConstraint->yRotation.getValue();
    fStates[8] = pcConstraint->zRotation.getValue();
    fStates[9] = pcConstraint->xLoad.getValue();
    fStates[10] = pcConstraint->yLoad.getValue();
    fStates[11] = pcConstraint->zLoad.getValue();
    fStates[12] = pcConstraint->xMoment.getValue();
    fStates[13] = pcConstraint->yMoment.getValue();
    fStates[14] = pcConstraint->zMoment.getValue();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->if_ref_node_x->setValue(fStates[0]);
    ui->if_ref_node_y->setValue(fStates[1]);
    ui->if_ref_node_z->setValue(fStates[2]);
    ui->if_ref_load_x->setValue(fStates[9]);
    ui->if_ref_load_y->setValue(fStates[10]);
    ui->if_ref_load_z->setValue(fStates[11]);
    ui->if_rot_load_x->setValue(fStates[12]);
    ui->if_rot_load_y->setValue(fStates[13]);
    ui->if_rot_load_z->setValue(fStates[14]);

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
    Fem::ConstraintRigidBody* pcConstraint =
        static_cast<Fem::ConstraintRigidBody*>(ConstraintView->getObject());
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
            for (std::vector<std::string>::iterator itr =
                     std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
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
                    QString msg = tr(
                        "Only one type of selection (vertex,face or edge) per constraint allowed!");
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
    Fem::ConstraintRigidBody* pcConstraint =
        static_cast<Fem::ConstraintRigidBody*>(ConstraintView->getObject());
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
            for (std::vector<std::string>::iterator itr =
                     std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
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

const std::string TaskFemConstraintRigidBody::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

double TaskFemConstraintRigidBody::get_xRefNode() const
{
    return ui->if_ref_node_x->rawValue();
}
double TaskFemConstraintRigidBody::get_yRefNode() const
{
    return ui->if_ref_node_y->rawValue();
}
double TaskFemConstraintRigidBody::get_zRefNode() const
{
    return ui->if_ref_node_z->rawValue();
}
double TaskFemConstraintRigidBody::get_xLoad() const
{
    return ui->if_ref_load_x->rawValue();
}
double TaskFemConstraintRigidBody::get_yLoad() const
{
    return ui->if_ref_load_y->rawValue();
}
double TaskFemConstraintRigidBody::get_zLoad() const
{
    return ui->if_ref_load_z->rawValue();
}
double TaskFemConstraintRigidBody::get_xMoment() const
{
    return ui->if_rot_load_x->rawValue();
}
double TaskFemConstraintRigidBody::get_yMoment() const
{
    return ui->if_rot_load_y->rawValue();
}
double TaskFemConstraintRigidBody::get_zMoment() const
{
    return ui->if_rot_load_z->rawValue();
}
// TODO: This needs to be implemented
bool TaskFemConstraintRigidBody::get_DefineRefNode() const
{
    return true;
}

bool TaskFemConstraintRigidBody::event(QEvent* e)
{
    return TaskFemConstraint::KeyEvent(e);
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

void TaskDlgFemConstraintRigidBody::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint RigidBody");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            ViewProviderFemConstraint::gethideMeshShowPartStr(
                (static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument())
                .c_str());  // OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintRigidBody::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintRigidBody* parameters =
        static_cast<const TaskFemConstraintRigidBody*>(parameter);
    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.xRefNode = %f",
                                name.c_str(),
                                parameters->get_xRefNode());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.yRefNode = %f",
                                name.c_str(),
                                parameters->get_yRefNode());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.zRefNode = %f",
                                name.c_str(),
                                parameters->get_zRefNode());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.xLoad = %f",
                                name.c_str(),
                                parameters->get_xLoad());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.yLoad = %f",
                                name.c_str(),
                                parameters->get_yLoad());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.zLoad = %f",
                                name.c_str(),
                                parameters->get_zLoad());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.xMoment = %f",
                                name.c_str(),
                                parameters->get_xMoment());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.yMoment = %f",
                                name.c_str(),
                                parameters->get_yMoment());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.zMoment = %f",
                                name.c_str(),
                                parameters->get_zMoment());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.DefineRefNode = %s",
                                name.c_str(),
                                parameters->get_DefineRefNode() ? "True" : "False");

        std::string scale = parameters->getScale();  // OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Scale = %s",
                                name.c_str(),
                                scale.c_str());  // OvG: implement modified scale
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintRigidBody::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintRigidBody.cpp"
