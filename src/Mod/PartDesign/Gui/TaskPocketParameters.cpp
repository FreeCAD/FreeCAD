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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "ui_TaskPocketParameters.h"
#include "TaskPocketParameters.h"
#include <Base/UnitsApi.h>
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
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"
#include "Workbench.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPocketParameters */

TaskPocketParameters::TaskPocketParameters(ViewProviderPocket *PocketView,QWidget *parent)
    : TaskSketchBasedParameters(PocketView, parent, "PartDesign_Pocket",tr("Pocket parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPocketParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->pocketLength, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplaneChanged(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversedChanged(bool)));
    connect(ui->changeMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->buttonFace, SIGNAL(pressed()),
            this, SLOT(onButtonFace()));
    connect(ui->lineFaceName, SIGNAL(textEdited(QString)),
            this, SLOT(onFaceName(QString)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->pocketLength->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);
    ui->buttonFace->blockSignals(true);
    ui->lineFaceName->blockSignals(true);
    ui->changeMode->blockSignals(true);

    // Get the feature data
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    double l = pcPocket->Length.getValue();
    bool midplane = pcPocket->Midplane.getValue();
    bool reversed = pcPocket->Reversed.getValue();
    int index = pcPocket->Type.getValue(); // must extract value here, clear() kills it!
    std::vector<std::string> subStrings = pcPocket->UpToFace.getSubValues();
    std::string upToFace;
    int faceId = -1;
    if (!subStrings.empty()) {
        upToFace = subStrings.front();
        if (upToFace.substr(0,4) == "Face")
            faceId = std::atoi(&upToFace[4]);
    }

    // Fill data into dialog elements
    ui->pocketLength->setMinimum(0);
    ui->pocketLength->setMaximum(INT_MAX);
    ui->pocketLength->setValue(l);
    ui->checkBoxMidplane->setChecked(midplane);
    ui->checkBoxReversed->setChecked(reversed);
    ui->lineFaceName->setText(faceId >= 0 ?
                              tr("Face") + QString::number(faceId) :
                              tr("No face selected"));
    ui->lineFaceName->setProperty("FaceName", QByteArray(upToFace.c_str()));
    ui->changeMode->clear();
    ui->changeMode->insertItem(0, tr("Dimension"));
    ui->changeMode->insertItem(1, tr("Through all"));
    ui->changeMode->insertItem(2, tr("To first"));
    ui->changeMode->insertItem(3, tr("Up to face"));
    ui->changeMode->setCurrentIndex(index);

    // Bind input fields to properties
    ui->pocketLength->bind(pcPocket->Length);

    ui->pocketLength->blockSignals(false);
    ui->checkBoxMidplane->blockSignals(false);
    ui->checkBoxReversed->blockSignals(false);
    ui->buttonFace->blockSignals(false);
    ui->lineFaceName->blockSignals(false);
    ui->changeMode->blockSignals(false);

    // Activate the Reverse option only if the support is a datum plane
    ui->checkBoxReversed->setVisible(pcPocket->isSupportDatum());

    updateUI(index);
 
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

void TaskPocketParameters::updateUI(int index)
{ 
    if (index == 0) { // Only this option requires a numeric value // Dimension
        ui->pocketLength->setEnabled(true);
        ui->pocketLength->selectAll();
        QMetaObject::invokeMethod(ui->pocketLength, "setFocus", Qt::QueuedConnection);
        ui->checkBoxMidplane->setEnabled(true);
        // Reverse only makes sense if Midplane is not true
        ui->checkBoxReversed->setEnabled(!ui->checkBoxMidplane->isChecked()); // Will flip direction of dimension
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 1) { // Through all
        ui->checkBoxMidplane->setEnabled(true);
        ui->checkBoxReversed->setEnabled(!ui->checkBoxMidplane->isChecked()); // Will flip direction of through all
        ui->pocketLength->setEnabled(false);
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 2) { // Neither value nor face required // To First
        ui->pocketLength->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false); // Can't have a midplane to a single face
        ui->checkBoxReversed->setEnabled(false); // Will change the direction it seeks for its first face? 
                                                 // Doesnt work so is currently disabled. Fix probably lies 
                                                 // somwhere in IF block on line 125 of FeaturePocket.cpp
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 3) { // Only this option requires to select a face // Up to face
        ui->pocketLength->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(false); // No need for reverse since user-chosen face will dtermine direction
        ui->buttonFace->setEnabled(true);
        ui->lineFaceName->setEnabled(true);
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->text().isEmpty())
            onButtonFace(true);
    }
}

void TaskPocketParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        QString refText = onAddSelection(msg);
        if (refText.length() > 0) {
            ui->lineFaceName->blockSignals(true);
            ui->lineFaceName->setText(refText);
            ui->lineFaceName->setProperty("FaceName", QByteArray(msg.pSubName));
            ui->lineFaceName->blockSignals(false);
            // Turn off reference selection mode
            onButtonFace(false);
        } else {
            ui->lineFaceName->blockSignals(true);
            ui->lineFaceName->setText(tr("No face selected"));
            ui->lineFaceName->setProperty("FaceName", QByteArray());
            ui->lineFaceName->blockSignals(false);
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        ui->lineFaceName->blockSignals(true);
        ui->lineFaceName->setText(tr("No face selected"));
        ui->lineFaceName->setProperty("FaceName", QByteArray());
        ui->lineFaceName->blockSignals(false);
    }
}

void TaskPocketParameters::onLengthChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Length.setValue(len);
    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onMidplaneChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Midplane.setValue(on);
    ui->checkBoxReversed->setEnabled(!on);
    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onReversedChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Reversed.setValue(on);
    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onModeChanged(int index)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());

    switch (index) {
        case 0:
            // Why? See below for "UpToFace"
            pcPocket->Type.setValue("Length");
            if (oldLength < Precision::Confusion())
                oldLength = 5.0;
            pcPocket->Length.setValue(oldLength);
            ui->pocketLength->setValue(oldLength);
            break;
        case 1:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("ThroughAll");
            break;
        case 2:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFirst");
            break;
        case 3:
            // Because of the code at the begining of Pocket::execute() which is used to detect
            // broken legacy parts, we must set the length to zero here!
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFace");
            pcPocket->Length.setValue(0.0);
            ui->pocketLength->setValue(0.0);
            break;
        default:
            pcPocket->Type.setValue("Length");
    }

    updateUI(index);

    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onButtonFace(const bool pressed) {
    TaskSketchBasedParameters::onButtonFace(pressed);

    // Update button if onButtonFace() is called explicitly
    ui->buttonFace->setChecked(pressed);
}

void TaskPocketParameters::onFaceName(const QString& text)
{
    ui->lineFaceName->setProperty("FaceName", TaskSketchBasedParameters::onFaceName(text));
}

double TaskPocketParameters::getLength(void) const
{
    return ui->pocketLength->value().getValue();
}

bool   TaskPocketParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

int TaskPocketParameters::getMode(void) const
{
    return ui->changeMode->currentIndex();
}

QByteArray TaskPocketParameters::getFaceName(void) const
{
    return ui->lineFaceName->property("FaceName").toByteArray();
}

const bool TaskPocketParameters::updateView() const
{
    return ui->checkBoxUpdateView->isChecked();
}

TaskPocketParameters::~TaskPocketParameters()
{
    delete ui;
}

void TaskPocketParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->pocketLength->blockSignals(true);
        ui->lineFaceName->blockSignals(true);
        ui->changeMode->blockSignals(true);
        int index = ui->changeMode->currentIndex();
        ui->retranslateUi(proxy);
        ui->changeMode->clear();
        ui->changeMode->addItem(tr("Dimension"));
        ui->changeMode->addItem(tr("Through all"));
        ui->changeMode->addItem(tr("To first"));
        ui->changeMode->addItem(tr("Up to face"));
        ui->changeMode->setCurrentIndex(index);

        QByteArray upToFace = this->getFaceName();
        int faceId = -1;
        bool ok = false;
        if (upToFace.indexOf("Face") == 0) {
            faceId = upToFace.remove(0,4).toInt(&ok);
        }
        ui->lineFaceName->setText(ok ?
                                  tr("Face") + QString::number(faceId) :
                                  tr("No face selected"));
        ui->pocketLength->blockSignals(false);
        ui->lineFaceName->blockSignals(false);
        ui->changeMode->blockSignals(false);
    }
}

void TaskPocketParameters::apply()
{
    std::string name = vp->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Pocket changed");
    ui->pocketLength->apply();
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",name.c_str(),getMode());
    std::string facename = getFaceName().data();
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    Part::Feature* support = pcPocket->getSupport();
    if (support != NULL && !facename.empty()) {
        QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
        buf = buf.arg(QString::fromUtf8(support->getNameInDocument()));
        buf = buf.arg(QString::fromStdString(facename));
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = %s", name.c_str(), buf.toStdString().c_str());
    } else
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = None", name.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i", name.c_str(), getReversed()?1:0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    if (!vp->getObject()->isValid())
        throw Base::Exception(vp->getObject()->getStatusString());
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPocketParameters::TaskDlgPocketParameters(ViewProviderPocket *PocketView)
    : TaskDlgSketchBasedParameters(PocketView)
{
    assert(vp);
    parameter  = new TaskPocketParameters(static_cast<ViewProviderPocket*>(vp));

    Content.push_back(parameter);
}

TaskDlgPocketParameters::~TaskDlgPocketParameters()
{

}

//==== calls from the TaskView ===============================================================

bool TaskDlgPocketParameters::accept()
{
    try {
        parameter->apply();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}


#include "moc_TaskPocketParameters.cpp"
