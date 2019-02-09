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
# include <Standard_Failure.hxx>
# include <boost/bind.hpp>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/DocumentObserver.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyExpressionEngine.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/Part/Gui/TaskAttacher.h>
#include <Mod/Part/Gui/AttacherTexts.h>
#include <Mod/Part/App/AttachExtension.h>
#include <Mod/Part/App/DatumFeature.h>

#include "ui_TaskAttacher.h"
#include "TaskAttacher.h"

using namespace PartGui;
using namespace Gui;
using namespace Attacher;

/* TRANSLATOR PartDesignGui::TaskAttacher */

// Create reference name from PropertyLinkSub values in a translatable fashion
const QString makeRefString(const App::DocumentObject* obj, const std::string& sub)
{
    if (obj == NULL)
        return QObject::tr("No reference selected");

    if (obj->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId()) ||
        obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()))
        // App::Plane, Line or Datum feature
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

void TaskAttacher::makeRefStrings(std::vector<QString>& refstrings, std::vector<std::string>& refnames) {
    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    std::vector<App::DocumentObject*> refs = pcAttach->Support.getValues();
    refnames = pcAttach->Support.getSubValues();

    for (size_t r = 0; r < 4; r++) {
        if ((r < refs.size()) && (refs[r] != NULL)) {
            refstrings.push_back(makeRefString(refs[r], refnames[r]));
        } else {
            refstrings.push_back(QObject::tr("No reference selected"));
            refnames.push_back("");
        }
    }
}

TaskAttacher::TaskAttacher(Gui::ViewProviderDocumentObject *ViewProvider,QWidget *parent, QString picture, QString text)
    : TaskBox(Gui::BitmapFactory().pixmap(picture.toLatin1()), text, true, parent),
      ViewProvider(ViewProvider)
{
    //check if we are attachable
    if (!ViewProvider->getObject()->hasExtension(Part::AttachExtension::getExtensionClassTypeId()))
        throw Base::RuntimeError("Object has no Part::AttachExtension");

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskAttacher();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->attachmentOffsetX, SIGNAL(valueChanged(double)), this, SLOT(onAttachmentOffsetXChanged(double)));
    connect(ui->attachmentOffsetY, SIGNAL(valueChanged(double)), this, SLOT(onAttachmentOffsetYChanged(double)));
    connect(ui->attachmentOffsetZ, SIGNAL(valueChanged(double)), this, SLOT(onAttachmentOffsetZChanged(double)));
    connect(ui->attachmentOffsetYaw, SIGNAL(valueChanged(double)), this, SLOT(onAttachmentOffsetYawChanged(double)));
    connect(ui->attachmentOffsetPitch, SIGNAL(valueChanged(double)), this, SLOT(onAttachmentOffsetPitchChanged(double)));
    connect(ui->attachmentOffsetRoll, SIGNAL(valueChanged(double)), this, SLOT(onAttachmentOffsetRollChanged(double)));
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
    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    std::vector<std::string> refnames = pcAttach->Support.getSubValues();

    ui->checkBoxFlip->setChecked(pcAttach->MapReversed.getValue());
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

    this->iActiveRef = 0;
    if (pcAttach->Support.getSize() == 0){
        autoNext = true;
    } else {
        autoNext = false;
    }

    ui->attachmentOffsetX->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Base.x")));
    ui->attachmentOffsetY->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Base.y")));
    ui->attachmentOffsetZ->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Base.z")));
    visibilityAutomation(true);
    updateAttachmentOffsetUI();
    updateReferencesUI();
    updateListOfModes();
    selectMapMode(eMapMode(pcAttach->MapMode.getValue()));
    updatePreview();

    // connect object deletion with slot
    auto bnd1 = boost::bind(&TaskAttacher::objectDeleted, this, _1);
    auto bnd2 = boost::bind(&TaskAttacher::documentDeleted, this, _1);
    Gui::Document* document = Gui::Application::Instance->getDocument(ViewProvider->getObject()->getDocument());
    connectDelObject = document->signalDeletedObject.connect(bnd1);
    connectDelDocument = document->signalDeleteDocument.connect(bnd2);
}

TaskAttacher::~TaskAttacher()
{
    try {
        visibilityAutomation(false);
    }
    catch (...) {
    }

    connectDelObject.disconnect();
    connectDelDocument.disconnect();
    delete ui;
}

void TaskAttacher::objectDeleted(const Gui::ViewProviderDocumentObject& view)
{
    if (ViewProvider == &view) {
        ViewProvider = nullptr;
        this->setDisabled(true);
    }
}

void TaskAttacher::documentDeleted(const Gui::Document&)
{
    ViewProvider = nullptr;
    this->setDisabled(true);
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

void TaskAttacher::updateReferencesUI()
{
    if (!ViewProvider)
        return;

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();

    std::vector<App::DocumentObject*> refs = pcAttach->Support.getValues();
    completed = false;

    // Get hints for further required references...
    // DeepSOIC: hint system became useless since inertial system attachment
    // modes have been introduced, because they accept any number of references
    // of any type, so the hint will always be 'Any'. I keep the logic
    // nevertheless, in case it is decided to resurrect hint system.

    pcAttach->attacher().suggestMapModes(this->lastSuggestResult);

    if (this->lastSuggestResult.message != SuggestResult::srOK) {
        if(this->lastSuggestResult.nextRefTypeHint.size() > 0){
            //message = "Need more references";
        }
    } else {
        completed = true;
    }

    updateRefButton(0);
    updateRefButton(1);
    updateRefButton(2);
    updateRefButton(3);
}

bool TaskAttacher::updatePreview()
{
    if (!ViewProvider)
        return false;

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    QString errMessage;
    bool attached = false;
    try{
        attached = pcAttach->positionBySupport();
    } catch (Base::Exception &err){
        errMessage = QString::fromLatin1(err.what());
    } catch (Standard_Failure &err){
        errMessage = tr("OCC error: %1").arg(QString::fromLatin1(err.GetMessageString()));
    } catch (...) {
        errMessage = tr("unknown error");
    }
    if (errMessage.length()>0){
        ui->message->setText(tr("Attachment mode failed: %1").arg(errMessage));
        ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: red;}"));
    } else {
        if (!attached){
            ui->message->setText(tr("Not attached"));
            ui->message->setStyleSheet(QString());
        } else {
            std::vector<QString> strs = AttacherGui::getUIStrings(pcAttach->attacher().getTypeId(),eMapMode(pcAttach->MapMode.getValue()));
            ui->message->setText(tr("Attached with mode %1").arg(strs[0]));
            ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: green;}"));
        }
    }
    QString splmLabelText = attached ? tr("Attachment Offset:") : tr("Attachment Offset (inactive - not attached):");
    ui->groupBox_AttachmentOffset->setTitle(splmLabelText);
    ui->groupBox_AttachmentOffset->setEnabled(attached);

    return attached;
}

QLineEdit* TaskAttacher::getLine(unsigned idx)
{
    switch(idx) {
        case 0: return ui->lineRef1;
        case 1: return ui->lineRef2;
        case 2: return ui->lineRef3;
        case 3: return ui->lineRef4;
        default: return NULL;
    }
}

void TaskAttacher::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (!ViewProvider)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (iActiveRef < 0)
            return;

        // Note: The validity checking has already been done in ReferenceSelection.cpp
        Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
        std::vector<App::DocumentObject*> refs = pcAttach->Support.getValues();
        std::vector<std::string> refnames = pcAttach->Support.getSubValues();
        App::DocumentObject* selObj = ViewProvider->getObject()->getDocument()->getObject(msg.pObjectName);
        if (selObj == ViewProvider->getObject()) return;//prevent self-referencing

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

        //bool error = false;
        try {
            pcAttach->Support.setValues(refs, refnames);
            updateListOfModes();
            eMapMode mmode = getActiveMapMode();//will be mmDeactivated, if selected or if no modes are available
            if(mmode == mmDeactivated){
                //error = true;
                this->completed = false;
            } else {
                this->completed = true;
            }
            pcAttach->MapMode.setValue(mmode);
            selectMapMode(mmode);
            updatePreview();
        }
        catch(Base::Exception& e) {
            //error = true;
            ui->message->setText(QString::fromLatin1(e.what()));
            ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: red;}"));
        }

        QLineEdit* line = getLine(iActiveRef);
        if (line != NULL) {
            line->blockSignals(true);
            line->setText(makeRefString(selObj, subname));
            line->setProperty("RefName", QByteArray(subname.c_str()));
            line->blockSignals(false);
        }

        if (autoNext) {
            if (iActiveRef == -1){
                //nothing to do
            } else if (iActiveRef == 4 || this->lastSuggestResult.nextRefTypeHint.size() == 0){
                iActiveRef = -1;
            } else {
                iActiveRef++;
            }
        }

        updateReferencesUI();
    }
}

void TaskAttacher::onAttachmentOffsetChanged(double /*val*/, int idx)
{
    if (!ViewProvider)
        return;

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    Base::Placement pl = pcAttach->AttachmentOffset.getValue();

    Base::Vector3d pos = pl.getPosition();
    if (idx == 0) {
        pos.x = ui->attachmentOffsetX->value().getValueAs(Base::Quantity::MilliMetre);
    }
    if (idx == 1) {
        pos.y = ui->attachmentOffsetY->value().getValueAs(Base::Quantity::MilliMetre);
    }
    if (idx == 2) {
        pos.z = ui->attachmentOffsetZ->value().getValueAs(Base::Quantity::MilliMetre);
    }
    if (idx >= 0  && idx <= 2){
        pl.setPosition(pos);
    }

    Base::Rotation rot = pl.getRotation();
    double yaw, pitch, roll;
    rot.getYawPitchRoll(yaw, pitch, roll);
    if (idx == 3) {
        yaw = ui->attachmentOffsetYaw->value().getValueAs(Base::Quantity::Degree);
    }
    if (idx == 4) {
        pitch = ui->attachmentOffsetPitch->value().getValueAs(Base::Quantity::Degree);
    }
    if (idx == 5) {
        roll = ui->attachmentOffsetRoll->value().getValueAs(Base::Quantity::Degree);
    }
    if (idx >= 3  &&  idx <= 5){
        rot.setYawPitchRoll(yaw,pitch,roll);
        pl.setRotation(rot);
    }

    pcAttach->AttachmentOffset.setValue(pl);
    updatePreview();
}

void TaskAttacher::onAttachmentOffsetXChanged(double val)
{
    onAttachmentOffsetChanged(val, 0);
}

void TaskAttacher::onAttachmentOffsetYChanged(double val)
{
    onAttachmentOffsetChanged(val, 1);
}

void TaskAttacher::onAttachmentOffsetZChanged(double val)
{
    onAttachmentOffsetChanged(val, 2);
}

void TaskAttacher::onAttachmentOffsetYawChanged(double val)
{
    onAttachmentOffsetChanged(val, 3);
}

void TaskAttacher::onAttachmentOffsetPitchChanged(double val)
{
    onAttachmentOffsetChanged(val, 4);
}

void TaskAttacher::onAttachmentOffsetRollChanged(double val)
{
    onAttachmentOffsetChanged(val, 5);
}

void TaskAttacher::onCheckFlip(bool on)
{
    if (!ViewProvider)
        return;

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    pcAttach->MapReversed.setValue(on);
    ViewProvider->getObject()->getDocument()->recomputeFeature(ViewProvider->getObject());
}

void TaskAttacher::onButtonRef(const bool checked, unsigned idx)
{
    autoNext = false;
    if (checked) {
        Gui::Selection().clearSelection();
        iActiveRef = idx;
    } else {
        iActiveRef = -1;
    }
    updateRefButton(0);
    updateRefButton(1);
    updateRefButton(2);
    updateRefButton(3);
}

void TaskAttacher::onButtonRef1(const bool checked) {
    onButtonRef(checked, 0);
}

void TaskAttacher::onButtonRef2(const bool checked) {
    onButtonRef(checked, 1);
}

void TaskAttacher::onButtonRef3(const bool checked) {
    onButtonRef(checked, 2);
}

void TaskAttacher::onButtonRef4(const bool checked) {
    onButtonRef(checked, 3);
}

void TaskAttacher::onModeSelect()
{
    if (!ViewProvider)
        return;

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    pcAttach->MapMode.setValue(getActiveMapMode());
    updatePreview();
}

void TaskAttacher::onRefName(const QString& text, unsigned idx)
{
    if (!ViewProvider)
        return;

    QLineEdit* line = getLine(idx);
    if (line == NULL) return;

    if (text.length() == 0) {
        // Reference was removed
        // Update the reference list
        Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
        std::vector<App::DocumentObject*> refs = pcAttach->Support.getValues();
        std::vector<std::string> refnames = pcAttach->Support.getSubValues();
        std::vector<App::DocumentObject*> newrefs;
        std::vector<std::string> newrefnames;
        for (size_t r = 0; r < refs.size(); r++) {
            if (r != idx) {
                newrefs.push_back(refs[r]);
                newrefnames.push_back(refnames[r]);
            }
        }
        pcAttach->Support.setValues(newrefs, newrefnames);
        updateListOfModes();
        pcAttach->MapMode.setValue(getActiveMapMode());
        selectMapMode(getActiveMapMode());

        updatePreview();

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
        updateReferencesUI();
        return;
    }

    QStringList parts = text.split(QChar::fromLatin1(':'));
    if (parts.length() < 2)
        parts.push_back(QString::fromLatin1(""));
    // Check whether this is the name of an App::Plane or Part::Datum feature
    App::DocumentObject* obj = ViewProvider->getObject()->getDocument()->getObject(parts[0].toLatin1());
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

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    std::vector<App::DocumentObject*> refs = pcAttach->Support.getValues();
    std::vector<std::string> refnames = pcAttach->Support.getSubValues();
    if (idx < refs.size()) {
        refs[idx] = obj;
        refnames[idx] = subElement.c_str();
    } else {
        refs.push_back(obj);
        refnames.push_back(subElement.c_str());
    }
    pcAttach->Support.setValues(refs, refnames);
    updateListOfModes();
    pcAttach->MapMode.setValue(getActiveMapMode());
    selectMapMode(getActiveMapMode());

    updateReferencesUI();
}

void TaskAttacher::updateRefButton(int idx)
{
    if (!ViewProvider)
        return;

    QAbstractButton* b;
    switch(idx){
        case 0: b = ui->buttonRef1; break;
        case 1: b = ui->buttonRef2; break;
        case 2: b = ui->buttonRef3; break;
        case 3: b = ui->buttonRef4; break;
        default: throw Base::IndexError("button index out of range");
    }

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    std::vector<App::DocumentObject*> refs = pcAttach->Support.getValues();

    int numrefs = refs.size();
    bool enable = true;
    if (idx > numrefs)
        enable = false;
    if (idx == numrefs && this->lastSuggestResult.nextRefTypeHint.size() == 0)
        enable = false;
    b->setEnabled(enable);

    b->setChecked(iActiveRef == idx);

    if (iActiveRef == idx) {
        b->setText(tr("Selecting..."));
    } else if (idx < static_cast<int>(this->lastSuggestResult.references_Types.size())){
        b->setText(AttacherGui::getShapeTypeText(this->lastSuggestResult.references_Types[idx]));
    } else {
        b->setText(tr("Reference%1").arg(idx+1));
    }
}

void TaskAttacher::updateAttachmentOffsetUI()
{
    if (!ViewProvider)
        return;

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    Base::Placement pl = pcAttach->AttachmentOffset.getValue();
    Base::Vector3d pos = pl.getPosition();
    Base::Rotation rot = pl.getRotation();
    double yaw, pitch, roll;
    rot.getYawPitchRoll(yaw, pitch, roll);

    bool bBlock = true;
    ui->attachmentOffsetX->blockSignals(bBlock);
    ui->attachmentOffsetY->blockSignals(bBlock);
    ui->attachmentOffsetZ->blockSignals(bBlock);
    ui->attachmentOffsetYaw->blockSignals(bBlock);
    ui->attachmentOffsetPitch->blockSignals(bBlock);
    ui->attachmentOffsetRoll->blockSignals(bBlock);

    ui->attachmentOffsetX->setValue(Base::Quantity(pos.x,Base::Unit::Length));
    ui->attachmentOffsetY->setValue(Base::Quantity(pos.y,Base::Unit::Length));
    ui->attachmentOffsetZ->setValue(Base::Quantity(pos.z,Base::Unit::Length));
    ui->attachmentOffsetYaw->setValue(yaw);
    ui->attachmentOffsetPitch->setValue(pitch);
    ui->attachmentOffsetRoll->setValue(roll);

    auto expressions = ViewProvider->getObject()->ExpressionEngine.getExpressions();
    bool bRotationBound = false;
    bRotationBound = bRotationBound ||
            expressions.find(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Rotation.Angle"))) != expressions.end();
    bRotationBound = bRotationBound ||
            expressions.find(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Rotation.Axis.x"))) != expressions.end();
    bRotationBound = bRotationBound ||
            expressions.find(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Rotation.Axis.y"))) != expressions.end();
    bRotationBound = bRotationBound ||
            expressions.find(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Rotation.Axis.z"))) != expressions.end();

    ui->attachmentOffsetYaw->setEnabled(!bRotationBound);
    ui->attachmentOffsetPitch->setEnabled(!bRotationBound);
    ui->attachmentOffsetRoll->setEnabled(!bRotationBound);

    QString tooltip = bRotationBound ? tr("Not editable because rotation of AttachmentOffset is bound by expressions.") : QString();
    ui->attachmentOffsetYaw->setToolTip(tooltip);
    ui->attachmentOffsetPitch->setToolTip(tooltip);
    ui->attachmentOffsetRoll->setToolTip(tooltip);

    bBlock = false;
    ui->attachmentOffsetX->blockSignals(bBlock);
    ui->attachmentOffsetY->blockSignals(bBlock);
    ui->attachmentOffsetZ->blockSignals(bBlock);
    ui->attachmentOffsetYaw->blockSignals(bBlock);
    ui->attachmentOffsetPitch->blockSignals(bBlock);
    ui->attachmentOffsetRoll->blockSignals(bBlock);
}

void TaskAttacher::updateListOfModes()
{
    if (!ViewProvider)
        return;

    //first up, remember currently selected mode.
    eMapMode curMode = mmDeactivated;
    auto sel = ui->listOfModes->selectedItems();
    if (sel.count() > 0)
        curMode = modesInList[ui->listOfModes->row(sel[0])];

    //obtain list of available modes:
    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    this->lastSuggestResult.bestFitMode = mmDeactivated;
    size_t lastValidModeItemIndex = mmDummy_NumberOfModes;

    if (pcAttach->Support.getSize() > 0){
        pcAttach->attacher().suggestMapModes(this->lastSuggestResult);
        modesInList = this->lastSuggestResult.allApplicableModes;
        modesInList.insert(modesInList.begin(), mmDeactivated); // always have the option to choose Deactivated mode

        //add reachable modes to the list, too, but gray them out (using lastValidModeItemIndex, later)
        lastValidModeItemIndex = modesInList.size()-1;
        for(std::pair<const eMapMode, refTypeStringList> &rm: this->lastSuggestResult.reachableModes){
            modesInList.push_back(rm.first);
        }
    } else {
        //no references - display all modes
        modesInList.clear();
        modesInList.push_back(mmDeactivated);

        for(  int mmode = 0  ;  mmode < mmDummy_NumberOfModes  ;  mmode++){
            if (pcAttach->attacher().modeEnabled[mmode])
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
            std::vector<QString> mstr = AttacherGui::getUIStrings(pcAttach->attacher().getTypeId(),mmode);
            ui->listOfModes->addItem(mstr[0]);
            QListWidgetItem* item = ui->listOfModes->item(i);
            QString tooltip = mstr[1];

            if (mmode != mmDeactivated) {
                tooltip += QString::fromLatin1("\n\n") + tr("Reference combinations:") + QString::fromLatin1("\n") +
                   AttacherGui::getRefListForMode(pcAttach->attacher(),mmode).join(QString::fromLatin1("\n"));
            }
            item->setToolTip(tooltip);

            if (mmode == curMode && curMode != mmDeactivated)
                iSelect = ui->listOfModes->item(i);
            if (i > lastValidModeItemIndex){
                //potential mode - can be reached by selecting more stuff
                item->setFlags(item->flags() & ~(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable));

                refTypeStringList &extraRefs = this->lastSuggestResult.reachableModes[mmode];
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
            } else if (mmode == this->lastSuggestResult.bestFitMode){
                //suggested mode - make bold
                assert (item);
                QFont fnt = item->font();
                fnt.setBold(true);
                item->setFont(fnt);
            }

        }
    }

    //restore selection
    if (iSelect)
        iSelect->setSelected(true);

    ui->listOfModes->blockSignals(false);
}

void TaskAttacher::selectMapMode(eMapMode mmode) {
    ui->listOfModes->blockSignals(true);

    for (size_t i = 0;  i < modesInList.size(); ++i) {
        if (modesInList[i] == mmode) {
            ui->listOfModes->item(i)->setSelected(true);
        }
    }

    ui->listOfModes->blockSignals(false);
}

Attacher::eMapMode TaskAttacher::getActiveMapMode()
{
    auto sel = ui->listOfModes->selectedItems();
    if (sel.count() > 0)
        return modesInList[ui->listOfModes->row(sel[0])];
    else {
        if (this->lastSuggestResult.message == SuggestResult::srOK)
            return this->lastSuggestResult.bestFitMode;
        else
            return mmDeactivated;
    };
}

void TaskAttacher::onRefName1(const QString& text)
{
    onRefName(text, 0);
}

void TaskAttacher::onRefName2(const QString& text)
{
    onRefName(text, 1);
}

void TaskAttacher::onRefName3(const QString& text)
{
    onRefName(text, 2);
}

void TaskAttacher::onRefName4(const QString &text)
{
    onRefName(text, 3);
}

bool TaskAttacher::getFlip() const
{
    return ui->checkBoxFlip->isChecked();
}

void TaskAttacher::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
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

void TaskAttacher::visibilityAutomation(bool opening_not_closing)
{
    if (opening_not_closing) {
        //crash guards
        if (!ViewProvider)
            return;
        if (!ViewProvider->getObject())
            return;
        if (!ViewProvider->getObject()->getNameInDocument())
            return;
        try{
            QString code = QString::fromLatin1(
                "import Show\n"
                "from Show.DepGraphTools import getAllDependent, isContainer\n"
                "tv = Show.TempoVis(App.ActiveDocument)\n"
                "dep_features = [o for o in getAllDependent(%1) if not isContainer(o)]\n"
                "if %1.isDerivedFrom('PartDesign::CoordinateSystem'):\n"
                "\tvisible_features = [feat for feat in %1.InList if feat.isDerivedFrom('PartDesign::FeaturePrimitive')]\n"
                "\tdep_features = [feat for feat in dep_features if feat not in visible_features]\n"
                "tv.hide(dep_features)\n"
                "if not %1.isDerivedFrom('PartDesign::CoordinateSystem'):\n"
                "\t\tif len(%1.Support) > 0:\n"
                "\t\t\ttv.show([lnk[0] for lnk in %1.Support])"
                );
            QByteArray code_2 = code.arg(
                QString::fromLatin1("App.ActiveDocument.") +
                QString::fromLatin1(ViewProvider->getObject()->getNameInDocument())
                ).toLatin1();
                Base::Interpreter().runString(code_2.constData());
        }
        catch (const Base::Exception &e){
            e.ReportException();
        }
        catch (const Py::Exception&) {
            Base::PyException e;
            e.ReportException();
        }
    }
    else {
        try {
            Base::Interpreter().runString("del(tv)");
        }
        catch (Base::Exception &e) {
            e.ReportException();
        }
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgAttacher::TaskDlgAttacher(Gui::ViewProviderDocumentObject *ViewProvider, bool createBox)
    : TaskDialog(),ViewProvider(ViewProvider), parameter(nullptr)
{
    assert(ViewProvider);
    setDocumentName(ViewProvider->getDocument()->getDocument()->getName());

    if(createBox) {
        parameter  = new TaskAttacher(ViewProvider);
        Content.push_back(parameter);
    }
}

TaskDlgAttacher::~TaskDlgAttacher()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgAttacher::open()
{

}

void TaskDlgAttacher::clicked(int)
{

}

bool TaskDlgAttacher::accept()
{
    try {
        Gui::DocumentT doc(getDocumentName());
        Gui::Document* document = doc.getDocument();
        if (!document || !ViewProvider)
            return true;

        Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
        std::string name = ViewProvider->getObject()->getNameInDocument();
        std::string appDocument = doc.getAppDocumentPython();
        std::string guiDocument = doc.getGuiDocumentPython();

        //DeepSOIC: changed this to heavily rely on dialog constantly updating feature properties
        if (pcAttach->AttachmentOffset.isTouched()){
            Base::Placement plm = pcAttach->AttachmentOffset.getValue();
            double yaw, pitch, roll;
            plm.getRotation().getYawPitchRoll(yaw,pitch,roll);
            Gui::Command::doCommand(Gui::Command::Doc,"%s.%s.AttachmentOffset = App.Placement(App.Vector(%.10f, %.10f, %.10f),  App.Rotation(%.10f, %.10f, %.10f))",
                                    appDocument.c_str(), name.c_str(),
                                    plm.getPosition().x, plm.getPosition().y, plm.getPosition().z,
                                    yaw, pitch, roll);
        }

        Gui::Command::doCommand(Gui::Command::Doc,"%s.%s.MapReversed = %s",
                                appDocument.c_str(), name.c_str(),
                                pcAttach->MapReversed.getValue() ? "True" : "False");

        Gui::Command::doCommand(Gui::Command::Doc,"%s.%s.Support = %s",
                                appDocument.c_str(), name.c_str(),
                                pcAttach->Support.getPyReprString().c_str());

        Gui::Command::doCommand(Gui::Command::Doc,"%s.%s.MapMode = '%s'",
                                appDocument.c_str(), name.c_str(),
                                AttachEngine::getModeName(eMapMode(pcAttach->MapMode.getValue())).c_str());

        Gui::Command::doCommand(Gui::Command::Doc,"%s.recompute()", appDocument.c_str());

        Gui::Command::doCommand(Gui::Command::Gui, "%s.resetEdit()", guiDocument.c_str());
        document->commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Datum dialog: Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgAttacher::reject()
{
    Gui::DocumentT doc(getDocumentName());
    Gui::Document* document = doc.getDocument();
    if (document) {
        // roll back the done things
        document->abortCommand();
        Gui::Command::doCommand(Gui::Command::Gui,"%s.resetEdit()", doc.getGuiDocumentPython().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"%s.recompute()", doc.getAppDocumentPython().c_str());
    }

    return true;
}

#include "moc_TaskAttacher.cpp"
