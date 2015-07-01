/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
#endif

#include "ui_TaskRevolutionParameters.h"
#include "TaskRevolutionParameters.h"
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/Origin.h>
#include <App/Line.h>
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
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "ReferenceSelection.h"
#include "TaskSketchBasedParameters.h"
#include "Workbench.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskRevolutionParameters */

TaskRevolutionParameters::TaskRevolutionParameters(ViewProviderRevolution *RevolutionView,QWidget *parent)
    : TaskSketchBasedParameters(RevolutionView, parent, "PartDesign_Revolution",tr("Revolution parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskRevolutionParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->revolveAngle, SIGNAL(valueChanged(double)),
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

    // Temporarily prevent unnecessary feature recomputes
    ui->revolveAngle->blockSignals(true);
    ui->axis->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);

    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(vp->getObject());
    double l = pcRevolution->Angle.getValue();
    bool mirrored = pcRevolution->Midplane.getValue();
    bool reversed = pcRevolution->Reversed.getValue();

    ui->revolveAngle->setValue(l);
    blockUpdate = false;
    updateUI();

    ui->checkBoxMidplane->setChecked(mirrored);
    ui->checkBoxReversed->setChecked(reversed);
    ui->revolveAngle->bind(pcRevolution->Angle);

    ui->revolveAngle->blockSignals(false);
    ui->axis->blockSignals(false);
    ui->checkBoxMidplane->blockSignals(false);
    ui->checkBoxReversed->blockSignals(false);

    setFocus ();
    
    //show the parts coordinate system axis for selection
    App::Part* part = getPartFor(vp->getObject(), false);
    if(part) {        
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(true, Gui::Application::Instance->activeDocument());
            origin->setTemporaryVisibilityAxis(true);
        }            
     } 
}

void TaskRevolutionParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(vp->getObject());

    App::DocumentObject* pcReferenceAxis = pcRevolution->ReferenceAxis.getValue();
    std::vector<std::string> sub = pcRevolution->ReferenceAxis.getSubValues();

    // Add user-defined sketch axes to the reference selection combo box
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcRevolution->Sketch.getValue());
    int maxcount=5;
    if (pcSketch)
        maxcount += pcSketch->getAxisCount();

    for (int i=ui->axis->count()-1; i >= 5; i--)
        ui->axis->removeItem(i);
    for (int i=ui->axis->count(); i < maxcount; i++)
        ui->axis->addItem(QString::fromAscii("Sketch axis %1").arg(i-5));

    bool undefined = false;
    if (pcReferenceAxis != NULL) {
        bool is_base_line = pcReferenceAxis->isDerivedFrom(App::Line::getClassTypeId());
        
        if(is_base_line && strcmp(static_cast<App::Line*>(pcReferenceAxis)->LineType.getValue(), App::Part::BaselineTypes[0])==0)
            ui->axis->setCurrentIndex(0);
        else if(is_base_line && strcmp(static_cast<App::Line*>(pcReferenceAxis)->LineType.getValue(), App::Part::BaselineTypes[1])==0)
            ui->axis->setCurrentIndex(1);
        else if(is_base_line && strcmp(static_cast<App::Line*>(pcReferenceAxis)->LineType.getValue(), App::Part::BaselineTypes[2])==0)
            ui->axis->setCurrentIndex(2);
        else if(!sub.empty() && sub.front() == "H_Axis")
            ui->axis->setCurrentIndex(3);
        else if(!sub.empty() && sub.front() == "V_Axis")
            ui->axis->setCurrentIndex(4);
        else if(!sub.empty() && sub.front().size() > 4 && sub.front().substr(0,4) == "Axis") {
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

void TaskRevolutionParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(vp->getObject());

        exitSelectionMode();
        if (!blockUpdate) {
            std::vector<std::string> axis;
            App::DocumentObject* selObj;
            getReferencedSelection(pcRevolution, msg, selObj, axis);
            pcRevolution->ReferenceAxis.setValue(selObj, axis);


            recomputeFeature();
            updateUI();
        }
        else {
            Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcRevolution->Sketch.getValue());
            int maxcount=5;
            if (pcSketch)
                maxcount += pcSketch->getAxisCount();
            for (int i=ui->axis->count()-1; i >= maxcount; i--)
                ui->axis->removeItem(i);

            std::vector<std::string> sub;
            App::DocumentObject* selObj;
            getReferencedSelection(pcRevolution, msg, selObj, sub);
            ui->axis->addItem(getRefStr(selObj, sub));
            ui->axis->setCurrentIndex(maxcount);
            ui->axis->addItem(tr("Select reference..."));
        }
    }
}


void TaskRevolutionParameters::onAngleChanged(double len)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(vp->getObject());
    pcRevolution->Angle.setValue(len);
    exitSelectionMode();
    recomputeFeature();
}

void TaskRevolutionParameters::onAxisChanged(int num)
{
    if (blockUpdate)
        return;
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(vp->getObject());
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcRevolution->Sketch.getValue());
    if (pcSketch) {
        App::DocumentObject *oldRefAxis = pcRevolution->ReferenceAxis.getValue();
        std::vector<std::string> oldSubRefAxis = pcRevolution->ReferenceAxis.getSubValues();

        int maxcount = pcSketch->getAxisCount()+5;
        if (num == 0) {
            pcRevolution->ReferenceAxis.setValue(getPartLines(App::Part::BaselineTypes[0]), std::vector<std::string>(1,""));
        }
        else if (num == 1) {
            pcRevolution->ReferenceAxis.setValue(getPartLines(App::Part::BaselineTypes[1]), std::vector<std::string>(1,""));
        }
        else if (num == 2) {
            pcRevolution->ReferenceAxis.setValue(getPartLines(App::Part::BaselineTypes[2]), std::vector<std::string>(1,""));
        }
        else if (num == 3) {
            pcRevolution->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,"H_Axis"));
            exitSelectionMode();
        } else if (num == 4) {
            pcRevolution->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,"V_Axis"));
            exitSelectionMode();
        } else if (num >= 5 && num < maxcount) {
            QString buf = QString::fromUtf8("Axis%1").arg(num-2);
            std::string str = buf.toStdString();
            pcRevolution->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,str));
            exitSelectionMode();
        } else if (num == ui->axis->count() - 1) {
            // enter reference selection mode
            TaskSketchBasedParameters::onSelectReference(true, true, false, true);
        } else if (num == maxcount)
            exitSelectionMode();

        App::DocumentObject *newRefAxis = pcRevolution->ReferenceAxis.getValue();
        const std::vector<std::string> &newSubRefAxis = pcRevolution->ReferenceAxis.getSubValues();
        if (oldRefAxis != newRefAxis ||
            oldSubRefAxis.size() != newSubRefAxis.size() ||
            oldSubRefAxis[0] != newSubRefAxis[0]) {
            bool reversed = pcRevolution->suggestReversed();
            if (reversed != pcRevolution->Reversed.getValue()) {
                pcRevolution->Reversed.setValue(reversed);
                ui->checkBoxReversed->blockSignals(true);
                ui->checkBoxReversed->setChecked(reversed);
                ui->checkBoxReversed->blockSignals(false);
            }
        }
    }

    updateUI();
    recomputeFeature();
}

void TaskRevolutionParameters::onMidplane(bool on)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(vp->getObject());
    pcRevolution->Midplane.setValue(on);
    recomputeFeature();
}

void TaskRevolutionParameters::onReversed(bool on)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(vp->getObject());
    pcRevolution->Reversed.setValue(on);
    recomputeFeature();
}

double TaskRevolutionParameters::getAngle(void) const
{
    return ui->revolveAngle->value().getValue();
}

void TaskRevolutionParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    // get the support and Sketch
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(vp->getObject());
    obj = static_cast<Sketcher::SketchObject*>(pcRevolution->Sketch.getValue());
    sub = std::vector<std::string>(1,"");
    int maxcount=5;
    if (obj)
        maxcount += static_cast<Part::Part2DObject*>(obj)->getAxisCount();

    if (obj) {
        int num = ui->axis->currentIndex();
        if(num  == 0) 
            obj = getPartLines(App::Part::BaselineTypes[0]);
        else if(num == 1)
            obj = getPartLines(App::Part::BaselineTypes[1]);
        else if(num == 2)
            obj = getPartLines(App::Part::BaselineTypes[2]);
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

bool   TaskRevolutionParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

bool   TaskRevolutionParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

TaskRevolutionParameters::~TaskRevolutionParameters()
{
    //hide the parts coordinate system axis for selection
    App::Part* part = getPartFor(vp->getObject(), false);
    if(part) {
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(false);
        }            
    }
    
    delete ui;
}

void TaskRevolutionParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskRevolutionParameters::apply()
{
    App::DocumentObject* revolve = vp->getObject();
    std::string name = revolve->getNameInDocument();

    // retrieve sketch and its support object
    App::DocumentObject* sketch = 0;
    App::DocumentObject* support = 0;
    if (revolve->getTypeId().isDerivedFrom(PartDesign::Revolution::getClassTypeId())) {
        sketch = static_cast<PartDesign::Revolution*>(revolve)->Sketch.getValue();
        try{//throws if no base
            support = static_cast<PartDesign::Revolution*>(revolve)->getBaseObject();
        } catch (Base::Exception) {
            support = NULL;
        }
    }

    //Gui::Command::openCommand("Revolution changed");
    ui->revolveAngle->apply();
    std::vector<std::string> sub;
    App::DocumentObject* obj;
    getReferenceAxis(obj, sub);
    std::string axis = getPythonStr(obj, sub);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ReferenceAxis = %s",name.c_str(),axis.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Midplane = %i",name.c_str(), getMidplane() ? 1 : 0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i",name.c_str(), getReversed() ? 1 : 0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    if (revolve->isValid()) {
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

TaskDlgRevolutionParameters::TaskDlgRevolutionParameters(ViewProviderRevolution *RevolutionView)
    : TaskDlgSketchBasedParameters(RevolutionView)
{
    assert(RevolutionView);
    parameter  = new TaskRevolutionParameters(RevolutionView);

    Content.push_back(parameter);
}

TaskDlgRevolutionParameters::~TaskDlgRevolutionParameters()
{

}

//==== calls from the TaskView ===============================================================

bool TaskDlgRevolutionParameters::accept()
{
    parameter->apply();
    return true;
}


#include "moc_TaskRevolutionParameters.cpp"
