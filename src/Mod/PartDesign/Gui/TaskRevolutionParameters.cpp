// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <QAbstractButton>
#include <QSignalBlocker>

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
#include <Mod/PartDesign/App/FeatureRevolved.h>
#include <Mod/PartDesign/App/Body.h>

#include "ui_TaskRevolutionParameters.h"
#include "TaskRevolutionParameters.h"
#include "ViewProviderGroove.h"
#include "ViewProviderRevolution.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskRevolutionParameters */

namespace
{

bool isLegacyTwoAngles(const std::string& method)
{
    return method == "?TwoAngles" || method == "TwoAngles";
}

}  // namespace

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
    , activeSelectionSide(Side::First)
{
    // we need a separate container widget to add all controls to
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);
    this->groupLayout()->addWidget(proxy);

    // bind property mirrors
    if (auto rev = getObject<PartDesign::Revolved>()) {
        isGroove = rev->getAddSubType() == PartDesign::Revolved::Subtractive;
        this->propSideType = &(rev->SideType);
        this->propReferenceAxis = &(rev->ReferenceAxis);
        this->propReversed = &(rev->Reversed);
    }
    else {
        throw Base::TypeError("The object is neither a groove nor a revolution.");
    }

    setupDialog();

    setUpdateBlocked(false);
    updateUI(Side::First);
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
    createSideControllers();

    ui->checkBoxMidplane->hide();
    ui->checkBoxReversed->setChecked(propReversed->getValue());

    setupSideDialog(m_side1);
    setupSideDialog(m_side2);

    translateSidesList(propSideType->getValue());
}

void TaskRevolutionParameters::createSideControllers()
{
    auto rev = getObject<PartDesign::Revolved>();

    m_side1.changeMode = ui->changeMode;
    m_side1.labelAngle = ui->labelAngle;
    m_side1.angleEdit = ui->revolveAngle;
    m_side1.buttonFace = ui->buttonFace;
    m_side1.lineFaceName = ui->lineFaceName;
    m_side1.Type = &rev->Type;
    m_side1.Angle = &rev->Angle;
    m_side1.UpToFace = &rev->UpToFace;

    m_side2.changeMode = ui->changeMode2;
    m_side2.labelAngle = ui->labelAngle2;
    m_side2.angleEdit = ui->revolveAngle2;
    m_side2.buttonFace = ui->buttonFace2;
    m_side2.lineFaceName = ui->lineFaceName2;
    m_side2.Type = &rev->Type2;
    m_side2.Angle = &rev->Angle2;
    m_side2.UpToFace = &rev->UpToFace2;
}

void TaskRevolutionParameters::setupSideDialog(SideController& side)
{
    side.angleEdit->setValue(side.Angle->getValue());
    side.angleEdit->setMaximum(side.Angle->getMaximum());
    side.angleEdit->setMinimum(side.Angle->getMinimum());
    side.angleEdit->bind(*side.Angle);

    App::DocumentObject* obj = side.UpToFace->getValue();
    std::vector<std::string> subStrings = side.UpToFace->getSubValues();
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
        side.lineFaceName->setText(QString::fromUtf8(obj->Label.getValue()));
        side.lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
    }
    else if (obj && faceId >= 0) {
        side.lineFaceName->setText(QStringLiteral("%1:%2%3").arg(
            QString::fromUtf8(obj->Label.getValue()),
            tr("Face"),
            QString::number(faceId)
        ));
        side.lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
    }
    else {
        side.lineFaceName->clear();
        side.lineFaceName->setProperty("FeatureName", QVariant());
    }

    side.lineFaceName->setProperty("FaceName", QByteArray(upToFace.c_str()));
    side.lineFaceName->setPlaceholderText(tr("No face selected"));

    int index = int(side.Type->getValue());
    if (static_cast<Mode>(index) == Mode::TwoAngles) {
        index = static_cast<int>(Mode::Angle);
    }
    translateModeList(side.changeMode, index);
}

void TaskRevolutionParameters::translateModeList(QComboBox* box, int index)
{
    box->clear();
    box->addItem(tr("Angle"));
    if (!isGroove) {
        box->addItem(tr("To last"));
    }
    else {
        box->addItem(tr("Through all"));
    }
    box->addItem(tr("To first"));
    box->addItem(tr("Up to face"));
    box->setCurrentIndex(index);
}

void TaskRevolutionParameters::translateSidesList(int index)
{
    ui->sidesMode->clear();
    ui->sidesMode->addItem(tr("One sided"));
    ui->sidesMode->addItem(tr("Two sided"));
    ui->sidesMode->addItem(tr("Symmetric"));
    ui->sidesMode->setCurrentIndex(index);
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

void TaskRevolutionParameters::updateSideUI(
    const SideController& side,
    Mode mode,
    bool isParentVisible,
    bool setFocus
)
{
    bool isAngleVisible = false;
    bool isFaceVisible = false;

    if (mode == Mode::Angle) {
        isAngleVisible = true;
        if (setFocus) {
            side.angleEdit->selectNumber();
            QMetaObject::invokeMethod(side.angleEdit, "setFocus", Qt::QueuedConnection);
        }
    }
    else if (mode == Mode::ToFace) {
        isFaceVisible = true;
        if (setFocus) {
            QMetaObject::invokeMethod(side.lineFaceName, "setFocus", Qt::QueuedConnection);
            if (side.lineFaceName->property("FeatureName").isNull()) {
                side.buttonFace->setChecked(true);
            }
        }
    }

    const bool finalAngleVisible = isParentVisible && isAngleVisible;
    side.angleEdit->setVisible(finalAngleVisible);
    side.angleEdit->setEnabled(finalAngleVisible);
    side.labelAngle->setVisible(finalAngleVisible);

    const bool finalFaceVisible = isParentVisible && isFaceVisible;
    side.buttonFace->setVisible(finalFaceVisible);
    side.buttonFace->setEnabled(finalFaceVisible);
    side.lineFaceName->setVisible(finalFaceVisible);
    side.lineFaceName->setEnabled(finalFaceVisible);
    if (!finalFaceVisible) {
        side.buttonFace->setChecked(false);
    }
}

void TaskRevolutionParameters::updateWholeUI(Side side)
{
    SidesMode sidesMode = static_cast<SidesMode>(ui->sidesMode->currentIndex());
    Mode mode1 = static_cast<Mode>(ui->changeMode->currentIndex());
    Mode mode2 = static_cast<Mode>(ui->changeMode2->currentIndex());

    const bool isSide2Visible = sidesMode == SidesMode::TwoSides;
    ui->side1Label->setVisible(isSide2Visible);
    ui->line1->setVisible(isSide2Visible);
    ui->side2Label->setVisible(isSide2Visible);
    ui->line2->setVisible(isSide2Visible);
    ui->typeLabel2->setVisible(isSide2Visible);
    ui->changeMode2->setVisible(isSide2Visible);

    updateSideUI(m_side1, mode1, true, side == Side::First);
    updateSideUI(m_side2, mode2, isSide2Visible, side == Side::Second);

    const bool symmetricAngleLike = sidesMode == SidesMode::Symmetric
        && (mode1 == Mode::Angle || (isGroove && mode1 == Mode::ThroughAll));
    ui->checkBoxReversed->setEnabled(!symmetricAngleLike);
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
    connect(ui->checkBoxReversed, &QCheckBox::toggled,
            this, &TaskRevolutionParameters::onReversed);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskRevolutionParameters::onUpdateView);
    connect(ui->changeMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskRevolutionParameters::onModeChangedSide1);
    connect(ui->changeMode2, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskRevolutionParameters::onModeChangedSide2);
    connect(ui->sidesMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskRevolutionParameters::onSidesModeChanged);
    // clang-format on

    connect(ui->buttonFace, &QAbstractButton::toggled, this, [this](bool checked) {
        onButtonFace(checked, Side::First);
    });
    connect(ui->buttonFace2, &QAbstractButton::toggled, this, [this](bool checked) {
        onButtonFace(checked, Side::Second);
    });
    connect(ui->lineFaceName, &QLineEdit::textEdited, this, [this](const QString& text) {
        onFaceName(text, Side::First);
    });
    connect(ui->lineFaceName2, &QLineEdit::textEdited, this, [this](const QString& text) {
        onFaceName(text, Side::Second);
    });
}

void TaskRevolutionParameters::updateUI(Side side)
{
    if (isUpdateBlocked()) {
        return;
    }

    Base::StateLocker lock(getUpdateBlockRef(), true);
    fillAxisCombo();
    updateWholeUI(side);
}

void TaskRevolutionParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionFace) {
            auto& side = getSideController(activeSelectionSide);
            QString refText = onAddSelection(msg, *side.UpToFace);
            if (refText.length() > 0) {
                QSignalBlocker block(side.lineFaceName);
                side.lineFaceName->setText(refText);
                side.lineFaceName->setProperty("FeatureName", QByteArray(msg.pObjectName));
                side.lineFaceName->setProperty("FaceName", QByteArray(msg.pSubName));
                // Turn off reference selection mode
                side.buttonFace->setChecked(false);
            }
            else {
                clearFaceName(side.lineFaceName);
            }
        }
        else {
            exitSelectionMode();
            std::vector<std::string> axis;
            App::DocumentObject* selObj {};
            if (getReferencedSelection(getObject(), msg, selObj, axis) && selObj) {
                propReferenceAxis->setValue(selObj, axis);

                recomputeFeature();
                updateUI(Side::First);

                setGizmoPositions();
            }
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection && selectionFace) {
        clearFaceName(getSideController(activeSelectionSide).lineFaceName);
    }
}

void TaskRevolutionParameters::onButtonFace(bool pressed, Side side)
{
    auto& sideCtrl = getSideController(side);
    if (pressed) {
        Side otherSide = side == Side::First ? Side::Second : Side::First;
        auto& otherCtrl = getSideController(otherSide);
        QSignalBlocker blockOther(otherCtrl.buttonFace);
        otherCtrl.buttonFace->setChecked(false);
        handleLineFaceNameNo(otherCtrl.lineFaceName);

        // to distinguish that this is NOT the axis selection
        selectionFace = true;
        activeSelectionSide = side;
        handleLineFaceNameClick(sideCtrl.lineFaceName);

        // only faces are allowed
        TaskSketchBasedParameters::onSelectReference(AllowSelection::FACE);
    }
    else if (activeSelectionSide == side) {
        selectionFace = false;
        handleLineFaceNameNo(sideCtrl.lineFaceName);
        sideCtrl.buttonFace->clearFocus();
        TaskSketchBasedParameters::onSelectReference(AllowSelection::NONE);
    }
}

void TaskRevolutionParameters::onFaceName(const QString& text, Side side)
{
    QLineEdit* lineFaceName = getSideController(side).lineFaceName;
    if (text.isEmpty()) {
        // if user cleared the text field then also clear the properties
        lineFaceName->setProperty("FeatureName", QVariant());
        lineFaceName->setProperty("FaceName", QVariant());
    }
    else {
        // expect that the label of an object is used
        QStringList parts = text.split(QChar::fromLatin1(':'));
        QString label = parts[0];
        QVariant name = objectNameByLabel(label, lineFaceName->property("FeatureName"));
        if (name.isValid()) {
            parts[0] = name.toString();
            QString uptoface = parts.join(QStringLiteral(":"));
            lineFaceName->setProperty("FeatureName", name);
            lineFaceName->setProperty("FaceName", setUpToFace(uptoface));
        }
        else {
            lineFaceName->setProperty("FeatureName", QVariant());
            lineFaceName->setProperty("FaceName", QVariant());
        }
    }
}

void TaskRevolutionParameters::translateFaceName(QLineEdit* lineFaceName)
{
    handleLineFaceNameNo(lineFaceName);
    QVariant featureName = lineFaceName->property("FeatureName");
    if (featureName.isValid()) {
        QStringList parts = lineFaceName->text().split(QChar::fromLatin1(':'));
        QByteArray upToFace = lineFaceName->property("FaceName").toByteArray();
        int faceId = -1;
        bool ok = false;
        if (upToFace.indexOf("Face") == 0) {
            faceId = upToFace.remove(0, 4).toInt(&ok);
        }

        if (ok) {
            lineFaceName->setText(QStringLiteral("%1:%2%3").arg(parts[0], tr("Face")).arg(faceId));
        }
        else {
            lineFaceName->setText(parts[0]);
        }
    }
}

QString TaskRevolutionParameters::getFaceName(QLineEdit* lineFaceName) const
{
    QVariant featureName = lineFaceName->property("FeatureName");
    if (featureName.isValid()) {
        QString faceName = lineFaceName->property("FaceName").toString();
        return getFaceReference(featureName.toString(), faceName);
    }

    return QStringLiteral("None");
}

void TaskRevolutionParameters::handleLineFaceNameClick(QLineEdit* lineEdit)
{
    lineEdit->setPlaceholderText(tr("Click on a face in the model"));
}

void TaskRevolutionParameters::handleLineFaceNameNo(QLineEdit* lineEdit)
{
    lineEdit->setPlaceholderText(tr("No face selected"));
}

void TaskRevolutionParameters::clearFaceName(QLineEdit* lineFaceName)
{
    QSignalBlocker block(lineFaceName);
    lineFaceName->clear();
    lineFaceName->setProperty("FeatureName", QVariant());
    lineFaceName->setProperty("FaceName", QVariant());
}

void TaskRevolutionParameters::onAngleChanged(double len)
{
    if (getObject()) {
        m_side1.Angle->setValue(len);
        exitSelectionMode();
        recomputeFeature();

        setGizmoPositions();
    }
}

void TaskRevolutionParameters::onAngle2Changed(double len)
{
    if (getObject()) {
        m_side2.Angle->setValue(len);
        exitSelectionMode();
        recomputeFeature();

        setGizmoPositions();
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

void TaskRevolutionParameters::onModeChangedSide1(int index)
{
    onModeChanged(index, Side::First);
}

void TaskRevolutionParameters::onModeChangedSide2(int index)
{
    onModeChanged(index, Side::Second);
}

void TaskRevolutionParameters::onModeChanged(int index, Side side)
{
    auto& sideCtrl = getSideController(side);

    switch (static_cast<Mode>(index)) {
        case Mode::Angle:
            sideCtrl.Type->setValue("Angle");
            break;
        case Mode::ToLast:
            sideCtrl.Type->setValue(isGroove ? "ThroughAll" : "UpToLast");
            break;
        case Mode::ToFirst:
            sideCtrl.Type->setValue("UpToFirst");
            break;
        case Mode::ToFace:
            sideCtrl.Type->setValue("UpToFace");
            if (sideCtrl.lineFaceName->text().isEmpty()) {
                sideCtrl.buttonFace->setChecked(true);
            }
            break;
        case Mode::TwoAngles:
            break;
    }

    updateUI(side);
    recomputeFeature();

    setGizmoPositions();
}

void TaskRevolutionParameters::onSidesModeChanged(int index)
{
    switch (static_cast<SidesMode>(index)) {
        case SidesMode::OneSide:
            propSideType->setValue("One side");
            updateUI(Side::First);
            break;
        case SidesMode::TwoSides:
            propSideType->setValue("Two sides");
            updateUI(Side::Second);
            break;
        case SidesMode::Symmetric:
            propSideType->setValue("Symmetric");
            updateUI(Side::First);
            break;
    }

    recomputeFeature();
    setGizmoPositions();
}

void TaskRevolutionParameters::onReversed(bool on)
{
    if (getObject()) {
        propReversed->setValue(on);
        recomputeFeature();

        setGizmoPositions();
    }
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

bool TaskRevolutionParameters::getReversed() const
{
    return ui->checkBoxReversed->isChecked();
}

int TaskRevolutionParameters::getMode() const
{
    return ui->changeMode->currentIndex();
}

int TaskRevolutionParameters::getMode2() const
{
    return ui->changeMode2->currentIndex();
}

int TaskRevolutionParameters::getSidesMode() const
{
    return ui->sidesMode->currentIndex();
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
        QSignalBlocker angle(ui->revolveAngle);
        QSignalBlocker angle2(ui->revolveAngle2);
        QSignalBlocker face(ui->lineFaceName);
        QSignalBlocker face2(ui->lineFaceName2);
        QSignalBlocker mode(ui->changeMode);
        QSignalBlocker mode2(ui->changeMode2);
        QSignalBlocker sidesMode(ui->sidesMode);

        ui->retranslateUi(proxy);

        // Translate mode items
        translateModeList(ui->changeMode, ui->changeMode->currentIndex());
        translateModeList(ui->changeMode2, ui->changeMode2->currentIndex());
        translateSidesList(ui->sidesMode->currentIndex());
        translateFaceName(ui->lineFaceName);
        translateFaceName(ui->lineFaceName2);
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
    FCMD_OBJ_CMD(tobj, "SideType = " << getSidesMode());
    FCMD_OBJ_CMD(tobj, "Reversed = " << (getReversed() ? 1 : 0));
    FCMD_OBJ_CMD(tobj, "Type = " << getMode());
    FCMD_OBJ_CMD(tobj, "Type2 = " << getMode2());

    QString facename = QStringLiteral("None");
    QString facename2 = QStringLiteral("None");
    if (static_cast<Mode>(getMode()) == Mode::ToFace) {
        facename = getFaceName(ui->lineFaceName);
    }
    if (static_cast<Mode>(getMode2()) == Mode::ToFace) {
        facename2 = getFaceName(ui->lineFaceName2);
    }
    FCMD_OBJ_CMD(tobj, "UpToFace = " << facename.toLatin1().data());
    FCMD_OBJ_CMD(tobj, "UpToFace2 = " << facename2.toLatin1().data());
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
    showDraggerHints();
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
    std::string revolutionType;
    std::string revolutionType2;
    auto getFeatureProps = [&](PartDesign::Revolved* feature) {
        if (!feature) {
            return false;
        }

        try {
            Part::TopoShape profile = feature->getProfileShape();
            if (profile.isNull()) {
                return false;
            }

            profile.getCenterOfGravity(profileCog);
            basePos = feature->Base.getValue();
            axisDir = feature->Axis.getValue();
            if (axisDir.IsNull()) {
                return false;
            }

            reversed = feature->Reversed.getValue();
            sideType = std::string(feature->SideType.getValueAsString());
            symmetric = sideType == "Symmetric";
            revolutionType = std::string(feature->Type.getValueAsString());
            revolutionType2 = std::string(feature->Type2.getValueAsString());
        }
        catch (...) {
            return false;
        }

        return true;
    };

    PartDesign::Revolved* feature = isGroove
        ? static_cast<PartDesign::Revolved*>(getObject<PartDesign::Groove>())
        : static_cast<PartDesign::Revolved*>(getObject<PartDesign::Revolution>());
    if (!feature) {
        gizmoContainer->visible = false;
        return;
    }

    if (!getFeatureProps(feature)) {
        return;
    }
    gizmoContainer->visible = true;

    auto diff = profileCog - basePos;
    axisDir.Normalize();
    auto axisComp = axisDir * diff.Dot(axisDir);
    auto normalComp = diff - axisComp;

    if (reversed) {
        axisDir = -axisDir;
    }

    rotationGizmo->Gizmo::setDraggerPlacement(basePos + axisComp, normalComp);
    rotationGizmo->getDraggerContainer()->setArcNormalDirection(Base::convertTo<SbVec3f>(axisDir));
    rotationGizmo->setVisibility(revolutionType == "Angle" || isLegacyTwoAngles(revolutionType));

    rotationGizmo2->Gizmo::setDraggerPlacement(basePos + axisComp, normalComp);
    rotationGizmo2->getDraggerContainer()->setArcNormalDirection(Base::convertTo<SbVec3f>(-axisDir));
    rotationGizmo2->setVisibility(
        (sideType == "Two sides" && revolutionType2 == "Angle") || isLegacyTwoAngles(revolutionType)
    );

    if (!symmetric) {
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
