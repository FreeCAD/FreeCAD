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
    connect(ui->changeMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->lineFaceName, SIGNAL(textEdited(QString)),
            this, SLOT(onFaceName(QString)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    double l = pcPocket->Length.getValue();
    int index = pcPocket->Type.getValue(); // must extract value here, clear() kills it!
    const char* upToFace = pcPocket->FaceName.getValue();

    ui->changeMode->clear();
    ui->changeMode->insertItem(0, tr("Dimension"));
    ui->changeMode->insertItem(1, tr("To last"));
    ui->changeMode->insertItem(2, tr("To first"));
    ui->changeMode->insertItem(3, tr("Through all"));
    ui->changeMode->insertItem(4, tr("Up to face"));
    ui->changeMode->setCurrentIndex(index);

    if (index == 0) { // Only this option requires a numeric value
        ui->doubleSpinBox->setMaximum(INT_MAX);
        ui->doubleSpinBox->setValue(l);
        ui->doubleSpinBox->selectAll();
        QMetaObject::invokeMethod(ui->doubleSpinBox, "setFocus", Qt::QueuedConnection);
        ui->lineFaceName->setEnabled(false);
    } else if (index == 4) { // Only this option requires to select a face
        ui->doubleSpinBox->setEnabled(false);
        ui->lineFaceName->setText(pcPocket->FaceName.isEmpty() ? tr("No face selected") : tr(upToFace));
    } else { // Neither value nor face required
        ui->doubleSpinBox->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
    }
 
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

void TaskPocketParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    if (pcPocket->Type.getValue() != 4) // ignore user selections if mode is not upToFace
        return;

    if (!msg.pSubName || msg.pSubName[0] == '\0')
        return;
    std::string element(msg.pSubName);
    if (element.substr(0,4) != "Face")
      return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        pcPocket->FaceName.setValue(element);
        pcPocket->getDocument()->recomputeFeature(pcPocket);
        ui->lineFaceName->setText(tr(element.c_str()));
    }
}

void TaskPocketParameters::onLengthChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    pcPocket->Length.setValue((float)len);
    pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onModeChanged(int index)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());

    switch (index) {
        case 0: pcPocket->Type.setValue("Length"); break;
        case 1: pcPocket->Type.setValue("UpToLast"); break;
        case 2: pcPocket->Type.setValue("UpToFirst"); break;
        case 3: pcPocket->Type.setValue("ThroughAll"); break;
        case 4: pcPocket->Type.setValue("UpToFace"); break;
        default: pcPocket->Type.setValue("Length");
    }

    if (index == 0) {
        ui->doubleSpinBox->setEnabled(true);
        ui->lineFaceName->setEnabled(false);
        ui->doubleSpinBox->setValue(pcPocket->Length.getValue());
    } else if (index == 4) {
        ui->lineFaceName->setEnabled(true);
        ui->doubleSpinBox->setEnabled(false);
        ui->lineFaceName->setText(tr(pcPocket->FaceName.getValue()));
    } else {
        ui->doubleSpinBox->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
    }

    pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onFaceName(const QString& text)
{
    if (text.left(4) != tr("Face"))
      return;

    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    pcPocket->FaceName.setValue(text.toUtf8());
    pcPocket->getDocument()->recomputeFeature(pcPocket);
}

double TaskPocketParameters::getLength(void) const
{
    return ui->doubleSpinBox->value();
}

int TaskPocketParameters::getMode(void) const
{
    return ui->changeMode->currentIndex();
}

const QString TaskPocketParameters::getFaceName(void) const
{
    return ui->lineFaceName->text();
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
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",name.c_str(),parameter->getMode());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.FaceName = \"%s\"",name.c_str(),parameter->getFaceName().toAscii().data());
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

    // roll back the done things
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
