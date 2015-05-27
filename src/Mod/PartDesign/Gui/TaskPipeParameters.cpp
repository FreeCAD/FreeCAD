/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include "ui_TaskPipeParameters.h"
#include "TaskPipeParameters.h"
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
#include <Mod/PartDesign/App/FeaturePipe.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"
#include "Workbench.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPipeParameters */

TaskPipeParameters::TaskPipeParameters(ViewProviderPipe *PipeView,bool newObj, QWidget *parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_Pipe",tr("Pipe parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPipeParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);
/*
    connect(ui->lengthEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplane(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversed(bool)));
    connect(ui->lengthEdit2, SIGNAL(valueChanged(double)),
            this, SLOT(onLength2Changed(double)));
    connect(ui->spinOffset, SIGNAL(valueChanged(double)),
            this, SLOT(onOffsetChanged(double)));
    connect(ui->changeMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->buttonFace, SIGNAL(clicked()),
            this, SLOT(onButtonFace()));
    connect(ui->lineFaceName, SIGNAL(textEdited(QString)),
            this, SLOT(onFaceName(QString)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));
*/
    this->groupLayout()->addWidget(proxy);
/*
    // Temporarily prevent unnecessary feature recomputes
    ui->lengthEdit->blockSignals(true);
    ui->lengthEdit2->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);
    ui->buttonFace->blockSignals(true);
    ui->lineFaceName->blockSignals(true);
    ui->changeMode->blockSignals(true);

    // set the history path
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PipeLength"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PipeLength2"));

    // Get the feature data
    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(PipeView->getObject());
    Base::Quantity l = pcPipe->Length.getQuantityValue();
    bool midplane = pcPipe->Midplane.getValue();
    bool reversed = pcPipe->Reversed.getValue();
    Base::Quantity l2 = pcPipe->Length2.getQuantityValue();
    int index = pcPipe->Type.getValue(); // must extract value here, clear() kills it!
    App::DocumentObject* obj = pcPipe->UpToFace.getValue();
    std::vector<std::string> subStrings = pcPipe->UpToFace.getSubValues();
    std::string upToFace;
    int faceId = -1;
    if ((obj != NULL) && !subStrings.empty()) {
        upToFace = subStrings.front();
        if (upToFace.substr(0,4) == "Face")
            faceId = std::atoi(&upToFace[4]);
    }

    // Fill data into dialog elements
    ui->lengthEdit->setMinimum(0);
    ui->lengthEdit->setMaximum(INT_MAX);
    ui->lengthEdit->setValue(l);
    ui->lengthEdit2->setMinimum(0);
    ui->lengthEdit2->setMaximum(INT_MAX);
    ui->lengthEdit2->setValue(l2);


    ui->checkBoxMidplane->setChecked(midplane);
    // According to bug #0000521 the reversed option
    // shouldn't be de-activated if the pad has a support face
    ui->checkBoxReversed->setChecked(reversed);
    if ((obj != NULL) && PartDesign::Feature::isDatum(obj))
        ui->lineFaceName->setText(QString::fromAscii(obj->getNameInDocument()));
    else if (faceId >= 0)
        ui->lineFaceName->setText(QString::fromAscii(obj->getNameInDocument()) + QString::fromAscii(":") + tr("Face") +
                                  QString::number(faceId));
    else
        ui->lineFaceName->setText(tr("No face selected"));
    ui->lineFaceName->setProperty("FaceName", QByteArray(upToFace.c_str()));
    ui->changeMode->clear();
    ui->changeMode->insertItem(0, tr("Dimension"));
    ui->changeMode->insertItem(1, tr("To last"));
    ui->changeMode->insertItem(2, tr("To first"));
    ui->changeMode->insertItem(3, tr("Up to face"));
    ui->changeMode->insertItem(4, tr("Two dimensions"));
    ui->changeMode->setCurrentIndex(index);

    // activate and de-activate dialog elements as appropriate
    ui->lengthEdit->blockSignals(false);
    ui->lengthEdit2->blockSignals(false);
    ui->checkBoxMidplane->blockSignals(false);
    ui->checkBoxReversed->blockSignals(false);
    ui->buttonFace->blockSignals(false);
    ui->lineFaceName->blockSignals(false);
    ui->changeMode->blockSignals(false);
    updateUI(index);

    // if it is a newly created object use the last value of the history
    if(newObj){
        ui->lengthEdit->setToLastUsedValue();
        ui->lengthEdit->selectNumber();
        ui->lengthEdit2->setToLastUsedValue();
        ui->lengthEdit2->selectNumber();
    }*/
}

void TaskPipeParameters::updateUI(int index)
{/*
    if (index == 0) {  // dimension
        ui->labelLength->setVisible(true);
        ui->spinOffset->setVisible(false);
        ui->spinOffset->setEnabled(false);
        ui->labelOffset->setVisible(false);
        ui->lengthEdit->setEnabled(true);
        ui->lengthEdit->selectNumber();
        // Make sure that the spin box has the focus to get key events
        // Calling setFocus() directly doesn't work because the spin box is not
        // yet visible.
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        ui->checkBoxMidplane->setEnabled(true);
        // Reverse only makes sense if Midplane is not true
        ui->checkBoxReversed->setEnabled(!ui->checkBoxMidplane->isChecked());
        ui->lengthEdit2->setEnabled(false);
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 1 || index == 2) { // up to first/last
        ui->labelLength->setVisible(false);
        ui->spinOffset->setVisible(true);
        ui->spinOffset->setEnabled(true);
        ui->labelOffset->setVisible(true);
        ui->lengthEdit->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(true);
        ui->lengthEdit2->setEnabled(false);
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 3) { // up to face
        ui->labelLength->setVisible(false);
        ui->spinOffset->setVisible(true);
        ui->spinOffset->setEnabled(true);
        ui->labelOffset->setVisible(true);
        ui->lengthEdit->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(false);
        ui->lengthEdit2->setEnabled(false);
        ui->buttonFace->setEnabled(true);
        ui->lineFaceName->setEnabled(true);
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->text().isEmpty() || (ui->lineFaceName->text() == tr("No face selected")))
            onButtonFace(true);
    } else { // two dimensions
        ui->labelLength->setVisible(true);
        ui->spinOffset->setVisible(false);
        ui->spinOffset->setEnabled(false);
        ui->labelOffset->setVisible(false);
        ui->lengthEdit->setEnabled(true);
        ui->lengthEdit->selectNumber();
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(false);
        ui->lengthEdit2->setEnabled(true);
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    }*/
}

void TaskPipeParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{/*
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        QString refText = onAddSelection(msg);
        if (refText.length() != 0) {
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
        ui->lineFaceName->setText(tr(""));
        ui->lineFaceName->setProperty("FaceName", QByteArray());
        ui->lineFaceName->blockSignals(false);
    }*/
}

TaskPipeParameters::~TaskPipeParameters()
{
    delete ui;
}

void TaskPipeParameters::changeEvent(QEvent *e)
{/*
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinOffset->blockSignals(true);
        ui->lengthEdit->blockSignals(true);
        ui->lengthEdit2->blockSignals(true);
        ui->lineFaceName->blockSignals(true);
        ui->changeMode->blockSignals(true);
        int index = ui->changeMode->currentIndex();
        ui->retranslateUi(proxy);
        ui->changeMode->clear();
        ui->changeMode->addItem(tr("Dimension"));
        ui->changeMode->addItem(tr("To last"));
        ui->changeMode->addItem(tr("To first"));
        ui->changeMode->addItem(tr("Up to face"));
        ui->changeMode->addItem(tr("Two dimensions"));
        ui->changeMode->setCurrentIndex(index);

        QStringList parts = ui->lineFaceName->text().split(QChar::fromAscii(':'));
        QByteArray upToFace = ui->lineFaceName->property("FaceName").toByteArray();
        int faceId = -1;
        bool ok = false;
        if (upToFace.indexOf("Face") == 0) {
            faceId = upToFace.remove(0,4).toInt(&ok);
        }
#if QT_VERSION >= 0x040700
        ui->lineFaceName->setPlaceholderText(tr("No face selected"));
#endif
        ui->lineFaceName->setText(ok ?
                                  parts[0] + QString::fromAscii(":") + tr("Face") + QString::number(faceId) :
                                  tr("No face selected"));
        ui->spinOffset->blockSignals(false);
        ui->lengthEdit->blockSignals(false);
        ui->lengthEdit2->blockSignals(false);
        ui->lineFaceName->blockSignals(false);
        ui->changeMode->blockSignals(false);
    }*/
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPipeParameters::TaskDlgPipeParameters(ViewProviderPipe *PipeView,bool newObj)
   : TaskDlgSketchBasedParameters(PipeView)
{
    assert(PipeView);
    parameter  = new TaskPipeParameters(PipeView,newObj);

    Content.push_back(parameter);
}

TaskDlgPipeParameters::~TaskDlgPipeParameters()
{

}

//==== calls from the TaskView ===============================================================


bool TaskDlgPipeParameters::accept()
{/*
    std::string name = vp->getObject()->getNameInDocument();

    // save the history 
    parameter->saveHistory();

    try {
        //Gui::Command::openCommand("Pipe changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f",name.c_str(),parameter->getLength());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i",name.c_str(),parameter->getReversed()?1:0);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Midplane = %i",name.c_str(),parameter->getMidplane()?1:0);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length2 = %f",name.c_str(),parameter->getLength2());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Offset = %f",name.c_str(),parameter->getOffset());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",name.c_str(),parameter->getMode());
        std::string facename = (const char*)parameter->getFaceName();

        if (!facename.empty()) {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = %s", name.c_str(), facename.c_str());
        } else
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = None", name.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!vp->getObject()->isValid())
            throw Base::Exception(vp->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;*/
}

//bool TaskDlgPipeParameters::reject()
//{
//    // get the support and Sketch
//    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(PipeView->getObject()); 
//    Sketcher::SketchObject *pcSketch = 0;
//    App::DocumentObject    *pcSupport = 0;
//    if (pcPipe->Sketch.getValue()) {
//        pcSketch = static_cast<Sketcher::SketchObject*>(pcPipe->Sketch.getValue()); 
//        pcSupport = pcSketch->Support.getValue();
//    }
//
//    // roll back the done things
//    Gui::Command::abortCommand();
//    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
//    
//    // if abort command deleted the object the support is visible again
//    if (!Gui::Application::Instance->getViewProvider(pcPipe)) {
//        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
//            Gui::Application::Instance->getViewProvider(pcSketch)->show();
//        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
//            Gui::Application::Instance->getViewProvider(pcSupport)->show();
//    }
//
//    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
//    //Gui::Command::commitCommand();
//
//    return true;
//}



#include "moc_TaskPipeParameters.cpp"
