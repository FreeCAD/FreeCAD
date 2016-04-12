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
#include <Mod/PartDesign/App/Body.h>
#include "ReferenceSelection.h"
#include "Workbench.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPadParameters */

TaskPadParameters::TaskPadParameters(ViewProviderPad *PadView,bool newObj, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Pad"),tr("Pad parameters"),true, parent),PadView(PadView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPadParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->lengthEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplane(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversed(bool)));
    connect(ui->lengthEdit2, SIGNAL(valueChanged(double)),
            this, SLOT(onLength2Changed(double)));
    connect(ui->changeMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->buttonFace, SIGNAL(clicked()),
            this, SLOT(onButtonFace()));
    connect(ui->lineFaceName, SIGNAL(textEdited(QString)),
            this, SLOT(onFaceName(QString)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->lengthEdit->blockSignals(true);
    ui->lengthEdit2->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);
    ui->buttonFace->blockSignals(true);
    ui->lineFaceName->blockSignals(true);
    ui->changeMode->blockSignals(true);

    // set the history path
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PadLength2"));

    // Get the feature data
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    Base::Quantity l = pcPad->Length.getQuantityValue();
    bool midplane = pcPad->Midplane.getValue();
    bool reversed = pcPad->Reversed.getValue();
    Base::Quantity l2 = pcPad->Length2.getQuantityValue();
    int index = pcPad->Type.getValue(); // must extract value here, clear() kills it!
    std::vector<std::string> subStrings = pcPad->UpToFace.getSubValues();
    std::string upToFace;
    int faceId = -1;
    if (!subStrings.empty()) {
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

    // Bind input fields to properties
    ui->lengthEdit->bind(pcPad->Length);
    ui->lengthEdit2->bind(pcPad->Length2);

    ui->checkBoxMidplane->setChecked(midplane);
    // According to bug #0000521 the reversed option
    // shouldn't be de-activated if the pad has a support face
    ui->checkBoxReversed->setChecked(reversed);
#if QT_VERSION >= 0x040700
    ui->lineFaceName->setPlaceholderText(tr("No face selected"));
#endif
    ui->lineFaceName->setText(faceId >= 0 ?
                              tr("Face") + QString::number(faceId) :
                              QString());
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
    }
}

void TaskPadParameters::updateUI(int index)
{
    if (index == 0) {  // dimension
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
        ui->lengthEdit->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(true);
        ui->lengthEdit2->setEnabled(false);
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    } else if (index == 3) { // up to face
        ui->lengthEdit->setEnabled(false);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(false);
        ui->lengthEdit2->setEnabled(false);
        ui->buttonFace->setEnabled(true);
        ui->lineFaceName->setEnabled(true);
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->text().isEmpty())
            onButtonFace(true);
    } else { // two dimensions
        ui->lengthEdit->setEnabled(true);
        ui->lengthEdit->selectNumber();
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        ui->checkBoxMidplane->setEnabled(false);
        ui->checkBoxReversed->setEnabled(false);
        ui->lengthEdit2->setEnabled(true);
        ui->buttonFace->setEnabled(false);
        ui->lineFaceName->setEnabled(false);
        onButtonFace(false);
    }
}

void TaskPadParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // Don't allow selection in other document
        if (strcmp(msg.pDocName, PadView->getObject()->getDocument()->getName()) != 0)
            return;

        if (!msg.pSubName || msg.pSubName[0] == '\0')
            return;
        std::string subName(msg.pSubName);
        if (subName.substr(0,4) != "Face")
            return;
        int faceId = std::atoi(&subName[4]);

        // Don't allow selection outside support
        PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
        Part::Feature* support = pcPad->getSupport();
        if (support == NULL) {
            // There is no support, so we can't select from it...
            // Turn off reference selection mode
            onButtonFace(false);
            return;
        }
        if (strcmp(msg.pObjectName, support->getNameInDocument()) != 0)
            return;

        std::vector<std::string> upToFaces(1,subName);
        pcPad->UpToFace.setValue(support, upToFaces);
        if (updateView())
            pcPad->getDocument()->recomputeFeature(pcPad);
        ui->lineFaceName->blockSignals(true);
        ui->lineFaceName->setText(tr("Face") + QString::number(faceId));
        ui->lineFaceName->setProperty("FaceName", QByteArray(subName.c_str()));
        ui->lineFaceName->blockSignals(false);
        // Turn off reference selection mode
        onButtonFace(false);
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        ui->lineFaceName->blockSignals(true);
        ui->lineFaceName->setText(tr(""));
        ui->lineFaceName->setProperty("FaceName", QByteArray());
        ui->lineFaceName->blockSignals(false);
    }
}

void TaskPadParameters::onLengthChanged(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Length.setValue(len);
    if (updateView())
        pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onMidplane(bool on)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Midplane.setValue(on);
    ui->checkBoxReversed->setEnabled(!on);
    if (updateView())
        pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onReversed(bool on)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Reversed.setValue(on);
    if (updateView())
        pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onLength2Changed(double len)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    pcPad->Length2.setValue(len);
    if (updateView())
        pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onModeChanged(int index)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());

    switch (index) {
        case 0:
            pcPad->Type.setValue("Length");
            // Avoid error message
            if (ui->lengthEdit->value() < Base::Quantity(Precision::Confusion(), Base::Unit::Length))
                ui->lengthEdit->setValue(5.0);
            break;
        case 1: pcPad->Type.setValue("UpToLast"); break;
        case 2: pcPad->Type.setValue("UpToFirst"); break;
        case 3: pcPad->Type.setValue("UpToFace"); break;
        default: pcPad->Type.setValue("TwoLengths");
    }

    updateUI(index);

    if (updateView())
        pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onButtonFace(const bool pressed)
{
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    Part::Feature* support = pcPad->getSupport();
    if (support == NULL) {
        // There is no support, so we can't select from it...
        return;
    }

    if (pressed) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            doc->setHide(PadView->getObject()->getNameInDocument());
            doc->setShow(support->getNameInDocument());
        }
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate
            (new ReferenceSelection(support, false, true, false));
    } else {
        Gui::Selection().rmvSelectionGate();
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            doc->setShow(PadView->getObject()->getNameInDocument());
            doc->setHide(support->getNameInDocument());
        }
    }

    // Update button if onButtonFace() is called explicitly
    ui->buttonFace->setChecked(pressed);
}

void TaskPadParameters::onFaceName(const QString& text)
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

    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    Part::Feature* support = pcPad->getSupport();
    if (support == NULL) {
        // There is no support, so we can't select from it...
        return;
    }
    std::vector<std::string> upToFaces(1,ss.str());
    pcPad->UpToFace.setValue(support, upToFaces);
    if (updateView())
        pcPad->getDocument()->recomputeFeature(pcPad);
}

void TaskPadParameters::onUpdateView(bool on)
{
    if (on) {
        PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
        pcPad->getDocument()->recomputeFeature(pcPad);
    }
}

double TaskPadParameters::getLength(void) const
{
    return ui->lengthEdit->value().getValue();
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
    return ui->lengthEdit2->value().getValue();
}

int TaskPadParameters::getMode(void) const
{
    return ui->changeMode->currentIndex();
}

QByteArray TaskPadParameters::getFaceName(void) const
{
    return ui->lineFaceName->property("FaceName").toByteArray();
}

const bool TaskPadParameters::updateView() const
{
    return ui->checkBoxUpdateView->isChecked();
}

TaskPadParameters::~TaskPadParameters()
{
    delete ui;
}

void TaskPadParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
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

        QByteArray upToFace = this->getFaceName();
        int faceId = -1;
        bool ok = false;
        if (upToFace.indexOf("Face") == 0) {
            faceId = upToFace.remove(0,4).toInt(&ok);
        }
#if QT_VERSION >= 0x040700
        ui->lineFaceName->setPlaceholderText(tr("No face selected"));
#endif
        ui->lineFaceName->setText(ok ?
                                  tr("Face") + QString::number(faceId) :
                                  QString());
        ui->lengthEdit->blockSignals(false);
        ui->lengthEdit2->blockSignals(false);
        ui->lineFaceName->blockSignals(false);
        ui->changeMode->blockSignals(false);
    }
}

void TaskPadParameters::saveHistory(void)
{
    // save the user values to history 
    ui->lengthEdit->pushToHistory();
    ui->lengthEdit2->pushToHistory();
}

void TaskPadParameters::apply()
{
    std::string name = PadView->getObject()->getNameInDocument();
    const char * cname = name.c_str();

    ui->lengthEdit->apply();

    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i",cname,getReversed()?1:0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Midplane = %i",cname,getMidplane()?1:0);

    ui->lengthEdit2->apply();

    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",cname,getMode());
    std::string facename = getFaceName().data();
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject());
    Part::Feature* support = pcPad->getSupport();

    if (support != NULL && !facename.empty()) {
        QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
        buf = buf.arg(QString::fromUtf8(support->getNameInDocument()));
        buf = buf.arg(QString::fromStdString(facename));
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = %s", cname, buf.toStdString().c_str());
    } else
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = None", cname);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    if (!PadView->getObject()->isValid())
        throw Base::Exception(PadView->getObject()->getStatusString());
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPadParameters::TaskDlgPadParameters(ViewProviderPad *PadView,bool newObj)
    : TaskDialog(),PadView(PadView)
{
    assert(PadView);
    parameter  = new TaskPadParameters(PadView,newObj);

    Content.push_back(parameter);
}

TaskDlgPadParameters::~TaskDlgPadParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgPadParameters::open()
{
    // a transaction is already open at creation time of the pad
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Edit pad");
        Gui::Command::openCommand((const char*)msg.toUtf8());
    }
}

void TaskDlgPadParameters::clicked(int)
{
}

bool TaskDlgPadParameters::accept()
{

    // save the history 
    parameter->saveHistory();

    try {
        //Gui::Command::openCommand("Pad changed");
        parameter->apply();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgPadParameters::reject()
{
    // get the support and Sketch
    PartDesign::Pad* pcPad = static_cast<PartDesign::Pad*>(PadView->getObject()); 
    Sketcher::SketchObject *pcSketch = 0;
    if (pcPad->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcPad->Sketch.getValue());
    }    

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    
    // if abort command deleted the object the sketch is visible again
    if (!Gui::Application::Instance->getViewProvider(pcPad)) {
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
    }

    // Body housekeeping
    if (ActivePartObject != NULL) {
        // Make the new Tip and the previous solid feature visible again
        App::DocumentObject* tip = ActivePartObject->Tip.getValue();
        App::DocumentObject* prev = ActivePartObject->getPrevSolidFeature();
        if (tip != NULL) {
            Gui::Application::Instance->getViewProvider(tip)->show();
            if ((tip != prev) && (prev != NULL))
                Gui::Application::Instance->getViewProvider(prev)->show();
        }
    }

    return true;
}



#include "moc_TaskPadParameters.cpp"
