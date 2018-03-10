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
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "ui_TaskPipeParameters.h"
#include "ui_TaskPipeOrientation.h"
#include "ui_TaskPipeScaling.h"
#include <ui_DlgReference.h>
#include "TaskPipeParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeaturePipe.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"
#include "Utils.h"
#include "TaskFeaturePick.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPipeParameters */


//**************************************************************************
//**************************************************************************
// Task Parameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskPipeParameters::TaskPipeParameters(ViewProviderPipe *PipeView, bool /*newObj*/, QWidget *parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_Additive_Pipe",tr("Pipe parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPipeParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->comboBoxTransition, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTransitionChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->buttonProfileBase, SIGNAL(toggled(bool)),
            this, SLOT(onBaseButton(bool)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(PipeView->getObject());
    Gui::Document* doc = PipeView->getDocument();

    //make sure the user sees all important things: the
    //spine/auxiliary spine he already selected
    if (pipe->Spine.getValue()) {
        auto* svp = doc->getViewProvider(pipe->Spine.getValue());
        spineShow = svp->isShow();
        svp->setVisible(true);
    }
        
    //add initial values    
    if(pipe->Spine.getValue())
        ui->profileBaseEdit->setText(QString::fromUtf8(pipe->Spine.getValue()->getNameInDocument()));
    std::vector<std::string> strings = pipe->Spine.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));

    ui->comboBoxTransition->setCurrentIndex(pipe->Transition.getValue());

    updateUI();
}

void TaskPipeParameters::updateUI()
{
    
}

void TaskPipeParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    ui->listWidgetReferences->addItem(QString::fromStdString(msg.pSubName));
                
                ui->profileBaseEdit->setText(QString::fromStdString(msg.pObjectName));
            }
            else if (selectionMode == refRemove) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, QString::fromUtf8(msg.pSubName));
                else {
                    ui->profileBaseEdit->clear();
                }                
            } else if(selectionMode == refObjAdd) {
                ui->listWidgetReferences->clear();
                ui->profileBaseEdit->setText(QString::fromUtf8(msg.pObjectName));
            }
            clearButtons();
            static_cast<ViewProviderPipe*>(vp)->highlightReferences(false, false);
            recomputeFeature();
        } 
        clearButtons();
        exitSelectionMode();
    }
}

TaskPipeParameters::~TaskPipeParameters()
{
    if (vp) {
        PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
        Gui::Document* doc = vp->getDocument();

        //make sure the user sees all important things: the
        //spine/auxiliary spine he already selected
        if (pipe->Spine.getValue()) {
            auto* svp = doc->getViewProvider(pipe->Spine.getValue());
            svp->setVisible(spineShow);
            spineShow = false;
        }

        static_cast<ViewProviderPipe*>(vp)->highlightReferences(false, false);
    }

    delete ui;
}

void TaskPipeParameters::onTransitionChanged(int idx) {
    
    static_cast<PartDesign::Pipe*>(vp->getObject())->Transition.setValue(idx);
    recomputeFeature();
}

void TaskPipeParameters::onButtonRefAdd(bool checked) {
    
    if (checked) {
        //clearButtons(refAdd);
        //hideObject();
        Gui::Selection().clearSelection();
        selectionMode = refAdd;
        static_cast<ViewProviderPipe*>(vp)->highlightReferences(true, false);
    }
}

void TaskPipeParameters::onButtonRefRemove(bool checked) {

    if (checked) {
        //clearButtons(refRemove);
        //hideObject();
        Gui::Selection().clearSelection();        
        selectionMode = refRemove;
        static_cast<ViewProviderPipe*>(vp)->highlightReferences(true, false);
    }
}

void TaskPipeParameters::onBaseButton(bool checked) {

    if (checked) {
        //clearButtons(refRemove);
        //hideObject();
        Gui::Selection().clearSelection();        
        selectionMode = refObjAdd;
        //DressUpView->highlightReferences(true);
    }
}


void TaskPipeParameters::onTangentChanged(bool checked) {

    static_cast<PartDesign::Pipe*>(vp->getObject())->SpineTangent.setValue(checked);
    recomputeFeature();
}


void TaskPipeParameters::removeFromListWidget(QListWidget* widget, QString itemstr) {

    QList<QListWidgetItem*> items = widget->findItems(itemstr, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

bool TaskPipeParameters::referenceSelected(const SelectionChanges& msg) const {
    
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
        std::vector<std::string> refs = static_cast<PartDesign::Pipe*>(vp->getObject())->Spine.getSubValues();
        std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

        if(selectionMode != refObjAdd) {
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
            refs.clear();
        }
        static_cast<PartDesign::Pipe*>(vp->getObject())->Spine.setValue(vp->getObject()->getDocument()->getObject(msg.pObjectName),
                                                                        refs);
        return true;
    }

    return false;
}

void TaskPipeParameters::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
    ui->buttonProfileBase->setChecked(false);
}

void TaskPipeParameters::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}


//**************************************************************************
//**************************************************************************
// Task Orientation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskPipeOrientation::TaskPipeOrientation(ViewProviderPipe* PipeView, bool /*newObj*/, QWidget* parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_Additive_Pipe", tr("Section orientation")) {

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPipeOrientation();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->comboBoxMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onOrientationChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->buttonProfileBase, SIGNAL(toggled(bool)),
            this, SLOT(onBaseButton(bool)));
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateUI(int)));
    connect(ui->curvelinear, SIGNAL(toggled(bool)),
            this, SLOT(onCurvelinearChanged(bool)));
    connect(ui->doubleSpinBoxX, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
    connect(ui->doubleSpinBoxY, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
    connect(ui->doubleSpinBoxZ, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
    
    this->groupLayout()->addWidget(proxy);
    
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(PipeView->getObject());
    Gui::Document* doc = Gui::Application::Instance->activeDocument(); 
    
    //make sure the user sees an important things: the base feature to select edges and the 
    //spine/auxiliary spine he already selected
    if(pipe->AuxillerySpine.getValue()) {
        auto* svp = doc->getViewProvider(pipe->AuxillerySpine.getValue());
        auxSpineShow = svp->isShow();
        svp->show();
    }

    //add initial values
    if(pipe->AuxillerySpine.getValue())
        ui->profileBaseEdit->setText(QString::fromUtf8(pipe->AuxillerySpine.getValue()->getNameInDocument()));
    std::vector<std::string> strings = pipe->AuxillerySpine.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
        
    ui->comboBoxMode->setCurrentIndex(pipe->Mode.getValue());
    ui->curvelinear->setChecked(pipe->AuxilleryCurvelinear.getValue());    

    updateUI(pipe->Mode.getValue());
}

TaskPipeOrientation::~TaskPipeOrientation()
{
    if (vp) {
        PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
        Gui::Document* doc = vp->getDocument();

        //make sure the user sees al important things: the base feature to select edges and the
        //spine/auxiliary spine he already selected
        if (pipe->AuxillerySpine.getValue()) {
            auto* svp = doc->getViewProvider(pipe->AuxillerySpine.getValue());
            svp->setVisible(auxSpineShow);
            auxSpineShow = false;
        }

        static_cast<ViewProviderPipe*>(vp)->highlightReferences(false, true);
    }
}

void TaskPipeOrientation::onOrientationChanged(int idx) {

    static_cast<PartDesign::Pipe*>(vp->getObject())->Mode.setValue(idx);
    recomputeFeature();
}

void TaskPipeOrientation::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
    ui->buttonProfileBase->setChecked(false);
}

void TaskPipeOrientation::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}

void TaskPipeOrientation::onButtonRefAdd(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();        
        selectionMode = refAdd;
        static_cast<ViewProviderPipe*>(vp)->highlightReferences(true, true);
    }
}

void TaskPipeOrientation::onButtonRefRemove(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();        
        selectionMode = refRemove;
        static_cast<ViewProviderPipe*>(vp)->highlightReferences(true, true);
    }
}

void TaskPipeOrientation::onBaseButton(bool checked)
{
    if (checked) {
        Gui::Selection().clearSelection();        
        selectionMode = refObjAdd;
    }
}

void TaskPipeOrientation::onTangentChanged(bool checked)
{
    Q_UNUSED(checked);
}

void TaskPipeOrientation::onCurvelinearChanged(bool checked)
{
    static_cast<PartDesign::Pipe*>(vp->getObject())->AuxilleryCurvelinear.setValue(checked);
    recomputeFeature();
}

void TaskPipeOrientation::onBinormalChanged(double)
{
    Base::Vector3d vec(ui->doubleSpinBoxX->value(),
                       ui->doubleSpinBoxY->value(),
                       ui->doubleSpinBoxZ->value());
    
    static_cast<PartDesign::Pipe*>(vp->getObject())->Binormal.setValue(vec);
    recomputeFeature();
}



void TaskPipeOrientation::onSelectionChanged(const SelectionChanges& msg) {

    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    ui->listWidgetReferences->addItem(QString::fromStdString(msg.pSubName));
                
                ui->profileBaseEdit->setText(QString::fromStdString(msg.pObjectName));
            }
            else if (selectionMode == refRemove) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, QString::fromUtf8(msg.pSubName));
                else {
                    ui->profileBaseEdit->clear();
                }                
            } else if(selectionMode == refObjAdd) {
                ui->listWidgetReferences->clear();
                ui->profileBaseEdit->setText(QString::fromUtf8(msg.pObjectName));
            }
            clearButtons();
            static_cast<ViewProviderPipe*>(vp)->highlightReferences(false, true);
            recomputeFeature();
        } 
        clearButtons();
        exitSelectionMode();
    }
}

bool TaskPipeOrientation::referenceSelected(const SelectionChanges& msg) const {

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
        std::vector<std::string> refs = static_cast<PartDesign::Pipe*>(vp->getObject())->AuxillerySpine.getSubValues();
        std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

        if(selectionMode != refObjAdd) {
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
            refs.clear();
        }
        static_cast<PartDesign::Pipe*>(vp->getObject())->AuxillerySpine.setValue(vp->getObject()->getDocument()->getObject(msg.pObjectName),
                                                                        refs);
        return true;
    }

    return false;
}

void TaskPipeOrientation::removeFromListWidget(QListWidget* widget, QString name) {

    QList<QListWidgetItem*> items = widget->findItems(name, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

void TaskPipeOrientation::updateUI(int idx) {

    //make sure we resize to the size of the current page
    for(int i=0; i<ui->stackedWidget->count(); ++i)
        ui->stackedWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    if (idx < ui->stackedWidget->count())
        ui->stackedWidget->widget(idx)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


//**************************************************************************
//**************************************************************************
// Task Scaling
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TaskPipeScaling::TaskPipeScaling(ViewProviderPipe* PipeView, bool /*newObj*/, QWidget* parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_Additive_Pipe", tr("Section transformation")) {

            // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPipeScaling();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->comboBoxScaling, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onScalingChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateUI(int)));
    
    this->groupLayout()->addWidget(proxy);
   
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(PipeView->getObject());
    ui->comboBoxScaling->setCurrentIndex(pipe->Transformation.getValue());
    
    updateUI(pipe->Mode.getValue());
}

TaskPipeScaling::~TaskPipeScaling() {

}

void TaskPipeScaling::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
}

void TaskPipeScaling::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}

void TaskPipeScaling::onButtonRefAdd(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();        
        selectionMode = refAdd;
        //static_cast<ViewProviderPipe*>(vp)->highlightReferences(true, true);
    }
}

void TaskPipeScaling::onButtonRefRemove(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();        
        selectionMode = refRemove;
        //static_cast<ViewProviderPipe*>(vp)->highlightReferences(true, true);
    }
}

void TaskPipeScaling::onScalingChanged(int idx) {

    updateUI(idx);
    static_cast<PartDesign::Pipe*>(vp->getObject())->Transformation.setValue(idx);
}

void TaskPipeScaling::onSelectionChanged(const SelectionChanges& msg) {

    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString objn = QString::fromStdString(msg.pObjectName);
                if(!objn.isEmpty())
                    ui->listWidgetReferences->addItem(objn);
            }
            else if (selectionMode == refRemove) {
                QString objn = QString::fromStdString(msg.pObjectName);
                if(!objn.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, objn);               
            }
            clearButtons();
            //static_cast<ViewProviderPipe*>(vp)->highlightReferences(false, true);
            recomputeFeature();
        } 
        clearButtons();
        exitSelectionMode();
    }
}

bool TaskPipeScaling::referenceSelected(const SelectionChanges& msg) const {

    
    if ((msg.Type == Gui::SelectionChanges::AddSelection) && (
                (selectionMode == refAdd) || (selectionMode == refRemove))) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();        
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;
       
        //every selection needs to be a profile in itself, hence currently only full objects are 
        //supported, not individual edges of a part
        
        //change the references 
        std::vector<App::DocumentObject*> refs = static_cast<PartDesign::Pipe*>(vp->getObject())->Sections.getValues();
        App::DocumentObject* obj = vp->getObject()->getDocument()->getObject(msg.pObjectName);
        std::vector<App::DocumentObject*>::iterator f = std::find(refs.begin(), refs.end(), obj);

        if (selectionMode == refAdd) {
            if (f == refs.end())
                refs.push_back(obj);
            else
                return false; // duplicate selection
        } else {
            if (f != refs.end())
                refs.erase(f);
            else
                return false;
        }        

        static_cast<PartDesign::Pipe*>(vp->getObject())->Sections.setValues(refs);
        return true;
    }

    return false;
}

void TaskPipeScaling::removeFromListWidget(QListWidget* widget, QString name) {

    QList<QListWidgetItem*> items = widget->findItems(name, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

void TaskPipeScaling::updateUI(int idx) {

    //make sure we resize to the size of the current page
    for(int i=0; i<ui->stackedWidget->count(); ++i)
        ui->stackedWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    if (idx < ui->stackedWidget->count())
        ui->stackedWidget->widget(idx)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPipeParameters::TaskDlgPipeParameters(ViewProviderPipe *PipeView,bool newObj)
   : TaskDlgSketchBasedParameters(PipeView)
{
    assert(PipeView);
    parameter    = new TaskPipeParameters(PipeView,newObj);
    orientation  = new TaskPipeOrientation(PipeView,newObj);
    scaling      = new TaskPipeScaling(PipeView,newObj);

    Content.push_back(parameter);
    Content.push_back(orientation);
    Content.push_back(scaling);
}

TaskDlgPipeParameters::~TaskDlgPipeParameters()
{

}

//==== calls from the TaskView ===============================================================


bool TaskDlgPipeParameters::accept()
{
    std::string name = vp->getObject()->getNameInDocument();

    
    //see what to do with external references
    //check the prerequisites for the selected objects
    //the user has to decide which option we should take if external references are used
    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(getPipeView()->getObject());
    auto pcActiveBody = PartDesignGui::getBodyFor(pcPipe, false);
  //auto pcActivePart = PartDesignGui::getPartFor(pcActiveBody, false);
    std::vector<App::DocumentObject*> copies;

    bool ext = false;
    if(!pcActiveBody->hasObject(pcPipe->Spine.getValue()) && !pcActiveBody->getOrigin()->hasObject(pcPipe->Spine.getValue()))
        ext = true;
    else if(pcPipe->AuxillerySpine.getValue() && !pcActiveBody->hasObject(pcPipe->AuxillerySpine.getValue()) && 
            !pcActiveBody->getOrigin()->hasObject(pcPipe->AuxillerySpine.getValue()))
            ext = true;
    else {
        for(App::DocumentObject* obj : pcPipe->Sections.getValues()) {
            if(!pcActiveBody->hasObject(obj) && !pcActiveBody->getOrigin()->hasObject(obj))
                ext = true;
        }
    }
    
    if(ext) {
        QDialog* dia = new QDialog;
        Ui_Dialog dlg;
        dlg.setupUi(dia);
        dia->setModal(true);
        int result = dia->exec();
        if(result == QDialog::DialogCode::Rejected)
            return false;
        else if(!dlg.radioXRef->isChecked()) {

            if(!pcActiveBody->hasObject(pcPipe->Spine.getValue()) && !pcActiveBody->getOrigin()->hasObject(pcPipe->Spine.getValue())) {
                pcPipe->Spine.setValue(PartDesignGui::TaskFeaturePick::makeCopy(pcPipe->Spine.getValue(), "", dlg.radioIndependent->isChecked()),
                                        pcPipe->Spine.getSubValues());
                copies.push_back(pcPipe->Spine.getValue());
            }
            else if(!pcActiveBody->hasObject(pcPipe->AuxillerySpine.getValue()) && !pcActiveBody->getOrigin()->hasObject(pcPipe->AuxillerySpine.getValue())){
                pcPipe->AuxillerySpine.setValue(PartDesignGui::TaskFeaturePick::makeCopy(pcPipe->AuxillerySpine.getValue(), "", dlg.radioIndependent->isChecked()),
                                        pcPipe->AuxillerySpine.getSubValues());
                copies.push_back(pcPipe->AuxillerySpine.getValue());
            }
    
            std::vector<App::DocumentObject*> objs;
            int index = 0;
            for(App::DocumentObject* obj : pcPipe->Sections.getValues()) {

                if(!pcActiveBody->hasObject(obj) && !pcActiveBody->getOrigin()->hasObject(obj)) {
                    objs.push_back(PartDesignGui::TaskFeaturePick::makeCopy(obj, "", dlg.radioIndependent->isChecked()));
                    copies.push_back(objs.back());
                }
                else
                    objs.push_back(obj);

                index++;
            }

            pcPipe->Sections.setValues(objs);
        }
    }
    
    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!vp->getObject()->isValid())
            throw Base::Exception(vp->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
        
        //we need to add the copied features to the body after the command action, as otherwise FreeCAD crashes unexplainably
        for(auto obj : copies) {
            //Dead code: pcActiveBody was previously used without checking for null, so it won't be null here either.
            //if(pcActiveBody)
            pcActiveBody->addObject(obj);
            //else if (pcActivePart)
            //    pcActivePart->addObject(obj);
        }
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromUtf8(e.what()));
        return false;
    }

    return true;
}

//bool TaskDlgPipeParameters::reject()
//{
//    // get the support and Sketch
//    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(PipeView->getObject()); 
//    Sketcher::SketchObject *pcSketch = 0;
//    App::DocumentObject    *pcSupport = 0;
//    if (pcPipe->Sketch.getValue()) {
//        pcSketch = static_cast<Sketcher::SketchObject*>(pcPipe->Sketch.getValue()); 
//        pcSupport = pcSketch->Support.getValue();
//    }
//
//    // roll back the done things
//    Gui::Command::abortCommand();
//    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
//    
//    // if abort command deleted the object the support is visible again
//    if (!Gui::Application::Instance->getViewProvider(pcPipe)) {
//        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
//            Gui::Application::Instance->getViewProvider(pcSketch)->show();
//        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
//            Gui::Application::Instance->getViewProvider(pcSupport)->show();
//    }
//
//    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
//    //Gui::Command::commitCommand();
//
//    return true;
//}



#include "moc_TaskPipeParameters.cpp"
