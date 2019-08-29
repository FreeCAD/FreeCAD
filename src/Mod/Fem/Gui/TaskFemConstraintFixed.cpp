/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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

#include "Mod/Fem/App/FemConstraintFixed.h"
#include "TaskFemConstraintFixed.h"
#include "ui_TaskFemConstraintFixed.h"
#include <App/Application.h>
#include <Gui/Command.h>



#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintFixed */

TaskFemConstraintFixed::TaskFemConstraintFixed(ViewProviderFemConstraintFixed *ConstraintView,QWidget *parent)
  : TaskFemConstraint(ConstraintView, parent, "fem-constraint-fixed")
{ //Note change "Fixed" in line above to new constraint name
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintFixed();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    QAction* action = new QAction(tr("Delete"), ui->lw_references);
    action->connect(action, SIGNAL(triggered()), this, SLOT(onReferenceDeleted()));
    ui->lw_references->addAction(action);
    ui->lw_references->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->lw_references, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));

    this->groupLayout()->addWidget(proxy);

/* Note: */
    // Get the feature data
    Fem::ConstraintFixed* pcConstraint = static_cast<Fem::ConstraintFixed*>(ConstraintView->getObject());

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements

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

TaskFemConstraintFixed::~TaskFemConstraintFixed()
{
    delete ui;
}

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
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size()==0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    Fem::ConstraintFixed* pcConstraint = static_cast<Fem::ConstraintFixed*>(ConstraintView->getObject());
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
            for (std::vector<std::string>::iterator itr=std::find(SubElements.begin(),SubElements.end(),subNames[subIt]);
                   itr!= SubElements.end();
                   itr =  std::find(++itr,SubElements.end(),subNames[subIt]))
            {// for every sub element in selection that matches one in old list
                if (obj==Objects[std::distance(SubElements.begin(),itr)]){//if selected sub element's object equals the one in old list then it was added before so don't add
                    addMe=false;
                }
            }
            // limit constraint such that only vertexes or faces or edges can be used depending on what was selected first
            std::string searchStr("");
            if (subNames[subIt].find("Vertex")!=std::string::npos)
                searchStr="Vertex";
            else if (subNames[subIt].find("Edge")!=std::string::npos)
                searchStr="Edge";
            else
                searchStr="Face";
            for (unsigned int iStr=0;iStr<(SubElements.size());++iStr){
                if ((SubElements[iStr].find(searchStr)==std::string::npos)&&(SubElements.size()>0)){
                    QString msg = tr("Only one type of selection (vertex,face or edge) per constraint allowed!");
                    QMessageBox::warning(this, tr("Selection error"), msg);
                    addMe=false;
                    break;
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

void TaskFemConstraintFixed::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
    if (selection.size()==0){
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }

    Fem::ConstraintFixed* pcConstraint = static_cast<Fem::ConstraintFixed*>(ConstraintView->getObject());
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

void TaskFemConstraintFixed::setSelection(QListWidgetItem* item){
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

void TaskFemConstraintFixed::onReferenceDeleted() {
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


void TaskFemConstraintFixed::changeEvent(QEvent *)
{
}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintFixed::TaskDlgFemConstraintFixed(ViewProviderFemConstraintFixed *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintFixed(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintFixed::open()
{
    // a transaction is already open at creation time of the panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint fixed");
        Gui::Command::openCommand((const char*)msg.toUtf8());
        ConstraintView->setVisible(true);
        Gui::Command::doCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
    }
}

bool TaskDlgFemConstraintFixed::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintFixed* parameters = static_cast<const TaskFemConstraintFixed*>(parameter);
    std::string scale = parameters->getScale();  //OvG: determine modified scale
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale
    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintFixed::reject()
{
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintFixed.cpp"
