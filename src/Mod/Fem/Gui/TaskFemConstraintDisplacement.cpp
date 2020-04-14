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

# include <QAction>
# include <QKeyEvent>
# include <QMessageBox>
# include <QRegExp>
# include <QTextStream>

# include <TopoDS.hxx>
# include <gp_Ax1.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <sstream>
#endif

#include "Mod/Fem/App/FemConstraintDisplacement.h"
#include "TaskFemConstraintDisplacement.h"
#include "ui_TaskFemConstraintDisplacement.h"
#include <App/Application.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Mod/Part/App/PartFeature.h>


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintDisplacement */

TaskFemConstraintDisplacement::TaskFemConstraintDisplacement(ViewProviderFemConstraintDisplacement *ConstraintView,QWidget *parent)
  : TaskFemConstraint(ConstraintView, parent, "FEM_ConstraintDisplacement")
{
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintDisplacement();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->lw_references);
    deleteAction->connect(deleteAction, SIGNAL(triggered()), this, SLOT(onReferenceDeleted()));

    connect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->lw_references, SIGNAL(itemClicked(QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    this->groupLayout()->addWidget(proxy);

    // Connect decimal value inputs
    connect(ui->spinxDisplacement, SIGNAL(valueChanged(double)),  this, SLOT(x_changed(double)));
    connect(ui->spinyDisplacement, SIGNAL(valueChanged(double)),  this, SLOT(y_changed(double)));
    connect(ui->spinzDisplacement, SIGNAL(valueChanged(double)),  this, SLOT(z_changed(double)));
    connect(ui->rotxv, SIGNAL(valueChanged(double)),  this, SLOT(x_rot(double)));
    connect(ui->rotyv, SIGNAL(valueChanged(double)),  this, SLOT(y_rot(double)));
    connect(ui->rotzv, SIGNAL(valueChanged(double)),  this, SLOT(z_rot(double)));
    // Connect check box values displacements
    connect(ui->dispxfix, SIGNAL(stateChanged(int)),  this, SLOT(fixx(int)));
    connect(ui->dispxfree, SIGNAL(stateChanged(int)),  this, SLOT(freex(int)));
    connect(ui->dispyfix, SIGNAL(stateChanged(int)),  this, SLOT(fixy(int)));
    connect(ui->dispyfree, SIGNAL(stateChanged(int)),  this, SLOT(freey(int)));
    connect(ui->dispzfix, SIGNAL(stateChanged(int)),  this, SLOT(fixz(int)));
    connect(ui->dispzfree, SIGNAL(stateChanged(int)),  this, SLOT(freez(int)));
    // Connect to check box values for rotations
    connect(ui->rotxfix, SIGNAL(stateChanged(int)),  this, SLOT(rotfixx(int)));
    connect(ui->rotxfree, SIGNAL(stateChanged(int)),  this, SLOT(rotfreex(int)));
    connect(ui->rotyfix, SIGNAL(stateChanged(int)),  this, SLOT(rotfixy(int)));
    connect(ui->rotyfree, SIGNAL(stateChanged(int)),  this, SLOT(rotfreey(int)));
    connect(ui->rotzfix, SIGNAL(stateChanged(int)),  this, SLOT(rotfixz(int)));
    connect(ui->rotzfree, SIGNAL(stateChanged(int)),  this, SLOT(rotfreez(int)));

    // Temporarily prevent unnecessary feature recomputes
    ui->spinxDisplacement->blockSignals(true);
    ui->spinyDisplacement->blockSignals(true);
    ui->spinzDisplacement->blockSignals(true);
    ui->rotxv->blockSignals(true);
    ui->rotyv->blockSignals(true);
    ui->rotzv->blockSignals(true);
    ui->dispxfix->blockSignals(true);
    ui->dispxfree->blockSignals(true);
    ui->dispyfix->blockSignals(true);
    ui->dispyfree->blockSignals(true);
    ui->dispzfix->blockSignals(true);
    ui->dispzfree->blockSignals(true);
    ui->rotxfix->blockSignals(true);
    ui->rotxfree->blockSignals(true);
    ui->rotyfix->blockSignals(true);
    ui->rotyfree->blockSignals(true);
    ui->rotzfix->blockSignals(true);
    ui->rotzfree->blockSignals(true);

    // Get the feature data
    Fem::ConstraintDisplacement* pcConstraint = static_cast<Fem::ConstraintDisplacement*>(ConstraintView->getObject());
    double fStates[6];
    bool bStates[12];
    fStates[0]=pcConstraint->xDisplacement.getValue();
    fStates[1]=pcConstraint->yDisplacement.getValue();
    fStates[2]=pcConstraint->zDisplacement.getValue();
    fStates[3]=pcConstraint->xRotation.getValue();
    fStates[4]=pcConstraint->yRotation.getValue();
    fStates[5]=pcConstraint->zRotation.getValue();
    bStates[0]=pcConstraint->xFix.getValue();
    bStates[1]=pcConstraint->xFree.getValue();
    bStates[2]=pcConstraint->yFix.getValue();
    bStates[3]=pcConstraint->yFree.getValue();
    bStates[4]=pcConstraint->zFix.getValue();
    bStates[5]=pcConstraint->zFree.getValue();
    bStates[6]=pcConstraint->rotxFix.getValue();
    bStates[7]=pcConstraint->rotxFree.getValue();
    bStates[8]=pcConstraint->rotyFix.getValue();
    bStates[9]=pcConstraint->rotyFree.getValue();
    bStates[10]=pcConstraint->rotzFix.getValue();
    bStates[11]=pcConstraint->rotzFree.getValue();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->spinxDisplacement->setValue(fStates[0]);
    ui->spinyDisplacement->setValue(fStates[1]);
    ui->spinzDisplacement->setValue(fStates[2]);
    ui->rotxv->setValue(fStates[3]);
    ui->rotyv->setValue(fStates[4]);
    ui->rotzv->setValue(fStates[5]);
    ui->dispxfix->setChecked(bStates[0]);
    ui->dispxfree->setChecked(bStates[1]);
    ui->dispyfix->setChecked(bStates[2]);
    ui->dispyfree->setChecked(bStates[3]);
    ui->dispzfix->setChecked(bStates[4]);
    ui->dispzfree->setChecked(bStates[5]);
    ui->rotxfix->setChecked(bStates[6]);
    ui->rotxfree->setChecked(bStates[7]);
    ui->rotyfix->setChecked(bStates[8]);
    ui->rotyfree->setChecked(bStates[9]);
    ui->rotzfix->setChecked(bStates[10]);
    ui->rotzfree->setChecked(bStates[11]);

    ui->lw_references->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->lw_references->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (Objects.size() > 0) {
        ui->lw_references->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }

    //Allow signals again
    ui->spinxDisplacement->blockSignals(false);
    ui->spinyDisplacement->blockSignals(false);
    ui->spinzDisplacement->blockSignals(false);
    ui->rotxv->blockSignals(false);
    ui->rotyv->blockSignals(false);
    ui->rotzv->blockSignals(false);
    ui->dispxfix->blockSignals(false);
    ui->dispxfree->blockSignals(false);
    ui->dispyfix->blockSignals(false);
    ui->dispyfree->blockSignals(false);
    ui->dispzfix->blockSignals(false);
    ui->dispzfree->blockSignals(false);
    ui->rotxfix->blockSignals(false);
    ui->rotxfree->blockSignals(false);
    ui->rotyfix->blockSignals(false);
    ui->rotyfree->blockSignals(false);
    ui->rotzfix->blockSignals(false);
    ui->rotzfree->blockSignals(false);

    //Selection buttons
    connect(ui->btnAdd, SIGNAL(clicked()),  this, SLOT(addToSelection()));
    connect(ui->btnRemove, SIGNAL(clicked()),  this, SLOT(removeFromSelection()));

    updateUI();
}

TaskFemConstraintDisplacement::~TaskFemConstraintDisplacement()
{
    delete ui;
}

void TaskFemConstraintDisplacement::updateUI()
{
    if (ui->lw_references->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

void TaskFemConstraintDisplacement::x_changed(double val){
    if (val!=0)
    {
        ui->dispxfree->setChecked(false);
        ui->dispxfix->setChecked(false);
    }
}

void TaskFemConstraintDisplacement::y_changed(double val){
    if (val!=0)
    {
        ui->dispyfree->setChecked(false);
        ui->dispyfix->setChecked(false);
    }
}

void TaskFemConstraintDisplacement::z_changed(double val){
    if (val!=0)
    {
        ui->dispzfree->setChecked(false);
        ui->dispzfix->setChecked(false);
    }
}

void TaskFemConstraintDisplacement::x_rot(double val){
    if (val!=0)
    {
        ui->rotxfree->setChecked(false);
        ui->rotxfix->setChecked(false);
    }
}

void TaskFemConstraintDisplacement::y_rot(double val){
    if (val!=0)
    {
        ui->rotyfree->setChecked(false);
        ui->rotyfix->setChecked(false);
    }
}

void TaskFemConstraintDisplacement::z_rot(double val){
     if (val!=0)
    {
        ui->rotzfree->setChecked(false);
        ui->rotzfix->setChecked(false);
    }
}

void TaskFemConstraintDisplacement::fixx(int val){
    if (val==2)
    {
        ui->dispxfree->setChecked(false);
        ui->spinxDisplacement->setValue(0);
    }
    else if (ui->spinxDisplacement->value()==0)
    {
         ui->dispxfree->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::freex(int val){
    if (val==2)
    {
        ui->dispxfix->setChecked(false);
        ui->spinxDisplacement->setValue(0);
    }
    else if (ui->spinxDisplacement->value()==0)
    {
         ui->dispxfix->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::fixy(int val){
    if (val==2)
    {
        ui->dispyfree->setChecked(false);
        ui->spinyDisplacement->setValue(0);
    }
    else if (ui->spinyDisplacement->value()==0)
    {
         ui->dispyfree->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::freey(int val){
    if (val==2)
    {
        ui->dispyfix->setChecked(false);
        ui->spinyDisplacement->setValue(0);
    }
    else if (ui->spinyDisplacement->value()==0)
    {
         ui->dispyfix->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::fixz(int val){
    if (val==2)
    {
        ui->dispzfree->setChecked(false);
        ui->spinzDisplacement->setValue(0);
    }
    else if (ui->spinzDisplacement->value()==0)
    {
         ui->dispzfree->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::freez(int val){
   if (val==2)
    {
        ui->dispzfix->setChecked(false);
        ui->spinzDisplacement->setValue(0);
    }
    else if (ui->spinzDisplacement->value()==0)
    {
         ui->dispzfix->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::rotfixx(int val){
    if (val==2)
    {
        ui->rotxfree->setChecked(false);
        ui->rotxv->setValue(0);
    }
    else if (ui->rotxv->value()==0)
    {
         ui->rotxfree->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::rotfreex(int val){
    if (val==2)
    {
        ui->rotxfix->setChecked(false);
        ui->rotxv->setValue(0);
    }
    else if (ui->rotxv->value()==0)
    {
         ui->rotxfix->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::rotfixy(int val){
    if (val==2)
    {
        ui->rotyfree->setChecked(false);
        ui->rotyv->setValue(0);
    }
    else if (ui->rotyv->value()==0)
    {
         ui->rotyfree->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::rotfreey(int val){
    if (val==2)
    {
        ui->rotyfix->setChecked(false);
        ui->rotyv->setValue(0);
    }
    else if (ui->rotyv->value()==0)
    {
         ui->rotyfix->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::rotfixz(int val){
    if (val==2)
    {
        ui->rotzfree->setChecked(false);
        ui->rotzv->setValue(0);
    }
    else if (ui->rotzv->value()==0)
    {
         ui->rotzfree->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::rotfreez(int val){
    if (val==2)
    {
        ui->rotzfix->setChecked(false);
        ui->rotzv->setValue(0);
    }
    else if (ui->rotzv->value()==0)
    {
         ui->rotzfix->setChecked(true);
    }
}

void TaskFemConstraintDisplacement::addToSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size() == 0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintDisplacement* pcConstraint = static_cast<Fem::ConstraintDisplacement*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end(); ++it){//for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it->getSubNames();
        App::DocumentObject* obj = it->getObject();
        for (size_t subIt = 0; subIt < (subNames.size()); ++subIt){// for every selected sub element
            bool addMe = true;
            for (std::vector<std::string>::iterator itr = std::find(SubElements.begin(), SubElements.end(), subNames[subIt]);
                   itr != SubElements.end();
                   itr = std::find(++itr, SubElements.end(), subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj == Objects[std::distance(SubElements.begin(), itr)]){//if selected sub element's object equals the one in old list then it was added before so don't add
                    addMe = false;
                }
            }
            // limit constraint such that only vertexes or faces or edges can be used depending on what was selected first
            std::string searchStr;
            if (subNames[subIt].find("Vertex") != std::string::npos)
                searchStr="Vertex";
            else if (subNames[subIt].find("Edge") != std::string::npos)
                searchStr = "Edge";
            else
                searchStr = "Face";
            for (unsigned int iStr = 0; iStr < (SubElements.size()); ++iStr){
                if ((SubElements[iStr].find(searchStr) == std::string::npos) && (SubElements.size() > 0)){
                    QString msg = tr("Only one type of selection (vertex,face or edge) per constraint allowed!");
                    QMessageBox::warning(this, tr("Selection error"), msg);
                    addMe=false;
                    break;
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

void TaskFemConstraintDisplacement::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size() == 0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintDisplacement* pcConstraint = static_cast<Fem::ConstraintDisplacement*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<size_t> itemsToDel;
    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin(); it != selection.end(); ++it){//for every selected object
        if (!it->isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it->getSubNames();
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

void TaskFemConstraintDisplacement::onReferenceDeleted() {
    TaskFemConstraintDisplacement::removeFromSelection(); //OvG: On right-click face is automatically selected, so just remove
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

double TaskFemConstraintDisplacement::get_spinxDisplacement() const{return ui->spinxDisplacement->value();}
double TaskFemConstraintDisplacement::get_spinyDisplacement() const{return ui->spinyDisplacement->value();}
double TaskFemConstraintDisplacement::get_spinzDisplacement() const{return ui->spinzDisplacement->value();}
double TaskFemConstraintDisplacement::get_rotxv() const{return ui->rotxv->value();}
double TaskFemConstraintDisplacement::get_rotyv() const{return ui->rotyv->value();}
double TaskFemConstraintDisplacement::get_rotzv() const{return ui->rotzv->value();}

bool TaskFemConstraintDisplacement::get_dispxfix() const{return ui->dispxfix->isChecked();}
bool TaskFemConstraintDisplacement::get_dispxfree() const{return ui->dispxfree->isChecked();}
bool TaskFemConstraintDisplacement::get_dispyfix() const{return ui->dispyfix->isChecked();}
bool TaskFemConstraintDisplacement::get_dispyfree() const{return ui->dispyfree->isChecked();}
bool TaskFemConstraintDisplacement::get_dispzfix() const{return ui->dispzfix->isChecked();}
bool TaskFemConstraintDisplacement::get_dispzfree() const{return ui->dispzfree->isChecked();}
bool TaskFemConstraintDisplacement::get_rotxfix() const{return ui->rotxfix->isChecked();}
bool TaskFemConstraintDisplacement::get_rotxfree() const{return ui->rotxfree->isChecked();}
bool TaskFemConstraintDisplacement::get_rotyfix() const{return ui->rotyfix->isChecked();}
bool TaskFemConstraintDisplacement::get_rotyfree() const{return ui->rotyfree->isChecked();}
bool TaskFemConstraintDisplacement::get_rotzfix() const{return ui->rotzfix->isChecked();}
bool TaskFemConstraintDisplacement::get_rotzfree() const{return ui->rotzfree->isChecked();}

bool TaskFemConstraintDisplacement::event(QEvent *e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintDisplacement::changeEvent(QEvent *)
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

TaskDlgFemConstraintDisplacement::TaskDlgFemConstraintDisplacement(ViewProviderFemConstraintDisplacement *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintDisplacement(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintDisplacement::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint displacement");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintDisplacement::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintDisplacement* parameterDisplacement = static_cast<const TaskFemConstraintDisplacement*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.xDisplacement = %f",
            name.c_str(), parameterDisplacement->get_spinxDisplacement());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.yDisplacement = %f",
            name.c_str(), parameterDisplacement->get_spinyDisplacement());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.zDisplacement = %f",
            name.c_str(), parameterDisplacement->get_spinzDisplacement());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.xRotation = %f",
            name.c_str(), parameterDisplacement->get_rotxv());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.yRotation = %f",
            name.c_str(), parameterDisplacement->get_rotyv());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.zRotation = %f",
            name.c_str(), parameterDisplacement->get_rotzv());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.xFree = %s",
            name.c_str(), parameterDisplacement->get_dispxfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.xFix = %s",
            name.c_str(), parameterDisplacement->get_dispxfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.yFree = %s",
            name.c_str(), parameterDisplacement->get_dispyfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.yFix = %s",
            name.c_str(), parameterDisplacement->get_dispyfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.zFree = %s",
            name.c_str(), parameterDisplacement->get_dispzfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.zFix = %s",
            name.c_str(), parameterDisplacement->get_dispzfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.rotxFree = %s",
            name.c_str(), parameterDisplacement->get_rotxfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.rotxFix = %s",
            name.c_str(), parameterDisplacement->get_rotxfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.rotyFree = %s",
            name.c_str(), parameterDisplacement->get_rotyfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.rotyFix = %s",
            name.c_str(), parameterDisplacement->get_rotyfix() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.rotzFree = %s",
            name.c_str(), parameterDisplacement->get_rotzfree() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.rotzFix = %s",
            name.c_str(), parameterDisplacement->get_rotzfix() ? "True" : "False");

        std::string scale = parameterDisplacement->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
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
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintDisplacement.cpp"
