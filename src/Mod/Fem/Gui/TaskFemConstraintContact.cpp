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

#include "Mod/Fem/App/FemConstraintContact.h"
#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintContact.h"
#include "ui_TaskFemConstraintContact.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintContact */

TaskFemConstraintContact::TaskFemConstraintContact(ViewProviderFemConstraintContact* ConstraintView,
                                                   QWidget* parent)
    : TaskFemConstraint(ConstraintView, parent, "FEM_ConstraintContact")
    , ui(new Ui_TaskFemConstraintContact)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    QAction* actionSlave = new QAction(tr("Delete"), ui->lw_referencesSlave);
    connect(actionSlave,
            &QAction::triggered,
            this,
            &TaskFemConstraintContact::onReferenceDeletedSlave);

    QAction* actionMaster = new QAction(tr("Delete"), ui->lw_referencesMaster);
    connect(actionMaster,
            &QAction::triggered,
            this,
            &TaskFemConstraintContact::onReferenceDeletedMaster);

    ui->lw_referencesSlave->addAction(actionSlave);
    ui->lw_referencesSlave->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->lw_referencesSlave,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintContact::setSelection);

    ui->lw_referencesMaster->addAction(actionMaster);
    ui->lw_referencesMaster->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->lw_referencesMaster,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintContact::setSelection);

    this->groupLayout()->addWidget(proxy);

    /* Note: */
    // Get the feature data
    Fem::ConstraintContact* pcConstraint =
        static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    double slope = pcConstraint->Slope.getValue();
    double friction = pcConstraint->Friction.getValue();

    // Fill data into dialog elements
    ui->spSlope->setMinimum(1.0);
    ui->spSlope->setValue(slope);
    ui->spFriction->setValue(friction);

    /* */

    ui->lw_referencesMaster->clear();
    ui->lw_referencesSlave->clear();

    // QMessageBox::warning(this, tr("Objects.size"), QString::number(Objects.size()));
    if (Objects.size() == 1) {
        QMessageBox::warning(this,
                             tr("Selection error"),
                             tr("Only one face in object! - moved to master face"));
        ui->lw_referencesMaster->addItem(makeRefText(Objects[0], SubElements[0]));
    }

    if (Objects.size() == 2) {
        ui->lw_referencesMaster->addItem(makeRefText(Objects[1], SubElements[1]));
        ui->lw_referencesSlave->addItem(makeRefText(Objects[0], SubElements[0]));
    }

    // Selection buttons
    connect(ui->btnAddSlave,
            &QToolButton::clicked,
            this,
            &TaskFemConstraintContact::addToSelectionSlave);
    connect(ui->btnRemoveSlave,
            &QToolButton::clicked,
            this,
            &TaskFemConstraintContact::removeFromSelectionSlave);

    connect(ui->btnAddMaster,
            &QToolButton::clicked,
            this,
            &TaskFemConstraintContact::addToSelectionMaster);
    connect(ui->btnRemoveMaster,
            &QToolButton::clicked,
            this,
            &TaskFemConstraintContact::removeFromSelectionMaster);

    updateUI();
}

TaskFemConstraintContact::~TaskFemConstraintContact() = default;

void TaskFemConstraintContact::updateUI()
{
    if (ui->lw_referencesSlave->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
    if (ui->lw_referencesMaster->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintContact::addToSelectionSlave()
{
    int rows = ui->lw_referencesSlave->model()->rowCount();
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (rows == 1) {
        QMessageBox::warning(
            this,
            tr("Selection error"),
            tr("Only one master face and one slave face for a contact constraint!"));
        Gui::Selection().clearSelection();
        return;
    }
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    if ((rows == 0) && (selection.size() >= 2)) {
        QMessageBox::warning(this,
                             tr("Selection error"),
                             tr("Only one slave face for a contact constraint!"));
        Gui::Selection().clearSelection();
        return;
    }
    Fem::ConstraintContact* pcConstraint =
        static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        App::DocumentObject* obj = it.getObject();

        if (subNames.size() != 1) {
            QMessageBox::warning(this,
                                 tr("Selection error"),
                                 tr("Only one slave face for a contact constraint!"));
            Gui::Selection().clearSelection();
            return;
        }
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
                QSignalBlocker block(ui->lw_referencesSlave);
                Objects.push_back(obj);
                SubElements.push_back(subName);
                ui->lw_referencesSlave->addItem(makeRefText(obj, subName));
            }
        }
    }
    // Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintContact::removeFromSelectionSlave()
{
    // gets vector of selected objects of active document
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintContact* pcConstraint =
        static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());
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
        QSignalBlocker block(ui->lw_referencesSlave);
        ui->lw_referencesSlave->clear();
    }
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintContact::addToSelectionMaster()
{
    int rows = ui->lw_referencesMaster->model()->rowCount();
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (rows == 1) {
        QMessageBox::warning(
            this,
            tr("Selection error"),
            tr("Only one master face and one slave face for a contact constraint!"));
        Gui::Selection().clearSelection();
        return;
    }
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    if ((rows == 0) && (selection.size() >= 2)) {
        QMessageBox::warning(this,
                             tr("Selection error"),
                             tr("Only one master for a contact constraint!"));
        Gui::Selection().clearSelection();
        return;
    }
    Fem::ConstraintContact* pcConstraint =
        static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        App::DocumentObject* obj = it.getObject();
        if (subNames.size() != 1) {
            QMessageBox::warning(this,
                                 tr("Selection error"),
                                 tr("Only one master face for a contact constraint!"));
            Gui::Selection().clearSelection();
            return;
        }
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
                QSignalBlocker block(ui->lw_referencesMaster);
                Objects.push_back(obj);
                SubElements.push_back(subName);
                ui->lw_referencesMaster->addItem(makeRefText(obj, subName));
            }
        }
    }
    // Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintContact::removeFromSelectionMaster()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintContact* pcConstraint =
        static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());
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
        QSignalBlocker block(ui->lw_referencesMaster);
        ui->lw_referencesMaster->clear();
    }
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintContact::onReferenceDeletedSlave()
{
    TaskFemConstraintContact::removeFromSelectionSlave();
}

void TaskFemConstraintContact::onReferenceDeletedMaster()
{
    TaskFemConstraintContact::removeFromSelectionMaster();
}

const std::string TaskFemConstraintContact::getReferences() const
{
    int rowsSlave = ui->lw_referencesSlave->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rowsSlave; r++) {
        items.push_back(ui->lw_referencesSlave->item(r)->text().toStdString());
    }

    int rowsMaster = ui->lw_referencesMaster->model()->rowCount();
    for (int r = 0; r < rowsMaster; r++) {
        items.push_back(ui->lw_referencesMaster->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

/* Note: */
double TaskFemConstraintContact::get_Slope() const
{
    return ui->spSlope->rawValue();
}

double TaskFemConstraintContact::get_Friction() const
{
    return ui->spFriction->value();
}

void TaskFemConstraintContact::changeEvent(QEvent*)
{}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintContact::TaskDlgFemConstraintContact(
    ViewProviderFemConstraintContact* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintContact(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintContact::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Contact constraint");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::runCommand(
            Gui::Command::Doc,
            ViewProviderFemConstraint::gethideMeshShowPartStr(
                (static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument())
                .c_str());  // OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintContact::accept()
{
    /* Note: */
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintContact* parameterContact =
        static_cast<const TaskFemConstraintContact*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Slope = %f",
                                name.c_str(),
                                parameterContact->get_Slope());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Friction = %f",
                                name.c_str(),
                                parameterContact->get_Friction());
        std::string scale = parameterContact->getScale();  // OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Scale = %s",
                                name.c_str(),
                                scale.c_str());  // OvG: implement modified scale
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
    /* */
    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintContact::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}
#include "moc_TaskFemConstraintContact.cpp"
