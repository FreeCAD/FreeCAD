/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
#include <Mod/Fem/App/FemConstraintFixed.h>

#include "TaskFemConstraintFixed.h"
#include "ui_TaskFemConstraintFixed.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintFixed */

TaskFemConstraintFixed::TaskFemConstraintFixed(ViewProviderFemConstraintFixed* ConstraintView,
                                               QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintFixed")
    , ui(new Ui_TaskFemConstraintFixed)
{  // Note change "Fixed" in line above to new constraint name
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_references);
    connect(deleteAction, &QAction::triggered, this, &TaskFemConstraintFixed::onReferenceDeleted);
    connect(ui->lw_references,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintFixed::setSelection);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintFixed::setSelection);

    this->groupLayout()->addWidget(proxy);

    /* Note: */
    // Get the feature data
    Fem::ConstraintFixed* pcConstraint =
        static_cast<Fem::ConstraintFixed*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements

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

TaskFemConstraintFixed::~TaskFemConstraintFixed() = default;

void TaskFemConstraintFixed::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintFixed::addToSelection()
{
    // gets vector of selected objects of active document
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintFixed* pcConstraint =
        static_cast<Fem::ConstraintFixed*>(ConstraintView->getObject());
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
            // limit constraint such that only vertexes or faces or edges can be used depending
            // on what was selected first
            std::string searchStr;
            if (subName.find("Vertex") != std::string::npos) {
                searchStr = "Vertex";
            }
            else if (subName.find("Edge") != std::string::npos) {
                searchStr = "Edge";
            }
            else {
                searchStr = "Face";
            }
            for (const auto& SubElement : SubElements) {
                if (SubElement.find(searchStr) == std::string::npos) {
                    QString msg = tr("Only one type of selection (vertex, face or edge) per "
                                     "analysis feature allowed!");
                    QMessageBox::warning(this, tr("Selection error"), msg);
                    addMe = false;
                    break;
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

void TaskFemConstraintFixed::removeFromSelection()
{
    // gets vector of selected objects of active document
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintFixed* pcConstraint =
        static_cast<Fem::ConstraintFixed*>(ConstraintView->getObject());
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

void TaskFemConstraintFixed::onReferenceDeleted()
{
    TaskFemConstraintFixed::removeFromSelection();
}

const std::string TaskFemConstraintFixed::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

bool TaskFemConstraintFixed::event(QEvent* e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintFixed::changeEvent(QEvent*)
{}

void TaskFemConstraintFixed::clearButtons(const SelectionChangeModes notThis)
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

TaskDlgFemConstraintFixed::TaskDlgFemConstraintFixed(ViewProviderFemConstraintFixed* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintFixed(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintFixed::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Fixed boundary condition");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            ViewProviderFemConstraint::gethideMeshShowPartStr(
                (static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument())
                .c_str());  // OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintFixed::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintFixed* parameters =
        static_cast<const TaskFemConstraintFixed*>(parameter);
    std::string scale = parameters->getScale();  // OvG: determine modified scale
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.Scale = %s",
                            name.c_str(),
                            scale.c_str());  // OvG: implement modified scale
    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintFixed::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintFixed.cpp"
