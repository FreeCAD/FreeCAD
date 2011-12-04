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
# include <QMessageBox>
#endif

#include "ui_TaskPadParameters.h"
#include "TaskPadParameters.h"
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
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPadParameters */

TaskPadParameters::TaskPadParameters(ViewProviderPad *PadView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Pad"),tr("Pad parameters"),true, parent),PadView(PadView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPadParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->doubleSpinBox, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));
    connect(ui->checkBoxMirrored, SIGNAL(toggled(bool)),
            this, SLOT(onMirrored(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversed(bool)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    double l = pcPad->Length.getValue();
    bool mirrored = pcPad->MirroredExtent.getValue();
    bool reversed = pcPad->Reversed.getValue();

    ui->doubleSpinBox->setValue(l);
    ui->doubleSpinBox->selectAll();
    ui->checkBoxMirrored->setChecked(mirrored);

    // check if the sketch has support
    Sketcher::SketchObject *pcSketch;
    if (pcPad->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcPad->Sketch.getValue());
        if (pcSketch->Support.getValue())
            // in case of sketch with support, reverse makes no sense (goes into the part)
            ui->checkBoxReversed->setEnabled(0);
        else
            ui->checkBoxReversed->setChecked(reversed);
    }

    ui->checkBoxReversed->setChecked(reversed);
    // Make sure that the spin box has the focus to get key events
    // Calling setFocus() directly doesn't work because the spin box is not
    // yet visible. 
    QMetaObject::invokeMethod(ui->doubleSpinBox, "setFocus", Qt::QueuedConnection);
}

void TaskPadParameters::onLengthChanged(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Length.setValue((float)len);
    pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onMirrored(bool on)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->MirroredExtent.setValue(on);
    pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onReversed(bool on)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Reversed.setValue(on);
    pcPad->getDocument()->recomputeFeature(pcPad);
}

double TaskPadParameters::getLength(void) const
{
    return ui->doubleSpinBox->value();
}

bool   TaskPadParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

bool   TaskPadParameters::getMirroredExtent(void) const
{
    return ui->checkBoxMirrored->isChecked();
}

TaskPadParameters::~TaskPadParameters()
{
    delete ui;
}

void TaskPadParameters::changeEvent(QEvent *e)
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

TaskDlgPadParameters::TaskDlgPadParameters(ViewProviderPad *PadView)
    : TaskDialog(),PadView(PadView)
{
    assert(PadView);
    parameter  = new TaskPadParameters(PadView);

    Content.push_back(parameter);
}

TaskDlgPadParameters::~TaskDlgPadParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgPadParameters::open()
{
    
}

void TaskDlgPadParameters::clicked(int)
{
    
}

bool TaskDlgPadParameters::accept()
{
    std::string name = PadView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand("Pad changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f",name.c_str(),parameter->getLength());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i",name.c_str(),parameter->getReversed()?1:0);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MirroredExtent = %i",name.c_str(),parameter->getMirroredExtent()?1:0);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgPadParameters::reject()
{
    // get the support and Sketch
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject()); 
    Sketcher::SketchObject *pcSketch;
    App::DocumentObject    *pcSupport;
    if (pcPad->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcPad->Sketch.getValue()); 
        pcSupport = pcSketch->Support.getValue();
    }

    // role back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    
    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcPad)) {
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
            Gui::Application::Instance->getViewProvider(pcSupport)->show();
    }

    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    //Gui::Command::commitCommand();

    return true;
}



#include "moc_TaskPadParameters.cpp"
