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

#include "ui_TaskPocketParameters.h"
#include "TaskPocketParameters.h"
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
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPocketParameters */

TaskPocketParameters::TaskPocketParameters(ViewProviderPocket *PocketView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Pocket"),tr("Pocket parameters"),true, parent),PocketView(PocketView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPocketParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->doubleSpinBox, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    double l = pcPocket->Length.getValue();

    ui->doubleSpinBox->setMaximum(INT_MAX);
    ui->doubleSpinBox->setValue(l);
    ui->doubleSpinBox->selectAll();
    QMetaObject::invokeMethod(ui->doubleSpinBox, "setFocus", Qt::QueuedConnection);
 
    //// check if the sketch has support
    //Sketcher::SketchObject *pcSketch;
    //if (pcPocket->Sketch.getValue()) {
    //    pcSketch = static_cast<Sketcher::SketchObject*>(pcPocket->Sketch.getValue());
    //    if (pcSketch->Support.getValue())
    //        // in case of sketch with support, reverse makes no sense (goes into the part)
    //        ui->checkBoxReversed->setEnabled(0);
    //    else
    //        ui->checkBoxReversed->setChecked(reversed);
    //}
}

void TaskPocketParameters::onLengthChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    pcPocket->Length.setValue((float)len);
    pcPocket->getDocument()->recomputeFeature(pcPocket);
}

double TaskPocketParameters::getLength(void) const
{
    return ui->doubleSpinBox->value();
}


TaskPocketParameters::~TaskPocketParameters()
{
    delete ui;
}

void TaskPocketParameters::changeEvent(QEvent *e)
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

TaskDlgPocketParameters::TaskDlgPocketParameters(ViewProviderPocket *PocketView)
    : TaskDialog(),PocketView(PocketView)
{
    assert(PocketView);
    parameter  = new TaskPocketParameters(PocketView);

    Content.push_back(parameter);
}

TaskDlgPocketParameters::~TaskDlgPocketParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgPocketParameters::open()
{
    
}

void TaskDlgPocketParameters::clicked(int)
{
    
}

bool TaskDlgPocketParameters::accept()
{
    std::string name = PocketView->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Pocket changed");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f",name.c_str(),parameter->getLength());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();

    return true;
}

bool TaskDlgPocketParameters::reject()
{
    // get the support and Sketch
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject()); 
    Sketcher::SketchObject *pcSketch;
    App::DocumentObject    *pcSupport;
    if (pcPocket->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcPocket->Sketch.getValue()); 
        pcSupport = pcSketch->Support.getValue();
    }

    // role back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    
    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcPocket)) {
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
            Gui::Application::Instance->getViewProvider(pcSupport)->show();
    }

    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    //Gui::Command::commitCommand();

    return true;
}



#include "moc_TaskPocketParameters.cpp"
