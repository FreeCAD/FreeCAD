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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderCoordinateSystem.h>
#include <Gui/Inventor/Draggers/Gizmo.h>
#include <Gui/Utilities.h>
#include <Gui/Inventor/Draggers/SoRotationDragger.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/Body.h>

#include "ui_TaskRevolutionParameters.h"
#include "TaskRevolutionParameters.h"
#include "ViewProviderGroove.h"
#include "ViewProviderRevolution.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskRevolutionParameters */

TaskRevolutionParameters::TaskRevolutionParameters(
    PartDesignGui::ViewProvider* RevolutionView,
    const char* pixname,
    const QString& title,
    QWidget* parent
)
    : TaskSketchBasedParameters(RevolutionView, parent, pixname, title)
    , ui(new Ui_TaskRevolutionParameters)
    , proxy(new QWidget(this))
    , selectionFace(false)
    , isGroove(false)
{
    // we need a separate container widget to add all controls to
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);
    this->groupLayout()->addWidget(proxy);

    // bind property mirrors
    if (auto rev = getObject<PartDesign::Revolution>()) {
        this->propAngle = &(rev->Angle);
        this->propAngle2 = &(rev->Angle2);
        this->propMidPlane = &(rev->Midplane);
        this->propReferenceAxis = &(rev->ReferenceAxis);
        this->propReversed = &(rev->Reversed);
        this->propUpToFace = &(rev->UpToFace);
        ui->revolveAngle->bind(rev->Angle);
        ui->revolveAngle2->bind(rev->Angle2);
    }
    else if (auto rev = getObject<PartDesign::Groove>()) {
        isGroove = true;
        this->propAngle = &(rev->Angle);
        this->propAngle2 = &(rev->Angle2);
        this->propMidPlane = &(rev->Midplane);
        this->propReferenceAxis = &(rev->ReferenceAxis);
        this->propReversed = &(rev->Reversed);
        this->propUpToFace = &(rev->UpToFace);
        ui->revolveAngle->bind(rev->Angle);
        ui->revolveAngle2->bind(rev->Angle2);
    }
    else {
        throw Base::TypeError("The object is neither a groove nor a revolution.");
    }

    setupDialog();

    setUpdateBlocked(false);
    updateUI(ui->changeMode->currentIndex());
    connectSignals();

    setFocus();

    // show the parts coordinate system axis for selection
    try {
        if (auto vpOrigin = getOriginView()) {
            vpOrigin->setTemporaryVisibility(Gui::DatumElement::Axes);
        }
    }
    catch (const Base::Exception& ex) {
        ex.reportException();
    }

    setupGizmos(RevolutionView);
}

Gui::ViewProviderCoordinateSystem* TaskRevolutionParameters::getOriginView() const
{
    // show the parts coordinate system axis for selection
    PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
    if (body) {
        App::Origin* origin = body->getOrigin();
        return freecad_cast<ViewProviderCoordinateSystem*>(
            Gui::Application::Instance->getViewProvider(origin)
        );
    }

    return nullptr;
}

void TaskRevolutionParameters::setupDialog()
{
    ui->checkBoxMidplane->setChecked(propMidPlane->getValue());
    ui->checkBoxReversed->setChecked(propReversed->getValue());

    ui->revolveAngle->setValue(propAngle->getValue());
    ui->revolveAngle->setMaximum(propAngle->getMaximum());
    ui->revolveAngle->setMinimum(propAngle->getMinimum());

    App::DocumentObject* obj = propUpToFace->getValue();
    std::vector<std::string> subStrings = propUpToFace->getSubValues();
    std::string upToFace;
    int faceId = -1;
    if (obj && !subStrings.empty()) {
        upToFace = subStrings.front();
        if (upToFace.compare(0, 4, "Face") == 0) {
            faceId = std::atoi(&upToFace[4]);
        }
    }

    // Set object labels
    if (obj && PartDesign::Feature::isDatum(obj)) {
        ui->lineFaceName->setText(QString::fromUtf8(obj->Label.getValue()));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
    }
    else if (obj && faceId >= 0) {
        ui->lineFaceName->setText(QStringLiteral("%1:%2%3").arg(
            QString::fromUtf8(obj->Label.getValue()),
            tr("Face"),
            QString::number(faceId)
        ));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
    }
    else {
        ui->lineFaceName->clear();
        ui->lineFaceName->setProperty("FeatureName", QVariant());
    }

    ui->lineFaceName->setProperty("FaceName", QByteArray(upToFace.c_str()));
    int index = 0;

    // TODO: This should also be implemented for groove
    if (!isGroove) {
        auto rev = getObject<PartDesign::Revolution>();
        ui->revolveAngle2->setValue(propAngle2->getValue());
        ui->revolveAngle2->setMaximum(propAngle2->getMaximum());
        ui->revolveAngle2->setMinimum(propAngle2->getMinimum());

        index = int(rev->Type.getValue());
    }
    else {
        auto rev = getObject<PartDesign::Groove>();
        ui->revolveAngle2->setValue(propAngle2->getValue());
        ui->revolveAngle2->setMaximum(propAngle2->getMaximum());
        ui->revolveAngle2->setMinimum(propAngle2->getMinimum());

        index = int(rev->Type.getValue());
    }

    translateModeList(index);
}

void TaskRevolutionParameters::translateModeList(int index)
{
    ui->changeMode->clear();
    ui->changeMode->addItem(tr("Angle"));
    if (!isGroove) {
        ui->changeMode->addItem(tr("To last"));
    }
    else {
        ui->changeMode->addItem(tr("Through all"));
    }
    ui->changeMode->addItem(tr("To first"));
    ui->changeMode->addItem(tr("Up to face"));
    ui->changeMode->addItem(tr("Two angles"));
    ui->changeMode->setCurrentIndex(index);
}

void TaskRevolutionParameters::fillAxisCombo(bool forceRefill)
{
    Base::StateLocker lock(getUpdateBlockRef(), true);

    if (axesInList.empty()) {
        // not filled yet, full refill
        forceRefill = true;
    }

    if (forceRefill) {
        ui->axis->clear();
        axesInList.clear();

        auto* pcFeat = getObject<PartDesign::ProfileBased>();
        if (!pcFeat) {
            throw Base::TypeError("The object is not profile-based.");
        }

        // add sketch axes
        if (auto* pcSketch = dynamic_cast<Part::Part2DObject*>(pcFeat->Profile.getValue())) {
            addAxisToCombo(pcSketch, "V_Axis", QObject::tr("Vertical sketch axis"));
            addAxisToCombo(pcSketch, "H_Axis", QObject::tr("Horizontal sketch axis"));
            for (int i = 0; i < pcSketch->getAxisCount(); i++) {
                QString itemText = QObject::tr("Construction line %1").arg(i + 1);
                std::stringstream sub;
                sub << "Axis" << i;
                addAxisToCombo(pcSketch, sub.str(), itemText);
            }
        }

        // add origin axes
        if (PartDesign::Body* body = PartDesign::Body::findBodyOf(pcFeat)) {
            try {
                App::Origin* orig = body->getOrigin();
                addAxisToCombo(orig->getX(), std::string(), tr("Base X-axis"));
                addAxisToCombo(orig->getY(), std::string(), tr("Base Y-axis"));
                addAxisToCombo(orig->getZ(), std::string(), tr("Base Z-axis"));
            }
            catch (const Base::Exception& ex) {
                ex.reportException();
            }
        }

        // add "Select reference"
        addAxisToCombo(nullptr, std::string(), tr("Select reference…"));
    }  // endif forceRefill

    // add current link, if not in list
    // first, figure out the item number for current axis
    int indexOfCurrent = -1;
    App::DocumentObject* ax = propReferenceAxis->getValue();
    const std::vector<std::string>& subList = propReferenceAxis->getSubValues();
    for (size_t i = 0; i < axesInList.size(); i++) {
        if (ax == axesInList[i]->getValue() && subList == axesInList[i]->getSubValues()) {
            indexOfCurrent = int(i);
        }
    }
    if (indexOfCurrent == -1 && ax) {
        assert(subList.size() <= 1);
        std::string sub;
        if (!subList.empty()) {
            sub = subList[0];
        }
        addAxisToCombo(ax, sub, getRefStr(ax, subList));
        indexOfCurrent = int(axesInList.size()) - 1;
    }

    // highlight current.
    if (indexOfCurrent != -1) {
        ui->axis->setCurrentIndex(indexOfCurrent);
    }
}

void TaskRevolutionParameters::addAxisToCombo(
    App::DocumentObject* linkObj,
    const std::string& linkSubname,
    const QString& itemText
)
{
    this->ui->axis->addItem(itemText);
    this->axesInList.emplace_back(new App::PropertyLinkSub());
    App::PropertyLinkSub& lnk = *(axesInList[axesInList.size() - 1]);
    lnk.setValue(linkObj, std::vector<std::string>(1, linkSubname));
}

void TaskRevolutionParameters::setCheckboxes(PartDesign::Revolution::RevolMethod mode)
{
    // disable/hide everything unless we are sure we don't need it
    // exception: the direction parameters are in any case visible
    bool isRevolveAngleVisible = false;
    bool isRevolveAngle2Visible = false;
    bool isMidplaneEnabled = false;
    bool isMidplaneVisible = false;
    bool isReversedEnabled = false;
    bool isFaceEditEnabled = false;

    if (mode == PartDesign::Revolution::RevolMethod::Angle) {
        isRevolveAngleVisible = true;
        ui->revolveAngle->selectNumber();
        QMetaObject::invokeMethod(ui->revolveAngle, "setFocus", Qt::QueuedConnection);
        isMidplaneVisible = true;
        isMidplaneEnabled = true;
        // Reverse only makes sense if Midplane is not true
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    else if (mode == PartDesign::Revolution::RevolMethod::ThroughAll && isGroove) {
        isMidplaneEnabled = true;
        isMidplaneVisible = true;
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    else if (mode == PartDesign::Revolution::RevolMethod::ToFirst) {
        isReversedEnabled = true;
    }
    else if (mode == PartDesign::Revolution::RevolMethod::ToFace) {
        isReversedEnabled = true;
        isFaceEditEnabled = true;
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->property("FeatureName").isNull()) {
            ui->buttonFace->setChecked(true);
        }
    }
    else if (mode == PartDesign::Revolution::RevolMethod::TwoAngles) {
        isRevolveAngleVisible = true;
        isRevolveAngle2Visible = true;
        isReversedEnabled = true;
    }

    ui->revolveAngle->setVisible(isRevolveAngleVisible);
    ui->revolveAngle->setEnabled(isRevolveAngleVisible);
    ui->labelAngle->setVisible(isRevolveAngleVisible);

    ui->revolveAngle2->setVisible(isRevolveAngle2Visible);
    ui->revolveAngle2->setEnabled(isRevolveAngle2Visible);
    ui->labelAngle2->setVisible(isRevolveAngle2Visible);

    ui->checkBoxMidplane->setEnabled(isMidplaneEnabled);
    ui->checkBoxMidplane->setVisible(isMidplaneVisible);

    ui->checkBoxReversed->setEnabled(isReversedEnabled);

    ui->buttonFace->setVisible(isFaceEditEnabled);
    ui->buttonFace->setEnabled(isFaceEditEnabled);
    ui->lineFaceName->setVisible(isFaceEditEnabled);
    ui->lineFaceName->setEnabled(isFaceEditEnabled);
    if (!isFaceEditEnabled) {
        ui->buttonFace->setChecked(false);
    }
}

void TaskRevolutionParameters::connectSignals()
{
    // clang-format off
    connect(ui->revolveAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskRevolutionParameters::onAngleChanged);
    connect(ui->revolveAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskRevolutionParameters::onAngle2Changed);
    connect(ui->axis, qOverload<int>(&QComboBox::activated),
            this, &TaskRevolutionParameters::onAxisChanged);
    connect(ui->checkBoxMidplane, &QCheckBox::toggled,
            this, &TaskRevolutionParameters::onMidplane);
    connect(ui->checkBoxReversed, &QCheckBox::toggled,
            this, &TaskRevolutionParameters::onReversed);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskRevolutionParameters::onUpdateView);
    connect(ui->changeMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskRevolutionParameters::onModeChanged);
    connect(ui->buttonFace, &QPushButton::toggled,
            this, &TaskRevolutionParameters::onButtonFace);
    connect(ui->lineFaceName, &QLineEdit::textEdited,
            this, &TaskRevolutionParameters::onFaceName);
    // clang-format on
}

void TaskRevolutionParameters::updateUI(int index)
{
    if (isUpdateBlocked()) {
        return;
    }

    Base::StateLocker lock(getUpdateBlockRef(), true);
    fillAxisCombo();
    setCheckboxes(static_cast<PartDesign::Revolution::RevolMethod>(index));
}

void TaskRevolutionParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        int mode = ui->changeMode->currentIndex();
        if (selectionFace) {
            auto rev = getObject<PartDesign::Revolution>();
            QString refText = onAddSelection(msg, rev->UpToFace);
            if (refText.length() > 0) {
                QSignalBlocker block(ui->lineFaceName);
                ui->lineFaceName->setText(refText);
                ui->lineFaceName->setProperty("FeatureName", QByteArray(msg.pObjectName));
                ui->lineFaceName->setProperty("FaceName", QByteArray(msg.pSubName));
                // Turn off reference selection mode
                ui->buttonFace->setChecked(false);
            }
            else {
                clearFaceName();
            }
        }
        else {
            exitSelectionMode();
            std::vector<std::string> axis;
            App::DocumentObject* selObj {};
            if (getReferencedSelection(getObject(), msg, selObj, axis) && selObj) {
                propReferenceAxis->setValue(selObj, axis);

                recomputeFeature();
                updateUI(mode);

                setGizmoPositions();
            }
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection && selectionFace) {
        clearFaceName();
    }
}

void TaskRevolutionParameters::onButtonFace(bool pressed)
{
    // to distinguish that this is NOT the axis selection
    selectionFace = pressed;

    // only faces are allowed
    TaskSketchBasedParameters::onSelectReference(pressed ? AllowSelection::FACE : AllowSelection::NONE);
}

void TaskRevolutionParameters::onFaceName(const QString& text)
{
    if (text.isEmpty()) {
        // if user cleared the text field then also clear the properties
        ui->lineFaceName->setProperty("FeatureName", QVariant());
        ui->lineFaceName->setProperty("FaceName", QVariant());
    }
    else {
        // expect that the label of an object is used
        QStringList parts = text.split(QChar::fromLatin1(':'));
        QString label = parts[0];
        QVariant name = objectNameByLabel(label, ui->lineFaceName->property("FeatureName"));
        if (name.isValid()) {
            parts[0] = name.toString();
            QString uptoface = parts.join(QStringLiteral(":"));
            ui->lineFaceName->setProperty("FeatureName", name);
            ui->lineFaceName->setProperty("FaceName", setUpToFace(uptoface));
        }
        else {
            ui->lineFaceName->setProperty("FeatureName", QVariant());
            ui->lineFaceName->setProperty("FaceName", QVariant());
        }
    }
}

void TaskRevolutionParameters::translateFaceName()
{
    ui->lineFaceName->setPlaceholderText(tr("No face selected"));
    QVariant featureName = ui->lineFaceName->property("FeatureName");
    if (featureName.isValid()) {
        QStringList parts = ui->lineFaceName->text().split(QChar::fromLatin1(':'));
        QByteArray upToFace = ui->lineFaceName->property("FaceName").toByteArray();
        int faceId = -1;
        bool ok = false;
        if (upToFace.indexOf("Face") == 0) {
            faceId = upToFace.remove(0, 4).toInt(&ok);
        }

        if (ok) {
            ui->lineFaceName->setText(QStringLiteral("%1:%2%3").arg(parts[0], tr("Face")).arg(faceId));
        }
        else {
            ui->lineFaceName->setText(parts[0]);
        }
    }
}

QString TaskRevolutionParameters::getFaceName() const
{
    QVariant featureName = ui->lineFaceName->property("FeatureName");
    if (featureName.isValid()) {
        QString faceName = ui->lineFaceName->property("FaceName").toString();
        return getFaceReference(featureName.toString(), faceName);
    }

    return QStringLiteral("None");
}

void TaskRevolutionParameters::clearFaceName()
{
    QSignalBlocker block(ui->lineFaceName);
    ui->lineFaceName->clear();
    ui->lineFaceName->setProperty("FeatureName", QVariant());
    ui->lineFaceName->setProperty("FaceName", QVariant());
}

void TaskRevolutionParameters::onAngleChanged(double len)
{
    if (getObject()) {
        propAngle->setValue(len);
        exitSelectionMode();
        recomputeFeature();
    }
}

void TaskRevolutionParameters::onAngle2Changed(double len)
{
    if (getObject()) {
        if (propAngle2) {
            propAngle2->setValue(len);
        }
        exitSelectionMode();
        recomputeFeature();
    }
}

void TaskRevolutionParameters::onAxisChanged(int num)
{
    if (isUpdateBlocked()) {
        return;
    }
    auto pcRevolution = getObject<PartDesign::ProfileBased>();

    if (axesInList.empty()) {
        return;
    }

    App::DocumentObject* oldRefAxis = propReferenceAxis->getValue();
    std::vector<std::string> oldSubRefAxis = propReferenceAxis->getSubValues();
    std::string oldRefName;
    if (!oldSubRefAxis.empty()) {
        oldRefName = oldSubRefAxis.front();
    }

    App::PropertyLinkSub& lnk = *(axesInList[num]);
    if (!lnk.getValue()) {
        // enter reference selection mode
        if (auto sketch = dynamic_cast<Part::Part2DObject*>(pcRevolution->Profile.getValue())) {
            Gui::cmdAppObjectShow(sketch);
        }
        TaskSketchBasedParameters::onSelectReference(
            AllowSelection::EDGE | AllowSelection::PLANAR | AllowSelection::CIRCLE
        );
    }
    else {
        if (!pcRevolution->getDocument()->isIn(lnk.getValue())) {
            Base::Console().error("Object was deleted\n");
            return;
        }
        propReferenceAxis->Paste(lnk);
        exitSelectionMode();
    }

    try {
        App::DocumentObject* newRefAxis = propReferenceAxis->getValue();
        const std::vector<std::string>& newSubRefAxis = propReferenceAxis->getSubValues();
        std::string newRefName;
        if (!newSubRefAxis.empty()) {
            newRefName = newSubRefAxis.front();
        }

        if (oldRefAxis != newRefAxis || oldSubRefAxis.size() != newSubRefAxis.size()
            || oldRefName != newRefName) {
            bool reversed = propReversed->getValue();
            if (pcRevolution->isDerivedFrom<PartDesign::Revolution>()) {
                reversed = static_cast<PartDesign::Revolution*>(pcRevolution)->suggestReversed();
            }
            if (pcRevolution->isDerivedFrom<PartDesign::Groove>()) {
                reversed = static_cast<PartDesign::Groove*>(pcRevolution)->suggestReversed();
            }

            if (reversed != propReversed->getValue()) {
                propReversed->setValue(reversed);
                ui->checkBoxReversed->blockSignals(true);
                ui->checkBoxReversed->setChecked(reversed);
                ui->checkBoxReversed->blockSignals(false);
            }
        }

        recomputeFeature();

        setGizmoPositions();
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskRevolutionParameters::onMidplane(bool on)
{
    if (getObject()) {
        propMidPlane->setValue(on);
        recomputeFeature();

        setGizmoPositions();
    }
}

void TaskRevolutionParameters::onReversed(bool on)
{
    if (getObject()) {
        propReversed->setValue(on);
        recomputeFeature();

        setGizmoPositions();
    }
}

void TaskRevolutionParameters::onModeChanged(int index)
{
    App::PropertyEnumeration* propEnum = isGroove ? &(getObject<PartDesign::Groove>()->Type)
                                                  : &(getObject<PartDesign::Revolution>()->Type);

    switch (static_cast<PartDesign::Revolution::RevolMethod>(index)) {
        case PartDesign::Revolution::RevolMethod::Angle:
            propEnum->setValue("Angle");
            break;
        case PartDesign::Revolution::RevolMethod::ToLast:
            propEnum->setValue(isGroove ? "ThroughAll" : "UpToLast");
            break;
        case PartDesign::Revolution::RevolMethod::ToFirst:
            propEnum->setValue("UpToFirst");
            break;
        case PartDesign::Revolution::RevolMethod::ToFace:
            propEnum->setValue("UpToFace");
            break;
        case PartDesign::Revolution::RevolMethod::TwoAngles:
            propEnum->setValue("TwoAngles");
            break;
    }

    updateUI(index);
    recomputeFeature();

    setGizmoPositions();
}

void TaskRevolutionParameters::getReferenceAxis(
    App::DocumentObject*& obj,
    std::vector<std::string>& sub
) const
{
    if (axesInList.empty()) {
        throw Base::RuntimeError("Not initialized!");
    }

    int num = ui->axis->currentIndex();
    const App::PropertyLinkSub& lnk = *(axesInList[num]);
    if (!lnk.getValue()) {
        throw Base::RuntimeError("Still in reference selection mode; reference wasn't selected yet");
    }

    auto revolution = getObject<PartDesign::ProfileBased>();
    if (!revolution->getDocument()->isIn(lnk.getValue())) {
        throw Base::RuntimeError("Object was deleted");
    }

    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

bool TaskRevolutionParameters::getMidplane() const
{
    return ui->checkBoxMidplane->isChecked();
}

bool TaskRevolutionParameters::getReversed() const
{
    return ui->checkBoxReversed->isChecked();
}

TaskRevolutionParameters::~TaskRevolutionParameters()
{
    // hide the parts coordinate system axis for selection
    try {
        if (auto vpOrigin = getOriginView()) {
            vpOrigin->resetTemporaryVisibility();
        }
    }
    catch (const Base::Exception& ex) {
        ex.reportException();
    }

    axesInList.clear();
}

void TaskRevolutionParameters::changeEvent(QEvent* event)
{
    TaskBox::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);

        // Translate mode items
        translateModeList(ui->changeMode->currentIndex());
    }
}

void TaskRevolutionParameters::apply()
{
    // Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Revolution changed"));
    ui->revolveAngle->apply();
    ui->revolveAngle2->apply();
    std::vector<std::string> sub;
    App::DocumentObject* obj {};
    getReferenceAxis(obj, sub);
    std::string axis = buildLinkSingleSubPythonStr(obj, sub);
    auto tobj = getObject();
    FCMD_OBJ_CMD(tobj, "ReferenceAxis = " << axis);
    FCMD_OBJ_CMD(tobj, "Midplane = " << (getMidplane() ? 1 : 0));
    FCMD_OBJ_CMD(tobj, "Reversed = " << (getReversed() ? 1 : 0));
    int mode = ui->changeMode->currentIndex();
    FCMD_OBJ_CMD(tobj, "Type = " << mode);
    QString facename = QStringLiteral("None");
    if (static_cast<PartDesign::Revolution::RevolMethod>(mode)
        == PartDesign::Revolution::RevolMethod::ToFace) {
        facename = getFaceName();
    }
    FCMD_OBJ_CMD(tobj, "UpToFace = " << facename.toLatin1().data());
}

void TaskRevolutionParameters::setupGizmos(ViewProvider* vp)
{
    if (!GizmoContainer::isEnabled()) {
        return;
    }

    rotationGizmo = new Gui::RadialGizmo(ui->revolveAngle);
    rotationGizmo2 = new Gui::RadialGizmo(ui->revolveAngle2);

    gizmoContainer = GizmoContainer::create({rotationGizmo, rotationGizmo2}, vp);
    rotationGizmo->flipArrow();
    rotationGizmo2->flipArrow();

    defaultGizmoMultFactor = rotationGizmo->getMultFactor();

    setGizmoPositions();
}

void TaskRevolutionParameters::setGizmoPositions()
{
    if (!gizmoContainer) {
        return;
    }

    Base::Vector3d profileCog;
    Base::Vector3d basePos;
    Base::Vector3d axisDir;
    bool reversed = false;
    bool symmetric = false;
    std::string sideType;

    auto getFeatureProps = [&](auto* feature) {
        if (!feature || feature->isError()) {
            return false;
        }
        Part::TopoShape profile = feature->getProfileShape();

        profile.getCenterOfGravity(profileCog);
        basePos = feature->Base.getValue();
        axisDir = feature->Axis.getValue();
        reversed = feature->Reversed.getValue();
        symmetric = feature->Midplane.getValue();
        sideType = std::string(feature->Type.getValueAsString());
        return true;
    };

    bool ret;
    if (isGroove) {
        auto groove = getObject<PartDesign::Groove>();
        ret = getFeatureProps(getObject<PartDesign::Groove>());
    }
    else {
        ret = getFeatureProps(getObject<PartDesign::Revolution>());
    }

    gizmoContainer->visible = ret;
    if (!ret) {
        return;
    }

    auto diff = profileCog - basePos;
    axisDir.Normalize();
    auto axisComp = axisDir * diff.Dot(axisDir);
    auto normalComp = diff - axisComp;

    if (reversed) {
        axisDir = -axisDir;
    }

    rotationGizmo->Gizmo::setDraggerPlacement(basePos + axisComp, normalComp);
    rotationGizmo->getDraggerContainer()->setArcNormalDirection(Base::convertTo<SbVec3f>(axisDir));
    rotationGizmo->setVisibility(sideType == "Angle" || sideType == "TwoAngles");

    rotationGizmo2->Gizmo::setDraggerPlacement(basePos + axisComp, normalComp);
    rotationGizmo2->getDraggerContainer()->setArcNormalDirection(Base::convertTo<SbVec3f>(-axisDir));
    rotationGizmo2->setVisibility(sideType == "TwoAngles");

    if (sideType == "TwoAngles" || !symmetric) {
        rotationGizmo->setMultFactor(defaultGizmoMultFactor);
    }
    else {
        rotationGizmo->setMultFactor(defaultGizmoMultFactor / 2.0);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TaskDlgRevolutionParameters::TaskDlgRevolutionParameters(ViewProviderRevolution* RevolutionView)
    : TaskDlgSketchBasedParameters(RevolutionView)
{
    assert(RevolutionView);
    Content.push_back(
        new TaskRevolutionParameters(RevolutionView, "PartDesign_Revolution", tr("Revolution Parameters"))
    );
    Content.push_back(preview);
}

TaskDlgGrooveParameters::TaskDlgGrooveParameters(ViewProviderGroove* GrooveView)
    : TaskDlgSketchBasedParameters(GrooveView)
{
    assert(GrooveView);
    Content.push_back(
        new TaskRevolutionParameters(GrooveView, "PartDesign_Groove", tr("Groove Parameters"))
    );
    Content.push_back(preview);
}


#include "moc_TaskRevolutionParameters.cpp"
