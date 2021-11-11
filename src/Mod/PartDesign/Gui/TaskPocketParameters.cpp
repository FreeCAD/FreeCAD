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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <Precision.hxx>
#endif

#include "ui_TaskPocketParameters.h"
#include "TaskPocketParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPocketParameters */

TaskPocketParameters::TaskPocketParameters(ViewProviderPocket *PocketView,QWidget *parent, bool newObj)
    : TaskSketchBasedParameters(PocketView, parent, "PartDesign_Pocket", tr("Pocket parameters"))
    , ui(new Ui_TaskPocketParameters)
    , oldLength(0)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
#if QT_VERSION >= 0x040700
    ui->lineFaceName->setPlaceholderText(tr("No face selected"));
#endif

    this->groupLayout()->addWidget(proxy);

    // set the history path
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength2"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketOffset"));

    // Get the feature data
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    Base::Quantity l = pcPocket->Length.getQuantityValue();
    Base::Quantity l2 = pcPocket->Length2.getQuantityValue();
    Base::Quantity off = pcPocket->Offset.getQuantityValue();
    bool alongNormal = pcPocket->AlongSketchNormal.getValue();
    bool useCustom = pcPocket->UseCustomVector.getValue();
    double xs = pcPocket->Direction.getValue().x;
    double ys = pcPocket->Direction.getValue().y;
    double zs = pcPocket->Direction.getValue().z;
    bool midplane = pcPocket->Midplane.getValue();
    bool reversed = pcPocket->Reversed.getValue();
    int index = pcPocket->Type.getValue(); // must extract value here, clear() kills it!
    App::DocumentObject* obj =  pcPocket->UpToFace.getValue();
    std::vector<std::string> subStrings = pcPocket->UpToFace.getSubValues();
    std::string upToFace;
    int faceId = -1;
    if ((obj != NULL) && !subStrings.empty()) {
        upToFace = subStrings.front();
        if (upToFace.substr(0,4) == "Face")
            faceId = std::atoi(&upToFace[4]);
    }

    // set decimals for the direction edits
    // do this here before the edits are filed to avoid rounding mistakes
    int UserDecimals = Base::UnitsApi::getDecimals();
    ui->XDirectionEdit->setDecimals(UserDecimals);
    ui->YDirectionEdit->setDecimals(UserDecimals);
    ui->ZDirectionEdit->setDecimals(UserDecimals);

    // Fill data into dialog elements
    // the direction combobox is later filled in updateUI()
    ui->lengthEdit->setValue(l);
    ui->lengthEdit2->setValue(l2);
    ui->offsetEdit->setValue(off);
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
    ui->checkBoxMidplane->setChecked(midplane);
    ui->checkBoxReversed->setChecked(reversed);

    // Set object labels
    if (obj && PartDesign::Feature::isDatum(obj)) {
        ui->lineFaceName->setText(QString::fromUtf8(obj->Label.getValue()));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
    }
    else if (obj && faceId >= 0) {
        ui->lineFaceName->setText(QString::fromLatin1("%1:%2%3")
                                  .arg(QString::fromUtf8(obj->Label.getValue()))
                                  .arg(tr("Face"))
                                  .arg(faceId));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
    }
    else {
        ui->lineFaceName->clear();
        ui->lineFaceName->setProperty("FeatureName", QVariant());
    }

    ui->lineFaceName->setProperty("FaceName", QByteArray(upToFace.c_str()));

    ui->changeMode->clear();
    ui->changeMode->insertItem(0, tr("Dimension"));
    ui->changeMode->insertItem(1, tr("Through all"));
    ui->changeMode->insertItem(2, tr("To first"));
    ui->changeMode->insertItem(3, tr("Up to face"));
    ui->changeMode->insertItem(4, tr("Two dimensions"));
    ui->changeMode->setCurrentIndex(index);

    // Bind input fields to properties
    ui->lengthEdit->bind(pcPocket->Length);
    ui->lengthEdit2->bind(pcPocket->Length2);
    ui->offsetEdit->bind(pcPocket->Offset);
    ui->XDirectionEdit->bind(App::ObjectIdentifier::parse(pcPocket, std::string("Direction.x")));
    ui->YDirectionEdit->bind(App::ObjectIdentifier::parse(pcPocket, std::string("Direction.y")));
    ui->ZDirectionEdit->bind(App::ObjectIdentifier::parse(pcPocket, std::string("Direction.z")));

    QMetaObject::connectSlotsByName(this);

    connect(ui->lengthEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));
    connect(ui->lengthEdit2, SIGNAL(valueChanged(double)),
            this, SLOT(onLength2Changed(double)));
    connect(ui->offsetEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onOffsetChanged(double)));
    connect(ui->directionCB, SIGNAL(activated(int)),
        this, SLOT(onDirectionCBChanged(int)));
    connect(ui->checkBoxAlongDirection, SIGNAL(toggled(bool)),
        this, SLOT(onAlongSketchNormalChanged(bool)));
    connect(ui->checkBoxDirection, SIGNAL(toggled(bool)),
        this, SLOT(onDirectionToggled(bool)));
    connect(ui->XDirectionEdit, SIGNAL(valueChanged(double)),
        this, SLOT(onXDirectionEditChanged(double)));
    connect(ui->YDirectionEdit, SIGNAL(valueChanged(double)),
        this, SLOT(onYDirectionEditChanged(double)));
    connect(ui->ZDirectionEdit, SIGNAL(valueChanged(double)),
        this, SLOT(onZDirectionEditChanged(double)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplaneChanged(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversedChanged(bool)));
    connect(ui->changeMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->buttonFace, SIGNAL(clicked()),
            this, SLOT(onButtonFace()));
    connect(ui->lineFaceName, SIGNAL(textEdited(QString)),
            this, SLOT(onFaceName(QString)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    this->propReferenceAxis = &(pcPocket->ReferenceAxis);

    // Due to signals attached after changes took took into effect we should update the UI now.
    updateUI(index);

    // if it is a newly created object use the last value of the history
    // TODO: newObj doesn't supplied normally by any caller (2015-07-24, Fat-Zer)
    if (newObj){
        ui->lengthEdit->setToLastUsedValue();
        ui->lengthEdit->selectNumber();
        ui->lengthEdit2->setToLastUsedValue();
        ui->lengthEdit2->selectNumber();
        ui->offsetEdit->setToLastUsedValue();
        ui->offsetEdit->selectNumber();
    }
}

void TaskPocketParameters::updateUI(int index)
{
    // update direction combobox
    fillDirectionCombo();

    // disable/hide everything unless we are sure we don't need it
    bool isLengthEditVisible  = false;
    bool isLengthEdit2Visible = false;    
    bool isOffsetEditVisible  = false;
    bool isOffsetEditEnabled  = true;
    bool isMidplateEnabled    = false;
    bool isReversedEnabled    = false;
    bool isFaceEditEnabled    = false;

    // dimension
    if (index == 0) {
        isLengthEditVisible = true;
        ui->lengthEdit->selectNumber();
        // Make sure that the spin box has the focus to get key events
        // Calling setFocus() directly doesn't work because the spin box is not
        // yet visible.
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        isMidplateEnabled = true;
        // Reverse only makes sense if Midplane is not true
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    // through all
    else if (index == 1) {
        isOffsetEditVisible = true;
        isOffsetEditEnabled = false; // offset may have some meaning for through all but it doesn't work
        isMidplateEnabled = true;
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    // up to first
    else if (index == 2) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;       // Will change the direction it seeks for its first face?
            // It may work not quite as expected but useful if sketch oriented upside-down.
            // (may happen in bodies)
            // FIXME: Fix probably lies somewhere in IF block on line 125 of FeaturePocket.cpp
    }
    // up to face
    else if (index == 3) {
        isOffsetEditVisible = true;
        isReversedEnabled = true;
        isFaceEditEnabled    = true;
        QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->property("FeatureName").isNull())
            onButtonFace(true);
    }
    // two dimensions
    else {
        isLengthEditVisible = true;
        isLengthEdit2Visible = true;
        isReversedEnabled = true;
    }    

    ui->lengthEdit->setVisible( isLengthEditVisible );
    ui->lengthEdit->setEnabled( isLengthEditVisible );
    ui->labelLength->setVisible( isLengthEditVisible );
    ui->checkBoxAlongDirection->setVisible(isLengthEditVisible);

    ui->lengthEdit2->setVisible( isLengthEdit2Visible );
    ui->lengthEdit2->setEnabled( isLengthEdit2Visible );
    ui->labelLength2->setVisible( isLengthEdit2Visible );

    ui->offsetEdit->setVisible( isOffsetEditVisible );
    ui->offsetEdit->setEnabled( isOffsetEditVisible && isOffsetEditEnabled );
    ui->labelOffset->setVisible( isOffsetEditVisible );

    ui->checkBoxMidplane->setEnabled( isMidplateEnabled );

    ui->checkBoxReversed->setEnabled( isReversedEnabled );

    ui->buttonFace->setEnabled( isFaceEditEnabled );
    ui->lineFaceName->setEnabled( isFaceEditEnabled );
    if (!isFaceEditEnabled) {
        onButtonFace(false);
    }
}

void TaskPocketParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // if we have an edge selection for the pocket direction
        if (!selectionFace) {
            std::vector<std::string> edge;
            App::DocumentObject* selObj;
            if (getReferencedSelection(vp->getObject(), msg, selObj, edge) && selObj) {
                exitSelectionMode();
                propReferenceAxis->setValue(selObj, edge);
                recomputeFeature();
                // update direction combobox
                fillDirectionCombo();
            }
        }
        else { // if we have a selection of a face
            QString refText = onAddSelection(msg);
            if (refText.length() > 0) {
                ui->lineFaceName->blockSignals(true);
                ui->lineFaceName->setText(refText);
                ui->lineFaceName->setProperty("FeatureName", QByteArray(msg.pObjectName));
                ui->lineFaceName->setProperty("FaceName", QByteArray(msg.pSubName));
                ui->lineFaceName->blockSignals(false);
                // Turn off reference selection mode
                onButtonFace(false);
            }
            else {
                ui->lineFaceName->blockSignals(true);
                ui->lineFaceName->clear();
                ui->lineFaceName->setProperty("FeatureName", QVariant());
                ui->lineFaceName->setProperty("FaceName", QVariant());
                ui->lineFaceName->blockSignals(false);
            }
        }
    } else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        ui->lineFaceName->blockSignals(true);
        ui->lineFaceName->clear();
        ui->lineFaceName->setProperty("FeatureName", QVariant());
        ui->lineFaceName->setProperty("FaceName", QVariant());
        ui->lineFaceName->blockSignals(false);
    }
}

void TaskPocketParameters::onLengthChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Length.setValue(len);
    recomputeFeature();
}

void TaskPocketParameters::onLength2Changed(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Length2.setValue(len);
    recomputeFeature();
}

void TaskPocketParameters::onOffsetChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Offset.setValue(len);
    recomputeFeature();
}

void TaskPocketParameters::fillDirectionCombo()
{
    bool oldVal_blockUpdate = blockUpdate;
    blockUpdate = true;

    if (axesInList.empty()) {
        ui->directionCB->clear();
        // add sketch normal
        PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        Part::Part2DObject* pcSketch = dynamic_cast<Part::Part2DObject*>(pcFeat->Profile.getValue());
        if (pcSketch)
            addAxisToCombo(pcSketch, "N_Axis", QObject::tr("Sketch normal"));
        // add the other entries
        addAxisToCombo(0, std::string(), tr("Select reference..."));
        // we start with the sketch normal as proposal for the custom direction
        if (pcSketch)
            addAxisToCombo(pcSketch, "N_Axis", QObject::tr("Custom direction"));
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
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    bool hasCustom = pcPocket->UseCustomVector.getValue();
    if (indexOfCurrent != -1 && !hasCustom)
        ui->directionCB->setCurrentIndex(indexOfCurrent);
    if (hasCustom)
        ui->directionCB->setCurrentIndex(2);

    blockUpdate = oldVal_blockUpdate;
}

void TaskPocketParameters::addAxisToCombo(App::DocumentObject* linkObj,
    std::string linkSubname, QString itemText)
{
    this->ui->directionCB->addItem(itemText);
    this->axesInList.emplace_back(new App::PropertyLinkSub);
    App::PropertyLinkSub& lnk = *(axesInList.back());
    lnk.setValue(linkObj, std::vector<std::string>(1, linkSubname));
}

void TaskPocketParameters::onDirectionCBChanged(int num)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());

    if (axesInList.empty() || !pcPocket)
        return;

    App::PropertyLinkSub& lnk = *(axesInList[num]);
    if (lnk.getValue() == 0) {
        // enter reference selection mode
        this->blockConnection(false);
        // to distinguish that this is the direction selection
        selectionFace = false;
        TaskSketchBasedParameters::onSelectReference(true, true, false, true, true);
        return;
    }
    else {
        if (!pcPocket->getDocument()->isIn(lnk.getValue())) {
            Base::Console().Error("Object was deleted\n");
            return;
        }
        propReferenceAxis->Paste(lnk);
        // in case user is in selection mode, but changed his mind before selecting anything
        exitSelectionMode();
    }

    try {
        recomputeFeature();
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }

    // disable AlongSketchNormal when the direction is already normal
    if (num == 0)
        ui->checkBoxAlongDirection->setEnabled(false);
    else
        ui->checkBoxAlongDirection->setEnabled(true);
    // if custom direction is used, show it
    if (num == 2) {
        ui->checkBoxDirection->setChecked(true);
        PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
        pcPocket->UseCustomVector.setValue(true);
    }
    else {
        ui->checkBoxDirection->setChecked(false);
        pcPocket->UseCustomVector.setValue(false);
    }
    // if we dont use custom direction, only allow to show its direction
    if (num != 2) {
        ui->XDirectionEdit->setEnabled(false);
        ui->YDirectionEdit->setEnabled(false);
        ui->ZDirectionEdit->setEnabled(false);
    }
    else {
        ui->XDirectionEdit->setEnabled(true);
        ui->YDirectionEdit->setEnabled(true);
        ui->ZDirectionEdit->setEnabled(true);
    }
    // recompute and update the direction
    recomputeFeature();
    updateDirectionEdits();
}

void TaskPocketParameters::onAlongSketchNormalChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->AlongSketchNormal.setValue(on);
    recomputeFeature();
}

void TaskPocketParameters::onDirectionToggled(bool on)
{
    if (on)
        ui->groupBoxDirection->show();
    else
        ui->groupBoxDirection->hide();
}

void TaskPocketParameters::onXDirectionEditChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Direction.setValue(len, pcPocket->Direction.getValue().y, pcPocket->Direction.getValue().z);
    recomputeFeature();
    // checking for case of a null vector is done in FeaturePocket.cpp
    // if there was a null vector, the normal vector of the sketch is used.
    // therefore the vector component edits must be updated
    updateDirectionEdits();
}

void TaskPocketParameters::onYDirectionEditChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Direction.setValue(pcPocket->Direction.getValue().x, len, pcPocket->Direction.getValue().z);
    recomputeFeature();
    updateDirectionEdits();
}

void TaskPocketParameters::onZDirectionEditChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Direction.setValue(pcPocket->Direction.getValue().x, pcPocket->Direction.getValue().y, len);
    recomputeFeature();
    updateDirectionEdits();
}

void TaskPocketParameters::updateDirectionEdits(void)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    // we don't want to execute the onChanged edits, but just update their contents
    ui->XDirectionEdit->blockSignals(true);
    ui->YDirectionEdit->blockSignals(true);
    ui->ZDirectionEdit->blockSignals(true);
    ui->XDirectionEdit->setValue(pcPocket->Direction.getValue().x);
    ui->YDirectionEdit->setValue(pcPocket->Direction.getValue().y);
    ui->ZDirectionEdit->setValue(pcPocket->Direction.getValue().z);
    ui->XDirectionEdit->blockSignals(false);
    ui->YDirectionEdit->blockSignals(false);
    ui->ZDirectionEdit->blockSignals(false);
}

void TaskPocketParameters::onMidplaneChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Midplane.setValue(on);
    ui->checkBoxReversed->setEnabled(!on);
    recomputeFeature();
}

void TaskPocketParameters::onReversedChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Reversed.setValue(on);
    recomputeFeature();
}

void TaskPocketParameters::onModeChanged(int index)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());

    switch (index) {
        case 0:
            // Why? See below for "UpToFace"
            if (oldLength < Precision::Confusion())
                oldLength = 5.0;
            pcPocket->Length.setValue(oldLength);
            ui->lengthEdit->setValue(oldLength);
            pcPocket->Type.setValue("Length");
            break;
        case 1:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("ThroughAll");
            break;
        case 2:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFirst");
            break;
        case 3:
            // Because of the code at the beginning of Pocket::execute() which is used to detect
            // broken legacy parts, we must set the length to zero here!
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFace");
            pcPocket->Length.setValue(0.0);
            ui->lengthEdit->setValue(0.0);
            break;
        default: 
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("TwoLengths");
    }

    updateUI(index);
    recomputeFeature();
}

void TaskPocketParameters::onButtonFace(const bool pressed)
{
    this->blockConnection(!pressed);

    TaskSketchBasedParameters::onSelectReference(pressed, false, true, false);

    // Update button if onButtonFace() is called explicitly
    ui->buttonFace->setChecked(pressed);
}

void TaskPocketParameters::onFaceName(const QString& text)
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

double TaskPocketParameters::getLength(void) const
{
    return ui->lengthEdit->value().getValue();
}

double TaskPocketParameters::getLength2(void) const
{
    return ui->lengthEdit2->value().getValue();
}

double TaskPocketParameters::getOffset(void) const
{
    return ui->offsetEdit->value().getValue();
}

bool   TaskPocketParameters::getAlongSketchNormal(void) const
{
    return ui->checkBoxAlongDirection->isChecked();
}

bool   TaskPocketParameters::getCustom(void) const
{
    return ui->checkBoxDirection->isChecked();
}

std::string TaskPocketParameters::getReferenceAxis(void) const
{
    std::vector<std::string> sub;
    App::DocumentObject* obj;
    getReferenceAxis(obj, sub);
    return buildLinkSingleSubPythonStr(obj, sub);
}

double TaskPocketParameters::getXDirection(void) const
{
    return ui->XDirectionEdit->value();
}

double TaskPocketParameters::getYDirection(void) const
{
    return ui->YDirectionEdit->value();
}

double TaskPocketParameters::getZDirection(void) const
{
    return ui->ZDirectionEdit->value();
}

bool   TaskPocketParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

bool   TaskPocketParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

int TaskPocketParameters::getMode(void) const
{
    return ui->changeMode->currentIndex();
}

QString TaskPocketParameters::getFaceName(void) const
{
    // 'Up to face' mode
    if (getMode() == 3) {
        QVariant featureName = ui->lineFaceName->property("FeatureName");
        if (featureName.isValid()) {
            QString faceName = ui->lineFaceName->property("FaceName").toString();
            return getFaceReference(featureName.toString(), faceName);
        }
    }
    return QString::fromLatin1("None");
}

TaskPocketParameters::~TaskPocketParameters()
{
}

void TaskPocketParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->lengthEdit->blockSignals(true);
        ui->lengthEdit2->blockSignals(true);
        ui->offsetEdit->blockSignals(true);
        ui->XDirectionEdit->blockSignals(true);
        ui->YDirectionEdit->blockSignals(true);
        ui->ZDirectionEdit->blockSignals(true);
        ui->directionCB->blockSignals(true);
        int index = ui->directionCB->currentIndex();
        ui->directionCB->clear();
        ui->directionCB->addItem(tr("Sketch normal"));
        ui->directionCB->addItem(tr("Select reference..."));
        ui->directionCB->addItem(tr("Custom direction"));
        ui->directionCB->setCurrentIndex(index);
        ui->lineFaceName->blockSignals(true);
        ui->changeMode->blockSignals(true);
        index = ui->changeMode->currentIndex();
        ui->retranslateUi(proxy);
        ui->changeMode->clear();
        ui->changeMode->addItem(tr("Dimension"));
        ui->changeMode->addItem(tr("Through all"));
        ui->changeMode->addItem(tr("To first"));
        ui->changeMode->addItem(tr("Up to face"));
        ui->changeMode->addItem(tr("Two dimensions"));
        ui->changeMode->setCurrentIndex(index);

#if QT_VERSION >= 0x040700
        ui->lineFaceName->setPlaceholderText(tr("No face selected"));
#endif
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
                                          .arg(parts[0])
                                          .arg(tr("Face"))
                                          .arg(faceId));
            }
            else {
                ui->lineFaceName->setText(parts[0]);
            }
        }

        ui->lengthEdit->blockSignals(false);
        ui->lengthEdit2->blockSignals(false);
        ui->offsetEdit->blockSignals(false);
        ui->lineFaceName->blockSignals(false);
        ui->changeMode->blockSignals(false);
    }
}

void TaskPocketParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if (axesInList.empty())
        throw Base::RuntimeError("Not initialized!");

    int num = ui->directionCB->currentIndex();
    const App::PropertyLinkSub& lnk = *(axesInList[num]);
    if (lnk.getValue() == 0) {
        // Note: Is is possible that a face of an object is directly pocketed without defining a profile shape
        obj = nullptr;
        sub.clear();
        //throw Base::RuntimeError("Still in reference selection mode; reference wasn't selected yet");
    }
    else {
        PartDesign::ProfileBased* pcDirection = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        if (!pcDirection->getDocument()->isIn(lnk.getValue()))
            throw Base::RuntimeError("Object was deleted");

        obj = lnk.getValue();
        sub = lnk.getSubValues();
    }
}

void TaskPocketParameters::saveHistory(void)
{
    // save the user values to history
    ui->lengthEdit->pushToHistory();
    ui->lengthEdit2->pushToHistory();
    ui->offsetEdit->pushToHistory();
}

void TaskPocketParameters::apply()
{
    auto obj = vp->getObject();

    ui->lengthEdit->apply();
    ui->lengthEdit2->apply();

    FCMD_OBJ_CMD(obj,"Type = " << getMode());
    QString facename = getFaceName();
    FCMD_OBJ_CMD(obj,"UpToFace = " << facename.toLatin1().data());
    FCMD_OBJ_CMD(obj,"Reversed = " << (getReversed()?1:0));
    FCMD_OBJ_CMD(obj,"Midplane = " << (getMidplane()?1:0));
    FCMD_OBJ_CMD(obj,"Offset = " << getOffset());
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPocketParameters::TaskDlgPocketParameters(ViewProviderPocket *PocketView)
    : TaskDlgSketchBasedParameters(PocketView)
{
    assert(vp);
    Content.push_back ( new TaskPocketParameters(PocketView ) );
}

#include "moc_TaskPocketParameters.cpp"
