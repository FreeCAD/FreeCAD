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
# include <QMessageBox>
# include <QAction>
# include <QRegExp>
# include <QTextStream>
# include <TopoDS.hxx>
# include <gp_Ax1.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <sstream>
#endif

#include "Mod/Fem/App/FemConstraintContact.h"
#include "TaskFemConstraintContact.h"
#include "ui_TaskFemConstraintContact.h"
#include <App/Application.h>
#include <Gui/Command.h>



#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintContact */

TaskFemConstraintContact::TaskFemConstraintContact(ViewProviderFemConstraintContact *ConstraintView,QWidget *parent)
  : TaskFemConstraint(ConstraintView, parent, "fem-constraint-contact")
{
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintContact();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    QAction* actionSlave = new QAction(tr("Delete"), ui->lw_referencesSlave);
    actionSlave->connect(actionSlave, SIGNAL(triggered()), this, SLOT(onReferenceDeletedSlave()));

    QAction* actionMaster = new QAction(tr("Delete"), ui->lw_referencesMaster);
    actionMaster->connect(actionMaster, SIGNAL(triggered()), this, SLOT(onReferenceDeletedMaster()));

    ui->lw_referencesSlave->addAction(actionSlave);
    ui->lw_referencesSlave->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->lw_referencesSlave, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    ui->lw_referencesMaster->addAction(actionMaster);
    ui->lw_referencesMaster->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->lw_referencesMaster, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    this->groupLayout()->addWidget(proxy);

/* Note: */
    // Get the feature data
    Fem::ConstraintContact* pcConstraint = static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    double S = pcConstraint->Slope.getValue();
    double F = pcConstraint->Friction.getValue();

    // Fill data into dialog elements
    ui->spSlope->setMinimum(1.0);
    ui->spSlope->setValue(S);
    ui->spFriction->setValue(F);

/* */

    ui->lw_referencesMaster->clear();
    ui->lw_referencesSlave->clear();

    // QMessageBox::warning(this, tr("Objects.size"), QString::number(Objects.size()));
    if (Objects.size() == 1) {
        QMessageBox::warning(this, tr("Selection error"), tr("Only one face in object! - moved to master face")); 
        ui->lw_referencesMaster->addItem(makeRefText(Objects[0], SubElements[0]));
    }

    if (Objects.size() == 2 ) {
        ui->lw_referencesMaster->addItem(makeRefText(Objects[1], SubElements[1]));		
        ui->lw_referencesSlave->addItem(makeRefText(Objects[0], SubElements[0]));
    }
}

    //Selection buttons
    connect(ui->btnAddSlave, SIGNAL(clicked()),  this, SLOT(addToSelectionSlave()));
    connect(ui->btnRemoveSlave, SIGNAL(clicked()),  this, SLOT(removeFromSelectionSlave()));

    connect(ui->btnAddMaster, SIGNAL(clicked()),  this, SLOT(addToSelectionMaster()));
    connect(ui->btnRemoveMaster, SIGNAL(clicked()),  this, SLOT(removeFromSelectionMaster()));

    updateUI();
}

TaskFemConstraintContact::~TaskFemConstraintContact()
{
    delete ui;
}

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
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();//gets vector of selected objects of active document
    if (rows==1){
        QMessageBox::warning(this, tr("Selection error"), tr("Only one master face and one slave face for a contact constraint!"));
        Gui::Selection().clearSelection();
        return;
    }

    if (selection.size()==0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    if ((rows==0) && (selection.size()>=2)){
        QMessageBox::warning(this, tr("Selection error"), tr("Only one slave face for a contact constraint!"));
        Gui::Selection().clearSelection();
        return;
    }

    Fem::ConstraintContact* pcConstraint = static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin();  it != selection.end(); ++it){//for every selected object
        if (static_cast<std::string>(it->getTypeName()).substr(0,4).compare(std::string("Part"))!=0){
            QMessageBox::warning(this, tr("Selection error"),tr("Selected object is not a part!"));
            return;
        }

        std::vector<std::string> subNames=it->getSubNames();
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(it->getFeatName());
        if (subNames.size()!=1){
            QMessageBox::warning(this, tr("Selection error"), tr("Only one slave face for a contact constraint!"));
            Gui::Selection().clearSelection();
            return;
        }
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
                disconnect(ui->lw_referencesSlave, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(setSelection(QListWidgetItem*)));
                Objects.push_back(obj);
                SubElements.push_back(subNames[subIt]);
                ui->lw_referencesSlave->addItem(makeRefText(obj, subNames[subIt]));
                connect(ui->lw_referencesSlave, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(setSelection(QListWidgetItem*)));
            }
        }
    }
    //Update UI
    pcConstraint->References.setValues(Objects,SubElements);
    updateUI();
}

void TaskFemConstraintContact::removeFromSelectionSlave()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size()==0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    Fem::ConstraintContact* pcConstraint = static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());
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
    disconnect(ui->lw_referencesSlave, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    ui->lw_referencesSlave->clear();
    connect(ui->lw_referencesSlave, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    pcConstraint->References.setValues(Objects,SubElements);
    updateUI();
}

void TaskFemConstraintContact::addToSelectionMaster()
{
    int rows = ui->lw_referencesMaster->model()->rowCount();
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();//gets vector of selected objects of active document
    if (rows==1){
        QMessageBox::warning(this, tr("Selection error"), tr("Only one master face and one slave face for a contact constraint!"));
        Gui::Selection().clearSelection();
        return;
    }

    if (selection.size()==0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    if ((rows==0) && (selection.size()>=2)){
        QMessageBox::warning(this, tr("Selection error"), tr("Only one master for a contact constraint!"));
        Gui::Selection().clearSelection();
        return;
    }

    Fem::ConstraintContact* pcConstraint = static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (std::vector<Gui::SelectionObject>::iterator it = selection.begin();  it != selection.end(); ++it){//for every selected object
        if (static_cast<std::string>(it->getTypeName()).substr(0,4).compare(std::string("Part"))!=0){
            QMessageBox::warning(this, tr("Selection error"),tr("Selected object is not a part!"));
            return;
        }

        std::vector<std::string> subNames=it->getSubNames();
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(it->getFeatName());
        if (subNames.size()!=1){
            QMessageBox::warning(this, tr("Selection error"), tr("Only one master face for a contact constraint!"));
            Gui::Selection().clearSelection();
            return;
        }
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
                disconnect(ui->lw_referencesMaster, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(setSelection(QListWidgetItem*)));
                Objects.push_back(obj);
                SubElements.push_back(subNames[subIt]);
                ui->lw_referencesMaster->addItem(makeRefText(obj, subNames[subIt]));
                connect(ui->lw_referencesMaster, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(setSelection(QListWidgetItem*)));
            }
        }
    }
    //Update UI
    pcConstraint->References.setValues(Objects,SubElements);
    updateUI();
}

void TaskFemConstraintContact::removeFromSelectionMaster()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size()==0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    Fem::ConstraintContact* pcConstraint = static_cast<Fem::ConstraintContact*>(ConstraintView->getObject());
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
    disconnect(ui->lw_referencesMaster, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    ui->lw_referencesMaster->clear();
    connect(ui->lw_referencesMaster, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    pcConstraint->References.setValues(Objects,SubElements);
    updateUI();
}

void TaskFemConstraintContact::setSelection(QListWidgetItem* item){
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

void TaskFemConstraintContact::onReferenceDeletedSlave() {
    TaskFemConstraintContact::removeFromSelectionSlave();
}

void TaskFemConstraintContact::onReferenceDeletedMaster() {
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

void TaskFemConstraintContact::changeEvent(QEvent *)
{
}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintContact::TaskDlgFemConstraintContact(ViewProviderFemConstraintContact *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintContact(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintContact::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint Contact");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::runCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintContact::accept()
{
/* Note: */
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintContact* parameterContact = static_cast<const TaskFemConstraintContact*>(parameter);

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Slope = %f",
            name.c_str(), parameterContact->get_Slope());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Friction = %f",
            name.c_str(), parameterContact->get_Friction());
        std::string scale = parameterContact->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
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
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}
#include "moc_TaskFemConstraintContact.cpp"
