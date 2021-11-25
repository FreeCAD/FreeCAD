/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <boost_bind_bind.hpp>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/DocumentObserver.h>
#include <Gui/ViewProviderOrigin.h>
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
#include <Gui/CommandT.h>
#include <Mod/Part/Gui/TaskAttacher.h>
#include <Mod/Part/Gui/AttacherTexts.h>
#include <Mod/Part/App/AttachExtension.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/SubShapeBinder.h>

#include "ui_TaskAttacher.h"
#include "TaskAttacher.h"

using namespace PartGui;
using namespace Gui;
using namespace Attacher;
namespace bp = boost::placeholders;

class Filter : public Gui::SelectionFilterGate
{
    std::set<App::DocumentObject *> inList;

public:
    Filter(const App::DocumentObject* support_)
        : Gui::SelectionFilterGate((Gui::SelectionFilter*)0)
    {
        if(support_) {
            inList = support_->getInListEx(true);
            inList.insert(const_cast<App::DocumentObject*>(support_));
        }
    }
    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName) override {
        (void)pDoc;
        (void)sSubName;
        if(!inList.count(pObj)) {
            return true;
        }
        else {
            this->notAllowedReason = QT_TR_NOOP("Selecting this will cause circular dependency.");
            return false;
        }
    }
};

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
    refnames = pcAttach->Support.getSubValues(false);

    for (size_t r = 0; r < 4; r++) {
        if ((r < refs.size()) && (refs[r] != NULL)) {
            refstrings.push_back(makeRefString(refs[r], refnames[r]));
            // for Origin or Datum features refnames is empty but we need a non-empty return value
            if (refnames[r].empty())
                refnames[r] = refs[r]->getNameInDocument();
        } else {
            refstrings.push_back(QObject::tr("No reference selected"));
            refnames.push_back("");
        }
    }
}

TaskAttacher::TaskAttacher(Gui::ViewProviderDocumentObject *ViewProvider, QWidget *parent,
                           QString picture, QString text, TaskAttacher::VisibilityFunction visFunc)
    : TaskBox(Gui::BitmapFactory().pixmap(picture.toLatin1()), text, true, parent)
    , SelectionObserver(ViewProvider)
    , ViewProvider(ViewProvider)
    , ui(new Ui_TaskAttacher)
    , visibilityFunc(visFunc)
{
    App::GetApplication().getActiveTransaction(&transactionID);

    //check if we are attachable
    if (!ViewProvider->getObject()->hasExtension(Part::AttachExtension::getExtensionClassTypeId()))
        throw Base::RuntimeError("Object has no Part::AttachExtension");

    editObjT = Gui::Selection().getExtendedContext(ViewProvider->getObject());

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    ui->lineRef1->installEventFilter(this);
    ui->lineRef1->setMouseTracking(true);
    ui->lineRef2->installEventFilter(this);
    ui->lineRef2->setMouseTracking(true);
    ui->lineRef3->installEventFilter(this);
    ui->lineRef3->setMouseTracking(true);
    ui->lineRef4->installEventFilter(this);
    ui->lineRef4->setMouseTracking(true);

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
    if (refnames.empty()) {
        auto group = App::GeoFeatureGroupExtension::getGroupOfObject(ViewProvider->getObject());
        if (group && group->hasExtension(App::OriginGroupExtension::getExtensionClassTypeId())) {
            auto originVp = Base::freecad_dynamic_cast<Gui::ViewProviderOrigin>(
                    Gui::Application::Instance->getViewProvider(
                        group->getExtensionByType<App::OriginGroupExtension>()->getOrigin()));
            if (originVp) {
                originFeat = originVp->getObject();
                originVp->setTemporaryVisibility(false, true);
            }
        }
    }
                
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

    // only activate the ref when there is no existing first attachment
    if (refnames[0].empty())
        this->iActiveRef = 0;
    else
        this->iActiveRef = -1;
    if (pcAttach->Support.getSize() == 0){
        autoNext = true;
    } else {
        autoNext = false;
    }

    ui->attachmentOffsetX->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Base.x")));
    ui->attachmentOffsetY->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Base.y")));
    ui->attachmentOffsetZ->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Base.z")));

    ui->attachmentOffsetYaw->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Rotation.Yaw")));
    ui->attachmentOffsetPitch->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Rotation.Pitch")));
    ui->attachmentOffsetRoll->bind(App::ObjectIdentifier::parse(ViewProvider->getObject(),std::string("AttachmentOffset.Rotation.Roll")));

    visibilityAutomation(true);
    updateAttachmentOffsetUI();
    updateReferencesUI();
    updateListOfModes();
    selectMapMode(eMapMode(pcAttach->MapMode.getValue()));
    updatePreview();

    Gui::Selection().addSelectionGate(new Filter(ViewProvider->getObject()));

    // connect object deletion with slot
    auto bnd1 = boost::bind(&TaskAttacher::objectDeleted, this, bp::_1);
    auto bnd2 = boost::bind(&TaskAttacher::documentDeleted, this, bp::_1);
    Gui::Document* document = Gui::Application::Instance->getDocument(ViewProvider->getObject()->getDocument());
    connectDelObject = document->signalDeletedObject.connect(bnd1);
    connectDelDocument = document->signalDeleteDocument.connect(bnd2);

    connectUndo = App::GetApplication().signalUndo.connect([this]() {refresh();});
    connectRedo = App::GetApplication().signalRedo.connect([this]() {refresh();});

    updateTimer.setSingleShot(true);
    QObject::connect(&updateTimer, &QTimer::timeout, [this]() {updatePreview();});
}

TaskAttacher::~TaskAttacher()
{
    Gui::Selection().rmvSelectionGate();
    try {
        visibilityAutomation(false);
    }
    catch (...) {
    }
    
    connectDelObject.disconnect();
    connectDelDocument.disconnect();
    connectUndo.disconnect();
    connectRedo.disconnect();

    detachSelection();

    auto originVp = Base::freecad_dynamic_cast<Gui::ViewProviderOrigin>(
            Gui::Application::Instance->getViewProvider(originFeat.getObject()));
    if (originVp)
        originVp->resetTemporaryVisibility();

    Gui::Selection().selStackPush();
    Gui::Selection().clearSelection();

    if (editOnClose) {
        try {
            if (editObjT.getSubObject()) {
                Gui::Selection().addSelection(editObjT);
                Gui::Selection().selStackPush();
                Gui::cmdGuiDocument(editObjT.getObject(),
                        std::ostringstream() << "setEdit("
                        << editObjT.getObjectPython() << ",0,u'"
                        << editObjT.getSubName() << "')");
            }
        } catch (Base::Exception &e) {
            e.ReportException();
        }
    }
}

void TaskAttacher::refresh()
{
    if (!ViewProvider)
        return;

    Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    updateAttachmentOffsetUI();
    updateReferencesUI();
    updateListOfModes();
    selectMapMode(eMapMode(pcAttach->MapMode.getValue()));
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

    setupTransaction();
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
    // Check if text is brighter than background, to improve legibility
    const bool dark_theme_found = (ui->buttonRef1->palette().color(QPalette::WindowText).value() > 128);

    if (errMessage.length()>0){
        ui->message->setText(tr("Attachment mode failed: %1").arg(errMessage));
        if (hasErrColor)
            ui->message->setStyleSheet(QStringLiteral("QLabel{color: %1;}").arg(errColor.name(QColor::HexRgb)));
        else
            ui->message->setStyleSheet(QString::fromLatin1(dark_theme_found ? "QLabel{color: indianred;}" : "QLabel{color: red;}"));
    } else {
        if (!attached){
            ui->message->setText(tr("Not attached"));
            ui->message->setStyleSheet(QString());
        } else {
            std::vector<QString> strs = AttacherGui::getUIStrings(pcAttach->attacher().getTypeId(),eMapMode(pcAttach->MapMode.getValue()));
            ui->message->setText(tr("Attached with mode %1").arg(strs[0]));
            if (hasMsgColor)
                ui->message->setStyleSheet(QStringLiteral("QLabel{color: %1;}").arg(msgColor.name(QColor::HexRgb)));
            else
                ui->message->setStyleSheet(QString::fromLatin1(dark_theme_found ? "QLabel{color: lightgreen;}" : "QLabel{color: green;}"));
        }
    }
    QString splmLabelText = attached ? tr("Attachment Offset (in local coordinates):") : tr("Attachment Offset (inactive - not attached):");
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

void TaskAttacher::setMessageColor(const QColor &color)
{
    msgColor = color;
    msgColorSet = true;
}

void TaskAttacher::setErrorColor(const QColor &color)
{
    errColor = color;
    errColorSet = true;
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
        std::vector<std::string> refnames = pcAttach->Support.getSubValues(false);

        App::SubObjectT sel;
        if (msg.pOriginalMsg)
            sel = msg.pOriginalMsg->Object;
        else
            sel = msg.Object;
        auto selObj = sel.getSubObject();
        if (!selObj)
            return;

        try {
            setupTransaction();

            // Remove subname for planes and datum features
            if (selObj->getLinkedObject(true)->isDerivedFrom(App::OriginFeature::getClassTypeId()))
                sel.setSubName(sel.getSubNameNoElement());

            if (editObjT.getSubObject())
                sel = Part::SubShapeBinder::import(sel, editObjT);
            selObj = sel.getSubObject();
            auto selElement = sel.getOldElementName();

            // eliminate duplicate selections
            for (size_t r = 0; r < refs.size(); r++) {
                if (selObj == refs[r]) {
                    if (refnames[r] == selElement)
                        return;
                    if (selElement.empty() || refnames[r].empty()) {
                        if (autoNext && iActiveRef > 0 && iActiveRef == (ssize_t) refnames.size()) {
                            --iActiveRef;
                            refs.pop_back();
                            refnames.pop_back();
                        }
                        break;
                    }
                }
            }
            if (iActiveRef < (ssize_t) refs.size()) {
                refs[iActiveRef] = selObj;
                refnames[iActiveRef] = selElement;
            } else {
                refs.push_back(selObj);
                refnames.push_back(selElement);
            }

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

            QLineEdit* line = getLine(iActiveRef);
            if (line != NULL) {
                line->blockSignals(true);
                line->setText(makeRefString(selObj, selElement));
                line->setProperty("RefName", QByteArray(selElement.c_str()));
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

        } catch(Base::Exception& e) {
            e.ReportException();
            //error = true;
            ui->message->setText(QString::fromLatin1(e.what()));
            ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: red;}"));
        }

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

    if (idx >= 3  &&  idx <= 5){
        double yaw, pitch, roll;
        yaw = ui->attachmentOffsetYaw->value().getValueAs(Base::Quantity::Degree);
        pitch = ui->attachmentOffsetPitch->value().getValueAs(Base::Quantity::Degree);
        roll = ui->attachmentOffsetRoll->value().getValueAs(Base::Quantity::Degree);
        Base::Rotation rot;
        rot.setYawPitchRoll(yaw,pitch,roll);
        pl.setRotation(rot);
    }

    try {
        setupTransaction();
        pcAttach->AttachmentOffset.setValue(pl);
        updatePreview();
    } catch(Base::Exception& e) {
        e.ReportException();
        ui->message->setText(QString::fromLatin1(e.what()));
        ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: red;}"));
    }
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

    try {
        setupTransaction();
        Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
        pcAttach->MapReversed.setValue(on);
        updatePreview();
    } catch(Base::Exception& e) {
        e.ReportException();
        ui->message->setText(QString::fromLatin1(e.what()));
        ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: red;}"));
    }
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

    try {
        setupTransaction();
        Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
        pcAttach->MapMode.setValue(getActiveMapMode());
        updatePreview();
    } catch (Base::Exception &e) {
        e.ReportException();
        ui->message->setText(QString::fromLatin1(e.what()));
        ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: red;}"));
    }
}

void TaskAttacher::onRefName(const QString& text, unsigned idx)
{
    if (!ViewProvider)
        return;

    QLineEdit* line = getLine(idx);
    if (line == NULL) return;

    try {
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

            setupTransaction();
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

        if (obj->getLinkedObject(true)->isDerivedFrom(App::Plane::getClassTypeId())) {
            // everything is OK (we assume a Part can only have exactly 3 App::Plane objects located at the base of the feature tree)
            subElement = "";
        } else if (obj->getLinkedObject(true)->isDerivedFrom(App::Line::getClassTypeId())) {
            // everything is OK (we assume a Part can only have exactly 3 App::Line objects located at the base of the feature tree)
            subElement = "";
        } else if (obj->getLinkedObject(true)->isDerivedFrom(Part::Datum::getClassTypeId())) {
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

        setupTransaction();

        if (editObjT.getSubObject()) {
            std::vector<App::DocumentObject*> objs;
            objs.push_back(obj);
            while (auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(objs.front()))
                objs.insert(objs.begin(), grp);
            App::SubObjectT objT(objs, subElement.c_str());
            objT = Part::SubShapeBinder::import(objT, editObjT);
            obj = objT.getObject();
            subElement = objT.getSubName();
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
        updatePreview();
    } catch (Base::Exception &e) {
        e.ReportException();
        ui->message->setText(QString::fromLatin1(e.what()));
        ui->message->setStyleSheet(QString::fromLatin1("QLabel{color: red;}"));
    }
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

    if (bRotationBound) {
        QString tooltip = tr("Not editable because rotation of AttachmentOffset is bound by expressions.");
        ui->attachmentOffsetYaw->setToolTip(tooltip);
        ui->attachmentOffsetPitch->setToolTip(tooltip);
        ui->attachmentOffsetRoll->setToolTip(tooltip);
    }

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
                tooltip += QString::fromLatin1("\n\n%1\n%2")
                        .arg(tr("Reference combinations:"),
                             AttacherGui::getRefListForMode(pcAttach->attacher(),mmode).join(QString::fromLatin1("\n")));
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
    else if (e->type() == QEvent::PaletteChange)
        updateTimer.start(100);
    else if (e->type() == QEvent::StyleChange) {
        hasMsgColor = msgColorSet;
        msgColorSet = false;
        hasErrColor = errColorSet;
        errColorSet = false;
    }
}

void TaskAttacher::visibilityAutomation(bool opening_not_closing)
{
    auto defvisfunc = [] (bool opening_not_closing,
                          const std::string &postfix,
                          Gui::ViewProviderDocumentObject* vp,
                          App::DocumentObject *editObj,
                          const std::string& editSubName) {
        if (opening_not_closing) {
            QString code = QString::fromLatin1(
                "import Show\n"
                "from Show.Containers import isAContainer\n"
                "_tv_%4 = Show.TempoVis(App.ActiveDocument, tag= 'PartGui::TaskAttacher')\n"
                "tvObj = %1\n"
                "dep_features = _tv_%4.get_all_dependent(%2, '%3')\n"
                "dep_features = [o for o in dep_features if not isAContainer(o)]\n"
                "if tvObj.isDerivedFrom('PartDesign::CoordinateSystem'):\n"
                "\tvisible_features = [feat for feat in tvObj.InList if feat.isDerivedFrom('PartDesign::FeaturePrimitive')]\n"
                "\tdep_features = [feat for feat in dep_features if feat not in visible_features]\n"
                "\tdel(visible_features)\n"
                "_tv_%4.hide(dep_features)\n"
                "_tv_%4.show(tvObj)\n"
                "del(dep_features)\n"
                "if not tvObj.isDerivedFrom('PartDesign::CoordinateSystem'):\n"
                "\t\tif len(tvObj.Support) > 0:\n"
                "\t\t\t_tv_%4.show([lnk[0] for lnk in tvObj.Support])\n"
                "del(tvObj)"
                ).arg(
                    QString::fromLatin1(Gui::Command::getObjectCmd(vp->getObject()).c_str()),
                    QString::fromLatin1(Gui::Command::getObjectCmd(editObj).c_str()),
                    QString::fromLatin1(editSubName.c_str()),
                    QString::fromLatin1(postfix.c_str()));
            Gui::Command::runCommand(Gui::Command::Gui,code.toLatin1().constData());
        } else if(postfix.size()) {
            QString code = QString::fromLatin1(
                "_tv_%1.restore()\n"
                "del(_tv_%1)"
                ).arg(QString::fromLatin1(postfix.c_str()));
            Gui::Command::runCommand(Gui::Command::Gui,code.toLatin1().constData());
        }
    };

    auto visAutoFunc = visibilityFunc ? visibilityFunc : defvisfunc;

    if (opening_not_closing) {
        //crash guards
        if (!ViewProvider)
            return;
        if (!ViewProvider->getObject())
            return;
        if (!ViewProvider->getObject()->getNameInDocument())
            return;

        auto editDoc = Gui::Application::Instance->editDocument();
        App::DocumentObject *editObj = ViewProvider->getObject();
        std::string editSubName;
        auto sels = Gui::Selection().getSelection(0,0,true);
        if(sels.size() && sels[0].pResolvedObject 
                       && sels[0].pResolvedObject->getLinkedObject()==editObj) 
        {
            editObj = sels[0].pObject;
            editSubName = sels[0].SubName;
        } else {
            ViewProviderDocumentObject *editVp = 0;
            if (editDoc) {
                editDoc->getInEdit(&editVp,&editSubName);
                if (editVp)
                    editObj = editVp->getObject();
            }
        }
        ObjectName = ViewProvider->getObject()->getNameInDocument();
        try {
            visAutoFunc(opening_not_closing, ObjectName, ViewProvider, editObj, editSubName);
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
            std::string objName;
            objName.swap(ObjectName);
            visAutoFunc(opening_not_closing, objName, nullptr, nullptr, std::string());
        }
        catch (Base::Exception &e) {
            e.ReportException();
        }
    }
}

void TaskAttacher::setupTransaction()
{
    if (!ViewProvider || !ViewProvider->getObject())
        return;

    auto obj = ViewProvider->getObject();
    if (!obj)
        return;

    int tid = 0;
    const char *name = App::GetApplication().getActiveTransaction(&tid);
    if(tid && tid == transactionID)
        return;

    std::string n(QT_TRANSLATE_NOOP("Command", "Attach"));
    n += " ";
    n += obj->Label.getValue();
    if(!name || n!=name)
        tid = App::GetApplication().setActiveTransaction(n.c_str());

    if(!transactionID)
        transactionID = tid;
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgAttacher::TaskDlgAttacher(Gui::ViewProviderDocumentObject *ViewProvider,
                                 bool createBox,
                                 const QString &picture,
                                 const QString &title)
    : TaskDialog(),ViewProvider(ViewProvider), parameter(nullptr)
{
    assert(ViewProvider);
    setDocumentName(ViewProvider->getDocument()->getDocument()->getName());

    if(createBox) {
        parameter  = new TaskAttacher(ViewProvider, nullptr, picture, title);
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

        parameter->setupTransaction();

        Part::AttachExtension* pcAttach = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
        auto obj = ViewProvider->getObject();

        //DeepSOIC: changed this to heavily rely on dialog constantly updating feature properties
        //if (pcAttach->AttachmentOffset.isTouched()){
            Base::Placement plm = pcAttach->AttachmentOffset.getValue();
            double yaw, pitch, roll;
            plm.getRotation().getYawPitchRoll(yaw,pitch,roll);
            Gui::cmdAppObjectArgs(obj, "AttachmentOffset = App.Placement(App.Vector(%.10f, %.10f, %.10f),  App.Rotation(%.10f, %.10f, %.10f))",
                                  plm.getPosition().x, plm.getPosition().y, plm.getPosition().z, yaw, pitch, roll);
        //}

        Gui::cmdAppObjectArgs(obj, "MapReversed = %s", pcAttach->MapReversed.getValue() ? "True" : "False");

        Gui::cmdAppObjectArgs(obj, "Support = %s", pcAttach->Support.getPyReprString().c_str());

        Gui::cmdAppObjectArgs(obj, "MapPathParameter = %f", pcAttach->MapPathParameter.getValue());

        Gui::cmdAppObjectArgs(obj, "MapMode = '%s'", AttachEngine::getModeName(eMapMode(pcAttach->MapMode.getValue())).c_str());
        Gui::Command::updateActive();

        Gui::cmdGuiDocument(obj, "resetEdit()");
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
        Gui::Command::doCommand(Gui::Command::Gui,"%s.resetEdit()", doc.getGuiDocumentPython().c_str());
    }

    // roll back the done things
    App::GetApplication().closeActiveTransaction(true);
    return true;
}

bool TaskAttacher::eventFilter(QObject *o, QEvent *ev)
{
    if (!ViewProvider)
        return false;
    switch(ev->type()) {
    case QEvent::Leave:
        Gui::Selection().rmvPreselect();
        break;
    case QEvent::Enter:
        if (auto edit = qobject_cast<QLineEdit*>(o)) {
            QStringList ref = edit->text().split(QLatin1String(":"));
            if (ref.size()) {
                auto obj = ViewProvider->getObject()->getDocument()->getObject(
                        ref[0].toLatin1().constData());
                if (obj) {
                    auto objT = editObjT.getParent();
                    if (auto group = App::GeoFeatureGroupExtension::getGroupOfObject(obj)) {
                        auto objs = objT.getSubObjectList();
                        int i = 0;
                        for (auto o : objs) {
                            ++i;
                            if (o == group)
                                break;
                        }
                        objs.resize(i);
                        objT = App::SubObjectT(objs);
                    }
                    objT = objT.getChild(obj);
                    if (ref.size() > 1)
                        objT.setSubName(objT.getSubName() + ref[1].toLatin1().constData());
                    Gui::Selection().setPreselect(
                            objT.getDocumentName().c_str(),
                            objT.getObjectName().c_str(),
                            objT.getSubName().c_str(),
                            0,0,0,2,true);
                }
            }
        }
        break;
    default:
        break;
    }
    return false;
}

#include "moc_TaskAttacher.cpp"
