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
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskRevolutionParameters */

TaskRevolutionParameters::TaskRevolutionParameters(ViewProviderRevolution *RevolutionView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Revolution"),tr("Revolution parameters"),true, parent),RevolutionView(RevolutionView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskRevolutionParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->doubleSpinBox, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->axis, SIGNAL(activated(int)),
            this, SLOT(onAxisChanged(int)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    double l = pcRevolution->Angle.getValue();
    Base::Vector3f Ax = pcRevolution->Axis.getValue();

    ui->doubleSpinBox->setValue(l);
    if(Ax.y > 0)
        ui->axis->setCurrentIndex(0);        
    else
        ui->axis->setCurrentIndex(1);
  
    setFocus ();
}

void TaskRevolutionParameters::onAngleChanged(double len)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    pcRevolution->Angle.setValue((float)len);
    pcRevolution->getDocument()->recomputeFeature(pcRevolution);
}

void TaskRevolutionParameters::onAxisChanged(int num)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    if(num == 0)
        pcRevolution->Axis.setValue(Base::Vector3f(0,1,0));
    else
        pcRevolution->Axis.setValue(Base::Vector3f(1,0,0));

    pcRevolution->getDocument()->recomputeFeature(pcRevolution);
}


double TaskRevolutionParameters::getAngle(void) const
{
    return ui->doubleSpinBox->value();
}

Base::Vector3f  TaskRevolutionParameters::getAxis(void) const
{
    if( ui->axis->currentIndex() == 0)
        return Base::Vector3f(0,1,0);
    else
        return Base::Vector3f(1,0,0);

}


TaskRevolutionParameters::~TaskRevolutionParameters()
{
    delete ui;
}

void TaskRevolutionParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgRevolutionParameters::TaskDlgRevolutionParameters(ViewProviderRevolution *RevolutionView)
    : TaskDialog(),RevolutionView(RevolutionView)
{
    assert(RevolutionView);
    parameter  = new TaskRevolutionParameters(RevolutionView);

    Content.push_back(parameter);
}

TaskDlgRevolutionParameters::~TaskDlgRevolutionParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgRevolutionParameters::open()
{
    
}

void TaskDlgRevolutionParameters::clicked(int)
{
    
}

bool TaskDlgRevolutionParameters::accept()
{
    std::string name = RevolutionView->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Revolution changed");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Angle = %f",name.c_str(),parameter->getAngle());
    Base::Vector3f axis = parameter->getAxis();
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Axis = FreeCAD.Vector(%f,%f,%f)",name.c_str(),axis.x,axis.y,axis.z);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();

    return true;
}

bool TaskDlgRevolutionParameters::reject()
{
    // get the support and Sketch
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject()); 
    Sketcher::SketchObject *pcSketch;
    App::DocumentObject    *pcSupport;
    if (pcRevolution->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcRevolution->Sketch.getValue()); 
        pcSupport = pcSketch->Support.getValue();
    }

    // role back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    
    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcRevolution)) {
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
            Gui::Application::Instance->getViewProvider(pcSupport)->show();
    }

    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    //Gui::Command::commitCommand();

    return true;
}



#include "moc_TaskRevolutionParameters.cpp"
