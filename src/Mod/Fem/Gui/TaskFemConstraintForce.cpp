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
#include <TopoDS.hxx>
#include <sstream>
#endif

#include <App/DocumentObject.h>
#include <App/OriginFeature.h>
#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/Fem/App/FemConstraintForce.h>
#include <Mod/Fem/App/FemTools.h>

#include "TaskFemConstraintForce.h"
#include "ui_TaskFemConstraintForce.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintForce */

TaskFemConstraintForce::TaskFemConstraintForce(ViewProviderFemConstraintForce* ConstraintView,
                                               QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintForce")
    , ui(new Ui_TaskFemConstraintForce)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // Get the feature data
    Fem::ConstraintForce* pcConstraint =
        static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    auto force = pcConstraint->Force.getQuantityValue();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<std::string> dirStrings = pcConstraint->Direction.getSubValues();
    QString dir;
    if (!dirStrings.empty()) {
        dir = makeRefText(pcConstraint->Direction.getValue(), dirStrings.front());
    }
    bool reversed = pcConstraint->Reversed.getValue();

    // Fill data into dialog elements
    ui->spinForce->setUnit(pcConstraint->Force.getUnit());
    ui->spinForce->setMinimum(0);
    ui->spinForce->setMaximum(FLOAT_MAX);
    ui->spinForce->setValue(force);
    ui->listReferences->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->listReferences->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (!Objects.empty()) {
        ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }
    ui->lineDirection->setText(dir.isEmpty() ? QString() : dir);
    ui->checkReverse->setChecked(reversed);

    // create a context menu for the listview of the references
    createDeleteAction(ui->listReferences);
    connect(deleteAction, &QAction::triggered, this, &TaskFemConstraintForce::onReferenceDeleted);
    connect(ui->buttonDirection,
            &QToolButton::clicked,
            this,
            &TaskFemConstraintForce::onButtonDirection);
    connect(ui->checkReverse, &QCheckBox::toggled, this, &TaskFemConstraintForce::onCheckReverse);
    connect(ui->listReferences,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintForce::setSelection);

    // Selection buttons
    buttonGroup->addButton(ui->btnAdd, (int)SelectionChangeModes::refAdd);
    buttonGroup->addButton(ui->btnRemove, (int)SelectionChangeModes::refRemove);

    ui->spinForce->bind(pcConstraint->Force);

    updateUI();
}

void TaskFemConstraintForce::updateUI()
{
    if (ui->listReferences->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintForce::addToSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintForce* pcConstraint =
        static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
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
                    QString msg = tr("Only one type of selection (vertex, face or edge) per "
                                     "analysis feature allowed!");
                    QMessageBox::warning(this, tr("Selection error"), msg);
                    addMe = false;
                    break;
                }
            }
            if (addMe) {
                QSignalBlocker block(ui->listReferences);
                Objects.push_back(obj);
                SubElements.push_back(subName);
                ui->listReferences->addItem(makeRefText(obj, subName));
            }
        }
    }
    // Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintForce::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintForce* pcConstraint =
        static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
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
        QSignalBlocker block(ui->listReferences);
        ui->listReferences->clear();
        for (unsigned int j = 0; j < Objects.size(); j++) {
            ui->listReferences->addItem(makeRefText(Objects[j], SubElements[j]));
        }
    }
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintForce::onReferenceDeleted()
{
    TaskFemConstraintForce::removeFromSelection();  // OvG: On right-click face is automatically
                                                    // selected, so just remove
}

std::pair<App::DocumentObject*, std::string>
TaskFemConstraintForce::getDirection(const std::vector<Gui::SelectionObject>& selection) const
{
    std::pair<App::DocumentObject*, std::string> link;
    if (selection.empty()) {
        return link;
    }

    // we only handle the first selected object
    Gui::SelectionObject selectionElement = selection.at(0);

    // Line or Plane
    Base::Type line = Base::Type::fromName("PartDesign::Line");
    Base::Type plane = Base::Type::fromName("PartDesign::Plane");
    if (selectionElement.isObjectTypeOf(App::Line::getClassTypeId())
        || selectionElement.isObjectTypeOf(App::Plane::getClassTypeId())) {
        link = std::make_pair(selectionElement.getObject(), std::string());
    }
    else if (selectionElement.isObjectTypeOf(line)) {
        link = std::make_pair(selectionElement.getObject(), std::string("Edge1"));
    }
    else if (selectionElement.isObjectTypeOf(plane)) {
        link = std::make_pair(selectionElement.getObject(), std::string("Face1"));
    }
    // Sub-element of Part object
    else if (selectionElement.isObjectTypeOf(Part::Feature::getClassTypeId())) {
        const std::vector<std::string>& subNames = selectionElement.getSubNames();
        if (subNames.size() != 1) {
            return link;
        }

        std::string subNamesElement = subNames[0];

        const Part::Feature* feat = static_cast<const Part::Feature*>(selectionElement.getObject());
        TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subNamesElement.c_str());

        if (ref.ShapeType() == TopAbs_EDGE) {
            if (Fem::Tools::isLinear(TopoDS::Edge(ref))) {
                link = std::make_pair(selectionElement.getObject(), subNamesElement);
            }
        }
        else if (ref.ShapeType() == TopAbs_FACE) {
            if (Fem::Tools::isPlanar(TopoDS::Face(ref))) {
                link = std::make_pair(selectionElement.getObject(), subNamesElement);
            }
        }
    }

    return link;
}

void TaskFemConstraintForce::onButtonDirection(const bool pressed)
{
    // sets the normal vector of the currently selecteed planar face as direction
    Q_UNUSED(pressed);

    clearButtons(SelectionChangeModes::none);

    auto link = getDirection(Gui::Selection().getSelectionEx());
    if (!link.first) {
        QMessageBox::warning(this, tr("Wrong selection"), tr("Select an edge or a face, please."));
        return;
    }

    try {
        std::vector<std::string> direction(1, link.second);
        Fem::ConstraintForce* pcConstraint =
            static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());

        // update the direction
        pcConstraint->Direction.setValue(link.first, direction);
        ui->lineDirection->setText(makeRefText(link.first, link.second));

        updateUI();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(this, tr("Wrong selection"), QString::fromLatin1(e.what()));
    }
}

void TaskFemConstraintForce::onCheckReverse(const bool pressed)
{
    Fem::ConstraintForce* pcConstraint =
        static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    pcConstraint->Reversed.setValue(pressed);
}

const std::string TaskFemConstraintForce::getForce() const
{
    return ui->spinForce->value().getSafeUserString().toStdString();
}

const std::string TaskFemConstraintForce::getReferences() const
{
    int rows = ui->listReferences->model()->rowCount();

    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->listReferences->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

const std::string TaskFemConstraintForce::getDirectionName() const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty()) {
        return "";
    }

    int pos = dir.find_last_of(":");
    return dir.substr(0, pos).c_str();
}

const std::string TaskFemConstraintForce::getDirectionObject() const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty()) {
        return "";
    }

    int pos = dir.find_last_of(":");
    return dir.substr(pos + 1).c_str();
}

bool TaskFemConstraintForce::getReverse() const
{
    return ui->checkReverse->isChecked();
}

TaskFemConstraintForce::~TaskFemConstraintForce() = default;

bool TaskFemConstraintForce::event(QEvent* e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintForce::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinForce->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->spinForce->blockSignals(false);
    }
}

void TaskFemConstraintForce::clearButtons(const SelectionChangeModes notThis)
{
    if (notThis != SelectionChangeModes::refAdd) {
        ui->btnAdd->setChecked(false);
    }
    if (notThis != SelectionChangeModes::refRemove) {
        ui->btnRemove->setChecked(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintForce::TaskDlgFemConstraintForce(ViewProviderFemConstraintForce* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintForce(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintForce::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Force load");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            ViewProviderFemConstraint::gethideMeshShowPartStr(
                (static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument())
                .c_str());  // OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintForce::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintForce* parameterForce =
        static_cast<const TaskFemConstraintForce*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Force = \"%s\"",
                                name.c_str(),
                                parameterForce->getForce().c_str());

        std::string dirname = parameterForce->getDirectionName().data();
        std::string dirobj = parameterForce->getDirectionObject().data();
        std::string scale = "1";

        if (!dirname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(dirname));
            buf = buf.arg(QString::fromStdString(dirobj));
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.Direction = %s",
                                    name.c_str(),
                                    buf.toStdString().c_str());
        }
        else {
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.Direction = None",
                                    name.c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Reversed = %s",
                                name.c_str(),
                                parameterForce->getReverse() ? "True" : "False");

        scale = parameterForce->getScale();  // OvG: determine modified scale
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

bool TaskDlgFemConstraintForce::reject()
{
    // roll back the changes
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintForce.cpp"
