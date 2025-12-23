// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <sstream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <Standard_Failure.hxx>


#include <App/Application.h>
#include <App/Document.h>
#include <App/ElementNamingUtils.h>
#include <App/ObjectIdentifier.h>
#include <App/Datums.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/DocumentObserver.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Base/Tools.h>
#include <Mod/Part/App/AttachExtension.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/Gui/AttacherTexts.h>
#include <Mod/Part/Gui/TaskAttacher.h>

#include "TaskAttacher.h"
#include "ViewProviderDatum.h"
#include "ViewProvider2DObject.h"

#include "ui_TaskAttacher.h"

#include <Gui/ViewParams.h>


using namespace PartGui;
using namespace Gui;
using namespace Attacher;
namespace sp = std::placeholders;

/* TRANSLATOR PartDesignGui::TaskAttacher */

// Create reference name from PropertyLinkSub values in a translatable fashion
const QString makeRefString(const App::DocumentObject* obj, const std::string& sub)
{
    if (!obj) {
        return QObject::tr("No reference selected");
    }

    if (obj->isDerivedFrom<App::DatumElement>() || obj->isDerivedFrom<Part::Datum>()) {
        return QString::fromLatin1(obj->getNameInDocument());
    }

    // Hide the TNP string from the user. ie show "Body.Pad.Face6"  and not :
    // "Body.Pad.;#a:1;:G0;XTR;:Hc94:8,F.Face6"
    App::ElementNamePair el;
    App::GeoFeature::resolveElement(obj, sub.c_str(), el, true);

    return QString::fromLatin1(obj->getNameInDocument())
        + (sub.length() > 0 ? QStringLiteral(":") : QString())
        + QString::fromLatin1(el.oldName.c_str());
}

void TaskAttacher::makeRefStrings(std::vector<QString>& refstrings, std::vector<std::string>& refnames)
{
    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    std::vector<App::DocumentObject*> refs = pcAttach->AttachmentSupport.getValues();
    refnames = pcAttach->AttachmentSupport.getSubValues();

    for (size_t r = 0; r < 4; r++) {
        if ((r < refs.size()) && (refs[r])) {
            refstrings.push_back(makeRefString(refs[r], refnames[r]));
            // for Origin or Datum features refnames is empty but we need a non-empty return value
            if (refnames[r].empty()) {
                refnames[r] = refs[r]->getNameInDocument();
            }
        }
        else {
            refstrings.push_back(QObject::tr("No reference selected"));
            refnames.emplace_back("");
        }
    }
}

TaskAttacher::TaskAttacher(
    Gui::ViewProviderDocumentObject* ViewProvider,
    QWidget* parent,
    QString picture,
    QString text,
    TaskAttacher::VisibilityFunction visFunc
)
    : TaskBox(Gui::BitmapFactory().pixmap(picture.toLatin1()), text, true, parent)
    , SelectionObserver(ViewProvider, true, Gui::ResolveMode::NoResolve)
    , ViewProvider(ViewProvider)
    , ui(new Ui_TaskAttacher)
    , visibilityFunc(visFunc)
    , completed(false)
{
    // check if we are attachable
    if (!ViewProvider->getObject()->hasExtension(Part::AttachExtension::getExtensionClassTypeId())) {
        throw Base::RuntimeError("Object has no Part::AttachExtension");
    }

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // clang-format off
    connect(ui->attachmentOffsetX, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this, &TaskAttacher::onAttachmentOffsetXChanged);
    connect(ui->attachmentOffsetY, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this, &TaskAttacher::onAttachmentOffsetYChanged);
    connect(ui->attachmentOffsetZ, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this, &TaskAttacher::onAttachmentOffsetZChanged);
    connect(ui->attachmentOffsetYaw, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this, &TaskAttacher::onAttachmentOffsetYawChanged);
    connect(ui->attachmentOffsetPitch, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this, &TaskAttacher::onAttachmentOffsetPitchChanged);
    connect(ui->attachmentOffsetRoll, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this, &TaskAttacher::onAttachmentOffsetRollChanged);
    connect(ui->checkBoxFlip, &QCheckBox::toggled,
        this, &TaskAttacher::onCheckFlip);
    connect(ui->buttonRef1, &QPushButton::clicked,
        this, &TaskAttacher::onButtonRef1);
    connect(ui->lineRef1, &QLineEdit::textEdited,
        this, &TaskAttacher::onRefName1);
    connect(ui->buttonRef2, &QPushButton::clicked,
        this, &TaskAttacher::onButtonRef2);
    connect(ui->lineRef2, &QLineEdit::textEdited,
        this, &TaskAttacher::onRefName2);
    connect(ui->buttonRef3, &QPushButton::clicked,
        this, &TaskAttacher::onButtonRef3);
    connect(ui->lineRef3, &QLineEdit::textEdited,
        this, &TaskAttacher::onRefName3);
    connect(ui->buttonRef4, &QPushButton::clicked,
        this, &TaskAttacher::onButtonRef4);
    connect(ui->lineRef4, &QLineEdit::textEdited,
        this, &TaskAttacher::onRefName4);
    connect(ui->listOfModes, &QListWidget::itemSelectionChanged,
        this, &TaskAttacher::onModeSelect);
    // clang-format on

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
    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    std::vector<std::string> refnames = pcAttach->AttachmentSupport.getSubValues();

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
    if (refnames[0].empty()) {
        this->iActiveRef = 0;
    }
    else {
        this->iActiveRef = -1;
    }
    if (pcAttach->AttachmentSupport.getSize() == 0) {
        autoNext = true;
    }
    else {
        autoNext = false;
    }

    ui->attachmentOffsetX->bind(
        App::ObjectIdentifier::parse(ViewProvider->getObject(), std::string("AttachmentOffset.Base.x"))
    );
    ui->attachmentOffsetY->bind(
        App::ObjectIdentifier::parse(ViewProvider->getObject(), std::string("AttachmentOffset.Base.y"))
    );
    ui->attachmentOffsetZ->bind(
        App::ObjectIdentifier::parse(ViewProvider->getObject(), std::string("AttachmentOffset.Base.z"))
    );

    ui->attachmentOffsetYaw->bind(
        App::ObjectIdentifier::parse(
            ViewProvider->getObject(),
            std::string("AttachmentOffset.Rotation.Yaw")
        )
    );
    ui->attachmentOffsetPitch->bind(
        App::ObjectIdentifier::parse(
            ViewProvider->getObject(),
            std::string("AttachmentOffset.Rotation.Pitch")
        )
    );
    ui->attachmentOffsetRoll->bind(
        App::ObjectIdentifier::parse(
            ViewProvider->getObject(),
            std::string("AttachmentOffset.Rotation.Roll")
        )
    );


    auto document = ViewProvider->getObject()->getDocument();
    for (auto planeDocumentObject : document->getObjectsOfType(App::Plane::getClassTypeId())) {
        auto planeViewProvider = Application::Instance->getViewProvider<Gui::ViewProviderPlane>(
            planeDocumentObject
        );

        if (!planeViewProvider) {
            continue;
        }

        modifiedPlaneViewProviders.emplace_back(planeViewProvider);

        planeViewProvider->setTemporaryScale(ViewParams::instance()->getDatumTemporaryScaleFactor());
        planeViewProvider->setLabelVisibility(true);
    };

    visibilityAutomation(true);
    updateAttachmentOffsetUI();
    updateReferencesUI();
    updateListOfModes();
    selectMapMode(eMapMode(pcAttach->MapMode.getValue()));
    updatePreview();
    showPlacementUtilities();

    // NOLINTBEGIN
    //  connect object deletion with slot
    auto bnd1 = std::bind(&TaskAttacher::objectDeleted, this, sp::_1);
    auto bnd2 = std::bind(&TaskAttacher::documentDeleted, this, sp::_1);
    // NOLINTEND
    Gui::Document* guiDocument = Gui::Application::Instance->getDocument(
        ViewProvider->getObject()->getDocument()
    );
    connectDelObject = guiDocument->signalDeletedObject.connect(bnd1);
    connectDelDocument = guiDocument->signalDeleteDocument.connect(bnd2);

    handleInitialSelection();
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

    for (auto& vp : modifiedPlaneViewProviders) {
        if (vp.expired()) {
            continue;
        }

        auto planeViewProvider = vp.get<Gui::ViewProviderPlane>();
        if (!planeViewProvider) {
            return;
        }

        planeViewProvider->resetTemporarySize();
        planeViewProvider->setLabelVisibility(false);
    }
}

void TaskAttacher::objectDeleted(const Gui::ViewProviderDocumentObject& view)
{
    if (ViewProvider == &view) {
        ViewProvider = nullptr;
        // if the object gets deleted we need to clear all overrides so it does not segfault
        overrides.clear();
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
    for (auto t : hint) {
        QString tText;
        tText = AttacherGui::getShapeTypeText(t);
        result += QString::fromLatin1(result.size() == 0 ? "" : "/") + tText;
    }

    return result;
}

void TaskAttacher::updateReferencesUI()
{
    if (!ViewProvider) {
        return;
    }

    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();

    std::vector<App::DocumentObject*> refs = pcAttach->AttachmentSupport.getValues();
    completed = false;

    // Get hints for further required references...
    // DeepSOIC: hint system became useless since inertial system attachment
    // modes have been introduced, because they accept any number of references
    // of any type, so the hint will always be 'Any'. I keep the logic
    // nevertheless, in case it is decided to resurrect hint system.

    pcAttach->attacher().suggestMapModes(this->lastSuggestResult);

    if (this->lastSuggestResult.message != SuggestResult::srOK) {
        if (!this->lastSuggestResult.nextRefTypeHint.empty()) {
            // message = "Need more references";
        }
    }
    else {
        completed = true;
    }

    updateRefButton(0);
    updateRefButton(1);
    updateRefButton(2);
    updateRefButton(3);
}

bool TaskAttacher::updatePreview()
{
    if (!ViewProvider) {
        return false;
    }

    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    QString errMessage;
    bool attached = false;
    try {
        attached = pcAttach->positionBySupport();
    }
    catch (Base::Exception& err) {
        errMessage = QCoreApplication::translate("Exception", err.what());
    }
    catch (Standard_Failure& err) {
        errMessage = tr("OCC error: %1").arg(QString::fromLatin1(err.GetMessageString()));
    }
    catch (...) {
        errMessage = tr("unknown error");
    }
    if (errMessage.length() > 0) {
        ui->message->setText(tr("Attachment mode failed: %1").arg(errMessage));
        ui->message->setStyleSheet(QStringLiteral("QLabel{color: red;}"));
    }
    else {
        if (!attached) {
            ui->message->setText(tr("Not attached"));
            ui->message->setStyleSheet(QString());
        }
        else {
            std::vector<QString> strs = AttacherGui::getUIStrings(
                pcAttach->attacher().getTypeId(),
                eMapMode(pcAttach->MapMode.getValue())
            );
            ui->message->setText(tr("Attached with mode %1").arg(strs[0]));
            ui->message->setStyleSheet(QStringLiteral("QLabel{color: green;}"));
        }
    }
    QString splmLabelText = attached ? tr("Attachment offset (in its local coordinate system):")
                                     : tr("Attachment offset (inactive - not attached):");
    ui->groupBox_AttachmentOffset->setTitle(splmLabelText);
    ui->groupBox_AttachmentOffset->setEnabled(attached);

    return attached;
}

QLineEdit* TaskAttacher::getLine(unsigned idx)
{
    switch (idx) {
        case 0:
            return ui->lineRef1;
        case 1:
            return ui->lineRef2;
        case 2:
            return ui->lineRef3;
        case 3:
            return ui->lineRef4;
        default:
            return nullptr;
    }
}

void TaskAttacher::findCorrectObjAndSubInThisContext(App::DocumentObject*& rootObj, std::string& sub)
{
    // The reference that we store must take into account the hierarchy of geoFeatures. For example:
    // - Part
    // - - Cube
    // - Sketch
    // if sketch is attached to Cube.Face1 then it must store Part:Cube.Face3 as Sketch is outside
    // of Part.
    // - Part
    // - - Cube
    // - - Sketch
    // In this example it must store Cube:Face3 because Sketch is inside Part, sibling of Cube.
    // So placement of Part is already taken into account.
    // - Part1
    // - - Part2
    // - - - Cube
    // - - Sketch
    // In this example it must store Part2:Cube.Face3 since Part1 is already taken into account.
    // - Part1
    // - - Part2
    // - - - Cube
    // - - Part3
    // - - - Sketch
    // In this example it's not possible because Sketch has Part3 placement. So it should be
    // rejected So we need to take the selection object and subname, and process them to get the
    // correct obj/sub based on attached and attaching objects positions.

    std::vector<std::string> names = Base::Tools::splitSubName(sub);
    if (!rootObj || names.size() < 2) {
        return;
    }
    names.insert(names.begin(), rootObj->getNameInDocument());

    App::Document* doc = rootObj->getDocument();
    App::DocumentObject* attachingObj = ViewProvider->getObject();     // Attaching object
    App::DocumentObject* subObj = rootObj->getSubObject(sub.c_str());  // Object being attached.
    if (!subObj || subObj == rootObj) {
        // Case of root object. We don't need to modify it.
        return;
    }
    if (subObj == attachingObj) {
        // prevent self-referencing
        rootObj = nullptr;
        return;
    }

    // Check if attachingObj is a root object. if so we keep the full path.
    auto* group = App::GeoFeatureGroupExtension::getGroupOfObject(attachingObj);
    if (!group) {
        if (attachingObj->getDocument() != rootObj->getDocument()) {
            // If it's not in same document then it's not a good selection
            rootObj = nullptr;
        }
        // if it's same document we keep the rootObj and sub unchanged.
        return;
    }

    bool groupPassed = false;
    for (size_t i = 0; i < names.size(); ++i) {
        App::DocumentObject* obj = doc->getObject(names[i].c_str());
        if (!obj) {
            Base::Console().translatedUserError(
                "TaskAttacher",
                "Unsuitable selection: '%s' cannot be attached to '%s' from within it's group "
                "'%s'.\n",
                attachingObj->getFullLabel(),
                subObj->getFullLabel(),
                group->getFullLabel()
            );
            rootObj = nullptr;
            return;
        }

        if (groupPassed) {
            rootObj = obj;

            // Rebuild 'sub' starting from the next element after the current 'name'
            sub = "";
            for (size_t j = i + 1; j < names.size(); ++j) {
                sub += names[j];
                if (j != names.size() - 1) {
                    sub += ".";  // Add a period between elements
                }
            }
            return;
        }

        // In case the attaching object is in a link to a part.
        // For instance :
        // - Part1
        // - - LinkToPart2
        // - - - Cube
        // - - - Sketch
        obj = obj->getLinkedObject();

        if (obj == group) {
            groupPassed = true;
        }
    }

    // if we reach this point it means that attaching object's group is outside of
    // the scope of the attached object. For instance:
    // - Part1
    // - - Part2
    // - - - Cube
    // - - Part3
    // - - - Sketch
    // In this case the selection is not acceptable.
    rootObj = nullptr;
}

void TaskAttacher::handleInitialSelection()
{
    // We handle initial selection only if it is not attached yet.
    App::DocumentObject* obj = ViewProvider->getObject();
    Part::AttachExtension* pcAttach = obj->getExtensionByType<Part::AttachExtension>();
    std::vector<App::DocumentObject*> refs = pcAttach->AttachmentSupport.getValues();

    if (!refs.empty()) {
        return;
    }
    std::vector<SubAndObjName> subAndObjNamePairs;

    auto sel = Gui::Selection().getSelectionEx(
        "",
        App::DocumentObject::getClassTypeId(),
        Gui::ResolveMode::NoResolve
    );
    for (auto& selObj : sel) {
        std::vector<std::string> subs = selObj.getSubNames();
        std::string objName = selObj.getFeatName();
        for (auto& sub : subs) {
            SubAndObjName objSubName = {objName, sub};
            subAndObjNamePairs.push_back(objSubName);
        }
    }
    addToReference(subAndObjNamePairs);
}

void TaskAttacher::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (!ViewProvider) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        SubAndObjName pair = {msg.pObjectName, msg.pSubName};
        addToReference(pair);
    }
}

void TaskAttacher::addToReference(const std::vector<SubAndObjName>& pairs)
{
    if (iActiveRef < 0) {
        return;
    }

    // Note: The validity checking has already been done in ReferenceSelection.cpp
    App::DocumentObject* obj = ViewProvider->getObject();
    Part::AttachExtension* pcAttach = obj->getExtensionByType<Part::AttachExtension>();

    for (auto& pair : pairs) {
        std::vector<App::DocumentObject*> refs = pcAttach->AttachmentSupport.getValues();
        std::vector<std::string> refnames = pcAttach->AttachmentSupport.getSubValues();

        App::DocumentObject* selObj = obj->getDocument()->getObject(pair.objName.c_str());
        std::string subname = pair.subName;
        findCorrectObjAndSubInThisContext(selObj, subname);
        if (!selObj) {
            return;
        }

        // Remove subname for planes and datum features
        if (selObj->isDerivedFrom<App::DatumElement>() || selObj->isDerivedFrom<Part::Datum>()) {
            subname = "";
        }

        // eliminate duplicate selections
        for (size_t r = 0; r < refs.size(); r++) {
            if ((refs[r] == selObj) && (refnames[r] == subname)) {
                return;
            }
        }

        if (autoNext && iActiveRef > 0 && iActiveRef == static_cast<int>(refnames.size())) {
            if (refs[iActiveRef - 1] == selObj && refnames[iActiveRef - 1].length() != 0
                && subname.length() == 0) {
                // A whole object was selected by clicking it twice. Fill it
                // into previous reference, where a sub-named reference filled by
                // the first click is already stored.

                iActiveRef--;
            }
        }
        if (iActiveRef < static_cast<int>(refs.size())) {
            refs[iActiveRef] = selObj;
            refnames[iActiveRef] = subname;
        }
        else {
            refs.push_back(selObj);
            refnames.push_back(subname);
        }

        pcAttach->AttachmentSupport.setValues(refs, refnames);

        QLineEdit* line = getLine(iActiveRef);
        if (line) {
            line->blockSignals(true);
            line->setText(makeRefString(selObj, subname));
            line->setProperty("RefName", QByteArray(subname.c_str()));
            line->blockSignals(false);
        }

        if (autoNext) {
            if (iActiveRef == -1) {
                // nothing to do
            }
            else if (iActiveRef == 4 || this->lastSuggestResult.nextRefTypeHint.empty()) {
                iActiveRef = -1;
            }
            else {
                iActiveRef++;
            }
        }
    }

    try {
        updateListOfModes();
        eMapMode mmode = getActiveMapMode();  // will be mmDeactivated, if selected or if no modes
                                              // are available
        if (mmode == mmDeactivated) {
            // error = true;
            this->completed = false;
        }
        else {
            this->completed = true;
        }
        pcAttach->MapMode.setValue(mmode);
        selectMapMode(mmode);
        updatePreview();
    }
    catch (Base::Exception& e) {
        ui->message->setText(QCoreApplication::translate("Exception", e.what()));
        ui->message->setStyleSheet(QStringLiteral("QLabel{color: red;}"));
    }

    updateReferencesUI();
}

void TaskAttacher::addToReference(SubAndObjName pair)
{
    addToReference({{{pair}}});
}

void TaskAttacher::onAttachmentOffsetChanged(double /*val*/, int idx)
{
    if (!ViewProvider) {
        return;
    }

    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
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
    if (idx >= 0 && idx <= 2) {
        pl.setPosition(pos);
    }

    if (idx >= 3 && idx <= 5) {
        double yaw, pitch, roll;
        yaw = ui->attachmentOffsetYaw->value().getValueAs(Base::Quantity::Degree);
        pitch = ui->attachmentOffsetPitch->value().getValueAs(Base::Quantity::Degree);
        roll = ui->attachmentOffsetRoll->value().getValueAs(Base::Quantity::Degree);
        Base::Rotation rot;
        rot.setYawPitchRoll(yaw, pitch, roll);
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
    if (!ViewProvider) {
        return;
    }

    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    pcAttach->MapReversed.setValue(on);
    ViewProvider->getObject()->recomputeFeature();
}

void TaskAttacher::onButtonRef(const bool checked, unsigned idx)
{
    autoNext = false;
    if (checked) {
        Gui::Selection().clearSelection();
        iActiveRef = idx;
    }
    else {
        iActiveRef = -1;
    }
    updateRefButton(0);
    updateRefButton(1);
    updateRefButton(2);
    updateRefButton(3);
}

void TaskAttacher::onButtonRef1(const bool checked)
{
    onButtonRef(checked, 0);
}

void TaskAttacher::onButtonRef2(const bool checked)
{
    onButtonRef(checked, 1);
}

void TaskAttacher::onButtonRef3(const bool checked)
{
    onButtonRef(checked, 2);
}

void TaskAttacher::onButtonRef4(const bool checked)
{
    onButtonRef(checked, 3);
}

void TaskAttacher::onModeSelect()
{
    if (!ViewProvider) {
        return;
    }

    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    pcAttach->MapMode.setValue(getActiveMapMode());
    updatePreview();
}

void TaskAttacher::onRefName(const QString& text, unsigned idx)
{
    if (!ViewProvider) {
        return;
    }

    QLineEdit* line = getLine(idx);
    if (!line) {
        return;
    }

    if (text.length() == 0) {
        // Reference was removed
        // Update the reference list
        Part::AttachExtension* pcAttach
            = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
        std::vector<App::DocumentObject*> refs = pcAttach->AttachmentSupport.getValues();
        std::vector<std::string> refnames = pcAttach->AttachmentSupport.getSubValues();
        std::vector<App::DocumentObject*> newrefs;
        std::vector<std::string> newrefnames;
        for (size_t r = 0; r < refs.size(); r++) {
            if (r != idx) {
                newrefs.push_back(refs[r]);
                newrefnames.push_back(refnames[r]);
            }
        }
        pcAttach->AttachmentSupport.setValues(newrefs, newrefnames);
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
    if (parts.length() < 2) {
        parts.push_back(QStringLiteral(""));
    }
    // Check whether this is the name of an App::Plane or Part::Datum feature
    App::DocumentObject* obj = ViewProvider->getObject()->getDocument()->getObject(parts[0].toLatin1());
    if (!obj) {
        return;
    }

    std::string subElement;

    if (obj->isDerivedFrom<App::Plane>()) {
        // everything is OK (we assume a Part can only have exactly 3 App::Plane objects located at
        // the base of the feature tree)
        subElement.clear();
    }
    else if (obj->isDerivedFrom<App::Line>()) {
        // everything is OK (we assume a Part can only have exactly 3 App::Line objects located at
        // the base of the feature tree)
        subElement.clear();
    }
    else if (obj->isDerivedFrom<Part::Datum>()) {
        subElement.clear();
    }
    else {
        // TODO: check validity of the text that was entered: Does subElement actually reference to
        // an element on the obj?

        auto getSubshapeName = [](const QString& part) -> std::string {
            // We must expect that "text" is the translation of "Face", "Edge" or "Vertex" followed
            // by an ID.
            QRegularExpression rx;
            QRegularExpressionMatch match;
            std::stringstream ss;

            rx.setPattern(QStringLiteral("^") + tr("Face") + QStringLiteral("(\\d+)$"));
            if (part.indexOf(rx, 0, &match) >= 0) {
                int faceId = match.captured(1).toInt();
                ss << "Face" << faceId;
                return ss.str();
            }

            rx.setPattern(QStringLiteral("^") + tr("Edge") + QStringLiteral("(\\d+)$"));
            if (part.indexOf(rx, 0, &match) >= 0) {
                int lineId = match.captured(1).toInt();
                ss << "Edge" << lineId;
                return ss.str();
            }

            rx.setPattern(QStringLiteral("^") + tr("Vertex") + QStringLiteral("(\\d+)$"));
            if (part.indexOf(rx, 0, &match) >= 0) {
                int vertexId = match.captured(1).toInt();
                ss << "Vertex" << vertexId;
                return ss.str();
            }

            // none of Edge/Vertex/Face. May be empty string.
            // Feed in whatever user supplied, even if invalid.
            ss << part.toLatin1().constData();
            return ss.str();
        };

        auto name = getSubshapeName(parts[1]);

        line->setProperty("RefName", QByteArray(name.c_str()));
        subElement = name;
    }

    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    std::vector<App::DocumentObject*> refs = pcAttach->AttachmentSupport.getValues();
    std::vector<std::string> refnames = pcAttach->AttachmentSupport.getSubValues();
    if (idx < refs.size()) {
        refs[idx] = obj;
        refnames[idx] = subElement;
    }
    else {
        refs.push_back(obj);
        refnames.emplace_back(subElement);
    }
    pcAttach->AttachmentSupport.setValues(refs, refnames);
    updateListOfModes();
    pcAttach->MapMode.setValue(getActiveMapMode());
    selectMapMode(getActiveMapMode());

    updateReferencesUI();
}

void TaskAttacher::updateRefButton(int idx)
{
    if (!ViewProvider) {
        return;
    }

    QAbstractButton* b;
    switch (idx) {
        case 0:
            b = ui->buttonRef1;
            break;
        case 1:
            b = ui->buttonRef2;
            break;
        case 2:
            b = ui->buttonRef3;
            break;
        case 3:
            b = ui->buttonRef4;
            break;
        default:
            throw Base::IndexError("button index out of range");
    }

    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    std::vector<App::DocumentObject*> refs = pcAttach->AttachmentSupport.getValues();

    int numrefs = refs.size();
    bool enable = true;
    if (idx > numrefs) {
        enable = false;
    }
    if (idx == numrefs && this->lastSuggestResult.nextRefTypeHint.empty()) {
        enable = false;
    }
    b->setEnabled(enable);

    b->setChecked(iActiveRef == idx);

    if (iActiveRef == idx) {
        b->setText(tr("Selecting…"));
    }
    else if (idx < static_cast<int>(this->lastSuggestResult.references_Types.size())) {
        b->setText(AttacherGui::getShapeTypeText(this->lastSuggestResult.references_Types[idx]));
    }
    else {
        b->setText(tr("Reference%1").arg(idx + 1));
    }
}

void TaskAttacher::updateAttachmentOffsetUI()
{
    if (!ViewProvider) {
        return;
    }

    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
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

    ui->attachmentOffsetX->setValue(Base::Quantity(pos.x, Base::Unit::Length));
    ui->attachmentOffsetY->setValue(Base::Quantity(pos.y, Base::Unit::Length));
    ui->attachmentOffsetZ->setValue(Base::Quantity(pos.z, Base::Unit::Length));
    ui->attachmentOffsetYaw->setValue(yaw);
    ui->attachmentOffsetPitch->setValue(pitch);
    ui->attachmentOffsetRoll->setValue(roll);

    auto expressions = ViewProvider->getObject()->ExpressionEngine.getExpressions();
    bool bRotationBound = false;
    bRotationBound = bRotationBound
        || expressions.find(
               App::ObjectIdentifier::parse(
                   ViewProvider->getObject(),
                   std::string("AttachmentOffset.Rotation.Angle")
               )
           ) != expressions.end();
    bRotationBound = bRotationBound
        || expressions.find(
               App::ObjectIdentifier::parse(
                   ViewProvider->getObject(),
                   std::string("AttachmentOffset.Rotation.Axis.x")
               )
           ) != expressions.end();
    bRotationBound = bRotationBound
        || expressions.find(
               App::ObjectIdentifier::parse(
                   ViewProvider->getObject(),
                   std::string("AttachmentOffset.Rotation.Axis.y")
               )
           ) != expressions.end();
    bRotationBound = bRotationBound
        || expressions.find(
               App::ObjectIdentifier::parse(
                   ViewProvider->getObject(),
                   std::string("AttachmentOffset.Rotation.Axis.z")
               )
           ) != expressions.end();

    ui->attachmentOffsetYaw->setEnabled(!bRotationBound);
    ui->attachmentOffsetPitch->setEnabled(!bRotationBound);
    ui->attachmentOffsetRoll->setEnabled(!bRotationBound);

    if (bRotationBound) {
        QString tooltip = tr(
            "Not editable because rotation of AttachmentOffset is bound by expressions."
        );
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
    if (!ViewProvider) {
        return;
    }

    // first up, remember currently selected mode.
    eMapMode curMode = mmDeactivated;
    auto sel = ui->listOfModes->selectedItems();
    if (!sel.isEmpty()) {
        curMode = modesInList[ui->listOfModes->row(sel[0])];
    }

    // obtain list of available modes:
    Part::AttachExtension* pcAttach
        = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
    this->lastSuggestResult.bestFitMode = mmDeactivated;
    size_t lastValidModeItemIndex = mmDummy_NumberOfModes;

    if (pcAttach->AttachmentSupport.getSize() > 0) {
        pcAttach->attacher().suggestMapModes(this->lastSuggestResult);
        modesInList = this->lastSuggestResult.allApplicableModes;
        modesInList.insert(modesInList.begin(), mmDeactivated);  // always have the option to choose
                                                                 // Deactivated mode

        // add reachable modes to the list, too, but gray them out (using lastValidModeItemIndex, later)
        lastValidModeItemIndex = modesInList.size() - 1;
        for (std::pair<const eMapMode, refTypeStringList>& rm :
             this->lastSuggestResult.reachableModes) {
            modesInList.push_back(rm.first);
        }
    }
    else {
        // no references - display all modes
        modesInList.clear();
        modesInList.push_back(mmDeactivated);

        for (int mmode = 0; mmode < mmDummy_NumberOfModes; mmode++) {
            if (pcAttach->attacher().modeEnabled[mmode]) {
                modesInList.push_back(eMapMode(mmode));
            }
        }
    }

    // populate list
    ui->listOfModes->blockSignals(true);
    ui->listOfModes->clear();
    QListWidgetItem* iSelect = nullptr;
    if (!modesInList.empty()) {
        for (size_t i = 0; i < modesInList.size(); ++i) {
            eMapMode mmode = modesInList[i];
            std::vector<QString> mstr
                = AttacherGui::getUIStrings(pcAttach->attacher().getTypeId(), mmode);
            ui->listOfModes->addItem(mstr[0]);
            QListWidgetItem* item = ui->listOfModes->item(i);
            QString tooltip = mstr[1];

            if (mmode != mmDeactivated) {
                tooltip += QStringLiteral("\n\n%1\n%2")
                               .arg(
                                   tr("Reference combinations:"),
                                   AttacherGui::getRefListForMode(pcAttach->attacher(), mmode)
                                       .join(QStringLiteral("\n"))
                               );
            }
            item->setToolTip(tooltip);

            if (mmode == curMode && curMode != mmDeactivated) {
                iSelect = ui->listOfModes->item(i);
            }
            if (i > lastValidModeItemIndex) {
                // potential mode - can be reached by selecting more stuff
                item->setFlags(
                    item->flags() & ~(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable)
                );

                refTypeStringList& extraRefs = this->lastSuggestResult.reachableModes[mmode];
                if (extraRefs.size() == 1) {
                    QStringList buf;
                    for (eRefType rt : extraRefs[0]) {
                        buf.append(AttacherGui::getShapeTypeText(rt));
                    }
                    item->setText(tr("%1 (add %2)").arg(item->text(), buf.join(QStringLiteral("+"))));
                }
                else {
                    item->setText(tr("%1 (add more references)").arg(item->text()));
                }
            }
            else if (mmode == this->lastSuggestResult.bestFitMode) {
                // suggested mode - make bold
                QFont fnt = item->font();
                fnt.setBold(true);
                item->setFont(fnt);
            }
        }
    }

    // restore selection
    if (iSelect) {
        iSelect->setSelected(true);
    }

    ui->listOfModes->blockSignals(false);
}

void TaskAttacher::selectMapMode(eMapMode mmode)
{
    ui->listOfModes->blockSignals(true);

    for (size_t i = 0; i < modesInList.size(); ++i) {
        if (modesInList[i] == mmode) {
            ui->listOfModes->item(i)->setSelected(true);
        }
    }

    ui->listOfModes->blockSignals(false);
}

void TaskAttacher::showPlacementUtilities()
{
    if (auto planarViewProvider = freecad_cast<PartGui::ViewProvider2DObject*>(ViewProvider)) {
        overrides.override(planarViewProvider->ShowPlane, true);
    }

    if (auto partViewProvider = freecad_cast<PartGui::ViewProviderPartExt*>(ViewProvider)) {
        overrides.override(partViewProvider->ShowPlacement, true);
    }
}

Attacher::eMapMode TaskAttacher::getActiveMapMode()
{
    auto sel = ui->listOfModes->selectedItems();
    if (!sel.isEmpty()) {
        return modesInList[ui->listOfModes->row(sel[0])];
    }
    else {
        if (this->lastSuggestResult.message == SuggestResult::srOK) {
            return this->lastSuggestResult.bestFitMode;
        }
        else {
            return mmDeactivated;
        }
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

void TaskAttacher::onRefName4(const QString& text)
{
    onRefName(text, 3);
}

bool TaskAttacher::getFlip() const
{
    return ui->checkBoxFlip->isChecked();
}

void TaskAttacher::changeEvent(QEvent* e)
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
    auto defvisfunc = [](bool opening_not_closing,
                         const std::string& postfix,
                         Gui::ViewProviderDocumentObject* vp,
                         App::DocumentObject* editObj,
                         const std::string& editSubName) {
        if (opening_not_closing) {
            QString code
                = QStringLiteral(
                      "import Show\n"
                      "from Show.Containers import isAContainer\n"
                      "_tv_%4 = Show.TempoVis(App.ActiveDocument, tag= 'PartGui::TaskAttacher')\n"
                      "tvObj = %1\n"
                      "dep_features = _tv_%4.get_all_dependent(%2, '%3')\n"
                      "dep_features = [o for o in dep_features if not isAContainer(o)]\n"
                      "if tvObj.isDerivedFrom('PartDesign::CoordinateSystem'):\n"
                      "\tvisible_features = [feat for feat in tvObj.InList if "
                      "feat.isDerivedFrom('PartDesign::FeaturePrimitive')]\n"
                      "\tdep_features = [feat for feat in dep_features if feat not in "
                      "visible_features]\n"
                      "\tdel(visible_features)\n"
                      "_tv_%4.hide(dep_features)\n"
                      "del(dep_features)\n"
                      "if not tvObj.isDerivedFrom('PartDesign::CoordinateSystem'):\n"
                      "\t\tif len(tvObj.AttachmentSupport) > 0:\n"
                      "\t\t\t_tv_%4.show([lnk[0] for lnk in tvObj.AttachmentSupport])\n"
                      "del(tvObj)"
                )
                      .arg(
                          QString::fromLatin1(Gui::Command::getObjectCmd(vp->getObject()).c_str()),
                          QString::fromLatin1(Gui::Command::getObjectCmd(editObj).c_str()),
                          QString::fromLatin1(editSubName.c_str()),
                          QString::fromLatin1(postfix.c_str())
                      );
            Gui::Command::runCommand(Gui::Command::Gui, code.toLatin1().constData());
        }
        else if (!postfix.empty()) {
            QString code = QStringLiteral(
                               "_tv_%1.restore()\n"
                               "del(_tv_%1)"
            )
                               .arg(QString::fromLatin1(postfix.c_str()));
            Gui::Command::runCommand(Gui::Command::Gui, code.toLatin1().constData());
        }
    };

    auto visAutoFunc = visibilityFunc ? visibilityFunc : defvisfunc;

    if (opening_not_closing) {
        // crash guards
        if (!ViewProvider) {
            return;
        }
        if (!ViewProvider->getObject()) {
            return;
        }
        if (!ViewProvider->getObject()->isAttachedToDocument()) {
            return;
        }

        auto editDoc = Gui::Application::Instance->editDocument();
        App::DocumentObject* editObj = ViewProvider->getObject();
        std::string editSubName;
        auto sels = Gui::Selection().getSelection(nullptr, Gui::ResolveMode::NoResolve, true);
        if (!sels.empty() && sels[0].pResolvedObject
            && sels[0].pResolvedObject->getLinkedObject() == editObj) {
            editObj = sels[0].pObject;
            editSubName = sels[0].SubName;
        }
        else {
            ViewProviderDocumentObject* editVp = nullptr;
            if (editDoc) {
                editDoc->getInEdit(&editVp, &editSubName);
                if (editVp) {
                    editObj = editVp->getObject();
                }
            }
        }
        ObjectName = ViewProvider->getObject()->getNameInDocument();
        try {
            visAutoFunc(opening_not_closing, ObjectName, ViewProvider, editObj, editSubName);
        }
        catch (const Base::Exception& e) {
            e.reportException();
        }
        catch (const Py::Exception&) {
            Base::PyException e;
            e.reportException();
        }
    }
    else {
        try {
            std::string objName;
            objName.swap(ObjectName);
            visAutoFunc(opening_not_closing, objName, nullptr, nullptr, std::string());
        }
        catch (Base::Exception& e) {
            e.reportException();
        }
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgAttacher::TaskDlgAttacher(
    Gui::ViewProviderDocumentObject* ViewProvider,
    bool createBox,
    std::function<void()> onAccept,
    std::function<void()> onReject
)
    : TaskDialog()
    , ViewProvider(ViewProvider)
    , parameter(nullptr)
    , onAccept(onAccept)
    , onReject(onReject)
    , accepted(false)
{
    assert(ViewProvider);
    setDocumentName(ViewProvider->getDocument()->getDocument()->getName());

    if (createBox) {
        parameter = new TaskAttacher(ViewProvider, nullptr, QString(), tr("Attachment"));
        Content.push_back(parameter);
    }
}

TaskDlgAttacher::~TaskDlgAttacher()
{
    if (accepted && onAccept) {
        onAccept();
    }
};

//==== calls from the TaskView ===============================================================


void TaskDlgAttacher::open()
{
    if (!Gui::Command::hasPendingCommand()) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Edit attachment"));
    }
}

void TaskDlgAttacher::clicked(int)
{}

bool TaskDlgAttacher::accept()
{
    try {
        Gui::DocumentT doc(getDocumentName());
        Gui::Document* document = doc.getDocument();
        if (!document || !ViewProvider) {
            return true;
        }

        Part::AttachExtension* pcAttach
            = ViewProvider->getObject()->getExtensionByType<Part::AttachExtension>();
        auto obj = ViewProvider->getObject();

        // DeepSOIC: changed this to heavily rely on dialog constantly updating feature properties
        // if (pcAttach->AttachmentOffset.isTouched()){
        Base::Placement plm = pcAttach->AttachmentOffset.getValue();
        double yaw, pitch, roll;
        plm.getRotation().getYawPitchRoll(yaw, pitch, roll);
        Gui::cmdAppObjectArgs(
            obj,
            "AttachmentOffset = App.Placement(App.Vector(%.10f, %.10f, %.10f),  "
            "App.Rotation(%.10f, %.10f, %.10f))",
            plm.getPosition().x,
            plm.getPosition().y,
            plm.getPosition().z,
            yaw,
            pitch,
            roll
        );
        //}

        Gui::cmdAppObjectArgs(
            obj,
            "MapReversed = %s",
            pcAttach->MapReversed.getValue() ? "True" : "False"
        );

        Gui::cmdAppObjectArgs(
            obj,
            "AttachmentSupport = %s",
            pcAttach->AttachmentSupport.getPyReprString().c_str()
        );

        Gui::cmdAppObjectArgs(obj, "MapPathParameter = %f", pcAttach->MapPathParameter.getValue());

        Gui::cmdAppObjectArgs(
            obj,
            "MapMode = '%s'",
            AttachEngine::getModeName(eMapMode(pcAttach->MapMode.getValue())).c_str()
        );
        Gui::cmdAppObject(obj, "recompute()");

        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(
            parameter,
            tr("Datum dialog: input error"),
            QCoreApplication::translate("Exception", e.what())
        );
        return false;
    }

    accepted = true;

    return true;
}

bool TaskDlgAttacher::reject()
{
    if (onReject) {
        onReject();
    }

    Gui::DocumentT doc(getDocumentName());
    Gui::Document* document = doc.getDocument();
    if (document) {
        // roll back the done things
        Gui::Command::abortCommand();
        Gui::Command::doCommand(Gui::Command::Doc, "%s.recompute()", doc.getAppDocumentPython().c_str());
    }

    accepted = false;

    return true;
}

#include "moc_TaskAttacher.cpp"
