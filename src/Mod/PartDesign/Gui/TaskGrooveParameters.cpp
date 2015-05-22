/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "ui_TaskGrooveParameters.h"
#include "TaskGrooveParameters.h"
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/Origin.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "Workbench.h"
#include "ReferenceSelection.h"
#include "TaskSketchBasedParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskGrooveParameters */

TaskGrooveParameters::TaskGrooveParameters(ViewProviderGroove *GrooveView,QWidget *parent)
    : TaskSketchBasedParameters(GrooveView, parent, "PartDesign_Groove",tr("Groove parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskGrooveParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->grooveAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->axis, SIGNAL(activated(int)),
            this, SLOT(onAxisChanged(int)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplane(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversed(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature updates
    ui->grooveAngle->blockSignals(true);
    ui->axis->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);

    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());
    double l = pcGroove->Angle.getValue();
    bool mirrored = pcGroove->Midplane.getValue();
    bool reversed = pcGroove->Reversed.getValue();

    ui->grooveAngle->setValue(l);
    ui->grooveAngle->bind(pcGroove->Angle);

    blockUpdate = false;
    updateUI();

    ui->checkBoxMidplane->setChecked(mirrored);
    ui->checkBoxReversed->setChecked(reversed);

    ui->grooveAngle->blockSignals(false);
    ui->axis->blockSignals(false);
    ui->checkBoxMidplane->blockSignals(false);
    ui->checkBoxReversed->blockSignals(false);

    setFocus ();
    
    //show the parts coordinate system axis for selection
    for(App::Part* part : App::GetApplication().getActiveDocument()->getObjectsOfType<App::Part>()) {
    
        if(part->hasObject(vp->getObject(), true)) {
            auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
            if(!app_origin.empty()) {
                ViewProviderOrigin* origin;
                origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
                origin->setTemporaryVisibilityMode(true, Gui::Application::Instance->activeDocument());
                origin->setTemporaryVisibilityAxis(true);
            }            
            break;
        }
    } 
}

void TaskGrooveParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());

    App::DocumentObject* pcReferenceAxis = pcGroove->ReferenceAxis.getValue();
    std::vector<std::string> sub = pcGroove->ReferenceAxis.getSubValues();

    // Add user-defined sketch axes to the reference selection combo box
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcGroove->Sketch.getValue());
    int maxcount=5;
    if (pcSketch)
        maxcount += pcSketch->getAxisCount();

    for (int i=ui->axis->count()-1; i >= 5; i--)
        ui->axis->removeItem(i);
    for (int i=ui->axis->count(); i < maxcount; i++)
        ui->axis->addItem(QString::fromAscii("Sketch axis %1").arg(i-5));

    bool undefined = false; 
    if (pcReferenceAxis != NULL) {
        
        if(strcmp(pcReferenceAxis->getNameInDocument(), App::Part::BaselineTypes[0])==0)
            ui->axis->setCurrentIndex(0);
        else if(strcmp(pcReferenceAxis->getNameInDocument(), App::Part::BaselineTypes[1])==0)
            ui->axis->setCurrentIndex(1);
        else if(strcmp(pcReferenceAxis->getNameInDocument(), App::Part::BaselineTypes[2])==0)
            ui->axis->setCurrentIndex(2);
        else if (!sub.empty() && sub.front() == "H_Axis")
            ui->axis->setCurrentIndex(3);
        else if (!sub.empty() && sub.front() == "V_Axis")
            ui->axis->setCurrentIndex(4);
        else if (!sub.empty() && sub.front().size() > 4 && sub.front().substr(0,4) == "Axis") {
            int pos = 5 + std::atoi(sub.front().substr(4,4000).c_str());
            if (pos <= maxcount)
                ui->axis->setCurrentIndex(pos);
            else
                undefined = true;
        } else {
            ui->axis->addItem(getRefStr(pcReferenceAxis, sub));
            ui->axis->setCurrentIndex(maxcount);
        }
    } else {
        undefined = true;
    }

    ui->axis->addItem(tr("Select reference..."));

    blockUpdate = false;
}

void TaskGrooveParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());

        exitSelectionMode();
        if (!blockUpdate) {
            std::vector<std::string> axis;
            App::DocumentObject* selObj;
            getReferencedSelection(pcGroove, msg, selObj, axis);
            pcGroove->ReferenceAxis.setValue(selObj, axis);


            recomputeFeature();
            updateUI();
        }
        else {
            Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcGroove->Sketch.getValue());
            int maxcount=2;
            if (pcSketch)
                maxcount += pcSketch->getAxisCount();
            for (int i=ui->axis->count()-1; i >= maxcount; i--)
                ui->axis->removeItem(i);

            std::vector<std::string> sub;
            App::DocumentObject* selObj;
            getReferencedSelection(pcGroove, msg, selObj, sub);
            ui->axis->addItem(getRefStr(selObj, sub));
            ui->axis->setCurrentIndex(maxcount);
            ui->axis->addItem(tr("Select reference..."));
        }
    }
}

void TaskGrooveParameters::onAngleChanged(double len)
{
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());
    pcGroove->Angle.setValue(len);
    exitSelectionMode();
    recomputeFeature();
}

void TaskGrooveParameters::onAxisChanged(int num)
{
    if (blockUpdate)
        return;
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcGroove->Sketch.getValue());
    if (pcSketch) {
        App::DocumentObject *oldRefAxis = pcGroove->ReferenceAxis.getValue();
        std::vector<std::string> oldSubRefAxis = pcGroove->ReferenceAxis.getSubValues();

        int maxcount = pcSketch->getAxisCount()+2;
        if (num == 0) {
            pcGroove->ReferenceAxis.setValue(pcGroove->getDocument()->getObject(App::Part::BaselineTypes[0]),
                                                 std::vector<std::string>(1,""));
        }
        else if (num == 1) {
            pcGroove->ReferenceAxis.setValue(pcGroove->getDocument()->getObject(App::Part::BaselineTypes[1]),
                                                 std::vector<std::string>(1,""));
        }
        else if (num == 2) {
            pcGroove->ReferenceAxis.setValue(pcGroove->getDocument()->getObject(App::Part::BaselineTypes[2]),
                                                 std::vector<std::string>(1,""));
        }
        else if (num == 3) {
            pcGroove->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,"H_Axis"));
            exitSelectionMode();
        } else if (num == 4) {
            pcGroove->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,"V_Axis"));
            exitSelectionMode();
        } else if (num >= 5 && num < maxcount) {
            QString buf = QString::fromUtf8("Axis%1").arg(num-2);
            std::string str = buf.toStdString();
            pcGroove->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,str));
            exitSelectionMode();
        }  else if (num == ui->axis->count() - 1) {
            // enter reference selection mode
            TaskSketchBasedParameters::onSelectReference(true, true, false, true);
        } else if (num == maxcount)
            exitSelectionMode();

        App::DocumentObject *newRefAxis = pcGroove->ReferenceAxis.getValue();
        const std::vector<std::string> &newSubRefAxis = pcGroove->ReferenceAxis.getSubValues();
        if (oldRefAxis != newRefAxis ||
            oldSubRefAxis.size() != newSubRefAxis.size() ||
            oldSubRefAxis[0] != newSubRefAxis[0]) {
            bool reversed = pcGroove->suggestReversed();
            if (reversed != pcGroove->Reversed.getValue()) {
                pcGroove->Reversed.setValue(reversed);
                ui->checkBoxReversed->blockSignals(true);
                ui->checkBoxReversed->setChecked(reversed);
                ui->checkBoxReversed->blockSignals(false);
            }
        }
    }

    updateUI();
    recomputeFeature();
}

void TaskGrooveParameters::onMidplane(bool on)
{
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());
    pcGroove->Midplane.setValue(on);
    recomputeFeature();
}

void TaskGrooveParameters::onReversed(bool on)
{
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());
    pcGroove->Reversed.setValue(on);
    recomputeFeature();
}

double TaskGrooveParameters::getAngle(void) const
{
    return ui->grooveAngle->value().getValue();
}

void TaskGrooveParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    // get the support and Sketch
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());
    obj = static_cast<Sketcher::SketchObject*>(pcGroove->Sketch.getValue());
    sub = std::vector<std::string>(1,"");
    int maxcount=5;
    if (obj)
        maxcount += static_cast<Part::Part2DObject*>(obj)->getAxisCount();

    if (obj) {
        int num = ui->axis->currentIndex();
        if(num  == 0) 
            obj = pcGroove->getDocument()->getObject(App::Part::BaselineTypes[0]);
        else if(num == 1)
            obj = pcGroove->getDocument()->getObject(App::Part::BaselineTypes[1]);
        else if(num == 2)
            obj = pcGroove->getDocument()->getObject(App::Part::BaselineTypes[2]);
        else if (num == 3)
            sub[0] = "H_Axis";
        else if (num == 4)
            sub[0] = "V_Axis";
        else if (num >= 5  && num < maxcount) {
            QString buf = QString::fromUtf8("Axis%1").arg(num-2);
            sub[0] = buf.toStdString();
        } else if (num == maxcount && ui->axis->count() == maxcount + 2) {
            QStringList parts = ui->axis->currentText().split(QChar::fromAscii(':'));
            obj = vp->getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
            if (parts.size() > 1)
                sub[0] = parts[1].toStdString();
        } else {
            obj = NULL;
        }
    }
    else
        obj = NULL;
}

bool   TaskGrooveParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

bool   TaskGrooveParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

TaskGrooveParameters::~TaskGrooveParameters()
{
    //hide the parts coordinate system axis for selection
    for(App::Part* part : App::GetApplication().getActiveDocument()->getObjectsOfType<App::Part>()) {
    
        if(part->hasObject(vp->getObject(), true)) {
            auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
            if(!app_origin.empty()) {
                ViewProviderOrigin* origin;
                origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
                origin->setTemporaryVisibilityMode(false);
            }            
            break;
        }
    } 
    
    delete ui;
}

void TaskGrooveParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskGrooveParameters::apply()
{
    App::DocumentObject* groove = vp->getObject();
    std::string name = groove->getNameInDocument();

    // retrieve sketch and its support object
    App::DocumentObject* sketch = 0;
    App::DocumentObject* support = 0;
    if (groove->getTypeId().isDerivedFrom(PartDesign::Groove::getClassTypeId())) {
        sketch = static_cast<PartDesign::Groove*>(groove)->Sketch.getValue<Sketcher::SketchObject*>();
        if (sketch) {
            support = static_cast<Sketcher::SketchObject*>(sketch)->Support.getValue();
        }
    }

    //Gui::Command::openCommand("Groove changed");
    ui->grooveAngle->apply();
    std::vector<std::string> sub;
    App::DocumentObject* obj;
    getReferenceAxis(obj, sub);
    std::string axis = getPythonStr(obj, sub);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ReferenceAxis = %s",name.c_str(),axis.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Midplane = %i",name.c_str(), getMidplane() ? 1 : 0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i",name.c_str(), getReversed() ? 1 : 0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    if (groove->isValid()) {
        if (sketch)
            Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().hide(\"%s\")",sketch->getNameInDocument());
        if (support)
            Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().hide(\"%s\")",support->getNameInDocument());
    }
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();
 }

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgGrooveParameters::TaskDlgGrooveParameters(ViewProviderGroove *GrooveView)
    : TaskDlgSketchBasedParameters(GrooveView)
{
    assert(vp);
    parameter  = new TaskGrooveParameters(static_cast<ViewProviderGroove*>(vp));

    Content.push_back(parameter);
}

TaskDlgGrooveParameters::~TaskDlgGrooveParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgGrooveParameters::open()
{
    // a transaction is already open at creation time of the groove
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Edit groove");
        Gui::Command::openCommand((const char*)msg.toUtf8());
    }
}

void TaskDlgGrooveParameters::clicked(int)
{

}

bool TaskDlgGrooveParameters::accept()
{
    parameter->apply();
    return true;
}

bool TaskDlgGrooveParameters::reject()
{
    // get the support and Sketch
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(vp->getObject());
    Sketcher::SketchObject *pcSketch = 0;
    if (pcGroove->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcGroove->Sketch.getValue());
    }    

    // role back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcGroove)) {
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
    }

//    // Body housekeeping
//    if (ActivePartObject != NULL) {
//        // Make the new Tip and the previous solid feature visible again
//        App::DocumentObject* tip = ActivePartObject->Tip.getValue();
//        App::DocumentObject* prev = ActivePartObject->getPrevSolidFeature();
//        if (tip != NULL) {
//            Gui::Application::Instance->getViewProvider(tip)->show();
//            if ((tip != prev) && (prev != NULL))
//                Gui::Application::Instance->getViewProvider(prev)->show();
//        }
//    }

    return true;
}


#include "moc_TaskGrooveParameters.cpp"
