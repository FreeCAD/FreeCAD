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
#include "ReferenceSelection.h"

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

    ui->doubleSpinBox->setDecimals(Base::UnitsApi::getDecimals());

    connect(ui->doubleSpinBox, SIGNAL(valueChanged(double)),
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
    ui->doubleSpinBox->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);
    ui->buttonFace->blockSignals(true);
    ui->lineFaceName->blockSignals(true);
    ui->changeMode->blockSignals(true);

    // Get the feature data
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
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
    ui->doubleSpinBox->setMinimum(0);
    ui->doubleSpinBox->setMaximum(INT_MAX);
    ui->doubleSpinBox->setValue(l);
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
    ui->checkBoxMidplane->setChecked(midplane);

    ui->doubleSpinBox->blockSignals(false);
    ui->checkBoxMidplane->blockSignals(false);
    ui->checkBoxReversed->blockSignals(false);
    ui->buttonFace->blockSignals(false);
    ui->lineFaceName->blockSignals(false);
    ui->changeMode->blockSignals(false);
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
        ui->doubleSpinBox->setEnabled(true);
        ui->doubleSpinBox->selectAll();
        QMetaObject::invokeMethod(ui->doubleSpinBox, "setFocus", Qt::QueuedConnection);
        ui->checkBoxMidplane->setEnabled(true);
        ui->checkBoxReversed->setEnabled(!ui->checkBoxMidplane->isChecked()); // Will flip direction of dimension
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 1) { // Through all
        ui->checkBoxMidplane->setEnabled(true);
        ui->checkBoxReversed->setEnabled(!ui->checkBoxMidplane->isChecked()); // Will flip direction of through all
        ui->doubleSpinBox->setEnabled(false);
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 2) { // Neither value nor face required // To First
        ui->doubleSpinBox->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false); // Can't have a midplane to a single face
        ui->checkBoxReversed->setEnabled(false); // Will change the direction it seeks for its first face? 
						 // Doesnt work so is currently disabled. Fix probably lies 
						 // somwhere in IF block on line 125 of FeaturePocket.cpp
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 3) { // Only this option requires to select a face // Up to face
        ui->doubleSpinBox->setEnabled(false);
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
        // Don't allow selection in other document
        if (strcmp(msg.pDocName, PocketView->getObject()->getDocument()->getName()) != 0)
            return;

        if (!msg.pSubName || msg.pSubName[0] == '\0')
            return;
        std::string subName(msg.pSubName);
        if (subName.substr(0,4) != "Face")
            return;
        int faceId = std::atoi(&subName[4]);

        // Don't allow selection outside of support
        PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
        Part::Feature* support = pcPocket->getSupport();
        if (support == NULL) {
            // There is no support, so we can't select from it...
            // Turn off reference selection mode
            onButtonFace(false);
            return;
        }
        if (strcmp(msg.pObjectName, support->getNameInDocument()) != 0)
            return;

        std::vector<std::string> upToFaces(1,subName);
        pcPocket->UpToFace.setValue(support, upToFaces);
        if (updateView())
            pcPocket->getDocument()->recomputeFeature(pcPocket);
        ui->lineFaceName->blockSignals(true);
        ui->lineFaceName->setText(tr("Face") + QString::number(faceId));
        ui->lineFaceName->setProperty("FaceName", QByteArray(subName.c_str()));
        ui->lineFaceName->blockSignals(false);
        // Turn off reference selection mode
        onButtonFace(false);
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
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    pcPocket->Length.setValue(len);
    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onMidplaneChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    pcPocket->Midplane.setValue(on);
    ui->checkBoxReversed->setEnabled(!on);
    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onReversedChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    pcPocket->Reversed.setValue(on);
    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onModeChanged(int index)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());

    switch (index) {
        case 0:
            // Why? See below for "UpToFace"
            pcPocket->Type.setValue("Length");
            if (oldLength < Precision::Confusion())
                oldLength = 5.0;
            pcPocket->Length.setValue(oldLength);
            ui->doubleSpinBox->setValue(oldLength);
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
            ui->doubleSpinBox->setValue(0.0);
            break;
        default:
            pcPocket->Type.setValue("Length");
    }

    updateUI(index);

    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onButtonFace(const bool pressed) {
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    Part::Feature* support = pcPocket->getSupport();
    if (support == NULL) {
        // There is no support, so we can't select from it...
        return;
    }

    if (pressed) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            doc->setHide(PocketView->getObject()->getNameInDocument());
            doc->setShow(support->getNameInDocument());
        }
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate
            (new ReferenceSelection(support, false, true, false));
    } else {
        Gui::Selection().rmvSelectionGate();
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            doc->setShow(PocketView->getObject()->getNameInDocument());
            doc->setHide(support->getNameInDocument());
        }
    }

    // Update button if onButtonFace() is called explicitly
    ui->buttonFace->setChecked(pressed);
}

void TaskPocketParameters::onFaceName(const QString& text)
{
    // We must expect that "text" is the translation of "Face" followed by an ID.
    QString name;
    QTextStream str(&name);
    str << "^" << tr("Face") << "(\\d+)$";
    QRegExp rx(name);
    if (text.indexOf(rx) < 0) {
        ui->lineFaceName->setProperty("FaceName", QByteArray());
        return;
    }

    int faceId = rx.cap(1).toInt();
    std::stringstream ss;
    ss << "Face" << faceId;
    ui->lineFaceName->setProperty("FaceName", QByteArray(ss.str().c_str()));

    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
    Part::Feature* support = pcPocket->getSupport();
    if (support == NULL) {
        // There is no support, so we can't select from it...
        return;
    }
    std::vector<std::string> upToFaces(1,ss.str());
    pcPocket->UpToFace.setValue(support, upToFaces);
    if (updateView())
        pcPocket->getDocument()->recomputeFeature(pcPocket);
}

void TaskPocketParameters::onUpdateView(bool on)
{
    if (on) {
        PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
        pcPocket->getDocument()->recomputeFeature(pcPocket);
    }
}

double TaskPocketParameters::getLength(void) const
{
    return ui->doubleSpinBox->value();
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
        ui->doubleSpinBox->blockSignals(true);
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
        ui->doubleSpinBox->blockSignals(false);
        ui->lineFaceName->blockSignals(false);
        ui->changeMode->blockSignals(false);
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

    try {
        //Gui::Command::openCommand("Pocket changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f",name.c_str(),parameter->getLength());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",name.c_str(),parameter->getMode());
        std::string facename = parameter->getFaceName().data();
        PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(PocketView->getObject());
        Part::Feature* support = pcPocket->getSupport();
        if (support != NULL && !facename.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromUtf8(support->getNameInDocument()));
            buf = buf.arg(QString::fromStdString(facename));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = %s", name.c_str(), buf.toStdString().c_str());
        } else
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = None", name.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!PocketView->getObject()->isValid())
            throw Base::Exception(PocketView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

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
