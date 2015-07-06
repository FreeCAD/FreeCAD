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
using namespace Attacher;

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
    } else if ((sub.size() > 6) && (sub.substr(0,6) == "Vertex")) {
        int subId = std::atoi(&sub[6]);
        return QString::fromAscii(obj->getNameInDocument()) + QString::fromAscii(":") + QObject::tr("Vertex") + QString::number(subId);
    } else {
        return QString::fromAscii(obj->getNameInDocument()) + QString::fromAscii(":") + QString::fromLatin1(sub.c_str());
    }

    return QObject::tr("No reference selected");
}

void TaskDatumParameters::makeRefStrings(std::vector<QString>& refstrings, std::vector<std::string>& refnames) {
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    std::vector<App::DocumentObject*> refs = pcDatum->Support.getValues();
    refnames = pcDatum->Support.getSubValues();

    for (int r = 0; r < 4; r++) {
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
    connect(ui->buttonRef4, SIGNAL(pressed()),
            this, SLOT(onButtonRef4()));
    connect(ui->lineRef4, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName4(QString)));
    connect(ui->listOfModes,SIGNAL(itemSelectionChanged()),
            this, SLOT(onModeSelect()));

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
    ui->buttonRef4->blockSignals(true);
    ui->lineRef4->blockSignals(true);
    ui->listOfModes->blockSignals(true);

    // Get the feature data    
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    //std::vector<App::DocumentObject*> refs = pcDatum->Support.getValues();
    std::vector<std::string> refnames = pcDatum->Support.getSubValues();

    //bool checked1 = pcDatum->Checked.getValue();
    double offset = pcDatum->superPlacement.getValue().getPosition().z;
    double offset2 = pcDatum->superPlacement.getValue().getPosition().y;
    double offset3 = pcDatum->superPlacement.getValue().getPosition().x;
    double angle = 0;
    Base::Vector3d val;
    pcDatum->superPlacement.getValue().getRotation().getValue(val, angle);

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
    ui->lineRef4->setText(refstrings[3]);
    ui->lineRef4->setProperty("RefName", QByteArray(refnames[3].c_str()));

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
    ui->buttonRef4->blockSignals(false);
    ui->lineRef4->blockSignals(false);
    ui->listOfModes->blockSignals(false);
    updateUI();
    updateListOfModes(eMapMode(pcDatum->MapMode.getValue()));
    
    //temporary show coordinate systems for selection
    App::Part* part = getPartFor(DatumView->getObject(), false);
    if(part) {        
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(true, Gui::Application::Instance->activeDocument());
            origin->setTemporaryVisibilityAxis(true);
            origin->setTemporaryVisibilityPlanes(true);
        }            
    }   
    if (pcDatum->Support.getSize() == 0)
        autoNext = true;
    else
        autoNext = false;
}

QString getShTypeText(eRefType type)
{
    //get rid of flags in type
    type = eRefType(type & (rtFlagHasPlacement - 1));

    const char* eRefTypeStrings[] = {
        QT_TR_NOOP("Any"),
        QT_TR_NOOP("Vertex"),
        QT_TR_NOOP("Edge"),
        QT_TR_NOOP("Face"),

        QT_TR_NOOP("Line"),
        QT_TR_NOOP("Curve"),
        QT_TR_NOOP("Circle"),
        QT_TR_NOOP("Conic"),
        QT_TR_NOOP("Ellipse"),
        QT_TR_NOOP("Parabola"),
        QT_TR_NOOP("Hyperbola"),

        QT_TR_NOOP("Plane"),
        QT_TR_NOOP("Sphere"),
        QT_TR_NOOP("Revolve"),
        QT_TR_NOOP("Cylinder"),
        QT_TR_NOOP("Torus"),
        QT_TR_NOOP("Cone"),
        //
        QT_TR_NOOP("Object"),
        QT_TR_NOOP("Solid"),
        QT_TR_NOOP("Wire"),
        NULL
    };

    if (type >= 0 && type < rtDummy_numberOfShapeTypes)
        if (eRefTypeStrings[int(type)])
            return QObject::tr(eRefTypeStrings[int(type)]);
    throw Base::Exception("getShTypeText: type value is wrong, or a string is missing in the list");
}

const QString makeRefText(std::set<eRefType> hint)
{
    QString result;
    for (std::set<eRefType>::const_iterator t = hint.begin(); t != hint.end(); t++) {
        QString tText;
        tText = getShTypeText(*t);
        result += QString::fromAscii(result.size() == 0 ? "" : "/") + tText;
    }

    return result;
}

void TaskDatumParameters::updateUI(std::string message, bool error)
{
    //set text if available
    if(!message.empty()) {
        ui->message->setText(QString::fromStdString(message));
        if(error)
            ui->message->setStyleSheet(QString::fromAscii("QLabel{color: red;}"));
        else 
            ui->message->setStyleSheet(QString::fromAscii("QLabel{color: green;}"));
    }
    
    ui->checkBoxFlip->setVisible(false);

    ui->labelOffset->setVisible(true);
    ui->spinOffset->setVisible(true);
    ui->labelOffset2->setVisible(true);
    ui->spinOffset2->setVisible(true);
    ui->labelOffset3->setVisible(true);
    ui->spinOffset3->setVisible(true);

    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    if (pcDatum->isDerivedFrom(Part::Datum::getClassTypeId())) {
        ui->labelAngle->setVisible(true);
        ui->spinAngle->setVisible(true);
    }

    std::vector<App::DocumentObject*> refs = pcDatum->Support.getValues();
    completed = false;

    // Get hints for further required references
    eSuggestResult msg;
    std::set<eRefType> hint;
    eMapMode suggMode = pcDatum->attacher().listMapModes(msg,0,&hint);

    if (msg != srOK) {
        if(hint.size() > 0)
            message = "Need more references";
    } else {
        completed = true;
    }

    double angle = 0;
    Base::Vector3d val;
    pcDatum->superPlacement.getValue().getRotation().getValue(val, angle);

    // Enable the next reference button
    int numrefs = refs.size();

    ui->buttonRef2->setEnabled(numrefs >= 1);
    ui->lineRef2->setEnabled(numrefs >= 1);
    ui->buttonRef3->setEnabled(numrefs >= 2);
    ui->lineRef3->setEnabled(numrefs >= 2);
    ui->buttonRef4->setEnabled(numrefs >= 3);
    ui->lineRef4->setEnabled(numrefs >= 3);

    ui->labelAngle->setEnabled(true);
    ui->spinAngle->setEnabled(true);

    QString hintText = makeRefText(hint);

    // Check if we have all required references
    if (hint.size() == 0) {
        ui->buttonRef2->setEnabled(numrefs >= 2);
        ui->lineRef2->setEnabled(numrefs >= 2);
        ui->buttonRef3->setEnabled(numrefs >= 3);
        ui->lineRef3->setEnabled(numrefs >= 3);
        ui->buttonRef4->setEnabled(numrefs >= 4);
        ui->lineRef4->setEnabled(numrefs >= 4);
        onButtonRef1(false); // No more references required
        completed = true;
        return;
    }

    if (hintText.size() != 0 && autoNext) {
        if (numrefs == 0) {
            onButtonRef1(true);
            autoNext = true;
        } else if (numrefs == 1) {
            ui->buttonRef2->setText(hintText);
            onButtonRef2(true);//will reset autonext, so...
            autoNext = true;
        } else if (numrefs == 2) {
            ui->buttonRef3->setText(hintText);
            onButtonRef3(true);//will reset autonext, so...
            autoNext = true;
        } else if (numrefs == 3) {
            ui->buttonRef4->setText(hintText);
            onButtonRef4(true);
            autoNext = false;
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
    else if (idx == 3)
        return ui->lineRef4;
    else
        return NULL;
}

void TaskDatumParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (iActiveRef < 0)
            return;

        // Note: The validity checking has already been done in ReferenceSelection.cpp
        Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
        std::vector<App::DocumentObject*> refs = pcDatum->Support.getValues();
        std::vector<std::string> refnames = pcDatum->Support.getSubValues();
        App::DocumentObject* selObj = pcDatum->getDocument()->getObject(msg.pObjectName);
        if (selObj == pcDatum) return;//prevent self-referencing
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

        if (iActiveRef < refs.size()) {
            refs[iActiveRef] = selObj;
            refnames[iActiveRef] = subname;
        } else {
            refs.push_back(selObj);
            refnames.push_back(subname);
        }
        
        bool error = false;
        std::string message("Selection accepted");
        try {
            pcDatum->Support.setValues(refs, refnames);
            updateListOfModes();
            eMapMode mmode = getActiveMapMode();//will be mmDeactivated, if no modes are available
            if(mmode == mmDeactivated){
                message = "Selection invalid";
                error = true;
                this->completed = false;
            } else {
                this->completed = true;
            }
            pcDatum->MapMode.setValue(mmode);
        }
        catch(Base::Exception& e) {
            error = true;
            message = std::string(e.what());
        }

        QLineEdit* line = getLine(iActiveRef);
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
    Base::Placement pl = pcDatum->superPlacement.getValue();
    Base::Vector3d pos = pl.getPosition();
    pos.z = val;
    pl.setPosition(pos);
    pcDatum->superPlacement.setValue(pl);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
    updateUI();
}

void TaskDatumParameters::onOffset2Changed(double val)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    Base::Placement pl = pcDatum->superPlacement.getValue();
    Base::Vector3d pos = pl.getPosition();
    pos.y = val;
    pl.setPosition(pos);
    pcDatum->superPlacement.setValue(pl);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
    updateUI();
}

void TaskDatumParameters::onOffset3Changed(double val)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    Base::Placement pl = pcDatum->superPlacement.getValue();
    Base::Vector3d pos = pl.getPosition();
    pos.x = val;
    pl.setPosition(pos);
    pcDatum->superPlacement.setValue(pl);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
    updateUI();
}

void TaskDatumParameters::onAngleChanged(double val)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    Base::Placement pl = pcDatum->superPlacement.getValue();
    Base::Rotation rot = pl.getRotation();
    Base::Vector3d ax;
    double ang;
    rot.getValue(ax,ang);
    ang = val;
    rot.setValue(ax,ang);
    pl.setRotation(rot);
    pcDatum->superPlacement.setValue(pl);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
    updateUI();
}

void TaskDatumParameters::onCheckFlip(bool on)
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    pcDatum->MapReversed.setValue(on);
    pcDatum->getDocument()->recomputeFeature(pcDatum);
}

void TaskDatumParameters::onButtonRef(const bool pressed, const int idx)
{
    autoNext = false;
    if (pressed) {
        Gui::Selection().clearSelection();
        iActiveRef = idx;
    } else {
        Gui::Selection().rmvSelectionGate();
        iActiveRef = -1;
    }
    ui->buttonRef1->setChecked(iActiveRef==0);
    ui->buttonRef2->setChecked(iActiveRef==1);
    ui->buttonRef3->setChecked(iActiveRef==2);
    ui->buttonRef4->setChecked(iActiveRef==3);
}

void TaskDatumParameters::onButtonRef1(const bool pressed) {
    onButtonRef(pressed, 0);
}
void TaskDatumParameters::onButtonRef2(const bool pressed) {
    onButtonRef(pressed, 1);
}
void TaskDatumParameters::onButtonRef3(const bool pressed) {
    onButtonRef(pressed, 2);
}
void TaskDatumParameters::onButtonRef4(const bool pressed) {
    onButtonRef(pressed, 3);
}

void TaskDatumParameters::onModeSelect()
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    pcDatum->MapMode.setValue(getActiveMapMode());
}

void TaskDatumParameters::onRefName(const QString& text, const int idx)
{
    QLineEdit* line = getLine(idx);
    if (line == NULL) return;

    if (text.length() == 0) {
        // Reference was removed
        // Update the reference list
        Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
        std::vector<App::DocumentObject*> refs = pcDatum->Support.getValues();
        std::vector<std::string> refnames = pcDatum->Support.getSubValues();
        std::vector<App::DocumentObject*> newrefs;
        std::vector<std::string> newrefnames;
        for (int r = 0; r < refs.size(); r++) {
            if (r != idx) {
                newrefs.push_back(refs[r]);
                newrefnames.push_back(refnames[r]);
            }
        }
        pcDatum->Support.setValues(newrefs, newrefnames);
        updateListOfModes();
        pcDatum->MapMode.setValue(getActiveMapMode());

        // Update the UI
        std::vector<QString> refstrings;
        makeRefStrings(refstrings, newrefnames);
        ui->lineRef1->setText(refstrings[0]);
        ui->lineRef1->setProperty("RefName", QByteArray(newrefnames[0].c_str()));
        ui->lineRef2->setText(refstrings[1]);
        ui->lineRef2->setProperty("RefName", QByteArray(newrefnames[1].c_str()));
        ui->lineRef3->setText(refstrings[2]);
        ui->lineRef3->setProperty("RefName", QByteArray(newrefnames[2].c_str()));
        ui->lineRef4->setText(refstrings[3]);
        ui->lineRef4->setProperty("RefName", QByteArray(newrefnames[3].c_str()));
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
                if (parts[1].indexOf(rx) >= 0) {
                    int vertexId = rx.cap(1).toInt();
                    ss << "Vertex" << vertexId;
                } else {
                    //none of Edge/Vertex/Face. May be empty string.
                    //Feed in whatever user supplied, even if invalid.
                    ss << parts[1].toLatin1().constData();
                }
            }
        }

        line->setProperty("RefName", QByteArray(ss.str().c_str()));
        subElement = ss.str();
    }

    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    std::vector<App::DocumentObject*> refs = pcDatum->Support.getValues();
    std::vector<std::string> refnames = pcDatum->Support.getSubValues();
    if (idx < refs.size()) {
        refs[idx] = obj;
        refnames[idx] = subElement.c_str();
    } else {
        refs.push_back(obj);
        refnames.push_back(subElement.c_str());
    }
    pcDatum->Support.setValues(refs, refnames);
    updateListOfModes();
    pcDatum->MapMode.setValue(getActiveMapMode());

    updateUI();
}

void TaskDatumParameters::updateListOfModes(eMapMode curMode)
{
    //first up, remember currently selected mode.
    if (curMode == mmDeactivated){
        auto sel = ui->listOfModes->selectedItems();
        if (sel.count() > 0)
            curMode = modesInList[ui->listOfModes->row(sel[0])];
    }

    //obtain list of available modes:
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    eMapMode suggMode = mmDeactivated;
    if (pcDatum->Support.getSize() > 0){
        eSuggestResult msg;
        suggMode = pcDatum->attacher().listMapModes(msg, &modesInList);
    } else {
        //no references - display all modes
        modesInList.clear();
        for(  int mmode = 0  ;  mmode < mmDummy_NumberOfModes  ;  mmode++){
            if (pcDatum->attacher().modeEnabled[mmode])
                modesInList.push_back(eMapMode(mmode));
        }
    }

    //populate list
    ui->listOfModes->blockSignals(true);
    ui->listOfModes->clear();
    QListWidgetItem* iSelect = 0;
    if (modesInList.size()>0) {
        for(  int i = 0  ;  i < modesInList.size()  ;  i++){
            eMapMode mmode = modesInList[i];
            ui->listOfModes->addItem(QString::fromLatin1(AttachEngine::eMapModeStrings[mmode]));
            if (mmode == curMode)
                iSelect = ui->listOfModes->item(i);
            if (mmode == suggMode){
                //make it bold
                QListWidgetItem* item = ui->listOfModes->item(i);
                assert (item);
                QFont fnt = item->font();
                fnt.setBold(true);
                item->setFont(fnt);
            }
        }
    }
    //restore selection
    ui->listOfModes->selectedItems().clear();
    if (iSelect)
        iSelect->setSelected(true);
    ui->listOfModes->blockSignals(false);
}

Attacher::eMapMode TaskDatumParameters::getActiveMapMode()
{
    auto sel = ui->listOfModes->selectedItems();
    if (sel.count() > 0)
        return modesInList[ui->listOfModes->row(sel[0])];
    else {
        Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
        eSuggestResult msg;
        return pcDatum->attacher().listMapModes(msg);
    };
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
void TaskDatumParameters::onRefName4(const QString &text)
{
    onRefName(text, 3);
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
    } else if (idx == 3) {
        obj = ui->lineRef4->text();
        sub = ui->lineRef4->property("RefName").toString();
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
    App::Part* part = getPartFor(DatumView->getObject(), false);
    if(part) {
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(false);
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
        ui->buttonRef4->blockSignals(true);
        ui->lineRef4->blockSignals(true);
        ui->retranslateUi(proxy);

        std::vector<std::string> refnames;
        std::vector<QString> refstrings;
        makeRefStrings(refstrings, refnames);
        ui->lineRef1->setText(refstrings[0]);
        ui->lineRef2->setText(refstrings[1]);
        ui->lineRef3->setText(refstrings[2]);
        ui->lineRef3->setText(refstrings[3]);
        updateListOfModes();
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
        ui->buttonRef4->blockSignals(false);
        ui->lineRef4->blockSignals(false);
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
    if (parameter->getActiveMapMode() == mmDeactivated) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Incompatible reference set"));
        msg.setText(tr("There is no attachment mode that fits the current set"
        " of references. If you choose to continue, the feature will remain where"
        " it is now, and will not be moved as the references change."
        " Continue?"));
        auto btYes = msg.addButton(QMessageBox::Yes);
        auto btNo =  msg.addButton(QMessageBox::No);
        msg.setDefaultButton(btNo);
        msg.setParent(parameter);
        msg.exec();
        if (msg.clickedButton() == btNo)
            return false;
    }

    std::string name = DatumView->getObject()->getNameInDocument();
    Datum* pcDatum = static_cast<Datum*>(DatumView->getObject());

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.superPlacement.Base.z = %f",name.c_str(),parameter->getOffset());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.superPlacement.Base.y = %f",name.c_str(),parameter->getOffset2());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.superPlacement.Base.x = %f",name.c_str(),parameter->getOffset3());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.superPlacement.Rotation.Angle = %f",name.c_str(),parameter->getAngle());
        //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Checked = %i",name.c_str(),parameter->getCheckBox1()?1:0);

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Support = %s", name.c_str(), pcDatum->Support.getPyReprString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MapMode = '%s'", name.c_str(), AttachEngine::eMapModeStrings[parameter->getActiveMapMode()]);

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
