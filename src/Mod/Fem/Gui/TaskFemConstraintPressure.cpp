/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
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

# include <QMessageBox>
# include <QAction>
# include <QRegExp>
# include <QTextStream>

# include <sstream>
#endif

#include "Mod/Fem/App/FemConstraintPressure.h"
#include "TaskFemConstraintPressure.h"
#include "ui_TaskFemConstraintPressure.h"
#include <App/Application.h>
#include <Gui/Command.h>



#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintPressure */

TaskFemConstraintPressure::TaskFemConstraintPressure(ViewProviderFemConstraintPressure *ConstraintView,QWidget *parent)
  : TaskFemConstraint(ConstraintView, parent, "fem-constraint-pressure")
{ //Note change "pressure" in line above to new constraint name
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintPressure();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    QAction* action = new QAction(tr("Delete"), ui->lw_references);
    action->connect(action, SIGNAL(triggered()), this, SLOT(onReferenceDeleted()));
    ui->lw_references->addAction(action);
    ui->lw_references->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    connect(ui->checkBoxReverse, SIGNAL(toggled(bool)),
        this, SLOT(onCheckReverse(bool)));

    this->groupLayout()->addWidget(proxy);

/* Note: */
    // Get the feature data
    Fem::ConstraintPressure* pcConstraint = static_cast<Fem::ConstraintPressure*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->if_pressure->setMinimum(0);
    ui->if_pressure->setMaximum(FLOAT_MAX);
    Base::Quantity p = Base::Quantity(1000 * (pcConstraint->Pressure.getValue()), Base::Unit::Stress);
    ui->if_pressure->setValue(p);
    bool reversed = pcConstraint->Reversed.getValue();
    ui->checkBoxReverse->setChecked(reversed);
/* */

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

TaskFemConstraintPressure::~TaskFemConstraintPressure()
{
    delete ui;
}

void TaskFemConstraintPressure::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintPressure::onCheckReverse(const bool pressed)
{
    Fem::ConstraintPressure* pcConstraint = static_cast<Fem::ConstraintPressure*>(ConstraintView->getObject());
    pcConstraint->Reversed.setValue(pressed);
 }

void TaskFemConstraintPressure::addToSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size()==0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    Fem::ConstraintPressure* pcConstraint = static_cast<Fem::ConstraintPressure*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin();  it != selection.end(); ++it){//for every selected object
        if (static_cast<std::string>(it->getTypeName()).substr(0,4).compare(std::string("Part"))!=0){
            QMessageBox::warning(this, tr("Selection error"),tr("Selected object is not a part!"));
            return;
        }

        std::vector<std::string> subNames=it->getSubNames();
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(it->getFeatName());


        for (unsigned int subIt=0;subIt<(subNames.size());++subIt){// for every selected sub element
            bool addMe=true;
        if (subNames[subIt].substr(0,4) != "Face") {
            QMessageBox::warning(this, tr("Selection error"), tr("Only faces can be picked"));
            return;
        }
            for (std::vector<std::string>::iterator itr=std::find(SubElements.begin(),SubElements.end(),subNames[subIt]);
                   itr!= SubElements.end();
                   itr =  std::find(++itr,SubElements.end(),subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj==Objects[std::distance(SubElements.begin(),itr)]){//if selected sub element's object equals the one in old list then it was added before so don't add
                    addMe=false;
                }
            }
            if (addMe){
                disconnect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(setSelection(QListWidgetItem*)));
                Objects.push_back(obj);
                SubElements.push_back(subNames[subIt]);
                ui->lw_references->addItem(makeRefText(obj, subNames[subIt]));
                connect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(setSelection(QListWidgetItem*)));
            }
        }
    }
    //Update UI
    pcConstraint->References.setValues(Objects,SubElements);
    updateUI();
}

void TaskFemConstraintPressure::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size()==0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    Fem::ConstraintPressure* pcConstraint = static_cast<Fem::ConstraintPressure*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<int> itemsToDel;
    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin();  it != selection.end(); ++it){//for every selected object
        if (static_cast<std::string>(it->getTypeName()).substr(0,4).compare(std::string("Part"))!=0){
            QMessageBox::warning(this, tr("Selection error"),tr("Selected object is not a part!"));
            return;
        }

        std::vector<std::string> subNames=it->getSubNames();
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(it->getFeatName());

        for (unsigned int subIt=0;subIt<(subNames.size());++subIt){// for every selected sub element
            for (std::vector<std::string>::iterator itr=std::find(SubElements.begin(),SubElements.end(),subNames[subIt]);
                itr!= SubElements.end();
                itr =  std::find(++itr,SubElements.end(),subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj==Objects[std::distance(SubElements.begin(),itr)]){//if selected sub element's object equals the one in old list then it was added before so mark for deletion
                    itemsToDel.push_back(std::distance(SubElements.begin(),itr));
                }
            }
        }
    }

    std::sort(itemsToDel.begin(),itemsToDel.end());
    while (itemsToDel.size()>0){
        Objects.erase(Objects.begin()+itemsToDel.back());
        SubElements.erase(SubElements.begin()+itemsToDel.back());
        itemsToDel.pop_back();
    }
    //Update UI
    disconnect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    ui->lw_references->clear();
    for (unsigned int j=0;j<Objects.size();j++){
        ui->lw_references->addItem(makeRefText(Objects[j], SubElements[j]));
    }
    connect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    pcConstraint->References.setValues(Objects,SubElements);
    updateUI();
}

void TaskFemConstraintPressure::setSelection(QListWidgetItem* item){
    std::string docName=ConstraintView->getObject()->getDocument()->getName();

    std::string s = item->text().toStdString();
    std::string delimiter = ":";

    size_t pos = 0;
    std::string objName;
    std::string subName;
    pos = s.find(delimiter);
    objName = s.substr(0, pos);
    s.erase(0, pos + delimiter.length());
    subName=s;

    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(docName.c_str(),objName.c_str(),subName.c_str(),0,0,0);
}

void TaskFemConstraintPressure::onReferenceDeleted() {
    TaskFemConstraintPressure::removeFromSelection();
}

const std::string TaskFemConstraintPressure::getReferences() const
{
    int rows = ui->lw_references->model()->rowCount();
    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->lw_references->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

/* Note: */
double TaskFemConstraintPressure::get_Pressure() const
{
    Base::Quantity pressure =  ui->if_pressure->getQuantity();
    double pressure_in_MPa = pressure.getValueAs(Base::Quantity::MegaPascal);
    return pressure_in_MPa;
}

bool TaskFemConstraintPressure::get_Reverse() const
{
    return ui->checkBoxReverse->isChecked();
}
/* */

void TaskFemConstraintPressure::changeEvent(QEvent *)
{
}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintPressure::TaskDlgFemConstraintPressure(ViewProviderFemConstraintPressure *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintPressure(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintPressure::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint pressure");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintPressure::accept()
{
/* Note: */
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintPressure* parameterPressure = static_cast<const TaskFemConstraintPressure*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Pressure = %f",
            name.c_str(), parameterPressure->get_Pressure());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %s",
            name.c_str(), parameterPressure->get_Reverse() ? "True" : "False");
        std::string scale = parameterPressure->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }
/* */
    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintPressure::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintPressure.cpp"
