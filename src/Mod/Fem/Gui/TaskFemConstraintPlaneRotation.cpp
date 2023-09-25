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
#include <TopoDS.hxx>
#include <sstream>
#endif

#include <App/DocumentObject.h>
#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/Fem/App/FemConstraintPlaneRotation.h>
#include <Mod/Fem/App/FemTools.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskFemConstraintPlaneRotation.h"
#include "ui_TaskFemConstraintPlaneRotation.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintPlaneRotation */

TaskFemConstraintPlaneRotation::TaskFemConstraintPlaneRotation(
    ViewProviderFemConstraintPlaneRotation* ConstraintView,
    QWidget* parent)
    : TaskFemConstraint(ConstraintView, parent, "FEM_ConstraintPlaneRotation")
    , ui(new Ui_TaskFemConstraintPlaneRotation)
{  // Note change "planerotation" in line above to new constraint name
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_references);
    connect(deleteAction,
            &QAction::triggered,
            this,
            &TaskFemConstraintPlaneRotation::onReferenceDeleted);
    connect(ui->lw_references,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintPlaneRotation::setSelection);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintPlaneRotation::setSelection);

    this->groupLayout()->addWidget(proxy);

    /* Note: */
    // Get the feature data
    Fem::ConstraintPlaneRotation* pcConstraint =
        static_cast<Fem::ConstraintPlaneRotation*>(ConstraintView->getObject());

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
    connect(ui->btnAdd,
            &QToolButton::clicked,
            this,
            &TaskFemConstraintPlaneRotation::addToSelection);
    connect(ui->btnRemove,
            &QToolButton::clicked,
            this,
            &TaskFemConstraintPlaneRotation::removeFromSelection);

    updateUI();
}

TaskFemConstraintPlaneRotation::~TaskFemConstraintPlaneRotation() = default;

void TaskFemConstraintPlaneRotation::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintPlaneRotation::addToSelection()
{
    int rows = ui->lw_references->model()->rowCount();
    if (rows == 1) {
        QMessageBox::warning(
            this,
            tr("Selection error"),
            tr("Only one face can be selected for a plane multi-point constraint!"));
        Gui::Selection().clearSelection();
        return;
    }
    else {
        std::vector<Gui::SelectionObject> selection =
            Gui::Selection()
                .getSelectionEx();  // gets vector of selected objects of active document
        if (selection.empty()) {
            QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
            return;
        }
        Fem::ConstraintPlaneRotation* pcConstraint =
            static_cast<Fem::ConstraintPlaneRotation*>(ConstraintView->getObject());
        std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
        std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

        for (auto& it : selection) {  // for every selected object
            if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
                QMessageBox::warning(this,
                                     tr("Selection error"),
                                     tr("Selected object is not a part!"));
                return;
            }
            const std::vector<std::string>& subNames = it.getSubNames();
            App::DocumentObject* obj = it.getObject();

            if (subNames.size() == 1) {
                for (const auto& subName : subNames) {  // for every selected sub element
                    bool addMe = true;
                    if ((subName.substr(0, 4) != "Face")) {
                        QMessageBox::warning(this,
                                             tr("Selection error"),
                                             tr("Only faces can be picked"));
                        return;
                    }
                    Part::Feature* feat = static_cast<Part::Feature*>(obj);
                    TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subName.c_str());
                    if ((subName.substr(0, 4) == "Face")) {
                        if (!Fem::Tools::isPlanar(TopoDS::Face(ref))) {
                            QMessageBox::warning(this,
                                                 tr("Selection error"),
                                                 tr("Only planar faces can be picked"));
                            return;
                        }
                    }
                    for (std::vector<std::string>::iterator itr =
                             std::find(SubElements.begin(), SubElements.end(), subName);
                         itr != SubElements.end();
                         itr = std::find(++itr,
                                         SubElements.end(),
                                         subName)) {  // for every sub element in selection
                                                      // that matches one in old list
                        if (obj
                            == Objects[std::distance(
                                SubElements.begin(),
                                itr)]) {  // if selected sub element's object equals the one in old
                                          // list then it was added before so don't add
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
            else {
                QMessageBox::warning(
                    this,
                    tr("Selection error"),
                    tr("Only one face can be selected for a plane multi-point constraint!"));
                Gui::Selection().clearSelection();
                return;
            }
            // Update UI
            pcConstraint->References.setValues(Objects, SubElements);
            updateUI();
        }
    }
}

void TaskFemConstraintPlaneRotation::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintPlaneRotation* pcConstraint =
        static_cast<Fem::ConstraintPlaneRotation*>(ConstraintView->getObject());
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

void TaskFemConstraintPlaneRotation::onReferenceDeleted()
{
    TaskFemConstraintPlaneRotation::removeFromSelection();
}

const std::string TaskFemConstraintPlaneRotation::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

bool TaskFemConstraintPlaneRotation::event(QEvent* e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintPlaneRotation::changeEvent(QEvent*)
{}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintPlaneRotation::TaskDlgFemConstraintPlaneRotation(
    ViewProviderFemConstraintPlaneRotation* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintPlaneRotation(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintPlaneRotation::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Plane multi-point constraint");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            ViewProviderFemConstraint::gethideMeshShowPartStr(
                (static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument())
                .c_str());  // OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintPlaneRotation::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintPlaneRotation* parameters =
        static_cast<const TaskFemConstraintPlaneRotation*>(parameter);
    std::string scale = parameters->getScale();  // OvG: determine modified scale
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.Scale = %s",
                            name.c_str(),
                            scale.c_str());  // OvG: implement modified scale
    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintPlaneRotation::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintPlaneRotation.cpp"
