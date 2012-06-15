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
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplane(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversed(bool)));
    connect(ui->doubleSpinBox2, SIGNAL(valueChanged(double)),
            this, SLOT(onLength2Changed(double)));
    connect(ui->changeMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->lineFaceName, SIGNAL(textEdited(QString)),
            this, SLOT(onFaceName(QString)));

    this->groupLayout()->addWidget(proxy);

    // Get the feature data
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    double l = pcPad->Length.getValue();
    bool midplane = pcPad->Midplane.getValue();
    bool reversed = pcPad->Reversed.getValue();
    double l2 = pcPad->Length2.getValue();
    int index = pcPad->Type.getValue(); // must extract value here, clear() kills it!
    const char* upToFace = pcPad->FaceName.getValue();

    // Fill data into dialog elements
    ui->doubleSpinBox->setMinimum(0);
    ui->doubleSpinBox->setMaximum(INT_MAX);
    ui->doubleSpinBox->setValue(l);
    ui->doubleSpinBox2->setMinimum(0);
    ui->doubleSpinBox2->setMaximum(INT_MAX);
    ui->doubleSpinBox2->setValue(l2);
    ui->checkBoxMidplane->setChecked(midplane);
    // According to bug #0000521 the reversed option
    // shouldn't be de-activated if the pad has a support face
    ui->checkBoxReversed->setChecked(reversed);
    ui->lineFaceName->setText(pcPad->FaceName.isEmpty() ? tr("No face selected") : tr(upToFace));
    ui->changeMode->clear();
    ui->changeMode->insertItem(0, tr("Dimension"));
    ui->changeMode->insertItem(1, tr("To last"));
    ui->changeMode->insertItem(2, tr("To first"));
    ui->changeMode->insertItem(3, tr("Up to face"));
    ui->changeMode->insertItem(4, tr("Two dimensions"));
    ui->changeMode->setCurrentIndex(index);

    // activate and de-activate dialog elements as appropriate
    updateUI(index);
}

void TaskPadParameters::updateUI(int index)
{
    if (index == 0) {  // dimension
        ui->doubleSpinBox->setEnabled(true);
        ui->doubleSpinBox->selectAll();
        // Make sure that the spin box has the focus to get key events
        // Calling setFocus() directly doesn't work because the spin box is not
        // yet visible.
        QMetaObject::invokeMethod(ui->doubleSpinBox, "setFocus", Qt::QueuedConnection);
        ui->checkBoxMidplane->setEnabled(true);
        ui->checkBoxReversed->setEnabled(true);
        ui->doubleSpinBox2->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
    } else if ((index == 1) || (index == 2)) { // up to first/last
        ui->doubleSpinBox->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(false);
        ui->doubleSpinBox2->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
    } else if (index == 3) { // up to face
        ui->doubleSpinBox->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(false);
        ui->doubleSpinBox2->setEnabled(false);
        ui->lineFaceName->setEnabled(true);
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
    } else { // two dimensions
        ui->doubleSpinBox->setEnabled(true);
        ui->doubleSpinBox->selectAll();
        QMetaObject::invokeMethod(ui->doubleSpinBox, "setFocus", Qt::QueuedConnection);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(false);
        ui->doubleSpinBox2->setEnabled(true);
        ui->lineFaceName->setEnabled(false);
    }
}

void TaskPadParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    if (pcPad->Type.getValue() != 3) // ignore user selections if mode is not upToFace
        return;

    if (!msg.pSubName || msg.pSubName[0] == '\0')
        return;
    std::string element(msg.pSubName);
    if (element.substr(0,4) != "Face")
      return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        pcPad->FaceName.setValue(element);
        pcPad->getDocument()->recomputeFeature(pcPad);
        ui->lineFaceName->setText(tr(element.c_str()));
    }
}

void TaskPadParameters::onLengthChanged(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Length.setValue((float)len);
    pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onMidplane(bool on)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Midplane.setValue(on);
    pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onReversed(bool on)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Reversed.setValue(on);
    pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onLength2Changed(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Length2.setValue((float)len);
    pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onModeChanged(int index)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());

    switch (index) {
        case 0: pcPad->Type.setValue("Length"); break;
        case 1: pcPad->Type.setValue("UpToLast"); break;
        case 2: pcPad->Type.setValue("UpToFirst"); break;
        case 3: pcPad->Type.setValue("UpToFace"); break;
        default: pcPad->Type.setValue("TwoLengths");
    }

    updateUI(index);

    pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onFaceName(const QString& text)
{
    if (text.left(4) != tr("Face"))
      return;

    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->FaceName.setValue(text.toUtf8());
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

bool   TaskPadParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

double TaskPadParameters::getLength2(void) const
{
    return ui->doubleSpinBox2->value();
}

int TaskPadParameters::getMode(void) const
{
    return ui->changeMode->currentIndex();
}

const QString TaskPadParameters::getFaceName(void) const
{
    return ui->lineFaceName->text();
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
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Midplane = %i",name.c_str(),parameter->getMidplane()?1:0);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length2 = %f",name.c_str(),parameter->getLength2());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",name.c_str(),parameter->getMode());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.FaceName = \"%s\"",name.c_str(),parameter->getFaceName().toAscii().data());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!PadView->getObject()->isValid())
            throw Base::Exception(PadView->getObject()->getStatusString());
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
