/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>

#include <Mod/PartDesign/App/ShapeBinder.h>
#include <Mod/Part/App/PartFeature.h>

#include "ReferenceSelection.h"
#include "Workbench.h"

#include "ui_TaskShapeBinder.h"
#include "TaskShapeBinder.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskShapeBinder */


//**************************************************************************
//**************************************************************************
// Task Parameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// TODO Review and cleanup the file (2015-09-11, Fat-Zer)

TaskShapeBinder::TaskShapeBinder(ViewProviderShapeBinder *view, bool /*newObj*/, QWidget *parent)
    : Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("PartDesign_ShapeBinder"),
                             tr("Datum shape parameters"), true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskShapeBinder();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->buttonBase, SIGNAL(toggled(bool)),
            this, SLOT(onBaseButton(bool)));
    
    this->groupLayout()->addWidget(proxy);
    
    Gui::Document* doc = Gui::Application::Instance->activeDocument(); 
    vp = view;
    
    //add initial values   
    App::GeoFeature* obj = nullptr;
    std::vector<std::string> subs;
            
    PartDesign::ShapeBinder::getFilteredReferences(&static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support, obj, subs);            
     
    if(obj)
        ui->baseEdit->setText(QString::fromUtf8(obj->getNameInDocument()));

    for (auto sub : subs)
        ui->listWidgetReferences->addItem(QString::fromStdString(sub));
 
    //make sure th euser sees al important things: the base feature to select edges and the 
    //spine/auxiliary spine he already selected
    if(obj) {
        auto* svp = doc->getViewProvider(obj);
        if(svp) {
            supportShow = svp->isShow();
            svp->setVisible(true);
        }
    }
    
    updateUI();
}

void TaskShapeBinder::updateUI()
{
    
}

void TaskShapeBinder::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    ui->listWidgetReferences->addItem(QString::fromStdString(msg.pSubName));
                
                ui->baseEdit->setText(QString::fromStdString(msg.pObjectName));
            }
            else if (selectionMode == refRemove) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, QString::fromUtf8(msg.pSubName));
                else {
                    ui->baseEdit->clear();
                }                
            } else if(selectionMode == refObjAdd) {
                ui->listWidgetReferences->clear();
                ui->baseEdit->setText(QString::fromUtf8(msg.pObjectName));
            }
            clearButtons();
            static_cast<ViewProviderShapeBinder*>(vp)->highlightReferences(false, false);
            vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
        } 
        clearButtons();
        exitSelectionMode();
    }
}

TaskShapeBinder::~TaskShapeBinder()
{/*
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    Gui::Document* doc = Gui::Application::Instance->activeDocument(); 
    
    //make sure th euser sees al important things: the base feature to select edges and the 
    //spine/auxiliary spine he already selected
    if(pipe->BaseFeature.getValue())
        doc->getViewProvider(pipe->BaseFeature.getValue())->hide();
    if(pipe->Spine.getValue()) {
        auto* svp = doc->getViewProvider(pipe->Spine.getValue());
        svp->setVisible(supportShow);
        supportShow = false;
    }
    static_cast<ViewProviderPipe*>(vp)->highlightReferences(false, false);
    */
    delete ui;
}

void TaskShapeBinder::changeEvent(QEvent *)
{
}

void TaskShapeBinder::onButtonRefAdd(bool checked) {
    
    if (checked) {
        //clearButtons(refAdd);
        //hideObject();
        Gui::Selection().clearSelection();
        selectionMode = refAdd;
        vp->highlightReferences(true, false);
    }
}

void TaskShapeBinder::onButtonRefRemove(bool checked) {

    if (checked) {
        //clearButtons(refRemove);
        //hideObject();
        Gui::Selection().clearSelection();        
        selectionMode = refRemove;
        vp->highlightReferences(true, false);
    }
}

void TaskShapeBinder::onBaseButton(bool checked) {

    if (checked) {
        //clearButtons(refRemove);
        //hideObject();
        Gui::Selection().clearSelection();        
        selectionMode = refObjAdd;
        //DressUpView->highlightReferences(true);
    }
}

void TaskShapeBinder::removeFromListWidget(QListWidget* widget, QString itemstr) {

    QList<QListWidgetItem*> items = widget->findItems(itemstr, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

bool TaskShapeBinder::referenceSelected(const SelectionChanges& msg) const {
    
    if ((msg.Type == Gui::SelectionChanges::AddSelection) && (
                (selectionMode == refAdd) || (selectionMode == refRemove) 
                || (selectionMode == refObjAdd))) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();        
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;
       
        //change the references 
        std::string subName(msg.pSubName);

        Part::Feature* selectedObj = nullptr;
        App::GeoFeature* obj = nullptr;
        std::vector<std::string> refs;

        PartDesign::ShapeBinder::getFilteredReferences(&static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support, obj, refs);

        // get selected object
        auto docObj = vp->getObject()->getDocument()->getObject(msg.pObjectName);
        if (docObj && docObj->isDerivedFrom(Part::Feature::getClassTypeId())) {
            selectedObj = static_cast<Part::Feature*>(docObj);
        }

        // ensure we have a valid object
        if (!selectedObj) {
            return false;
        }
        if (!obj) {
            // Support has not been set before
            obj = selectedObj;
        }

        if(selectionMode != refObjAdd) {
            // ensure the new selected subref belongs to the same object
            if (strcmp(msg.pObjectName, obj->getNameInDocument()) != 0)
                return false;

            std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

            if (selectionMode == refAdd) {
                if (f == refs.end())
                    refs.push_back(subName);
                else
                    return false; // duplicate selection
            } else {
                if (f != refs.end())
                    refs.erase(f);
                else
                    return false;
            }
        }
        else {
            // change object
            refs.clear();
            obj = selectedObj;
        }
        
        static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support.setValue(obj, refs);

        return true;
    }

    return false;
}

void TaskShapeBinder::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
    ui->buttonBase->setChecked(false);
}

void TaskShapeBinder::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgShapeBinder::TaskDlgShapeBinder(ViewProviderShapeBinder *view, bool newObj)
   : Gui::TaskView::TaskDialog()
{
    assert(view);
    parameter    = new TaskShapeBinder(view, newObj);
    vp = view;

    Content.push_back(parameter);
}

TaskDlgShapeBinder::~TaskDlgShapeBinder()
{

}

//==== calls from the TaskView ===============================================================


bool TaskDlgShapeBinder::accept()
{
    std::string name = vp->getObject()->getNameInDocument();

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!vp->getObject()->isValid())
            throw Base::RuntimeError(vp->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromUtf8(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgShapeBinder::reject()
{
  // roll back the done things
  Gui::Command::abortCommand();
  Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
  Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
  return true;
}

#include "moc_TaskShapeBinder.cpp"
