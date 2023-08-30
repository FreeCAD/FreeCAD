/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QSignalBlocker>
#endif

#include <App/Document.h>
#include <Base/UnitsApi.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureExtrude.h>

#include "ui_TaskPadPocketParameters.h"
#include "TaskExtrudeParameters.h"
#include "ReferenceSelection.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskExtrudeParameters */

TaskExtrudeParameters::TaskExtrudeParameters(ViewProviderSketchBased *SketchBasedView, QWidget *parent,
                                             const std::string& pixmapname, const QString& parname)
    : TaskSketchBasedParameters(SketchBasedView, parent, pixmapname, parname)
    , propReferenceAxis(nullptr)
    , ui(new Ui_TaskPadPocketParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    handleLineFaceNameNo();

    Gui::ButtonGroup* group = new Gui::ButtonGroup(this);
    group->addButton(ui->checkBoxMidplane);
    group->addButton(ui->checkBoxReversed);
    group->setExclusive(true);

    this->groupLayout()->addWidget(proxy);
}

TaskExtrudeParameters::~TaskExtrudeParameters() = default;

void TaskExtrudeParameters::setupDialog()
{
    // Get the feature data
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    Base::Quantity l = extrude->Length.getQuantityValue();
    Base::Quantity l2 = extrude->Length2.getQuantityValue();
    Base::Quantity off = extrude->Offset.getQuantityValue();
    Base::Quantity taper = extrude->TaperAngle.getQuantityValue();
    Base::Quantity taper2 = extrude->TaperAngle2.getQuantityValue();

    bool alongNormal = extrude->AlongSketchNormal.getValue();
    bool useCustom = extrude->UseCustomVector.getValue();

    double xs = extrude->Direction.getValue().x;
    double ys = extrude->Direction.getValue().y;
    double zs = extrude->Direction.getValue().z;

    bool midplane = extrude->Midplane.getValue();
    bool reversed = extrude->Reversed.getValue();

    int index = extrude->Type.getValue(); // must extract value here, clear() kills it!
    App::DocumentObject* obj =  extrude->UpToFace.getValue();
    std::vector<std::string> subStrings = extrude->UpToFace.getSubValues();
    std::string upToFace;
    int faceId = -1;
    if (obj && !subStrings.empty()) {
        upToFace = subStrings.front();
        if (upToFace.compare(0, 4, "Face") == 0)
            faceId = std::atoi(&upToFace[4]);
    }

    // set decimals for the direction edits
    // do this here before the edits are filled to avoid rounding mistakes
    int UserDecimals = Base::UnitsApi::getDecimals();
    ui->XDirectionEdit->setDecimals(UserDecimals);
    ui->YDirectionEdit->setDecimals(UserDecimals);
    ui->ZDirectionEdit->setDecimals(UserDecimals);

    // Fill data into dialog elements
    // the direction combobox is later filled in updateUI()
    ui->lengthEdit->setValue(l);
    ui->lengthEdit2->setValue(l2);
    ui->offsetEdit->setValue(off);
    ui->taperEdit->setMinimum(extrude->TaperAngle.getMinimum());
    ui->taperEdit->setMaximum(extrude->TaperAngle.getMaximum());
    ui->taperEdit->setSingleStep(extrude->TaperAngle.getStepSize());
    ui->taperEdit->setValue(taper);
    ui->taperEdit2->setMinimum(extrude->TaperAngle2.getMinimum());
    ui->taperEdit2->setMaximum(extrude->TaperAngle2.getMaximum());
    ui->taperEdit2->setSingleStep(extrude->TaperAngle2.getStepSize());
    ui->taperEdit2->setValue(taper2);

    ui->checkBoxAlongDirection->setChecked(alongNormal);
    ui->checkBoxDirection->setChecked(useCustom);
    onDirectionToggled(useCustom);

    // disable to change the direction if not custom
    if (!useCustom) {
        ui->XDirectionEdit->setEnabled(false);
        ui->YDirectionEdit->setEnabled(false);
        ui->ZDirectionEdit->setEnabled(false);
    }
    ui->XDirectionEdit->setValue(xs);
    ui->YDirectionEdit->setValue(ys);
    ui->ZDirectionEdit->setValue(zs);

    // Bind input fields to properties
    ui->lengthEdit->bind(extrude->Length);
    ui->lengthEdit2->bind(extrude->Length2);
    ui->offsetEdit->bind(extrude->Offset);
    ui->taperEdit->bind(extrude->TaperAngle);
    ui->taperEdit2->bind(extrude->TaperAngle2);
    ui->XDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, std::string("Direction.x")));
    ui->YDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, std::string("Direction.y")));
    ui->ZDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, std::string("Direction.z")));

    ui->checkBoxMidplane->setChecked(midplane);
    // According to bug #0000521 the reversed option
    // shouldn't be de-activated if the pad has a support face
    ui->checkBoxReversed->setChecked(reversed);

    // Set object labels
    if (obj && PartDesign::Feature::isDatum(obj)) {
        ui->lineFaceName->setText(QString::fromUtf8(obj->Label.getValue()));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
    }
    else if (obj && faceId >= 0) {
        ui->lineFaceName->setText(QString::fromLatin1("%1:%2%3")
                                  .arg(QString::fromUtf8(obj->Label.getValue()),
                                       tr("Face"),
                                       QString::number(faceId)));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
    }
    else {
        ui->lineFaceName->clear();
        ui->lineFaceName->setProperty("FeatureName", QVariant());
    }

    ui->lineFaceName->setProperty("FaceName", QByteArray(upToFace.c_str()));

    translateModeList(index);

    connectSlots();

    this->propReferenceAxis = &(extrude->ReferenceAxis);

    // Due to signals attached after changes took took into effect we should update the UI now.
    updateUI(index);
}

void TaskExtrudeParameters::readValuesFromHistory()
{
    ui->lengthEdit->setToLastUsedValue();
    ui->lengthEdit->selectNumber();
    ui->lengthEdit2->setToLastUsedValue();
    ui->lengthEdit2->selectNumber();
    ui->offsetEdit->setToLastUsedValue();
    ui->offsetEdit->selectNumber();
    ui->taperEdit->setToLastUsedValue();
    ui->taperEdit->selectNumber();
    ui->taperEdit2->setToLastUsedValue();
    ui->taperEdit2->selectNumber();
}

void TaskExtrudeParameters::connectSlots()
{
    QMetaObject::connectSlotsByName(this);

    connect(ui->lengthEdit, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this, &TaskExtrudeParameters::onLengthChanged);
    connect(ui->lengthEdit2, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this, &TaskExtrudeParameters::onLength2Changed);
    connect(ui->offsetEdit, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this, &TaskExtrudeParameters::onOffsetChanged);
    connect(ui->taperEdit, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this, &TaskExtrudeParameters::onTaperChanged);
    connect(ui->taperEdit2, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this, &TaskExtrudeParameters::onTaper2Changed);
    connect(ui->directionCB, qOverload<int>(&QComboBox::activated),
        this, &TaskExtrudeParameters::onDirectionCBChanged);
    connect(ui->checkBoxAlongDirection, &QCheckBox::toggled,
        this, &TaskExtrudeParameters::onAlongSketchNormalChanged);
    connect(ui->checkBoxDirection, &QCheckBox::toggled,
        this, &TaskExtrudeParameters::onDirectionToggled);
    connect(ui->XDirectionEdit, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &TaskExtrudeParameters::onXDirectionEditChanged);
    connect(ui->YDirectionEdit, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &TaskExtrudeParameters::onYDirectionEditChanged);
    connect(ui->ZDirectionEdit, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &TaskExtrudeParameters::onZDirectionEditChanged);
    connect(ui->checkBoxMidplane, &QCheckBox::toggled,
        this, &TaskExtrudeParameters::onMidplaneChanged);
    connect(ui->checkBoxReversed, &QCheckBox::toggled,
        this, &TaskExtrudeParameters::onReversedChanged);
    connect(ui->changeMode, qOverload<int>(&QComboBox::currentIndexChanged),
        this, &TaskExtrudeParameters::onModeChanged);
    connect(ui->buttonFace, &QPushButton::toggled,
        this, &TaskExtrudeParameters::onButtonFace);
    connect(ui->lineFaceName, &QLineEdit::textEdited,
        this, &TaskExtrudeParameters::onFaceName);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
        this, &TaskExtrudeParameters::onUpdateView);
}

void TaskExtrudeParameters::tryRecomputeFeature()
{
    try {
        // recompute and update the direction
        recomputeFeature();
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

void TaskExtrudeParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // if we have an edge selection for the extrude direction
        if (!selectionFace) {
            selectedReferenceAxis(msg);
        }
        // if we have a selection of a face
        else {
            QString refText = onAddSelection(msg);
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
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection && selectionFace) {
        clearFaceName();
    }
}

void TaskExtrudeParameters::selectedReferenceAxis(const Gui::SelectionChanges& msg)
{
    std::vector<std::string> edge;
    App::DocumentObject* selObj;
    if (getReferencedSelection(vp->getObject(), msg, selObj, edge) && selObj) {
        exitSelectionMode();
        propReferenceAxis->setValue(selObj, edge);
        tryRecomputeFeature();
        // update direction combobox
        fillDirectionCombo();
    }
}

void TaskExtrudeParameters::clearFaceName()
{
    QSignalBlocker block(ui->lineFaceName);
    ui->lineFaceName->clear();
    ui->lineFaceName->setProperty("FeatureName", QVariant());
    ui->lineFaceName->setProperty("FaceName", QVariant());

}

void TaskExtrudeParameters::onLengthChanged(double len)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->Length.setValue(len);
    tryRecomputeFeature();
}

void TaskExtrudeParameters::onLength2Changed(double len)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->Length2.setValue(len);
    tryRecomputeFeature();
}

void TaskExtrudeParameters::onOffsetChanged(double len)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->Offset.setValue(len);
    tryRecomputeFeature();
}

void TaskExtrudeParameters::onTaperChanged(double angle)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->TaperAngle.setValue(angle);
    tryRecomputeFeature();
}

void TaskExtrudeParameters::onTaper2Changed(double angle)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->TaperAngle2.setValue(angle);
    tryRecomputeFeature();
}

bool TaskExtrudeParameters::hasProfileFace(PartDesign::ProfileBased* profile) const
{
    try {
        Part::Feature* pcFeature = profile->getVerifiedObject();
        Base::Vector3d SketchVector = profile->getProfileNormal();
        Q_UNUSED(pcFeature)
        Q_UNUSED(SketchVector)
        return true;
    }
    catch (const Base::Exception&) {
    }

    return false;
}

void TaskExtrudeParameters::fillDirectionCombo()
{
    bool oldVal_blockUpdate = blockUpdate;
    blockUpdate = true;

    if (axesInList.empty()) {
        bool hasFace = false;
        ui->directionCB->clear();
        // we can have sketches or faces
        // for sketches just get the sketch normal
        PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        Part::Part2DObject* pcSketch = dynamic_cast<Part::Part2DObject*>(pcFeat->Profile.getValue());
        // for faces we test if it is verified and if we can get its normal
        if (!pcSketch) {
            hasFace = hasProfileFace(pcFeat);
        }

        if (pcSketch)
            addAxisToCombo(pcSketch, "N_Axis", tr("Sketch normal"));
        else if (hasFace)
            addAxisToCombo(pcFeat->Profile.getValue(), std::string(), tr("Face normal"), false);

        // add the other entries
        addAxisToCombo(nullptr, std::string(), tr("Select reference..."));

        // we start with the sketch normal as proposal for the custom direction
        if (pcSketch)
            addAxisToCombo(pcSketch, "N_Axis", tr("Custom direction"));
        else if (hasFace)
            addAxisToCombo(pcFeat->Profile.getValue(), std::string(), tr("Custom direction"), false);
    }

    // add current link, if not in list
    // first, figure out the item number for current axis
    int indexOfCurrent = -1;
    App::DocumentObject* ax = propReferenceAxis->getValue();
    const std::vector<std::string>& subList = propReferenceAxis->getSubValues();
    for (size_t i = 0; i < axesInList.size(); i++) {
        if (ax == axesInList[i]->getValue() && subList == axesInList[i]->getSubValues()) {
            indexOfCurrent = i;
            break;
        }
    }
    // if the axis is not yet listed in the combobox
    if (indexOfCurrent == -1 && ax) {
        assert(subList.size() <= 1);
        std::string sub;
        if (!subList.empty())
            sub = subList[0];
        addAxisToCombo(ax, sub, getRefStr(ax, subList));
        indexOfCurrent = axesInList.size() - 1;
        // the axis is not the normal, thus enable along direction
        ui->checkBoxAlongDirection->setEnabled(true);
        // we don't have custom direction thus disable its settings
        ui->XDirectionEdit->setEnabled(false);
        ui->YDirectionEdit->setEnabled(false);
        ui->ZDirectionEdit->setEnabled(false);
    }

    // highlight either current index or set custom direction
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    bool hasCustom = extrude->UseCustomVector.getValue();
    if (indexOfCurrent != -1 && !hasCustom)
        ui->directionCB->setCurrentIndex(indexOfCurrent);
    if (hasCustom)
        ui->directionCB->setCurrentIndex(DirectionModes::Custom);

    blockUpdate = oldVal_blockUpdate;
}

void TaskExtrudeParameters::addAxisToCombo(App::DocumentObject* linkObj, std::string linkSubname,
                                           QString itemText, bool hasSketch)
{
    this->ui->directionCB->addItem(itemText);
    this->axesInList.emplace_back(new App::PropertyLinkSub);
    App::PropertyLinkSub& lnk = *(axesInList.back());
    // if we have a face, we leave the link empty since we cannot
    // store the face normal as sublink
    if (hasSketch)
        lnk.setValue(linkObj, std::vector<std::string>(1, linkSubname));
}

void TaskExtrudeParameters::setCheckboxes(Modes mode, Type type)
{
    // disable/hide everything unless we are sure we don't need it
    // exception: the direction parameters are in any case visible
    bool isLengthEditVisible = false;
    bool isLengthEdit2Visible = false;
    bool isOffsetEditVisible = false;
    bool isOffsetEditEnabled = true;
    bool isMidplaneEnabled = false;
    bool isMidplaneVisible = false;
    bool isReversedEnabled = false;
    bool isFaceEditVisible = false;
    bool isTaperEditVisible = false;
    bool isTaperEdit2Visible = false;

    if (mode == Modes::Dimension) {
        isLengthEditVisible = true;
        ui->lengthEdit->selectNumber();
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        isTaperEditVisible = true;
        isMidplaneVisible = true;
        isMidplaneEnabled = true;
        // Reverse only makes sense if Midplane is not true
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    else if (mode == Modes::ThroughAll && type == Type::Pocket) {
        isOffsetEditVisible = true;
        isOffsetEditEnabled = false; // offset may have some meaning for through all but it doesn't work
        isMidplaneEnabled = true;
        isMidplaneVisible = true;
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    else if (mode == Modes::ToLast && type == Type::Pad) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;
    }
    else if (mode == Modes::ToFirst) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;
    }
    else if (mode == Modes::ToFace) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;
        isFaceEditVisible = true;
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->property("FeatureName").isNull())
            ui->buttonFace->setChecked(true);
    }
    else if (mode == Modes::TwoDimensions) {
        isLengthEditVisible = true;
        isLengthEdit2Visible = true;
        isTaperEditVisible = true;
        isTaperEdit2Visible = true;
        isReversedEnabled = true;
    }

    ui->lengthEdit->setVisible(isLengthEditVisible);
    ui->lengthEdit->setEnabled(isLengthEditVisible);
    ui->labelLength->setVisible(isLengthEditVisible);
    ui->checkBoxAlongDirection->setVisible(isLengthEditVisible);

    ui->lengthEdit2->setVisible(isLengthEdit2Visible);
    ui->lengthEdit2->setEnabled(isLengthEdit2Visible);
    ui->labelLength2->setVisible(isLengthEdit2Visible);

    ui->offsetEdit->setVisible(isOffsetEditVisible);
    ui->offsetEdit->setEnabled(isOffsetEditVisible && isOffsetEditEnabled);
    ui->labelOffset->setVisible(isOffsetEditVisible);

    ui->taperEdit->setVisible(isTaperEditVisible);
    ui->taperEdit->setEnabled(isTaperEditVisible);
    ui->labelTaperAngle->setVisible(isTaperEditVisible);

    ui->taperEdit2->setVisible(isTaperEdit2Visible);
    ui->taperEdit2->setEnabled(isTaperEdit2Visible);
    ui->labelTaperAngle2->setVisible(isTaperEdit2Visible);

    ui->checkBoxMidplane->setEnabled(isMidplaneEnabled);
    ui->checkBoxMidplane->setVisible(isMidplaneVisible);

    ui->checkBoxReversed->setEnabled(isReversedEnabled);

    ui->buttonFace->setVisible(isFaceEditVisible);
    ui->lineFaceName->setVisible(isFaceEditVisible);
    if (!isFaceEditVisible) {
        ui->buttonFace->setChecked(false);
    }
}

void TaskExtrudeParameters::onDirectionCBChanged(int num)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());

    if (axesInList.empty())
        return;

    // we use this scheme for 'num'
    // 0: normal to sketch or face
    // 1: selection mode
    // 2: custom
    // 3-x: edges selected in the 3D model

    // check the axis
    // when the link is empty we are either in selection mode
    // or we are normal to a face
    App::PropertyLinkSub& lnk = *(axesInList[num]);
    if (num == DirectionModes::Select) {
        // to distinguish that this is the direction selection
        selectionFace = false;
        setDirectionMode(num);
        TaskSketchBasedParameters::onSelectReference(AllowSelection::EDGE |
                                                     AllowSelection::PLANAR |
                                                     AllowSelection::CIRCLE);
    }
    else {
        if (lnk.getValue()) {
            if (!extrude->getDocument()->isIn(lnk.getValue())) {
                Base::Console().Error("Object was deleted\n");
                return;
            }
            propReferenceAxis->Paste(lnk);
        }

        // in case the user is in selection mode, but changed his mind before selecting anything
        exitSelectionMode();

        setDirectionMode(num);
        extrude->ReferenceAxis.setValue(lnk.getValue(), lnk.getSubValues());
        tryRecomputeFeature();
        updateDirectionEdits();
    }
}

void TaskExtrudeParameters::onAlongSketchNormalChanged(bool on)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->AlongSketchNormal.setValue(on);
    tryRecomputeFeature();
}

void TaskExtrudeParameters::onDirectionToggled(bool on)
{
    if (on)
        ui->groupBoxDirection->show();
    else
        ui->groupBoxDirection->hide();
}

void TaskExtrudeParameters::onXDirectionEditChanged(double len)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->Direction.setValue(len, extrude->Direction.getValue().y, extrude->Direction.getValue().z);
    tryRecomputeFeature();
    // checking for case of a null vector is done in FeatureExtrude.cpp
    // if there was a null vector, the normal vector of the sketch is used.
    // therefore the vector component edits must be updated
    updateDirectionEdits();
}

void TaskExtrudeParameters::onYDirectionEditChanged(double len)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->Direction.setValue(extrude->Direction.getValue().x, len, extrude->Direction.getValue().z);
    tryRecomputeFeature();
    updateDirectionEdits();
}

void TaskExtrudeParameters::onZDirectionEditChanged(double len)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->Direction.setValue(extrude->Direction.getValue().x, extrude->Direction.getValue().y, len);
    tryRecomputeFeature();
    updateDirectionEdits();
}

void TaskExtrudeParameters::updateDirectionEdits()
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    // we don't want to execute the onChanged edits, but just update their contents
    QSignalBlocker xdir(ui->XDirectionEdit);
    QSignalBlocker ydir(ui->YDirectionEdit);
    QSignalBlocker zdir(ui->ZDirectionEdit);
    ui->XDirectionEdit->setValue(extrude->Direction.getValue().x);
    ui->YDirectionEdit->setValue(extrude->Direction.getValue().y);
    ui->ZDirectionEdit->setValue(extrude->Direction.getValue().z);
}

void TaskExtrudeParameters::setDirectionMode(int index)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    // disable AlongSketchNormal when the direction is already normal
    if (index == DirectionModes::Normal)
        ui->checkBoxAlongDirection->setEnabled(false);
    else
        ui->checkBoxAlongDirection->setEnabled(true);

    // if custom direction is used, show it
    if (index == DirectionModes::Custom) {
        ui->checkBoxDirection->setChecked(true);
        extrude->UseCustomVector.setValue(true);
    }
    else {
        extrude->UseCustomVector.setValue(false);
    }

    // if we don't use custom direction, only allow to show its direction
    if (index != DirectionModes::Custom) {
        ui->XDirectionEdit->setEnabled(false);
        ui->YDirectionEdit->setEnabled(false);
        ui->ZDirectionEdit->setEnabled(false);
    }
    else {
        ui->XDirectionEdit->setEnabled(true);
        ui->YDirectionEdit->setEnabled(true);
        ui->ZDirectionEdit->setEnabled(true);
    }

}

void TaskExtrudeParameters::onMidplaneChanged(bool on)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->Midplane.setValue(on);
    ui->checkBoxReversed->setEnabled(!on);
    tryRecomputeFeature();
}

void TaskExtrudeParameters::onReversedChanged(bool on)
{
    PartDesign::FeatureExtrude* extrude = static_cast<PartDesign::FeatureExtrude*>(vp->getObject());
    extrude->Reversed.setValue(on);
    ui->checkBoxMidplane->setEnabled(!on);
    // update the direction
    tryRecomputeFeature();
    updateDirectionEdits();
}

void TaskExtrudeParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if (axesInList.empty())
        throw Base::RuntimeError("Not initialized!");

    int num = ui->directionCB->currentIndex();
    const App::PropertyLinkSub& lnk = *(axesInList[num]);
    if (!lnk.getValue()) {
        // Note: It is possible that a face of an object is directly padded/pocketed without defining a profile shape
        obj = nullptr;
        sub.clear();
    }
    else {
        PartDesign::ProfileBased* pcDirection = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        if (!pcDirection->getDocument()->isIn(lnk.getValue()))
            throw Base::RuntimeError("Object was deleted");

        obj = lnk.getValue();
        sub = lnk.getSubValues();
    }
}

void TaskExtrudeParameters::onButtonFace(const bool checked)
{
    if (!checked)
        handleLineFaceNameNo();
    else
        handleLineFaceNameClick(); // sets placeholder text

    // to distinguish that this is the direction selection
    selectionFace = true;

    // only faces are allowed
    TaskSketchBasedParameters::onSelectReference(checked ? AllowSelection::FACE : AllowSelection::NONE);
}

void TaskExtrudeParameters::onFaceName(const QString& text)
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
            QString uptoface = parts.join(QString::fromLatin1(":"));
            ui->lineFaceName->setProperty("FeatureName", name);
            ui->lineFaceName->setProperty("FaceName", setUpToFace(uptoface));
        }
        else {
            ui->lineFaceName->setProperty("FeatureName", QVariant());
            ui->lineFaceName->setProperty("FaceName", QVariant());
        }
    }
}

void TaskExtrudeParameters::translateFaceName()
{
    handleLineFaceNameNo();
    QVariant featureName = ui->lineFaceName->property("FeatureName");
    if (featureName.isValid()) {
        QStringList parts = ui->lineFaceName->text().split(QChar::fromLatin1(':'));
        QByteArray upToFace = ui->lineFaceName->property("FaceName").toByteArray();
        int faceId = -1;
        bool ok = false;
        if (upToFace.indexOf("Face") == 0) {
            faceId = upToFace.remove(0,4).toInt(&ok);
        }

        if (ok) {
            ui->lineFaceName->setText(QString::fromLatin1("%1:%2%3")
                                      .arg(parts[0], tr("Face")).arg(faceId));
        }
        else {
            ui->lineFaceName->setText(parts[0]);
        }
    }
}

double TaskExtrudeParameters::getOffset() const
{
    return ui->offsetEdit->value().getValue();
}

bool TaskExtrudeParameters::getAlongSketchNormal() const
{
    return ui->checkBoxAlongDirection->isChecked();
}

bool TaskExtrudeParameters::getCustom() const
{
    return (ui->directionCB->currentIndex() == DirectionModes::Custom);
}

std::string TaskExtrudeParameters::getReferenceAxis() const
{
    std::vector<std::string> sub;
    App::DocumentObject* obj;
    getReferenceAxis(obj, sub);
    return buildLinkSingleSubPythonStr(obj, sub);
}

double TaskExtrudeParameters::getXDirection() const
{
    return ui->XDirectionEdit->value();
}

double TaskExtrudeParameters::getYDirection() const
{
    return ui->YDirectionEdit->value();
}

double TaskExtrudeParameters::getZDirection() const
{
    return ui->ZDirectionEdit->value();
}

bool TaskExtrudeParameters::getReversed() const
{
    return ui->checkBoxReversed->isChecked();
}

bool TaskExtrudeParameters::getMidplane() const
{
    return ui->checkBoxMidplane->isChecked();
}

int TaskExtrudeParameters::getMode() const
{
    return ui->changeMode->currentIndex();
}

QString TaskExtrudeParameters::getFaceName() const
{
    QVariant featureName = ui->lineFaceName->property("FeatureName");
    if (featureName.isValid()) {
        QString faceName = ui->lineFaceName->property("FaceName").toString();
        return getFaceReference(featureName.toString(), faceName);
    }

    return QString::fromLatin1("None");
}

void TaskExtrudeParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        QSignalBlocker length(ui->lengthEdit);
        QSignalBlocker length2(ui->lengthEdit2);
        QSignalBlocker offset(ui->offsetEdit);
        QSignalBlocker taper(ui->taperEdit);
        QSignalBlocker taper2(ui->taperEdit2);
        QSignalBlocker xdir(ui->XDirectionEdit);
        QSignalBlocker ydir(ui->YDirectionEdit);
        QSignalBlocker zdir(ui->ZDirectionEdit);
        QSignalBlocker dir(ui->directionCB);
        QSignalBlocker face(ui->lineFaceName);
        QSignalBlocker mode(ui->changeMode);

        // Save all items
        QStringList items;
        for (int i = 0; i < ui->directionCB->count(); i++)
            items << ui->directionCB->itemText(i);

        // Translate direction items
        int index = ui->directionCB->currentIndex();
        ui->retranslateUi(proxy);

        // Keep custom items
        for (int i = 0; i < ui->directionCB->count(); i++)
            items.pop_front();
        ui->directionCB->addItems(items);
        ui->directionCB->setCurrentIndex(index);

        // Translate mode items
        translateModeList(ui->changeMode->currentIndex());

        translateFaceName();
    }
}

void TaskExtrudeParameters::saveHistory()
{
    // save the user values to history
    ui->lengthEdit->pushToHistory();
    ui->lengthEdit2->pushToHistory();
    ui->offsetEdit->pushToHistory();
    ui->taperEdit->pushToHistory();
    ui->taperEdit2->pushToHistory();
}

void TaskExtrudeParameters::applyParameters(QString facename)
{
    auto obj = vp->getObject();

    ui->lengthEdit->apply();
    ui->lengthEdit2->apply();
    ui->taperEdit->apply();
    ui->taperEdit2->apply();
    FCMD_OBJ_CMD(obj, "UseCustomVector = " << (getCustom() ? 1 : 0));
    FCMD_OBJ_CMD(obj, "Direction = ("
        << getXDirection() << ", " << getYDirection() << ", " << getZDirection() << ")");
    FCMD_OBJ_CMD(obj, "ReferenceAxis = " << getReferenceAxis());
    FCMD_OBJ_CMD(obj, "AlongSketchNormal = " << (getAlongSketchNormal() ? 1 : 0));
    FCMD_OBJ_CMD(obj, "Type = " << getMode());
    FCMD_OBJ_CMD(obj, "UpToFace = " << facename.toLatin1().data());
    FCMD_OBJ_CMD(obj, "Reversed = " << (getReversed() ? 1 : 0));
    FCMD_OBJ_CMD(obj, "Midplane = " << (getMidplane() ? 1 : 0));
    FCMD_OBJ_CMD(obj, "Offset = " << getOffset());
}

void TaskExtrudeParameters::onModeChanged(int)
{
    // implement in sub-class
}

void TaskExtrudeParameters::updateUI(int)
{
    // implement in sub-class
}

void TaskExtrudeParameters::translateModeList(int)
{
    // implement in sub-class
}

void TaskExtrudeParameters::handleLineFaceNameClick()
{
    ui->lineFaceName->setPlaceholderText(tr("Click on a face in the model"));
}

void TaskExtrudeParameters::handleLineFaceNameNo()
{
    ui->lineFaceName->setPlaceholderText(tr("No face selected"));
}

#include "moc_TaskExtrudeParameters.cpp"
