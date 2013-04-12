/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>*
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

#include "ui_TaskDatumParameters.h"
#include "TaskDatumParameters.h"
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
#include <Mod/PartDesign/App/DatumFeature.h>
#include <Mod/PartDesign/App/Body.h>
#include "ReferenceSelection.h"
#include "Workbench.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDatumParameters */

// Create reference name from PropertyLinkSub values in a translatable fashion
const QString makeRefString(const QString& obj, const std::string& sub)
{
    if ((sub.size() > 4) && (sub.substr(4) == "Face")) {
        int subId = std::atoi(&sub[4]);
        return obj + QObject::tr(":Face") + QString::number(subId);
    } else if ((sub.size() > 4) && (sub.substr(4) == "Edge")) {
        int subId = std::atoi(&sub[4]);
        return obj + QObject::tr(":Edge") + QString::number(subId);
    } if ((sub.size() > 6) && (sub.substr(6) == "Vertex")) {
        int subId = std::atoi(&sub[6]);
        return obj + QObject::tr(":Vertex") + QString::number(subId);
    } else {
        return QObject::tr("No reference selected");
    }
}

TaskDatumParameters::TaskDatumParameters(ViewProviderDatum *DatumView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("iconName"),tr("Datum parameters"),true, parent),DatumView(DatumView)
{    
    TaskBox(Gui::BitmapFactory().pixmap((QString::fromAscii("PartDesign_") + DatumView->datumType).toAscii()), DatumView->datumType + tr(" parameters"), true, parent);
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskDatumParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->spinValue1, SIGNAL(valueChanged(double)),
            this, SLOT(onValue1Changed(double)));
    connect(ui->checkBox1, SIGNAL(toggled(bool)),
            this, SLOT(onCheckBox1(bool)));
    connect(ui->buttonRef1, SIGNAL(pressed()),
            this, SLOT(onButtonRef1()));
    connect(ui->lineRef1, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName(QString)));
    connect(ui->buttonRef2, SIGNAL(pressed()),
            this, SLOT(onButtonRef2()));
    connect(ui->lineRef2, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName(QString)));
    connect(ui->buttonRef3, SIGNAL(pressed()),
            this, SLOT(onButtonRef3()));
    connect(ui->lineRef3, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName(QString)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->spinValue1->blockSignals(true);
    ui->checkBox1->blockSignals(true);
    ui->buttonRef1->blockSignals(true);
    ui->lineRef1->blockSignals(true);
    ui->buttonRef2->blockSignals(true);
    ui->lineRef2->blockSignals(true);
    ui->buttonRef3->blockSignals(true);
    ui->lineRef3->blockSignals(true);

    // Get the feature data    
    PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(DatumView->getObject());
    std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
    std::vector<std::string> refnames = pcDatum->References.getSubValues();
    std::vector<QString> refstrings;
    for (int r = 0; r < 3; r++)
        if ((r < refs.size()) && (refs[r] != NULL)) {
            refstrings.push_back(makeRefString(QString::fromAscii(refs[r]->getNameInDocument()), refnames[r]));
        } else {
            refstrings.push_back(tr("No reference selected"));
            refnames.push_back("");
        }

    //bool checked1 = pcDatum->Checked.getValue();
    std::vector<float> vals = pcDatum->Values.getValues();

    // Fill data into dialog elements
    ui->spinValue1->setValue(vals[0]);
    //ui->checkBox1->setChecked(checked1);
    ui->lineRef1->setText(refstrings[0]);
    ui->lineRef1->setProperty("RefName", QByteArray(refnames[0].c_str()));
    ui->lineRef2->setText(refstrings[1]);
    ui->lineRef2->setProperty("RefName", QByteArray(refnames[1].c_str()));
    ui->lineRef3->setText(refstrings[2]);
    ui->lineRef3->setProperty("RefName", QByteArray(refnames[2].c_str()));

    // activate and de-activate dialog elements as appropriate
    ui->spinValue1->blockSignals(false);
    ui->checkBox1->blockSignals(false);
    ui->buttonRef1->blockSignals(false);
    ui->lineRef1->blockSignals(false);
    ui->buttonRef2->blockSignals(false);
    ui->lineRef2->blockSignals(false);
    ui->buttonRef3->blockSignals(false);
    ui->lineRef3->blockSignals(false);
    updateUI();
}

QLineEdit* TaskDatumParameters::getCurrentLine()
{
    if (refSelectionMode == 0)
        return ui->lineRef1;
    else if (refSelectionMode == 1)
        return ui->lineRef2;
    else if (refSelectionMode == 2)
        return ui->lineRef3;
    else
        return NULL;
}

void TaskDatumParameters::updateUI()
{
    ui->checkBox1->setEnabled(false);
    onButtonRef1(true);
}

void TaskDatumParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (refSelectionMode < 0)
            return;
        // Don't allow selection in other document
        if (strcmp(msg.pDocName, DatumView->getObject()->getDocument()->getName()) != 0)
            return;

        if (!msg.pSubName || msg.pSubName[0] == '\0')
            return;
        std::string subName(msg.pSubName);
        if (((subName.size() > 4) && (subName.substr(0,4) != "Face") && (subName.substr(0,4) != "Edge")) ||
            ((subName.size() > 6) && (subName.substr(0,6) != "Vertex")))
            return;

        // Don't allow selection outside Tip solid feature
        App::DocumentObject* solid = PartDesignGui::ActivePartObject->getPrevSolidFeature();
        if (solid == NULL) {
            // There is no solid feature yet, so we can't select from it...
            // Turn off reference selection mode
            onButtonRef1(false);
            return;
        }
        if (strcmp(msg.pObjectName, solid->getNameInDocument()) != 0)
            return;

        PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(DatumView->getObject());
        std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
        std::vector<std::string> refnames = pcDatum->References.getSubValues();
        if (refSelectionMode < refs.size()) {
            refs[refSelectionMode] = solid;
            refnames[refSelectionMode] = subName;
        } else {
            refs.push_back(solid);
            refnames.push_back(subName);
        }
        pcDatum->References.setValues(refs, refnames);
        pcDatum->getDocument()->recomputeFeature(pcDatum);

        QLineEdit* line = getCurrentLine();
        if (line != NULL) {
            line->blockSignals(true);
            line->setText(makeRefString(QString::fromAscii(solid->getNameInDocument()), subName));
            line->setProperty("RefName", QByteArray(subName.c_str()));
            line->blockSignals(false);
        }

        // Turn off reference selection mode
        onButtonRef1(false);
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        QLineEdit* line = getCurrentLine();
        if (line != NULL) {
            line->blockSignals(true);
            line->setText(tr("No reference selected"));
            line->setProperty("RefName", QByteArray());
            line->blockSignals(false);
        }
    }
}

void TaskDatumParameters::onValue1Changed(double val)
{
    PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(DatumView->getObject());
    std::vector<float> vals = pcDatum->Values.getValues();
    vals[0] = val;
    pcDatum->Values.setValues(vals);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
}

void TaskDatumParameters::onCheckBox1(bool on)
{
    PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(DatumView->getObject());
    //pcDatum->Reversed.setValue(on);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
}

void TaskDatumParameters::onButtonRef(const bool pressed, const int idx)
{
    App::DocumentObject* solid = PartDesignGui::ActivePartObject->getPrevSolidFeature();
    if (solid == NULL) {
        // There is no solid feature, so we can't select from it...
        return;
    }

    if (pressed) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            doc->setHide(DatumView->getObject()->getNameInDocument());
            doc->setShow(solid->getNameInDocument());
        }
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate
            (new ReferenceSelection(solid, true, true, false, true));
        refSelectionMode = idx;
    } else {
        Gui::Selection().rmvSelectionGate();
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            doc->setShow(DatumView->getObject()->getNameInDocument());
            doc->setHide(solid->getNameInDocument());
        }

        refSelectionMode = -1;
    }
}

void TaskDatumParameters::onButtonRef1(const bool pressed) {
    onButtonRef(pressed, 0);
    // Update button if onButtonRef1() is called explicitly
    ui->buttonRef1->setChecked(pressed);
}
void TaskDatumParameters::onButtonRef2(const bool pressed) {
    onButtonRef(pressed, 1);
    // Update button if onButtonRef1() is called explicitly
    ui->buttonRef2->setChecked(pressed);
}
void TaskDatumParameters::onButtonRef3(const bool pressed) {
    onButtonRef(pressed, 2);
    // Update button if onButtonRef1() is called explicitly
    ui->buttonRef3->setChecked(pressed);
}

void TaskDatumParameters::onRefName(const QString& text)
{
    // We must expect that "text" is the translation of "Face", "Edge" or "Vertex" followed by an ID.
    QString name;
    QTextStream str(&name);
    QRegExp rx(name);
    std::stringstream ss;
    QLineEdit* line = getCurrentLine();
    if (line == NULL) return;

    str << "^" << tr("Face") << "(\\d+)$";
    if (text.indexOf(rx) < 0) {
        line->setProperty("RefName", QByteArray());
        return;
    } else {
        int faceId = rx.cap(1).toInt();
        ss << "Face" << faceId;
    }
    str << "^" << tr("Edge") << "(\\d+)$";
    if (text.indexOf(rx) < 0) {
        line->setProperty("RefName", QByteArray());
        return;
    } else {
        int lineId = rx.cap(1).toInt();
        ss << "Edge" << lineId;
    }
    str << "^" << tr("Vertex") << "(\\d+)$";
    if (text.indexOf(rx) < 0) {
        line->setProperty("RefName", QByteArray());
        return;
    } else {
        int vertexId = rx.cap(1).toInt();
        ss << "Vertex" << vertexId;
    }
    line->setProperty("RefName", QByteArray(ss.str().c_str()));

    PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(DatumView->getObject());
    App::DocumentObject* solid = PartDesignGui::ActivePartObject->getPrevSolidFeature();
    if (solid == NULL) {
        // There is no solid, so we can't select from it...
        return;
    }
    std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
    std::vector<std::string> refnames = pcDatum->References.getSubValues();
    if (refSelectionMode < refs.size()) {
        refs[refSelectionMode] = solid;
        refnames[refSelectionMode] = ss.str();
    } else {
        refs.push_back(solid);
        refnames.push_back(ss.str());
    }
    pcDatum->References.setValues(refs, refnames);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
}

double TaskDatumParameters::getValue1() const
{
    return ui->spinValue1->value();
}

bool   TaskDatumParameters::getCheck1() const
{
    return ui->checkBox1->isChecked();
}

QString TaskDatumParameters::getRefName(const int idx) const
{
    if (idx == 0)
        return ui->lineRef1->property("RefName").toString();
    else if (idx == 1)
        return ui->lineRef2->property("RefName").toString();
    else if (idx == 2)
        return ui->lineRef3->property("RefName").toString();
}

TaskDatumParameters::~TaskDatumParameters()
{
    delete ui;
}

void TaskDatumParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinValue1->blockSignals(true);
        ui->checkBox1->blockSignals(true);
        ui->buttonRef1->blockSignals(true);
        ui->lineRef1->blockSignals(true);
        ui->buttonRef2->blockSignals(true);
        ui->lineRef2->blockSignals(true);
        ui->buttonRef3->blockSignals(true);
        ui->lineRef3->blockSignals(true);
        ui->retranslateUi(proxy);       

        PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(DatumView->getObject());
        std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
        std::vector<std::string> refnames = pcDatum->References.getSubValues();
        std::vector<QString> refstrings;
        for (int r = 0; r < 3; r++)
            if ((r < refs.size()) && (refs[r] != NULL))
                refstrings.push_back(makeRefString(QString::fromAscii(refs[r]->getNameInDocument()), refnames[r]));
            else
                refstrings.push_back(tr("No reference selected"));

        ui->lineRef1->setText(refstrings[0]);
        ui->lineRef2->setText(refstrings[1]);
        ui->lineRef3->setText(refstrings[2]);

        ui->spinValue1->blockSignals(false);
        ui->checkBox1->blockSignals(false);
        ui->buttonRef1->blockSignals(false);
        ui->lineRef1->blockSignals(false);
        ui->buttonRef2->blockSignals(false);
        ui->lineRef2->blockSignals(false);
        ui->buttonRef3->blockSignals(false);
        ui->lineRef3->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDatumParameters::TaskDlgDatumParameters(ViewProviderDatum *DatumView)
    : TaskDialog(),DatumView(DatumView)
{
    assert(DatumView);
    parameter  = new TaskDatumParameters(DatumView);

    Content.push_back(parameter);
}

TaskDlgDatumParameters::~TaskDlgDatumParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgDatumParameters::open()
{
    
}

void TaskDlgDatumParameters::clicked(int)
{
    
}

bool TaskDlgDatumParameters::accept()
{
    std::string name = DatumView->getObject()->getNameInDocument();

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Values = [%f]",name.c_str(),parameter->getValue1());
        //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Checked = %i",name.c_str(),parameter->getCheckBox1()?1:0);

        App::DocumentObject* solid = PartDesignGui::ActivePartObject->getPrevSolidFeature();
        if (solid != NULL) {
            QString buf = QString::fromAscii("[");
            for (int r = 0; r < 3; r++) {
                QString refname = parameter->getRefName(r);
                if (refname != tr("No reference selected"))
                    buf += QString::fromAscii(r == 0 ? "" : ",") +
                            QString::fromAscii("App.activeDocument().") + QString::fromUtf8(solid->getNameInDocument()) +
                            QString::fromAscii(",") + refname + QString::fromAscii(")");
            }
            buf += QString::fromAscii("]");
            if (buf.size() > 2)
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.References = %s", name.c_str(), buf.toStdString().c_str());
        }

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!DatumView->getObject()->isValid())
            throw Base::Exception(DatumView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Datum dialog: Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgDatumParameters::reject()
{
    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

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



#include "moc_TaskDatumParameters.cpp"
