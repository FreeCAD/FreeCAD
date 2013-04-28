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

#include "ui_TaskFilletParameters.h"
#include "TaskFilletParameters.h"
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
#include <Mod/PartDesign/App/FeatureFillet.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskFilletParameters */

TaskFilletParameters::TaskFilletParameters(ViewProviderFillet *FilletView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Part_Fillet"),tr("Fillet parameters"),true, parent),FilletView(FilletView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskFilletParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->doubleSpinBox, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(FilletView->getObject());
    double r = pcFillet->Radius.getValue();

    ui->doubleSpinBox->setMaximum(INT_MAX);
    ui->doubleSpinBox->setValue(r);
    ui->doubleSpinBox->selectAll();
    QMetaObject::invokeMethod(ui->doubleSpinBox, "setFocus", Qt::QueuedConnection);
}

void TaskFilletParameters::onLengthChanged(double len)
{
    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(FilletView->getObject());
    pcFillet->Radius.setValue(len);
    pcFillet->getDocument()->recomputeFeature(pcFillet);
}

double TaskFilletParameters::getLength(void) const
{
    return ui->doubleSpinBox->value();
}


TaskFilletParameters::~TaskFilletParameters()
{
    delete ui;
}

void TaskFilletParameters::changeEvent(QEvent *e)
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

TaskDlgFilletParameters::TaskDlgFilletParameters(ViewProviderFillet *FilletView)
    : TaskDialog(),FilletView(FilletView)
{
    assert(FilletView);
    parameter  = new TaskFilletParameters(FilletView);

    Content.push_back(parameter);
}

TaskDlgFilletParameters::~TaskDlgFilletParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgFilletParameters::open()
{
    
}

void TaskDlgFilletParameters::clicked(int)
{
    
}

bool TaskDlgFilletParameters::accept()
{
    std::string name = FilletView->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Fillet changed");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Radius = %f",name.c_str(),parameter->getLength());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();

    return true;
}

bool TaskDlgFilletParameters::reject()
{
    // get the support and Sketch
    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(FilletView->getObject()); 
    App::DocumentObject    *pcSupport;
    pcSupport = pcFillet->Base.getValue();

    // role back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    
    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcFillet)) {
        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
            Gui::Application::Instance->getViewProvider(pcSupport)->show();
    }

    return true;
}



#include "moc_TaskFilletParameters.cpp"
