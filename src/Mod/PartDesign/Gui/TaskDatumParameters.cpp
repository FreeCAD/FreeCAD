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
#include <App/Plane.h>
#include <App/Line.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/App/Body.h>
#include "ReferenceSelection.h"
#include "Workbench.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDatumParameters */

// Create reference name from PropertyLinkSub values in a translatable fashion
const QString makeRefString(const App::DocumentObject* obj, const std::string& sub)
{
    if (obj == NULL)
        return QObject::tr("No reference selected");

    if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId()) ||
        obj->getTypeId().isDerivedFrom(App::Line::getClassTypeId()) ||
        obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()))
        // App::Plane, Liine or Datum feature
        return QString::fromAscii(obj->getNameInDocument());

    if ((sub.size() > 4) && (sub.substr(0,4) == "Face")) {
        int subId = std::atoi(&sub[4]);
        return QString::fromAscii(obj->getNameInDocument()) + QString::fromAscii(":") + QObject::tr("Face") + QString::number(subId);
    } else if ((sub.size() > 4) && (sub.substr(0,4) == "Edge")) {
        int subId = std::atoi(&sub[4]);
        return QString::fromAscii(obj->getNameInDocument()) + QString::fromAscii(":") + QObject::tr("Edge") + QString::number(subId);
    } if ((sub.size() > 6) && (sub.substr(0,6) == "Vertex")) {
        int subId = std::atoi(&sub[6]);
        return QString::fromAscii(obj->getNameInDocument()) + QString::fromAscii(":") + QObject::tr("Vertex") + QString::number(subId);
    }

    return QObject::tr("No reference selected");
}

void TaskDatumParameters::makeRefStrings(std::vector<QString>& refstrings, std::vector<std::string>& refnames) {
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
    refnames = pcDatum->References.getSubValues();

    for (int r = 0; r < 3; r++) {
        if ((r < refs.size()) && (refs[r] != NULL)) {
            refstrings.push_back(makeRefString(refs[r], refnames[r]));
        } else {
            refstrings.push_back(QObject::tr("No reference selected"));
            refnames.push_back("");
        }
    }
}

TaskDatumParameters::TaskDatumParameters(ViewProviderDatum *DatumView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap((QString::fromAscii("PartDesign_") + DatumView->datumType).toAscii()),
              DatumView->datumType + tr(" parameters"), true, parent),
      DatumView(DatumView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskDatumParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->spinOffset, SIGNAL(valueChanged(double)),
            this, SLOT(onOffsetChanged(double)));
    connect(ui->spinOffset2, SIGNAL(valueChanged(double)),
            this, SLOT(onOffset2Changed(double)));
    connect(ui->spinOffset3, SIGNAL(valueChanged(double)),
            this, SLOT(onOffset3Changed(double)));
    connect(ui->spinAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->checkBoxFlip, SIGNAL(toggled(bool)),
            this, SLOT(onCheckFlip(bool)));
    connect(ui->buttonRef1, SIGNAL(pressed()),
            this, SLOT(onButtonRef1()));
    connect(ui->lineRef1, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName1(QString)));
    connect(ui->buttonRef2, SIGNAL(pressed()),
            this, SLOT(onButtonRef2()));
    connect(ui->lineRef2, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName2(QString)));
    connect(ui->buttonRef3, SIGNAL(pressed()),
            this, SLOT(onButtonRef3()));
    connect(ui->lineRef3, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName3(QString)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->spinOffset->blockSignals(true);
    ui->spinOffset2->blockSignals(true);
    ui->spinOffset3->blockSignals(true);
    ui->spinAngle->blockSignals(true);
    ui->checkBoxFlip->blockSignals(true);
    ui->buttonRef1->blockSignals(true);
    ui->lineRef1->blockSignals(true);
    ui->buttonRef2->blockSignals(true);
    ui->lineRef2->blockSignals(true);
    ui->buttonRef3->blockSignals(true);
    ui->lineRef3->blockSignals(true);

    // Get the feature data    
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    //std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
    std::vector<std::string> refnames = pcDatum->References.getSubValues();

    //bool checked1 = pcDatum->Checked.getValue();
    double offset = pcDatum->Offset.getValue();
    double offset2 = pcDatum->Offset2.getValue();
    double offset3 = pcDatum->Offset3.getValue();
    double angle = pcDatum->Angle.getValue();

    // Fill data into dialog elements
    ui->spinOffset->setValue(offset);
    ui->spinOffset2->setValue(offset2);
    ui->spinOffset3->setValue(offset3);
    ui->spinAngle->setValue(angle);
    //ui->checkBoxFlip->setChecked(checked1);
    std::vector<QString> refstrings;
    makeRefStrings(refstrings, refnames);
    ui->lineRef1->setText(refstrings[0]);
    ui->lineRef1->setProperty("RefName", QByteArray(refnames[0].c_str()));
    ui->lineRef2->setText(refstrings[1]);
    ui->lineRef2->setProperty("RefName", QByteArray(refnames[1].c_str()));
    ui->lineRef3->setText(refstrings[2]);
    ui->lineRef3->setProperty("RefName", QByteArray(refnames[2].c_str()));

    // activate and de-activate dialog elements as appropriate
    ui->spinOffset->blockSignals(false);
    ui->spinOffset2->blockSignals(false);
    ui->spinOffset3->blockSignals(false);
    ui->spinAngle->blockSignals(false);
    ui->checkBoxFlip->blockSignals(false);
    ui->buttonRef1->blockSignals(false);
    ui->lineRef1->blockSignals(false);
    ui->buttonRef2->blockSignals(false);
    ui->lineRef2->blockSignals(false);
    ui->buttonRef3->blockSignals(false);
    ui->lineRef3->blockSignals(false);
    updateUI();
    
    //temporary show coordinate systems for selection
    for(App::Part* part : App::GetApplication().getActiveDocument()->getObjectsOfType<App::Part>()) {
    
        if(part->hasObject(DatumView->getObject(), true)) {
            auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
            if(!app_origin.empty()) {
                ViewProviderOrigin* origin;
                origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
                static_cast<Gui::ViewProviderOrigin*>(origin)->setTemporaryVisibilityMode(true, Gui::Application::Instance->activeDocument());
                static_cast<Gui::ViewProviderOrigin*>(origin)->setTemporaryVisibilityAxis(true);
                static_cast<Gui::ViewProviderOrigin*>(origin)->setTemporaryVisibilityPlanes(true);
            }            
            break;
        }
    }    
}

const QString makeRefText(std::set<QString> hint)
{
    QString result;
    for (std::set<QString>::const_iterator t = hint.begin(); t != hint.end(); t++) {
        QString tText;
        if (((*t) == QObject::tr("DPLANE")) || ((*t) == QObject::tr("Plane")))
            tText = QObject::tr("Plane");
        else if (((*t) == QObject::tr("DLINE")) || ((*t) == QObject::tr("Line")))
            tText = QObject::tr("Line");
        else if (((*t) == QObject::tr("DCIRCLE")) || ((*t) == QObject::tr("Circle")))
            tText = QObject::tr("Circle");
        else if (((*t) == QObject::tr("DPOINT")) || ((*t) == QObject::tr("Point")))
            tText = QObject::tr("Point");
        else if (((*t) == QObject::tr("DCYLINDER")) || ((*t) == QObject::tr("Cylinder")))
            tText = QObject::tr("Cylinder");
        else if ((*t) == QObject::tr("Done"))
            tText = QObject::tr("Done");
        result += QString::fromAscii(result.size() == 0 ? "" : "/") + tText;
        // Note: ANGLE is not passed back here but needs separate treatment
    }

    return result;
}

void TaskDatumParameters::updateUI(std::string message, bool error)
{
    //set text if available
    if(!message.empty()) {
        ui->message->setText(QString::fromStdString(message));            
    }
    
    ui->checkBoxFlip->setVisible(false);

    int numOffsets = static_cast<Part::Datum*>(DatumView->getObject())->offsetsAllowed();
    if (numOffsets == 0) {
        ui->labelOffset->setVisible(false);
        ui->spinOffset->setVisible(false);
        ui->labelOffset2->setVisible(false);
        ui->spinOffset2->setVisible(false);
        ui->labelOffset3->setVisible(false);
        ui->spinOffset3->setVisible(false);
    } else if (numOffsets == 1) {
        ui->labelOffset->setVisible(true);
        ui->spinOffset->setVisible(true);
        ui->labelOffset2->setVisible(false);
        ui->spinOffset2->setVisible(false);
        ui->labelOffset3->setVisible(false);
        ui->spinOffset3->setVisible(false);
    } else if (numOffsets == 2) {
        ui->labelOffset->setVisible(true);
        ui->spinOffset->setVisible(true);
        ui->labelOffset2->setVisible(true);
        ui->spinOffset2->setVisible(true);
        ui->labelOffset3->setVisible(false);
        ui->spinOffset3->setVisible(false);
    } else if (numOffsets == 3) {
       ui->labelOffset->setVisible(true);
       ui->spinOffset->setVisible(true);
       ui->labelOffset2->setVisible(true);
       ui->spinOffset2->setVisible(true);
       ui->labelOffset3->setVisible(true);
       ui->spinOffset3->setVisible(true);
   }

    if (DatumView->datumType != QObject::tr("Plane")) {
        ui->labelAngle->setVisible(false);
        ui->spinAngle->setVisible(false);
    }

    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
    completed = false;

    // Get hints for further required references
    std::set<QString> hint = pcDatum->getHint();    

    if (hint == std::set<QString>()) {
        if(!refs.empty())
            QMessageBox::warning(this, tr("Illegal selection"), tr("This feature cannot be created with this combination of references"));
        
        if (refs.size() == 1) {
            onButtonRef1(true);
        } else if (refs.size() == 2) {
            onButtonRef2(true);
        } else if (refs.size() == 3) {
            onButtonRef3(true);
        }
        return;
    }

    double angle = pcDatum->Angle.getValue();
    bool needAngle = (hint.find(QObject::tr("Angle")) != hint.end());
    hint.erase(QObject::tr("Angle"));

    // Enable the next reference button
    int numrefs = refs.size();
    if (needAngle && hint.empty())
        numrefs--;
    if (numrefs == 0) {
        ui->buttonRef2->setEnabled(false);
        ui->lineRef2->setEnabled(false);
        ui->buttonRef3->setEnabled(false);
        ui->lineRef3->setEnabled(false);
    } else if (numrefs == 1) {
        ui->buttonRef2->setEnabled(true);
        ui->lineRef2->setEnabled(true);
        ui->buttonRef3->setEnabled(false);
        ui->lineRef3->setEnabled(false);
    } else if (numrefs == 2) {
        ui->buttonRef2->setEnabled(true);
        ui->lineRef2->setEnabled(true);
        ui->buttonRef3->setEnabled(true);
        ui->lineRef3->setEnabled(true);
    }
    if (needAngle) {
        ui->labelAngle->setEnabled(true);
        ui->spinAngle->setEnabled(true);
    } else if (fabs(angle) < Precision::Confusion()) {
        ui->labelAngle->setEnabled(false);
        ui->spinAngle->setEnabled(false);
    }

    QString hintText = makeRefText(hint);

    // Check if we have all required references
    if (hintText == QObject::tr("Done")) {
        if (refs.size() == 1) {
            ui->buttonRef2->setEnabled(false);
            ui->lineRef2->setEnabled(false);
            ui->buttonRef3->setEnabled(false);
            ui->lineRef3->setEnabled(false);
        } else if (refs.size() == 2) {
            ui->buttonRef3->setEnabled(false);
            ui->lineRef3->setEnabled(false);
        }
        if (fabs(angle) < Precision::Confusion()) {
            ui->labelAngle->setEnabled(false);
            ui->spinAngle->setEnabled(false);
        }
        onButtonRef1(false); // No more references required
        completed = true;
        return;
    }

    if (hintText.size() != 0) {
        if (numrefs == 0) {
            onButtonRef1(true);
        } else if (numrefs == 1) {
            ui->buttonRef2->setText(hintText);
            onButtonRef2(true);
        } else if (numrefs == 2) {
            ui->buttonRef3->setText(hintText);
            onButtonRef3(true);
        }
    }
}

QLineEdit* TaskDatumParameters::getLine(const int idx)
{
    if (idx == 0)
        return ui->lineRef1;
    else if (idx == 1)
        return ui->lineRef2;
    else if (idx == 2)
        return ui->lineRef3;
    else
        return NULL;
}

void TaskDatumParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (refSelectionMode < 0)
            return;

        // Note: The validity checking has already been done in ReferenceSelection.cpp
        Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
        std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
        std::vector<std::string> refnames = pcDatum->References.getSubValues();
        App::DocumentObject* selObj = pcDatum->getDocument()->getObject(msg.pObjectName);
        if (selObj == pcDatum) return;
        std::string subname = msg.pSubName;

        // Remove subname for planes and datum features
        if (selObj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId()) ||
            selObj->getTypeId().isDerivedFrom(App::Line::getClassTypeId()) ||
            selObj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()))
            subname = "";

        // eliminate duplicate selections
        for (int r = 0; r < refs.size(); r++)
            if ((refs[r] == selObj) && (refnames[r] == subname))
                return;

        if (refSelectionMode < refs.size()) {
            refs[refSelectionMode] = selObj;
            refnames[refSelectionMode] = subname;
        } else {
            refs.push_back(selObj);
            refnames.push_back(subname);
        }
        
        bool error = false;
        std::string message("Selection accepted");
        try {
            pcDatum->References.setValues(refs, refnames);
        }
        catch(Base::Exception& e) {
            error = true;
            message = std::string(e.what());
        }

        QLineEdit* line = getLine(refSelectionMode);
        if (line != NULL) {
            line->blockSignals(true);
            line->setText(makeRefString(selObj, subname));
            line->setProperty("RefName", QByteArray(subname.c_str()));
            line->blockSignals(false);
        }

        updateUI(message, error);
    }
}

void TaskDatumParameters::onOffsetChanged(double val)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    pcDatum->Offset.setValue(val);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
    updateUI();
}

void TaskDatumParameters::onOffset2Changed(double val)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    pcDatum->Offset2.setValue(val);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
    updateUI();
}

void TaskDatumParameters::onOffset3Changed(double val)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    pcDatum->Offset3.setValue(val);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
    updateUI();
}

void TaskDatumParameters::onAngleChanged(double val)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    pcDatum->Angle.setValue(val);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
    updateUI();
}

void TaskDatumParameters::onCheckFlip(bool on)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    //pcDatum->Reversed.setValue(on);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
}

void TaskDatumParameters::onButtonRef(const bool pressed, const int idx)
{
    // Note: Even if there is no solid, App::Plane and Part::Datum can still be selected
	PartDesign::Body* activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
	App::DocumentObject* solid = activeBody->getPrevSolidFeature();

    if (pressed) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate
            (new ReferenceSelection(solid, true, true, false, true));
        refSelectionMode = idx;
    } else {
        Gui::Selection().rmvSelectionGate();
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

void TaskDatumParameters::onRefName(const QString& text, const int idx)
{
    QLineEdit* line = getLine(idx);
    if (line == NULL) return;

    if (text.length() == 0) {
        // Reference was removed
        // Update the reference list
        Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
        std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
        std::vector<std::string> refnames = pcDatum->References.getSubValues();
        std::vector<App::DocumentObject*> newrefs;
        std::vector<std::string> newrefnames;
        for (int r = 0; r < refs.size(); r++) {
            if (r != idx) {
                newrefs.push_back(refs[r]);
                newrefnames.push_back(refnames[r]);
            }
        }
        pcDatum->References.setValues(newrefs, newrefnames);

        // Update the UI
        std::vector<QString> refstrings;
        makeRefStrings(refstrings, newrefnames);
        ui->lineRef1->setText(refstrings[0]);
        ui->lineRef1->setProperty("RefName", QByteArray(newrefnames[0].c_str()));
        ui->lineRef2->setText(refstrings[1]);
        ui->lineRef2->setProperty("RefName", QByteArray(newrefnames[1].c_str()));
        ui->lineRef3->setText(refstrings[2]);
        ui->lineRef3->setProperty("RefName", QByteArray(newrefnames[2].c_str()));
        updateUI();
        return;
    }

    QStringList parts = text.split(QChar::fromAscii(':'));
    if (parts.length() < 2)
        parts.push_back(QString::fromAscii(""));
    // Check whether this is the name of an App::Plane or Part::Datum feature
    App::DocumentObject* obj = DatumView->getObject()->getDocument()->getObject(parts[0].toAscii());
    if (obj == NULL) return;

    std::string subElement;
	PartDesign::Body* activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);

    if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        // everything is OK (we assume a Part can only have exactly 3 App::Plane objects located at the base of the feature tree)
        subElement = "";
    } else if (obj->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
        // everything is OK (we assume a Part can only have exactly 3 App::Line objects located at the base of the feature tree)
        subElement = "";
    } else if (obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
        if (!activeBody->hasFeature(obj))
            return;
        subElement = "";
    } else {
        // TODO: check validity of the text that was entered: Does subElement actually reference to an element on the obj?

        // We must expect that "text" is the translation of "Face", "Edge" or "Vertex" followed by an ID.
        QRegExp rx;
        std::stringstream ss;

        rx.setPattern(QString::fromAscii("^") + tr("Face") + QString::fromAscii("(\\d+)$"));
        if (parts[1].indexOf(rx) >= 0) {
            int faceId = rx.cap(1).toInt();
            ss << "Face" << faceId;
        } else {
            rx.setPattern(QString::fromAscii("^") + tr("Edge") + QString::fromAscii("(\\d+)$"));
            if (parts[1].indexOf(rx) >= 0) {
                int lineId = rx.cap(1).toInt();
                ss << "Edge" << lineId;
            } else {
                rx.setPattern(QString::fromAscii("^") + tr("Vertex") + QString::fromAscii("(\\d+)$"));
                if (parts[1].indexOf(rx) < 0) {
                    line->setProperty("RefName", QByteArray());
                    return;
                } else {
                    int vertexId = rx.cap(1).toInt();
                    ss << "Vertex" << vertexId;
                }
            }
        }

        line->setProperty("RefName", QByteArray(ss.str().c_str()));
        subElement = ss.str();
    }

    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    std::vector<App::DocumentObject*> refs = pcDatum->References.getValues();
    std::vector<std::string> refnames = pcDatum->References.getSubValues();
    if (idx < refs.size()) {
        refs[idx] = obj;
        refnames[idx] = subElement.c_str();
    } else {
        refs.push_back(obj);
        refnames.push_back(subElement.c_str());
    }
    pcDatum->References.setValues(refs, refnames);
    updateUI();
}

void TaskDatumParameters::onRefName1(const QString& text)
{
    onRefName(text, 0);
}
void TaskDatumParameters::onRefName2(const QString& text)
{
    onRefName(text, 1);
}
void TaskDatumParameters::onRefName3(const QString& text)
{
    onRefName(text, 2);
}

double TaskDatumParameters::getOffset() const
{
    return ui->spinOffset->value();
}

double TaskDatumParameters::getOffset2() const
{
    return ui->spinOffset2->value();
}

double TaskDatumParameters::getOffset3() const
{
    return ui->spinOffset3->value();
}

double TaskDatumParameters::getAngle() const
{
    return ui->spinAngle->value();
}

bool   TaskDatumParameters::getFlip() const
{
    return ui->checkBoxFlip->isChecked();
}

QString TaskDatumParameters::getReference(const int idx) const
{
    QString obj, sub;

    if (idx == 0) {
        obj = ui->lineRef1->text();
        sub =  ui->lineRef1->property("RefName").toString();
    } else if (idx == 1) {
        obj = ui->lineRef2->text();
        sub = ui->lineRef2->property("RefName").toString();
    } else if (idx == 2) {
        obj = ui->lineRef3->text();
        sub = ui->lineRef3->property("RefName").toString();
    }

    obj = obj.left(obj.indexOf(QString::fromAscii(":")));

    if (obj == tr("No reference selected"))
        return QString::fromAscii("");
    else
        return QString::fromAscii("(App.activeDocument().") + obj + QString::fromAscii(", '") + sub + QString::fromAscii("')");
}

TaskDatumParameters::~TaskDatumParameters()
{
    //end temporary view mode of coordinate system
     for(App::Part* part : App::GetApplication().getActiveDocument()->getObjectsOfType<App::Part>()) {
    
        if(part->hasObject(DatumView->getObject(), true)) {
            auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
            if(!app_origin.empty()) {
                ViewProviderOrigin* origin;
                origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
                static_cast<Gui::ViewProviderOrigin*>(origin)->setTemporaryVisibilityMode(false);
            }            
            break;
        }
    }
    
    delete ui;
}

void TaskDatumParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinOffset->blockSignals(true);
        ui->spinOffset2->blockSignals(true);
        ui->spinOffset3->blockSignals(true);
        ui->spinAngle->blockSignals(true);
        ui->checkBoxFlip->blockSignals(true);
        ui->buttonRef1->blockSignals(true);
        ui->lineRef1->blockSignals(true);
        ui->buttonRef2->blockSignals(true);
        ui->lineRef2->blockSignals(true);
        ui->buttonRef3->blockSignals(true);
        ui->lineRef3->blockSignals(true);
        ui->retranslateUi(proxy);       

        std::vector<std::string> refnames;
        std::vector<QString> refstrings;
        makeRefStrings(refstrings, refnames);
        ui->lineRef1->setText(refstrings[0]);
        ui->lineRef2->setText(refstrings[1]);
        ui->lineRef3->setText(refstrings[2]);
        // TODO: Translate DatumView->datumType ?

        ui->spinOffset->blockSignals(false);
        ui->spinOffset2->blockSignals(false);
        ui->spinOffset3->blockSignals(false);
        ui->spinAngle->blockSignals(false);
        ui->checkBoxFlip->blockSignals(false);
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
    if (!parameter->isCompleted()) {
        QMessageBox::warning(parameter, tr("Not enough references"), tr("Please select further references to complete this feature"));
        return false;
    }

    std::string name = DatumView->getObject()->getNameInDocument();

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Offset = %f",name.c_str(),parameter->getOffset());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Offset2 = %f",name.c_str(),parameter->getOffset2());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Offset3 = %f",name.c_str(),parameter->getOffset3());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Angle = %f",name.c_str(),parameter->getAngle());
        //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Checked = %i",name.c_str(),parameter->getCheckBox1()?1:0);

		PartDesign::Body* activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
		App::DocumentObject* solid = activeBody->getPrevSolidFeature();
        if (solid != NULL) {
            QString buf = QString::fromAscii("[");
            for (int r = 0; r < 3; r++) {
                QString ref = parameter->getReference(r);
                if (ref != QString::fromAscii(""))
                    buf += QString::fromAscii(r == 0 ? "" : ",") + ref;
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
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");

	PartDesign::Body* activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
    // Body housekeeping
	if (activeBody != NULL) {
        // Make the new Tip and the previous solid feature visible again
		App::DocumentObject* tip = activeBody->Tip.getValue();
		App::DocumentObject* prev = activeBody->getPrevSolidFeature();
        if (tip != NULL) {
            Gui::Application::Instance->getViewProvider(tip)->show();
            if ((tip != prev) && (prev != NULL))
                Gui::Application::Instance->getViewProvider(prev)->show();
        }
    }

    return true;
}



#include "moc_TaskDatumParameters.cpp"
