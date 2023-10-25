/***************************************************************************
 *   Copyright (c) 2015, 2023 FreeCAD Developers                           *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
 *            Uwe Stöhr <uwestoehr@lyx.org>                                *
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
#include <Mod/Fem/App/FemConstraintDisplacement.h>

#include "TaskFemConstraintDisplacement.h"
#include "ui_TaskFemConstraintDisplacement.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintDisplacement */

TaskFemConstraintDisplacement::TaskFemConstraintDisplacement(
    ViewProviderFemConstraintDisplacement* ConstraintView,
    QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintDisplacement")
    , ui(new Ui_TaskFemConstraintDisplacement)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_references);
    connect(deleteAction,
            &QAction::triggered,
            this,
            &TaskFemConstraintDisplacement::onReferenceDeleted);

    connect(ui->lw_references,
            &QListWidget::currentItemChanged,
            this,
            &TaskFemConstraintDisplacement::setSelection);
    connect(ui->lw_references,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintDisplacement::setSelection);

    this->groupLayout()->addWidget(proxy);

    // setup ranges
    ui->spinxDisplacement->setMinimum(-FLOAT_MAX);
    ui->spinxDisplacement->setMaximum(FLOAT_MAX);
    ui->spinyDisplacement->setMinimum(-FLOAT_MAX);
    ui->spinyDisplacement->setMaximum(FLOAT_MAX);
    ui->spinzDisplacement->setMinimum(-FLOAT_MAX);
    ui->spinzDisplacement->setMaximum(FLOAT_MAX);
    ui->spinxRotation->setMinimum(-FLOAT_MAX);
    ui->spinxRotation->setMaximum(FLOAT_MAX);
    ui->spinyRotation->setMinimum(-FLOAT_MAX);
    ui->spinyRotation->setMaximum(FLOAT_MAX);
    ui->spinzRotation->setMinimum(-FLOAT_MAX);
    ui->spinzRotation->setMaximum(FLOAT_MAX);

    // Get the feature data
    Fem::ConstraintDisplacement* pcConstraint =
        static_cast<Fem::ConstraintDisplacement*>(ConstraintView->getObject());
    Base::Quantity fStates[6] {};
    const char* sStates[3] {};
    bool bStates[16] {};
    fStates[0] = pcConstraint->xDisplacement.getQuantityValue();
    fStates[1] = pcConstraint->yDisplacement.getQuantityValue();
    fStates[2] = pcConstraint->zDisplacement.getQuantityValue();
    fStates[3] = pcConstraint->xRotation.getQuantityValue();
    fStates[4] = pcConstraint->yRotation.getQuantityValue();
    fStates[5] = pcConstraint->zRotation.getQuantityValue();
    sStates[0] = pcConstraint->xDisplacementFormula.getValue();
    sStates[1] = pcConstraint->yDisplacementFormula.getValue();
    sStates[2] = pcConstraint->zDisplacementFormula.getValue();
    bStates[0] = pcConstraint->xFix.getValue();
    bStates[1] = pcConstraint->xFree.getValue();
    bStates[2] = pcConstraint->yFix.getValue();
    bStates[3] = pcConstraint->yFree.getValue();
    bStates[4] = pcConstraint->zFix.getValue();
    bStates[5] = pcConstraint->zFree.getValue();
    bStates[6] = pcConstraint->rotxFix.getValue();
    bStates[7] = pcConstraint->rotxFree.getValue();
    bStates[8] = pcConstraint->rotyFix.getValue();
    bStates[9] = pcConstraint->rotyFree.getValue();
    bStates[10] = pcConstraint->rotzFix.getValue();
    bStates[11] = pcConstraint->rotzFree.getValue();
    bStates[12] = pcConstraint->hasXFormula.getValue();
    bStates[13] = pcConstraint->hasYFormula.getValue();
    bStates[14] = pcConstraint->hasZFormula.getValue();
    bStates[15] = pcConstraint->useFlowSurfaceForce.getValue();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    ui->lw_references->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->lw_references->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (!Objects.empty()) {
        ui->lw_references->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }

    // Connect check box values displacements
    connect(ui->dispxfix, &QCheckBox::toggled, this, &TaskFemConstraintDisplacement::fixx);
    connect(ui->DisplacementXFormulaCB,
            &QCheckBox::toggled,
            this,
            &TaskFemConstraintDisplacement::formulaX);
    connect(ui->dispyfix, &QCheckBox::toggled, this, &TaskFemConstraintDisplacement::fixy);
    connect(ui->DisplacementYFormulaCB,
            &QCheckBox::toggled,
            this,
            &TaskFemConstraintDisplacement::formulaY);
    connect(ui->dispzfix, &QCheckBox::toggled, this, &TaskFemConstraintDisplacement::fixz);
    connect(ui->DisplacementZFormulaCB,
            &QCheckBox::toggled,
            this,
            &TaskFemConstraintDisplacement::formulaZ);
    connect(ui->FlowForceCB, &QCheckBox::toggled, this, &TaskFemConstraintDisplacement::flowForce);
    // Connect to check box values for rotations
    connect(ui->rotxfix, &QCheckBox::toggled, this, &TaskFemConstraintDisplacement::rotfixx);
    connect(ui->rotyfix, &QCheckBox::toggled, this, &TaskFemConstraintDisplacement::rotfixy);
    connect(ui->rotzfix, &QCheckBox::toggled, this, &TaskFemConstraintDisplacement::rotfixz);

    // Fill data into dialog elements
    ui->spinxDisplacement->setValue(fStates[0]);
    ui->spinyDisplacement->setValue(fStates[1]);
    ui->spinzDisplacement->setValue(fStates[2]);
    ui->spinxRotation->setValue(fStates[3]);
    ui->spinyRotation->setValue(fStates[4]);
    ui->spinzRotation->setValue(fStates[5]);
    ui->DisplacementXFormulaLE->setText(QString::fromUtf8(sStates[0]));
    ui->DisplacementYFormulaLE->setText(QString::fromUtf8(sStates[1]));
    ui->DisplacementZFormulaLE->setText(QString::fromUtf8(sStates[2]));
    ui->dispxfix->setChecked(bStates[0]);
    ui->DisplacementXGB->setChecked(!bStates[1]);
    ui->dispyfix->setChecked(bStates[2]);
    ui->DisplacementYGB->setChecked(!bStates[3]);
    ui->dispzfix->setChecked(bStates[4]);
    ui->DisplacementZGB->setChecked(!bStates[5]);
    ui->rotxfix->setChecked(bStates[6]);
    ui->RotationXGB->setChecked(!bStates[7]);
    ui->rotyfix->setChecked(bStates[8]);
    ui->RotationYGB->setChecked(!bStates[9]);
    ui->rotzfix->setChecked(bStates[10]);
    ui->RotationZGB->setChecked(!bStates[11]);
    ui->DisplacementXFormulaCB->setChecked(bStates[12]);
    ui->DisplacementYFormulaCB->setChecked(bStates[13]);
    ui->DisplacementZFormulaCB->setChecked(bStates[14]);
    ui->FlowForceCB->setChecked(bStates[15]);

    // Selection buttons
    buttonGroup->addButton(ui->btnAdd, (int)SelectionChangeModes::refAdd);
    buttonGroup->addButton(ui->btnRemove, (int)SelectionChangeModes::refRemove);

    // Bind input fields to properties
    ui->spinxDisplacement->bind(pcConstraint->xDisplacement);
    ui->spinyDisplacement->bind(pcConstraint->yDisplacement);
    ui->spinzDisplacement->bind(pcConstraint->zDisplacement);
    ui->spinxRotation->bind(pcConstraint->xRotation);
    ui->spinyRotation->bind(pcConstraint->yRotation);
    ui->spinzRotation->bind(pcConstraint->zRotation);

    updateUI();
}

TaskFemConstraintDisplacement::~TaskFemConstraintDisplacement() = default;

void TaskFemConstraintDisplacement::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintDisplacement::fixx(bool state)
{
    if (state) {
        ui->spinxDisplacement->setValue(0);
        ui->DisplacementXFormulaCB->setChecked(!state);
    }
    ui->spinxDisplacement->setEnabled(!state);
    ui->DisplacementXFormulaCB->setEnabled(!state);
}

void TaskFemConstraintDisplacement::formulaX(bool state)
{
    ui->spinxDisplacement->setEnabled(!state);
    ui->dispxfix->setEnabled(!state);
    ui->DisplacementXFormulaLE->setEnabled(state);
}

void TaskFemConstraintDisplacement::fixy(bool state)
{
    if (state) {
        ui->spinyDisplacement->setValue(0);
        ui->DisplacementYFormulaCB->setChecked(!state);
    }
    ui->spinyDisplacement->setEnabled(!state);
    ui->DisplacementYFormulaCB->setEnabled(!state);
}

void TaskFemConstraintDisplacement::formulaY(bool state)
{
    ui->spinyDisplacement->setEnabled(!state);
    ui->dispyfix->setEnabled(!state);
    ui->DisplacementYFormulaLE->setEnabled(state);
}

void TaskFemConstraintDisplacement::fixz(bool state)
{
    if (state) {
        ui->spinzDisplacement->setValue(0);
        ui->DisplacementZFormulaCB->setChecked(!state);
    }
    ui->spinzDisplacement->setEnabled(!state);
    ui->DisplacementZFormulaCB->setEnabled(!state);
}

void TaskFemConstraintDisplacement::formulaZ(bool state)
{
    ui->spinzDisplacement->setEnabled(!state);
    ui->dispzfix->setEnabled(!state);
    ui->DisplacementZFormulaLE->setEnabled(state);
}

void TaskFemConstraintDisplacement::flowForce(bool state)
{
    if (state) {
        ui->DisplacementXGB->setChecked(!state);
        ui->DisplacementYGB->setChecked(!state);
        ui->DisplacementZGB->setChecked(!state);
        ui->RotationXGB->setChecked(!state);
        ui->RotationYGB->setChecked(!state);
        ui->RotationZGB->setChecked(!state);
    }
}

void TaskFemConstraintDisplacement::rotfixx(bool state)
{
    if (state) {
        ui->spinxRotation->setValue(0);
    }
    ui->spinxRotation->setEnabled(!state);
}

void TaskFemConstraintDisplacement::formulaRotx(bool state)
{
    ui->spinxRotation->setEnabled(!state);
    ui->rotxfix->setEnabled(!state);
}

void TaskFemConstraintDisplacement::rotfixy(bool state)
{
    if (state) {
        ui->spinyRotation->setValue(0);
    }
    ui->spinyRotation->setEnabled(!state);
}

void TaskFemConstraintDisplacement::formulaRoty(bool state)
{
    ui->spinyRotation->setEnabled(!state);
    ui->rotyfix->setEnabled(!state);
}

void TaskFemConstraintDisplacement::rotfixz(bool state)
{
    if (state) {
        ui->spinzRotation->setValue(0);
    }
    ui->spinzRotation->setEnabled(!state);
}

void TaskFemConstraintDisplacement::formulaRotz(bool state)
{
    ui->spinzRotation->setEnabled(!state);
    ui->rotzfix->setEnabled(!state);
}

void TaskFemConstraintDisplacement::addToSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintDisplacement* pcConstraint =
        static_cast<Fem::ConstraintDisplacement*>(ConstraintView->getObject());
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
            // limit constraint such that only vertexes or faces or edges can be used depending on
            // what was selected first
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
                    QString msg = tr("Only one type of selection (vertex,face or edge) per "
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

void TaskFemConstraintDisplacement::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintDisplacement* pcConstraint =
        static_cast<Fem::ConstraintDisplacement*>(ConstraintView->getObject());
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

void TaskFemConstraintDisplacement::onReferenceDeleted()
{
    TaskFemConstraintDisplacement::removeFromSelection();  // OvG: On right-click face is
                                                           // automatically selected, so just remove
}

const std::string TaskFemConstraintDisplacement::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

std::string TaskFemConstraintDisplacement::get_spinxDisplacement() const
{
    return ui->spinxDisplacement->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintDisplacement::get_spinyDisplacement() const
{
    return ui->spinyDisplacement->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintDisplacement::get_spinzDisplacement() const
{
    return ui->spinzDisplacement->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintDisplacement::get_spinxRotation() const
{
    return ui->spinxRotation->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintDisplacement::get_spinyRotation() const
{
    return ui->spinyRotation->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintDisplacement::get_spinzRotation() const
{
    return ui->spinzRotation->value().getSafeUserString().toStdString();
}

std::string TaskFemConstraintDisplacement::get_xFormula() const
{
    QString xFormula = ui->DisplacementXFormulaLE->text();
    xFormula.replace(QString::fromLatin1("\""), QString::fromLatin1("\\\""));
    return xFormula.toStdString();
}

std::string TaskFemConstraintDisplacement::get_yFormula() const
{
    QString yFormula = ui->DisplacementYFormulaLE->text();
    yFormula.replace(QString::fromLatin1("\""), QString::fromLatin1("\\\""));
    return yFormula.toStdString();
}

std::string TaskFemConstraintDisplacement::get_zFormula() const
{
    QString zFormula = ui->DisplacementZFormulaLE->text();
    zFormula.replace(QString::fromLatin1("\""), QString::fromLatin1("\\\""));
    return zFormula.toStdString();
}

bool TaskFemConstraintDisplacement::get_dispxfix() const
{
    return ui->dispxfix->isChecked();
}

bool TaskFemConstraintDisplacement::get_dispxfree() const
{
    return !ui->DisplacementXGB->isChecked();
}

bool TaskFemConstraintDisplacement::get_hasDispXFormula() const
{
    return ui->DisplacementXFormulaCB->isChecked();
}

bool TaskFemConstraintDisplacement::get_dispyfix() const
{
    return ui->dispyfix->isChecked();
}

bool TaskFemConstraintDisplacement::get_dispyfree() const
{
    return !ui->DisplacementYGB->isChecked();
}

bool TaskFemConstraintDisplacement::get_hasDispYFormula() const
{
    return ui->DisplacementYFormulaCB->isChecked();
}

bool TaskFemConstraintDisplacement::get_dispzfix() const
{
    return ui->dispzfix->isChecked();
}

bool TaskFemConstraintDisplacement::get_dispzfree() const
{
    return !ui->DisplacementZGB->isChecked();
}

bool TaskFemConstraintDisplacement::get_hasDispZFormula() const
{
    return ui->DisplacementZFormulaCB->isChecked();
}

bool TaskFemConstraintDisplacement::get_rotxfix() const
{
    return ui->rotxfix->isChecked();
}

bool TaskFemConstraintDisplacement::get_rotxfree() const
{
    return !ui->RotationXGB->isChecked();
}

bool TaskFemConstraintDisplacement::get_rotyfix() const
{
    return ui->rotyfix->isChecked();
}

bool TaskFemConstraintDisplacement::get_rotyfree() const
{
    return !ui->RotationYGB->isChecked();
}

bool TaskFemConstraintDisplacement::get_rotzfix() const
{
    return ui->rotzfix->isChecked();
}

bool TaskFemConstraintDisplacement::get_rotzfree() const
{
    return !ui->RotationZGB->isChecked();
}

bool TaskFemConstraintDisplacement::get_useFlowSurfaceForce() const
{
    return ui->FlowForceCB->isChecked();
}

bool TaskFemConstraintDisplacement::event(QEvent* e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintDisplacement::changeEvent(QEvent*)
{
    //    TaskBox::changeEvent(e);
    //    if (e->type() == QEvent::LanguageChange) {
    //        ui->if_pressure->blockSignals(true);
    //        ui->retranslateUi(proxy);
    //        ui->if_pressure->blockSignals(false);
    //    }
}

void TaskFemConstraintDisplacement::clearButtons(const SelectionChangeModes notThis)
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

TaskDlgFemConstraintDisplacement::TaskDlgFemConstraintDisplacement(
    ViewProviderFemConstraintDisplacement* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintDisplacement(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintDisplacement::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Displacement boundary condition");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            ViewProviderFemConstraint::gethideMeshShowPartStr(
                (static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument())
                .c_str());  // OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintDisplacement::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintDisplacement* parameterDisplacement =
        static_cast<const TaskFemConstraintDisplacement*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.xDisplacement = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_spinxDisplacement().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.xDisplacementFormula = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_xFormula().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.yDisplacement = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_spinyDisplacement().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.yDisplacementFormula = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_yFormula().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.zDisplacement = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_spinzDisplacement().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.zDisplacementFormula = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_zFormula().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.xRotation = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_spinxRotation().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.yRotation = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_spinyRotation().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.zRotation = \"%s\"",
                                name.c_str(),
                                parameterDisplacement->get_spinzRotation().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.xFree = %s",
                                name.c_str(),
                                parameterDisplacement->get_dispxfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.xFix = %s",
                                name.c_str(),
                                parameterDisplacement->get_dispxfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.hasXFormula = %s",
                                name.c_str(),
                                parameterDisplacement->get_hasDispXFormula() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.yFree = %s",
                                name.c_str(),
                                parameterDisplacement->get_dispyfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.yFix = %s",
                                name.c_str(),
                                parameterDisplacement->get_dispyfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.hasYFormula = %s",
                                name.c_str(),
                                parameterDisplacement->get_hasDispYFormula() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.zFree = %s",
                                name.c_str(),
                                parameterDisplacement->get_dispzfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.zFix = %s",
                                name.c_str(),
                                parameterDisplacement->get_dispzfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.hasZFormula = %s",
                                name.c_str(),
                                parameterDisplacement->get_hasDispZFormula() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.rotxFree = %s",
                                name.c_str(),
                                parameterDisplacement->get_rotxfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.rotxFix = %s",
                                name.c_str(),
                                parameterDisplacement->get_rotxfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.rotyFree = %s",
                                name.c_str(),
                                parameterDisplacement->get_rotyfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.rotyFix = %s",
                                name.c_str(),
                                parameterDisplacement->get_rotyfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.rotzFree = %s",
                                name.c_str(),
                                parameterDisplacement->get_rotzfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.rotzFix = %s",
                                name.c_str(),
                                parameterDisplacement->get_rotzfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.useFlowSurfaceForce = %s",
                                name.c_str(),
                                parameterDisplacement->get_useFlowSurfaceForce() ? "True"
                                                                                 : "False");

        std::string scale = parameterDisplacement->getScale();  // OvG: determine modified scale
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

bool TaskDlgFemConstraintDisplacement::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintDisplacement.cpp"
