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
# include <boost/bind.hpp>
#endif

#include <Base/Console.h>
#include <ui_DlgReference.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Part/Gui/AttacherTexts.h>

#include "ReferenceSelection.h"
#include "Utils.h"

#include "ui_TaskDatumParameters.h"
#include "TaskDatumParameters.h"
#include "TaskFeaturePick.h"

using namespace PartDesignGui;
using namespace Gui;
using namespace Attacher;

/* TRANSLATOR PartDesignGui::TaskDatumParameters */

// Create reference name from PropertyLinkSub values in a translatable fashion
const QString makeRefString(const App::DocumentObject* obj, const std::string& sub)
{
    if (obj == NULL)
        return QObject::tr("No reference selected");

    if (obj->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId()) ||
        obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()))
        // App::Plane, Liine or Datum feature
        return QString::fromLatin1(obj->getNameInDocument());

    if ((sub.size() > 4) && (sub.substr(0,4) == "Face")) {
        int subId = std::atoi(&sub[4]);
        return QString::fromLatin1(obj->getNameInDocument()) + QString::fromLatin1(":") + QObject::tr("Face") + QString::number(subId);
    } else if ((sub.size() > 4) && (sub.substr(0,4) == "Edge")) {
        int subId = std::atoi(&sub[4]);
        return QString::fromLatin1(obj->getNameInDocument()) + QString::fromLatin1(":") + QObject::tr("Edge") + QString::number(subId);
    } else if ((sub.size() > 6) && (sub.substr(0,6) == "Vertex")) {
        int subId = std::atoi(&sub[6]);
        return QString::fromLatin1(obj->getNameInDocument()) + QString::fromLatin1(":") + QObject::tr("Vertex") + QString::number(subId);
    } else {
        //something else that face/edge/vertex. Can be empty string.
        return QString::fromLatin1(obj->getNameInDocument())
                + (sub.length()>0 ? QString::fromLatin1(":") : QString())
                + QString::fromLatin1(sub.c_str());
    }
}

void TaskDatumParameters::makeRefStrings(std::vector<QString>& refstrings, std::vector<std::string>& refnames) {
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    std::vector<App::DocumentObject*> refs = pcDatum->Support.getValues();
    refnames = pcDatum->Support.getSubValues();

    for (size_t r = 0; r < 4; r++) {
        if ((r < refs.size()) && (refs[r] != NULL)) {
            refstrings.push_back(makeRefString(refs[r], refnames[r]));
        } else {
            refstrings.push_back(QObject::tr("No reference selected"));
            refnames.push_back("");
        }
    }
}

TaskDatumParameters::TaskDatumParameters(ViewProviderDatum *DatumView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap((QString::fromLatin1("PartDesign_") + DatumView->datumType).toLatin1()),
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
    connect(ui->buttonRef1, SIGNAL(clicked(bool)),
            this, SLOT(onButtonRef1(bool)));
    connect(ui->lineRef1, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName1(QString)));
    connect(ui->buttonRef2, SIGNAL(clicked(bool)),
            this, SLOT(onButtonRef2(bool)));
    connect(ui->lineRef2, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName2(QString)));
    connect(ui->buttonRef3, SIGNAL(clicked(bool)),
            this, SLOT(onButtonRef3(bool)));
    connect(ui->lineRef3, SIGNAL(textEdited(QString)),
            this, SLOT(onRefName3(QString)));
    connect(ui->buttonRef4, SIGNAL(clicked(bool)),
            this, SLOT(onButtonRef4(bool)));
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
    PartDesign::Body * body = PartDesign::Body::findBodyOf(DatumView->getObject());
    if (body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, true);
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what () );
        }
    }

    if (pcDatum->Support.getSize() == 0)
        autoNext = true;
    else
        autoNext = false;

    DatumView->setPickable(false);

    // connect object deletion with slot
    auto bnd = boost::bind(&TaskDatumParameters::objectDeleted, this, _1);
    Gui::Document* document = Gui::Application::Instance->getDocument(DatumView->getObject()->getDocument());
    connectDelObject = document->signalDeletedObject.connect(bnd);
}

TaskDatumParameters::~TaskDatumParameters()
{
    connectDelObject.disconnect();
    if (DatumView)
        resetViewMode();
    delete ui;
}

void TaskDatumParameters::resetViewMode()
{
    //end temporary view mode of coordinate system
    PartDesign::Body * body = PartDesign::Body::findBodyOf(DatumView->getObject());
    if (body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
        catch (const Base::Exception &ex) {
            Base::Console().Error("%s\n", ex.what());
        }
    }

    DatumView->setPickable(true);
}

void TaskDatumParameters::objectDeleted(const Gui::ViewProviderDocumentObject& view)
{
    if (DatumView == &view)
        DatumView = nullptr;
}

const QString makeHintText(std::set<eRefType> hint)
{
    QString result;
    for (std::set<eRefType>::const_iterator t = hint.begin(); t != hint.end(); t++) {
        QString tText;
        tText = AttacherGui::getShapeTypeText(*t);
        result += QString::fromLatin1(result.size() == 0 ? "" : "/") + tText;
    }

    return result;
}

void TaskDatumParameters::updateUI(std::string message, bool error)
{
    //set text if available
    if(!message.empty()) {
        ui->message->setText(QString::fromStdString(message));
        if(error)
            ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: red;}"));
        else
            ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: green;}"));
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
    SuggestResult sugr;

    pcDatum->attacher().suggestMapModes(sugr);

    if (sugr.message != SuggestResult::srOK) {
        if(sugr.nextRefTypeHint.size() > 0)
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

    QString hintText = makeHintText(sugr.nextRefTypeHint);

    // Check if we have all required references
    if (sugr.nextRefTypeHint.size() == 0) {
        ui->buttonRef2->setEnabled(numrefs >= 2);
        ui->lineRef2->setEnabled(numrefs >= 2);
        ui->buttonRef3->setEnabled(numrefs >= 3);
        ui->lineRef3->setEnabled(numrefs >= 3);
        ui->buttonRef4->setEnabled(numrefs >= 4);
        ui->lineRef4->setEnabled(numrefs >= 4);
        completed = true;
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
            autoNext = true;
        } else if (numrefs == 4) {
            onButtonRef4(false);
            autoNext = false;
        }
    }
}

QLineEdit* TaskDatumParameters::getLine(unsigned idx)
{
    switch(idx) {
        case 0: return ui->lineRef1;
        case 1: return ui->lineRef2;
        case 2: return ui->lineRef3;
        case 3: return ui->lineRef4;
        default: return NULL;
    }
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
        if (selObj->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId()) ||
            selObj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()))
            subname = "";

        // eliminate duplicate selections
        for (size_t r = 0; r < refs.size(); r++)
            if ((refs[r] == selObj) && (refnames[r] == subname))
                return;

        if (autoNext && iActiveRef > 0 && iActiveRef == (ssize_t) refnames.size()){
            if (refs[iActiveRef-1] == selObj
                && refnames[iActiveRef-1].length() != 0 && subname.length() == 0){
                //A whole object was selected by clicking it twice. Fill it
                //into previous reference, where a sub-named reference filled by
                //the first click is already stored.

                iActiveRef--;
            }
        }
        if (iActiveRef < (ssize_t) refs.size()) {
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

void TaskDatumParameters::onButtonRef(const bool checked, unsigned idx)
{
    autoNext = false;
    if (checked) {
        Gui::Selection().clearSelection();
        iActiveRef = idx;
    } else {
        iActiveRef = -1;
    }
    ui->buttonRef1->setChecked(iActiveRef==0);
    ui->buttonRef2->setChecked(iActiveRef==1);
    ui->buttonRef3->setChecked(iActiveRef==2);
    ui->buttonRef4->setChecked(iActiveRef==3);
}

void TaskDatumParameters::onButtonRef1(const bool checked) {
    onButtonRef(checked, 0);
}
void TaskDatumParameters::onButtonRef2(const bool checked) {
    onButtonRef(checked, 1);
}
void TaskDatumParameters::onButtonRef3(const bool checked) {
    onButtonRef(checked, 2);
}
void TaskDatumParameters::onButtonRef4(const bool checked) {
    onButtonRef(checked, 3);
}

void TaskDatumParameters::onModeSelect()
{
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    pcDatum->MapMode.setValue(getActiveMapMode());
}

void TaskDatumParameters::onRefName(const QString& text, unsigned idx)
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
        for (size_t r = 0; r < refs.size(); r++) {
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

    QStringList parts = text.split(QChar::fromLatin1(':'));
    if (parts.length() < 2)
        parts.push_back(QString::fromLatin1(""));
    // Check whether this is the name of an App::Plane or Part::Datum feature
    App::DocumentObject* obj = DatumView->getObject()->getDocument()->getObject(parts[0].toLatin1());
    if (obj == NULL) return;

    std::string subElement;

    if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        // everything is OK (we assume a Part can only have exactly 3 App::Plane objects located at the base of the feature tree)
        subElement = "";
    } else if (obj->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
        // everything is OK (we assume a Part can only have exactly 3 App::Line objects located at the base of the feature tree)
        subElement = "";
    } else if (obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
        subElement = "";
    } else {
        // TODO: check validity of the text that was entered: Does subElement actually reference to an element on the obj?

        // We must expect that "text" is the translation of "Face", "Edge" or "Vertex" followed by an ID.
        QRegExp rx;
        std::stringstream ss;

        rx.setPattern(QString::fromLatin1("^") + tr("Face") + QString::fromLatin1("(\\d+)$"));
        if (parts[1].indexOf(rx) >= 0) {
            int faceId = rx.cap(1).toInt();
            ss << "Face" << faceId;
        } else {
            rx.setPattern(QString::fromLatin1("^") + tr("Edge") + QString::fromLatin1("(\\d+)$"));
            if (parts[1].indexOf(rx) >= 0) {
                int lineId = rx.cap(1).toInt();
                ss << "Edge" << lineId;
            } else {
                rx.setPattern(QString::fromLatin1("^") + tr("Vertex") + QString::fromLatin1("(\\d+)$"));
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
    SuggestResult sugr;
    sugr.bestFitMode = mmDeactivated;
    int lastValidModeItemIndex = mmDummy_NumberOfModes;
    if (pcDatum->Support.getSize() > 0){
        pcDatum->attacher().suggestMapModes(sugr);
        modesInList = sugr.allApplicableModes;
        //add reachable modes to the list, too, but gray them out (using lastValidModeItemIndex, later)
        lastValidModeItemIndex = modesInList.size()-1;
        for(std::pair<const eMapMode, refTypeStringList> &rm: sugr.reachableModes){
            modesInList.push_back(rm.first);
        }
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
        for (size_t i = 0  ;  i < modesInList.size()  ;  ++i){
            eMapMode mmode = modesInList[i];
            std::vector<QString> mstr = AttacherGui::getUIStrings(pcDatum->attacher().getTypeId(),mmode);
            ui->listOfModes->addItem(mstr[0]);
            QListWidgetItem* item = ui->listOfModes->item(i);
            item->setToolTip(mstr[1] + QString::fromLatin1("\n\n") +
                             tr("Reference combinations:\n") +
                             AttacherGui::getRefListForMode(pcDatum->attacher(),mmode).join(QString::fromLatin1("\n")));
            if (mmode == curMode)
                iSelect = ui->listOfModes->item(i);
            if (i > lastValidModeItemIndex){
                //potential mode - can be reached by selecting more stuff
                item->setFlags(item->flags() & ~(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable));

                refTypeStringList &extraRefs = sugr.reachableModes[mmode];
                if (extraRefs.size() == 1){
                    QStringList buf;
                    for(eRefType rt : extraRefs[0]){
                        buf.append(AttacherGui::getShapeTypeText(rt));
                    }
                    item->setText(tr("%1 (add %2)").arg(
                                      item->text(),
                                      buf.join(QString::fromLatin1("+"))
                                      ));
                } else {
                    item->setText(tr("%1 (add more references)").arg(item->text()));
                }
            } else if (mmode == sugr.bestFitMode){
                //suggested mode - make bold
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
        SuggestResult sugr;
        pcDatum->attacher().suggestMapModes(sugr);
        if (sugr.message == SuggestResult::srOK)
            return sugr.bestFitMode;
        else
            return mmDeactivated;
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
    std::string name = DatumView->getObject()->getNameInDocument();
    Part::Datum* pcDatum = static_cast<Part::Datum*>(DatumView->getObject());
    auto pcActiveBody = PartDesignGui::getBodyFor(pcDatum, false);
    auto pcActivePart = PartDesignGui::getPartFor(pcActiveBody, false);
    std::vector<App::DocumentObject*> copies;

    //see if we are able to assign a mode
    if (parameter->getActiveMapMode() == mmDeactivated) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Incompatible reference set"));
        msg.setText(tr("There is no attachment mode that fits the current set"
        " of references. If you choose to continue, the feature will remain where"
        " it is now, and will not be moved as the references change."
        " Continue?"));
        msg.addButton(QMessageBox::Yes);
        auto btNo =  msg.addButton(QMessageBox::No);
        msg.setDefaultButton(btNo);
        msg.setIcon(QMessageBox::Warning);
        msg.exec();
        if (msg.clickedButton() == btNo)
            return false;
    }

    //see what to do with external references
    //check the prerequisites for the selected objects
    //the user has to decide which option we should take if external references are used
    bool ext = false;
    for(App::DocumentObject* obj : pcDatum->Support.getValues()) {
        if(!pcActiveBody->hasFeature(obj) && !pcActiveBody->getOrigin()->hasObject(obj))
            ext = true;
    }
    if(ext) {
        // TODO rewrite this to be shared with CmdPartDesignNewSketch::activated() (2015-10-20, Fat-Zer)
        QDialog* dia = new QDialog;
        Ui_Dialog dlg;
        dlg.setupUi(dia);
        dia->setModal(true);
        int result = dia->exec();
        if(result == QDialog::DialogCode::Rejected)
            return false;
        else if(!dlg.radioXRef->isChecked()) {

            std::vector<App::DocumentObject*> objs;
            std::vector<std::string> subs = pcDatum->Support.getSubValues();
            int index = 0;
            for(App::DocumentObject* obj : pcDatum->Support.getValues()) {

                if(!pcActiveBody->hasFeature(obj) && !pcActiveBody->getOrigin()->hasObject(obj)) {
                    objs.push_back(PartDesignGui::TaskFeaturePick::makeCopy(obj, subs[index], dlg.radioIndependent->isChecked()));
                    copies.push_back(objs.back());
                    subs[index] = "";
                }
                else
                    objs.push_back(obj);

                index++;
            }

            pcDatum->Support.setValues(objs, subs);
        }
    }

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.superPlacement.Base.z = %f",name.c_str(),parameter->getOffset());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.superPlacement.Base.y = %f",name.c_str(),parameter->getOffset2());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.superPlacement.Base.x = %f",name.c_str(),parameter->getOffset3());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.superPlacement.Rotation.Angle = %f",name.c_str(),parameter->getAngle());
        //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Checked = %i",name.c_str(),parameter->getCheckBox1()?1:0);

        //here it is assumed that the support was already assigned, it just outputs a dummy Python command to the console
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Support = %s", name.c_str(), pcDatum->Support.getPyReprString().c_str());

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MapMode = '%s'", name.c_str(), AttachEngine::getModeName(parameter->getActiveMapMode()).c_str());

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!DatumView->getObject()->isValid())
            throw Base::Exception(DatumView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();

        //we need to add the copied features to the body after the command action, as otherwise freecad crashs unexplainable
        for(auto obj : copies) {
            if(pcActiveBody)
                pcActiveBody->addFeature(obj);
            else if (pcActivePart)
                pcActivePart->addObject(obj);
        }
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Datum dialog: Input error"), QString::fromLatin1(e.what()));
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

    return true;
}



#include "moc_TaskDatumParameters.cpp"
