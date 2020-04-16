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
# include <sstream>

# include <QAction>
# include <QKeyEvent>
# include <QMessageBox>
# include <QRegExp>
# include <QTextStream>

# include <Precision.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Plane.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <Geom_Line.hxx>
# include <gp_Lin.hxx>
#endif

#include "ui_TaskFemConstraintForce.h"
#include "TaskFemConstraintForce.h"
#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyGeo.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/Fem/App/FemConstraintForce.h>
#include <Mod/Fem/App/FemTools.h>
#include <Mod/Part/App/PartFeature.h>


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintForce */

TaskFemConstraintForce::TaskFemConstraintForce(ViewProviderFemConstraintForce *ConstraintView,QWidget *parent)
    : TaskFemConstraint(ConstraintView, parent, "FEM_ConstraintForce")
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintForce();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->listReferences);
    deleteAction->connect(deleteAction, SIGNAL(triggered()), this, SLOT(onReferenceDeleted()));

    connect(ui->spinForce, SIGNAL(valueChanged(double)),
            this, SLOT(onForceChanged(double)));
    connect(ui->buttonDirection, SIGNAL(pressed()),
            this, SLOT(onButtonDirection()));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));
    connect(ui->listReferences, SIGNAL(itemClicked(QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->spinForce->blockSignals(true);
    ui->listReferences->blockSignals(true);
    ui->buttonDirection->blockSignals(true);
    ui->checkReverse->blockSignals(true);

    // Get the feature data
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    double f = pcConstraint->Force.getValue();
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<std::string> dirStrings = pcConstraint->Direction.getSubValues();
    QString dir;
    if (!dirStrings.empty())
        dir = makeRefText(pcConstraint->Direction.getValue(), dirStrings.front());
    bool reversed = pcConstraint->Reversed.getValue();

    // Fill data into dialog elements
    ui->spinForce->setMinimum(0);
    ui->spinForce->setMaximum(FLOAT_MAX);
    ui->spinForce->setValue(f);
    ui->listReferences->clear();
    for (std::size_t i = 0; i < Objects.size(); i++)
        ui->listReferences->addItem(makeRefText(Objects[i], SubElements[i]));
    if (Objects.size() > 0)
        ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    ui->lineDirection->setText(dir.isEmpty() ? QString() : dir);
    ui->checkReverse->setChecked(reversed);

    ui->spinForce->blockSignals(false);
    ui->listReferences->blockSignals(false);
    ui->buttonDirection->blockSignals(false);
    ui->checkReverse->blockSignals(false);

    //Selection buttons
    connect(ui->btnAdd, SIGNAL(clicked()), this, SLOT(addToSelection()));
    connect(ui->btnRemove, SIGNAL(clicked()), this, SLOT(removeFromSelection()));

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
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size() == 0) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end(); ++it) {//for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it->getSubNames();
        App::DocumentObject* obj = it->getObject();
        for (size_t subIt = 0; subIt < (subNames.size()); ++subIt) {// for every selected sub element
            bool addMe = true;
            for (std::vector<std::string>::iterator itr = std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
                itr != SubElements.end();
                itr = std::find(++itr, SubElements.end(), subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj == Objects[std::distance(SubElements.begin(), itr)]) {//if selected sub element's object equals the one in old list then it was added before so don't add
                    addMe = false;
                }
            }
            // limit constraint such that only vertexes or faces or edges can be used depending on what was selected first
            std::string searchStr;
            if (subNames[subIt].find("Vertex") != std::string::npos)
                searchStr = "Vertex";
            else if (subNames[subIt].find("Edge") != std::string::npos)
                searchStr = "Edge";
            else
                searchStr = "Face";

            for (size_t iStr = 0; iStr < (SubElements.size()); ++iStr) {
                if (SubElements[iStr].find(searchStr) == std::string::npos) {
                    QString msg = tr("Only one type of selection (vertex,face or edge) per constraint allowed!");
                    QMessageBox::warning(this, tr("Selection error"), msg);
                    addMe = false;
                    break;
                }
            }
            if (addMe) {
                QSignalBlocker block(ui->listReferences);
                Objects.push_back(obj);
                SubElements.push_back(subNames[subIt]);
                ui->listReferences->addItem(makeRefText(obj, subNames[subIt]));
            }
        }
    }
    //Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintForce::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size() == 0) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<size_t> itemsToDel;
    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end(); ++it) {//for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it->getSubNames();
        App::DocumentObject* obj = it->getObject();

        for (size_t subIt = 0; subIt < (subNames.size()); ++subIt) {// for every selected sub element
            for (std::vector<std::string>::iterator itr = std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
                itr != SubElements.end();
                itr = std::find(++itr, SubElements.end(), subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj == Objects[std::distance(SubElements.begin(), itr)]) {//if selected sub element's object equals the one in old list then it was added before so mark for deletion
                    itemsToDel.push_back(std::distance(SubElements.begin(), itr));
                }
            }
        }
    }
    std::sort(itemsToDel.begin(), itemsToDel.end());
    while (itemsToDel.size() > 0) {
        Objects.erase(Objects.begin() + itemsToDel.back());
        SubElements.erase(SubElements.begin() + itemsToDel.back());
        itemsToDel.pop_back();
    }
    //Update UI
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

void TaskFemConstraintForce::onForceChanged(double f)
{
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    pcConstraint->Force.setValue(f);
}

void TaskFemConstraintForce::onReferenceDeleted() {
    TaskFemConstraintForce::removeFromSelection(); //OvG: On right-click face is automatically selected, so just remove
}

void TaskFemConstraintForce::onButtonDirection(const bool pressed)
{
    // sets the normal vector of the currently selecteed planar face as direction

    Q_UNUSED(pressed)
    //get vector of selected objects of active document
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); 
    if (selection.size() == 0) {
        QMessageBox::warning(this, tr("Empty selection"), tr("Select an edge or a face, please."));
        return;
    }
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());

    // we only handle the first selected object
    Gui::SelectionObject& selectionElement = selection.at(0);

    // we can only handle part objects
    if (!selectionElement.isObjectTypeOf(Part::Feature::getClassTypeId())) {
        QMessageBox::warning(this, tr("Wrong selection"), tr("Selected object is not a part object!"));
        return;
    }
    // get the names of the subobjects
    const std::vector<std::string>& subNames = selectionElement.getSubNames();

    if (subNames.size() != 1) {
        QMessageBox::warning(this, tr("Wrong selection"), tr("Only one planar face or edge can be selected!"));
        return;
    }
    // we are now sure we only have one object
    std::string subNamesElement = subNames[0];
    // vector for the direction
    std::vector<std::string> direction(1, subNamesElement);

    Part::Feature* feat = static_cast<Part::Feature*>(selectionElement.getObject());
    TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subNamesElement.c_str());

    if (subNamesElement.substr(0, 4) == "Face") {
        if (!Fem::Tools::isPlanar(TopoDS::Face(ref))) {
            QMessageBox::warning(this, tr("Wrong selection"), tr("Only planar faces can be picked for 3D"));
            return;
        }
    }
    else if (subNamesElement.substr(0, 4) == "Edge") { // 2D or 3D can use edge as direction vector
        if (!Fem::Tools::isLinear(TopoDS::Edge(ref))) {
            QMessageBox::warning(this, tr("Wrong selection"), tr("Only planar edges can be picked for 2D"));
            return;
        }
    }
    else {
        QMessageBox::warning(this, tr("Wrong selection"), tr("Only faces for 3D part or edges for 2D can be picked"));
        return;
    }

    // update the direction
    pcConstraint->Direction.setValue(feat, direction);
    ui->lineDirection->setText(makeRefText(feat, subNamesElement));

    //Update UI
    updateUI(); 
}

void TaskFemConstraintForce::onCheckReverse(const bool pressed)
{
    Fem::ConstraintForce* pcConstraint = static_cast<Fem::ConstraintForce*>(ConstraintView->getObject());
    pcConstraint->Reversed.setValue(pressed);
}

double TaskFemConstraintForce::getForce(void) const
{
    return ui->spinForce->value().getValue();
}

const std::string TaskFemConstraintForce::getReferences() const
{
    int rows = ui->listReferences->model()->rowCount();

    std::vector<std::string> items;
    for (int r = 0; r < rows; r++)
        items.push_back(ui->listReferences->item(r)->text().toStdString());
    return TaskFemConstraint::getReferences(items);
}

const std::string TaskFemConstraintForce::getDirectionName(void) const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty())
        return "";

    int pos = dir.find_last_of(":");
    return dir.substr(0, pos).c_str();
}

const std::string TaskFemConstraintForce::getDirectionObject(void) const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty())
        return "";

    int pos = dir.find_last_of(":");
    return dir.substr(pos+1).c_str();
}

bool TaskFemConstraintForce::getReverse() const
{
    return ui->checkReverse->isChecked();
}

TaskFemConstraintForce::~TaskFemConstraintForce()
{
    delete ui;
}

bool TaskFemConstraintForce::event(QEvent *e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintForce::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinForce->blockSignals(true);
        ui->retranslateUi(proxy);
        ui->spinForce->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintForce::TaskDlgFemConstraintForce(ViewProviderFemConstraintForce *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintForce(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintForce::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint force");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintForce::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintForce* parameterForce = static_cast<const TaskFemConstraintForce*>(parameter);

    try {
        //Gui::Command::openCommand("FEM force constraint changed");

        if (parameterForce->getForce()<=0)
        {
            QMessageBox::warning(parameter, tr("Input error"), tr("Please specify a force greater than 0"));
            return false;
        }
        else
        {
            QByteArray num = QByteArray::number(parameterForce->getForce());
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Force = %s",name.c_str(), num.data());
        }

        std::string dirname = parameterForce->getDirectionName().data();
        std::string dirobj = parameterForce->getDirectionObject().data();
        std::string scale = "1";

        if (!dirname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(dirname));
            buf = buf.arg(QString::fromStdString(dirobj));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = %s", name.c_str(), buf.toStdString().c_str());
        } else {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = None", name.c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %s", name.c_str(), parameterForce->getReverse() ? "True" : "False");

        scale = parameterForce->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
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
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintForce.cpp"
