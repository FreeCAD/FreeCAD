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
#include <QSignalBlocker>
#include <QAction>
#endif

#include <App/Document.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Gui/Command.h>
#include <Gui/Tools.h>
#include <Mod/PartDesign/App/FeatureExtrude.h>

#include "ui_TaskPadPocketParameters.h"
#include "TaskExtrudeParameters.h"
#include "TaskTransformedParameters.h"
#include "ReferenceSelection.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskExtrudeParameters */

TaskExtrudeParameters::TaskExtrudeParameters(ViewProviderExtrude* SketchBasedView,
                                             QWidget* parent,
                                             const std::string& pixmapname,
                                             const QString& parname)
    : TaskSketchBasedParameters(SketchBasedView, parent, pixmapname, parname)
    , propReferenceAxis(nullptr)
    , ui(new Ui_TaskPadPocketParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    handleLineFaceNameNo(ui->lineFaceName);
    handleLineFaceNameNo(ui->lineFaceName2);

    Gui::ButtonGroup* group = new Gui::ButtonGroup(this);
    group->addButton(ui->checkBoxReversed);
    group->setExclusive(true);

    this->groupLayout()->addWidget(proxy);
}

TaskExtrudeParameters::~TaskExtrudeParameters() = default;

void TaskExtrudeParameters::setupDialog()
{
    auto createRemoveAction = [this]() -> QAction* {
        auto action = new QAction(tr("Remove"), this);
        action->setShortcut(Gui::QtTools::deleteKeySequence());
        action->setShortcutVisibleInContextMenu(true);
        return action;
    };

    unselectShapeFaceAction = createRemoveAction();
    unselectShapeFaceAction2 = createRemoveAction();

    createSideControllers();

    // --- Global, Non-Side-Specific Setup ---
    auto extrude = getObject<PartDesign::FeatureExtrude>();

    int UserDecimals = Base::UnitsApi::getDecimals();
    ui->XDirectionEdit->setDecimals(UserDecimals);
    ui->YDirectionEdit->setDecimals(UserDecimals);
    ui->ZDirectionEdit->setDecimals(UserDecimals);

    ui->checkBoxAlongDirection->setChecked(extrude->AlongSketchNormal.getValue());
    ui->checkBoxDirection->setChecked(extrude->UseCustomVector.getValue());
    onDirectionToggled(ui->checkBoxDirection->isChecked());

    ui->XDirectionEdit->setValue(extrude->Direction.getValue().x);
    ui->YDirectionEdit->setValue(extrude->Direction.getValue().y);
    ui->ZDirectionEdit->setValue(extrude->Direction.getValue().z);

    ui->XDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, "Direction.x"));
    ui->YDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, "Direction.y"));
    ui->ZDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, "Direction.z"));

    ui->checkBoxReversed->setChecked(extrude->Reversed.getValue());

    // --- Per-Side Setup using the Helper ---
    setupSideDialog(m_side1);
    setupSideDialog(m_side2);

    // --- Final Global UI State Setup ---
    translateSidesList(extrude->SideType.getValue());

    connectSlots();

    this->propReferenceAxis = &(extrude->ReferenceAxis);
    updateUI(Side::First);
}

void TaskExtrudeParameters::setupSideDialog(SideController& side)
{
    // --- Get initial values from the correct side's properties ---
    Base::Quantity length = side.Length->getQuantityValue();
    Base::Quantity offset = side.Offset->getQuantityValue();
    Base::Quantity taper = side.TaperAngle->getQuantityValue();
    int typeIndex = side.Type->getValue();

    // --- Set up UI widgets with initial values ---
    side.lengthEdit->setValue(length);
    side.offsetEdit->setValue(offset);
    side.taperEdit->setMinimum(side.TaperAngle->getMinimum());
    side.taperEdit->setMaximum(side.TaperAngle->getMaximum());
    side.taperEdit->setSingleStep(side.TaperAngle->getStepSize());
    side.taperEdit->setValue(taper);

    // --- Bind UI widgets to the correct properties ---
    side.lengthEdit->bind(*side.Length);
    side.offsetEdit->bind(*side.Offset);
    side.taperEdit->bind(*side.TaperAngle);

    // --- Handle "Up to face" label logic ---
    App::DocumentObject* faceObj = side.UpToFace->getValue();
    std::vector<std::string> subStrings = side.UpToFace->getSubValues();
    std::string upToFaceName;
    int faceId = -1;
    if (faceObj && !subStrings.empty()) {
        upToFaceName = subStrings.front();
        if (upToFaceName.rfind("Face", 0) == 0) {  // starts_with
            faceId = std::atoi(&upToFaceName[4]);
        }
    }

    if (faceObj && PartDesign::Feature::isDatum(faceObj)) {
        side.lineFaceName->setText(QString::fromUtf8(faceObj->Label.getValue()));
        side.lineFaceName->setProperty("FeatureName", QByteArray(faceObj->getNameInDocument()));
    }
    else if (faceObj && faceId >= 0) {
        side.lineFaceName->setText(
            QStringLiteral("%1:%2%3").arg(QString::fromUtf8(faceObj->Label.getValue()),
                                          tr("Face"),
                                          QString::number(faceId)));
        side.lineFaceName->setProperty("FeatureName", QByteArray(faceObj->getNameInDocument()));
    }
    else {
        side.lineFaceName->clear();
        side.lineFaceName->setProperty("FeatureName", QVariant());
    }
    side.lineFaceName->setProperty("FaceName", QByteArray(upToFaceName.c_str()));

    // --- Update shape-related UI ---
    updateShapeName(side.lineShapeName, *side.UpToShape);
    updateShapeFaces(side.listWidgetReferences, *side.UpToShape);

    // --- Set up the mode combobox and list widget context menu ---
    translateModeList(side.changeMode, typeIndex);

    side.listWidgetReferences->addAction(side.unselectShapeFaceAction);
    side.listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
    side.checkBoxAllFaces->setChecked(side.listWidgetReferences->count() == 0);
}

void TaskExtrudeParameters::createSideControllers()
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();

    // --- Initialize Side 1 Controller ---
    m_side1.changeMode = ui->changeMode;
    m_side1.labelLength = ui->labelLength;
    m_side1.labelOffset = ui->labelOffset;
    m_side1.labelTaperAngle = ui->labelTaperAngle;
    m_side1.lengthEdit = ui->lengthEdit;
    m_side1.offsetEdit = ui->offsetEdit;
    m_side1.taperEdit = ui->taperEdit;
    m_side1.lineFaceName = ui->lineFaceName;
    m_side1.buttonFace = ui->buttonFace;
    m_side1.lineShapeName = ui->lineShapeName;
    m_side1.buttonShape = ui->buttonShape;
    m_side1.listWidgetReferences = ui->listWidgetReferences;
    m_side1.buttonShapeFace = ui->buttonShapeFace;
    m_side1.checkBoxAllFaces = ui->checkBoxAllFaces;
    m_side1.upToShapeList = ui->upToShapeList;
    m_side1.upToShapeFaces = ui->upToShapeFaces;
    m_side1.unselectShapeFaceAction = unselectShapeFaceAction;

    m_side1.Type = &extrude->Type;
    m_side1.Length = &extrude->Length;
    m_side1.Offset = &extrude->Offset;
    m_side1.TaperAngle = &extrude->TaperAngle;
    m_side1.UpToFace = &extrude->UpToFace;
    m_side1.UpToShape = &extrude->UpToShape;

    // --- Initialize Side 2 Controller ---
    m_side2.changeMode = ui->changeMode2;
    m_side2.labelLength = ui->labelLength2;
    m_side2.labelOffset = ui->labelOffset2;
    m_side2.labelTaperAngle = ui->labelTaperAngle2;
    m_side2.lengthEdit = ui->lengthEdit2;
    m_side2.offsetEdit = ui->offsetEdit2;
    m_side2.taperEdit = ui->taperEdit2;
    m_side2.lineFaceName = ui->lineFaceName2;
    m_side2.buttonFace = ui->buttonFace2;
    m_side2.lineShapeName = ui->lineShapeName2;
    m_side2.buttonShape = ui->buttonShape2;
    m_side2.listWidgetReferences = ui->listWidgetReferences2;
    m_side2.buttonShapeFace = ui->buttonShapeFace2;
    m_side2.checkBoxAllFaces = ui->checkBoxAllFaces2;
    m_side2.upToShapeList = ui->upToShapeList2;
    m_side2.upToShapeFaces = ui->upToShapeFaces2;
    m_side2.unselectShapeFaceAction = unselectShapeFaceAction2;

    m_side2.Type = &extrude->Type2;
    m_side2.Length = &extrude->Length2;
    m_side2.Offset = &extrude->Offset2;
    m_side2.TaperAngle = &extrude->TaperAngle2;
    m_side2.UpToFace = &extrude->UpToFace2;
    m_side2.UpToShape = &extrude->UpToShape2;
}

void TaskExtrudeParameters::readValuesFromHistory()
{
    ui->lengthEdit->setToLastUsedValue();
    ui->lengthEdit->selectNumber();
    ui->lengthEdit2->setToLastUsedValue();
    ui->lengthEdit2->selectNumber();
    ui->offsetEdit->setToLastUsedValue();
    ui->offsetEdit->selectNumber();
    ui->offsetEdit2->setToLastUsedValue();
    ui->offsetEdit2->selectNumber();
    ui->taperEdit->setToLastUsedValue();
    ui->taperEdit->selectNumber();
    ui->taperEdit2->setToLastUsedValue();
    ui->taperEdit2->selectNumber();
}

void TaskExtrudeParameters::connectSlots()
{
    QMetaObject::connectSlotsByName(this);

    auto connectSideSlots = [this](auto& side, Side sideEnum, auto modeChangedSlot) {
        connect(side.lengthEdit,
                qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
                this,
                [this, sideEnum](double val) {
                    onLengthChanged(val, sideEnum);
                });
        connect(side.offsetEdit,
                qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
                this,
                [this, sideEnum](double val) {
                    onOffsetChanged(val, sideEnum);
                });
        connect(side.taperEdit,
                qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
                this,
                [this, sideEnum](double val) {
                    onTaperChanged(val, sideEnum);
                });
        connect(side.changeMode,
                qOverload<int>(&QComboBox::currentIndexChanged),
                this,
                modeChangedSlot);
        connect(side.buttonFace, &QToolButton::toggled, this, [this, sideEnum](bool checked) {
            onSelectFaceToggle(checked, sideEnum);
        });
        connect(side.lineFaceName,
                &QLineEdit::textEdited,
                this,
                [this, sideEnum](const QString& text) {
                    onFaceName(text, sideEnum);
                });
        connect(side.checkBoxAllFaces, &QCheckBox::toggled, this, [this, sideEnum](bool checked) {
            onAllFacesToggled(checked, sideEnum);
        });
        connect(side.buttonShape, &QToolButton::toggled, this, [this, sideEnum](bool checked) {
            onSelectShapeToggle(checked, sideEnum);
        });
        connect(side.buttonShapeFace, &QToolButton::toggled, this, [this, sideEnum](bool checked) {
            onSelectShapeFacesToggle(checked, sideEnum);
        });
        connect(side.unselectShapeFaceAction, &QAction::triggered, this, [this, sideEnum]() {
            onUnselectShapeFacesTrigger(sideEnum);
        });
    };

    // Use the lambda to connect slots for both sides
    connectSideSlots(m_side1, Side::First, &TaskExtrudeParameters::onModeChanged_Side1);
    connectSideSlots(m_side2, Side::Second, &TaskExtrudeParameters::onModeChanged_Side2);

    // clang-format off
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
    connect(ui->checkBoxReversed, &QCheckBox::toggled,
            this, &TaskExtrudeParameters::onReversedChanged);
    connect(ui->sidesMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskExtrudeParameters::onSidesModeChanged);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskExtrudeParameters::onUpdateView);
    // clang-format on
}

void TaskExtrudeParameters::onModeChanged_Side1(int index)
{
    onModeChanged(index, Side::First);
}

void TaskExtrudeParameters::onModeChanged_Side2(int index)
{
    onModeChanged(index, Side::Second);
}

void TaskExtrudeParameters::onSelectShapeFacesToggle(bool checked, Side side)
{
    auto& sideCtrl = getSideController(side);
    if (checked) {
        setSelectionMode(SelectShapeFaces, side);
        sideCtrl.buttonShapeFace->setText(tr("Preview"));
    }
    else {
        setSelectionMode(None);
        sideCtrl.buttonShapeFace->setText(tr("Select Faces"));
    }
}

void PartDesignGui::TaskExtrudeParameters::onUnselectShapeFacesTrigger(Side side)
{
    auto& sideCtrl = getSideController(side);

    auto selected = sideCtrl.listWidgetReferences->selectedItems();
    auto faces = getShapeFaces(*sideCtrl.UpToShape);


    faces.erase(std::remove_if(faces.begin(), faces.end(), [selected](const std::string& face) {
        for (auto& item : selected) {
            if (item->text().toStdString() == face) {
                return true;
            }
        }

        return false;
    }));

    sideCtrl.UpToShape->setValue(sideCtrl.UpToShape->getValue(), faces);

    updateShapeFaces(sideCtrl.listWidgetReferences, *sideCtrl.UpToShape);
}

void TaskExtrudeParameters::setSelectionMode(SelectionMode mode, Side side)
{
    if (selectionMode == mode && activeSelectionSide == side) {
        return;
    }

    const auto updateCheckedForSide = [mode, side](Side relatedSide,
                                                QAbstractButton* buttonFace,
                                                QAbstractButton* buttonShape,
                                                QAbstractButton* buttonShapeFace) {
        buttonFace->setChecked(mode == SelectFace && side == relatedSide);
        buttonShape->setChecked(mode == SelectShape && side == relatedSide);
        buttonShapeFace->setChecked(mode == SelectShapeFaces && side == relatedSide);
    };
    updateCheckedForSide(Side::First, ui->buttonFace, ui->buttonShape, ui->buttonShapeFace);
    updateCheckedForSide(Side::Second, ui->buttonFace2, ui->buttonShape2, ui->buttonShapeFace2);

    selectionMode = mode;
    activeSelectionSide = side;

    switch (mode) {
        case SelectShape:
            onSelectReference(AllowSelection::WHOLE);
            Gui::Selection().addSelectionGate(
                new SelectionFilterGate("SELECT Part::Feature COUNT 1"));
            break;
        case SelectFace:
            onSelectReference(AllowSelection::FACE);
            break;
        case SelectShapeFaces: {
            onSelectReference(AllowSelection::FACE);
            auto& sideCtrl = getSideController(activeSelectionSide);
            getViewObject<ViewProviderExtrude>()->highlightShapeFaces(
                getShapeFaces(*sideCtrl.UpToShape));
            break;
        }
        case SelectReferenceAxis:
            onSelectReference(AllowSelection::EDGE | AllowSelection::PLANAR
                              | AllowSelection::CIRCLE);
            break;
        default:
            getViewObject<ViewProviderExtrude>()->highlightShapeFaces({});
            onSelectReference(AllowSelection::NONE);
    }
}

void TaskExtrudeParameters::tryRecomputeFeature()
{
    try {
        // recompute and update the direction
        recomputeFeature();
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskExtrudeParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    auto& sideCtrl = getSideController(activeSelectionSide);

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        switch (selectionMode) {
            case SelectShape:
                selectedShape(msg, sideCtrl);
                break;
            case SelectShapeFaces:
                selectedShapeFace(msg, sideCtrl);
                break;

            case SelectFace:
                selectedFace(msg, sideCtrl);
                break;

            case SelectReferenceAxis:
                selectedReferenceAxis(msg);
                break;

            default:
                // no-op
                break;
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        if (selectionMode == SelectFace) {
            clearFaceName(sideCtrl.lineFaceName);
        }
    }
}

void TaskExtrudeParameters::selectedReferenceAxis(const Gui::SelectionChanges& msg)
{
    std::vector<std::string> edge;
    App::DocumentObject* selObj;

    if (getReferencedSelection(getObject(), msg, selObj, edge) && selObj) {
        setSelectionMode(None);

        propReferenceAxis->setValue(selObj, edge);
        tryRecomputeFeature();

        // update direction combobox
        fillDirectionCombo();
    }
}

void TaskExtrudeParameters::selectedShapeFace(const Gui::SelectionChanges& msg,
                                              SideController& side)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto document = extrude->getDocument();

    if (strcmp(msg.pDocName, document->getName()) != 0) {
        return;
    }

    // Get the base shape from the correct side's property
    auto base = static_cast<Part::Feature*>(side.UpToShape->getValue());
    if (!base) {
        base = static_cast<Part::Feature*>(extrude);
    }
    else if (strcmp(msg.pObjectName, base->getNameInDocument()) != 0) {
        return;
    }

    std::vector<std::string> faces = getShapeFaces(*side.UpToShape);
    const std::string subName(msg.pSubName);

    if (subName.empty()) {
        return;
    }

    // Add or remove the face from the list
    if (const auto positionInList = std::ranges::find(faces, subName);
        positionInList != faces.end()) {
        faces.erase(positionInList);  // Remove it if it exists
    }
    else {
        faces.push_back(subName);  // Add it if it's new
    }

    side.UpToShape->setValue(base, faces);

    updateShapeFaces(side.listWidgetReferences, *side.UpToShape);

    tryRecomputeFeature();
}

void PartDesignGui::TaskExtrudeParameters::selectedFace(const Gui::SelectionChanges& msg,
                                                        SideController& side)
{
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

    setSelectionMode(None);
}

void PartDesignGui::TaskExtrudeParameters::selectedShape(const Gui::SelectionChanges& msg,
                                                         SideController& side)
{
    auto document = getObject()->getDocument();

    if (strcmp(msg.pDocName, document->getName()) != 0) {
        return;
    }

    Gui::Selection().clearSelection();

    auto ref = document->getObject(msg.pObjectName);

    side.UpToShape->setValue(ref);

    side.checkBoxAllFaces->setChecked(true);

    setSelectionMode(None);

    updateShapeName(side.lineShapeName, *side.UpToShape);
    updateShapeFaces(side.listWidgetReferences, *side.UpToShape);

    tryRecomputeFeature();
}

void TaskExtrudeParameters::clearFaceName(QLineEdit* lineEdit)
{
    QSignalBlocker block(lineEdit);
    lineEdit->clear();
    lineEdit->setProperty("FeatureName", QVariant());
    lineEdit->setProperty("FaceName", QVariant());
}

void TaskExtrudeParameters::updateShapeName(QLineEdit* lineEdit, App::PropertyLinkSubList& prop)
{
    QSignalBlocker block(lineEdit);

    auto shape = prop.getValue();

    if (shape) {
        lineEdit->setText(QString::fromStdString(shape->getFullName()));
    }
    else {
        lineEdit->setText({});
        lineEdit->setPlaceholderText(tr("No shape selected"));
    }
}

void TaskExtrudeParameters::updateShapeFaces(QListWidget* list, App::PropertyLinkSubList& prop)
{
    auto faces = getShapeFaces(prop);

    list->clear();
    for (auto& ref : faces) {
        list->addItem(QString::fromStdString(ref));
    }

    if (selectionMode == SelectShapeFaces) {
        getViewObject<ViewProviderExtrude>()->highlightShapeFaces(faces);
    }
}

std::vector<std::string> PartDesignGui::TaskExtrudeParameters::getShapeFaces(App::PropertyLinkSubList& prop)
{
    std::vector<std::string> faces;

    auto allRefs = prop.getSubValues();

    std::copy_if(allRefs.begin(),
                 allRefs.end(),
                 std::back_inserter(faces),
                 [](const std::string& ref) {
                     return boost::starts_with(ref, "Face");
                 });

    return faces;
}

void TaskExtrudeParameters::onLengthChanged(double len, Side side)
{
    getSideController(side).Length->setValue(len);
    tryRecomputeFeature();
}

void TaskExtrudeParameters::onOffsetChanged(double len, Side side)
{
    getSideController(side).Offset->setValue(len);
    tryRecomputeFeature();
}

void TaskExtrudeParameters::onTaperChanged(double angle, Side side)
{
    getSideController(side).TaperAngle->setValue(angle);
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
    Base::StateLocker lock(getUpdateBlockRef(), true);

    if (axesInList.empty()) {
        bool hasFace = false;
        ui->directionCB->clear();
        // we can have sketches or faces
        // for sketches just get the sketch normal
        auto pcFeat = getObject<PartDesign::ProfileBased>();
        Part::Part2DObject* pcSketch =
            dynamic_cast<Part::Part2DObject*>(pcFeat->Profile.getValue());
        // for faces we test if it is verified and if we can get its normal
        if (!pcSketch) {
            hasFace = hasProfileFace(pcFeat);
        }

        if (pcSketch) {
            addAxisToCombo(pcSketch, "N_Axis", tr("Sketch normal"));
        }
        else if (hasFace) {
            addAxisToCombo(pcFeat->Profile.getValue(), std::string(), tr("Face normal"), false);
        }

        // add the other entries
        addAxisToCombo(nullptr, std::string(), tr("Select reference…"));

        // we start with the sketch normal as proposal for the custom direction
        if (pcSketch) {
            addAxisToCombo(pcSketch, "N_Axis", tr("Custom direction"));
        }
        else if (hasFace) {
            addAxisToCombo(pcFeat->Profile.getValue(),
                           std::string(),
                           tr("Custom direction"),
                           false);
        }
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
        if (!subList.empty()) {
            sub = subList[0];
        }
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
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    bool hasCustom = extrude->UseCustomVector.getValue();
    if (indexOfCurrent != -1 && !hasCustom) {
        ui->directionCB->setCurrentIndex(indexOfCurrent);
    }
    if (hasCustom) {
        ui->directionCB->setCurrentIndex(DirectionModes::Custom);
    }
}

void TaskExtrudeParameters::addAxisToCombo(App::DocumentObject* linkObj,
                                           std::string linkSubname,
                                           QString itemText,
                                           bool hasSketch)
{
    this->ui->directionCB->addItem(itemText);
    this->axesInList.emplace_back(new App::PropertyLinkSub);
    App::PropertyLinkSub& lnk = *(axesInList.back());
    // if we have a face, we leave the link empty since we cannot
    // store the face normal as sublink
    if (hasSketch) {
        lnk.setValue(linkObj, std::vector<std::string>(1, linkSubname));
    }
}

void TaskExtrudeParameters::updateWholeUI(Type type, Side side)
{
    SidesMode sidesMode = static_cast<SidesMode>(ui->sidesMode->currentIndex());
    Mode mode1 = static_cast<Mode>(ui->changeMode->currentIndex());
    Mode mode2 = static_cast<Mode>(ui->changeMode2->currentIndex());

    // --- Global UI visibility based on SidesMode ---
    const bool isSide2GroupVisible = (sidesMode == SidesMode::TwoSides);
    ui->side1Label->setVisible(isSide2GroupVisible);
    ui->side2Label->setVisible(isSide2GroupVisible);
    ui->line1->setVisible(isSide2GroupVisible);
    ui->line2->setVisible(isSide2GroupVisible);
    ui->typeLabel2->setVisible(isSide2GroupVisible);
    ui->changeMode2->setVisible(isSide2GroupVisible);

    // --- Configure each side using the helper method ---
    // Side 1 is always conceptually visible, and we pass whether it should receive focus.
    updateSideUI(m_side1, type, mode1, true, (side == Side::First));
    // Side 2 is only visible if in TwoSides mode, and we pass whether it should receive focus.
    updateSideUI(m_side2, type, mode2, isSide2GroupVisible, (side == Side::Second));

    bool side1HasLength = (mode1 == Mode::Dimension);
    bool side2HasLength = (sidesMode == SidesMode::TwoSides && mode2 == Mode::Dimension);
    ui->checkBoxAlongDirection->setVisible(side1HasLength || side2HasLength);
  
    ui->checkBoxReversed->setEnabled(sidesMode != SidesMode::Symmetric || mode1 != Mode::Dimension);
}

void TaskExtrudeParameters::updateSideUI(const SideController& s,
                                        Type featureType,
                                        Mode sideMode,
                                        bool isParentVisible,
                                        bool setFocus)
{
    // Default states for all controls for this side
    bool isLengthVisible = false;
    bool isOffsetVisible = false;
    bool isOffsetEnabled = true;
    bool isTaperVisible = false;
    bool isFaceVisible = false;
    bool isShapeVisible = false;

    // This logic block is a direct translation of the original 'if/else if' chain
    if (sideMode == Mode::Dimension) {
        isLengthVisible = true;
        isTaperVisible = true;
        if (setFocus) {
            s.lengthEdit->selectNumber();
            QMetaObject::invokeMethod(s.lengthEdit, "setFocus", Qt::QueuedConnection);
        }
    }
    else if (sideMode == Mode::ThroughAll && featureType == Type::Pocket) {
        isOffsetVisible = true;
        isOffsetEnabled = false;  // "through all" pocket offset doesn't work
        isTaperVisible = true;
    }
    else if (sideMode == Mode::ToLast && featureType == Type::Pad) {
        isOffsetVisible = true;
    }
    else if (sideMode == Mode::ToFirst) {
        isOffsetVisible = true;
    }
    else if (sideMode == Mode::ToFace) {
        isOffsetVisible = true;
        isFaceVisible = true;
        if (setFocus) {
            QMetaObject::invokeMethod(s.lineFaceName, "setFocus", Qt::QueuedConnection);
            // Go into reference selection mode if no face has been selected yet
            if (s.lineFaceName->property("FeatureName").isNull()) {
                s.buttonFace->setChecked(true);
            }
        }
    }
    else if (sideMode == Mode::ToShape) {
        isShapeVisible = true;
        if (setFocus) {
            if (!s.checkBoxAllFaces->isChecked()) {
                s.buttonShapeFace->setChecked(true);
            }
        }
    }

    // Apply visibility based on the logic above AND the parent visibility.
    // This single 'isParentVisible' check correctly hides all of Side 2's UI at once.
    const bool finalLengthVisible = isParentVisible && isLengthVisible;
    s.labelLength->setVisible(finalLengthVisible);
    s.lengthEdit->setVisible(finalLengthVisible);
    s.lengthEdit->setEnabled(finalLengthVisible);

    const bool finalOffsetVisible = isParentVisible && isOffsetVisible;
    s.labelOffset->setVisible(finalOffsetVisible);
    s.offsetEdit->setVisible(finalOffsetVisible);
    s.offsetEdit->setEnabled(finalOffsetVisible && isOffsetEnabled);

    const bool finalTaperVisible = isParentVisible && isTaperVisible;
    s.labelTaperAngle->setVisible(finalTaperVisible);
    s.taperEdit->setVisible(finalTaperVisible);
    s.taperEdit->setEnabled(finalTaperVisible);

    s.buttonFace->setVisible(isParentVisible && isFaceVisible);
    s.lineFaceName->setVisible(isParentVisible && isFaceVisible);
    if (!isFaceVisible) {
        s.buttonFace->setChecked(false);  // Ensure button is unchecked when hidden
    }

    s.upToShapeList->setVisible(isParentVisible && isShapeVisible);
}

void TaskExtrudeParameters::onDirectionCBChanged(int num)
{
    if (axesInList.empty()) {
        return;
    }

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
        setSelectionMode(SelectReferenceAxis);
        setDirectionMode(num);
    }
    else if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        if (lnk.getValue()) {
            if (!extrude->getDocument()->isIn(lnk.getValue())) {
                Base::Console().error("Object was deleted\n");
                return;
            }
            propReferenceAxis->Paste(lnk);
        }

        // in case the user is in selection mode, but changed his mind before selecting anything
        setSelectionMode(None);
        setDirectionMode(num);

        extrude->ReferenceAxis.setValue(lnk.getValue(), lnk.getSubValues());
        tryRecomputeFeature();
        updateDirectionEdits();
    }
}

void TaskExtrudeParameters::onAlongSketchNormalChanged(bool on)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->AlongSketchNormal.setValue(on);
        tryRecomputeFeature();
    }
}

void TaskExtrudeParameters::onDirectionToggled(bool on)
{
    if (on) {
        ui->groupBoxDirection->show();
    }
    else {
        ui->groupBoxDirection->hide();
    }
}

void TaskExtrudeParameters::onAllFacesToggled(bool on, Side side)
{
    auto& sideCtrl = getSideController(side);
    sideCtrl.upToShapeFaces->setVisible(!on);
    sideCtrl.buttonShapeFace->setChecked(false);

    if (on) {
        if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
            extrude->UpToShape.setValue(extrude->UpToShape.getValue());
            updateShapeFaces(sideCtrl.listWidgetReferences, *sideCtrl.UpToShape);
            tryRecomputeFeature();
        }
    }
}

void TaskExtrudeParameters::onXDirectionEditChanged(double len)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->Direction.setValue(len,
                                    extrude->Direction.getValue().y,
                                    extrude->Direction.getValue().z);
        tryRecomputeFeature();
        // checking for case of a null vector is done in FeatureExtrude.cpp
        // if there was a null vector, the normal vector of the sketch is used.
        // therefore the vector component edits must be updated
        updateDirectionEdits();
    }
}

void TaskExtrudeParameters::onYDirectionEditChanged(double len)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->Direction.setValue(extrude->Direction.getValue().x,
                                    len,
                                    extrude->Direction.getValue().z);
        tryRecomputeFeature();
        updateDirectionEdits();
    }
}

void TaskExtrudeParameters::onZDirectionEditChanged(double len)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->Direction.setValue(extrude->Direction.getValue().x,
                                    extrude->Direction.getValue().y,
                                    len);
        tryRecomputeFeature();
        updateDirectionEdits();
    }
}

void TaskExtrudeParameters::updateDirectionEdits()
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
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
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    if (!extrude) {
        return;
    }

    // disable AlongSketchNormal when the direction is already normal
    if (index == DirectionModes::Normal) {
        ui->checkBoxAlongDirection->setEnabled(false);
    }
    else {
        ui->checkBoxAlongDirection->setEnabled(true);
    }

    // if custom direction is used, show it
    if (index == DirectionModes::Custom) {
        ui->checkBoxDirection->setChecked(true);
        extrude->UseCustomVector.setValue(true);
    }
    else {
        extrude->UseCustomVector.setValue(false);
    }

    // if we don't use custom direction, only allow one to show its direction
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

void TaskExtrudeParameters::onReversedChanged(bool on)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->Reversed.setValue(on);
        // update the direction
        tryRecomputeFeature();
        updateDirectionEdits();
    }
}

void TaskExtrudeParameters::getReferenceAxis(App::DocumentObject*& obj,
                                             std::vector<std::string>& sub) const
{
    if (axesInList.empty()) {
        throw Base::RuntimeError("Not initialized!");
    }

    int num = ui->directionCB->currentIndex();
    const App::PropertyLinkSub& lnk = *(axesInList[num]);
    if (!lnk.getValue()) {
        // Note: It is possible that a face of an object is directly padded/pocketed without
        // defining a profile shape
        obj = nullptr;
        sub.clear();
    }
    else {
        auto pcDirection = getObject<PartDesign::ProfileBased>();
        if (!pcDirection->getDocument()->isIn(lnk.getValue())) {
            throw Base::RuntimeError("Object was deleted");
        }

        obj = lnk.getValue();
        sub = lnk.getSubValues();
    }
}

void TaskExtrudeParameters::onSelectFaceToggle(const bool checked, Side side)
{
    auto& sideCtrl = getSideController(side);
    if (checked) {
        handleLineFaceNameClick(sideCtrl.lineFaceName);
        setSelectionMode(SelectFace, side);
    }
    else {
        handleLineFaceNameNo(sideCtrl.lineFaceName);
    }
}

void TaskExtrudeParameters::onSelectShapeToggle(bool checked, Side side)
{
    auto& sideCtrl = getSideController(side);
    if (checked) {
        setSelectionMode(SelectShape, side);
        sideCtrl.lineShapeName->setText({});
        sideCtrl.lineShapeName->setPlaceholderText(tr("Click on a shape in the model"));
    }
    else {
        setSelectionMode(None);
        updateShapeName(sideCtrl.lineShapeName, *sideCtrl.UpToShape);
    }
}

void TaskExtrudeParameters::onFaceName(const QString& text, Side side)
{
    auto& sideCtrl = getSideController(side);
    changeFaceName(sideCtrl.lineFaceName, text);
}

void TaskExtrudeParameters::changeFaceName(QLineEdit* lineEdit, const QString& text)
{
    if (text.isEmpty()) {
        // if user cleared the text field then also clear the properties
        lineEdit->setProperty("FeatureName", QVariant());
        lineEdit->setProperty("FaceName", QVariant());
    }
    else {
        // expect that the label of an object is used
        QStringList parts = text.split(QChar::fromLatin1(':'));
        QString label = parts[0];
        QVariant name = objectNameByLabel(label, lineEdit->property("FeatureName"));
        if (name.isValid()) {
            parts[0] = name.toString();
            QString uptoface = parts.join(QStringLiteral(":"));
            lineEdit->setProperty("FeatureName", name);
            lineEdit->setProperty("FaceName", setUpToFace(uptoface));
        }
        else {
            lineEdit->setProperty("FeatureName", QVariant());
            lineEdit->setProperty("FaceName", QVariant());
        }
    }
}

void TaskExtrudeParameters::translateFaceName(QLineEdit* lineEdit)
{
    handleLineFaceNameNo(lineEdit);
    QVariant featureName = lineEdit->property("FeatureName");
    if (featureName.isValid()) {
        QStringList parts = lineEdit->text().split(QChar::fromLatin1(':'));
        QByteArray upToFace = lineEdit->property("FaceName").toByteArray();
        int faceId = -1;
        bool ok = false;
        if (upToFace.indexOf("Face") == 0) {
            faceId = upToFace.remove(0, 4).toInt(&ok);
        }

        if (ok) {
            lineEdit->setText(
                QStringLiteral("%1:%2%3").arg(parts[0], tr("Face")).arg(faceId));
        }
        else {
            lineEdit->setText(parts[0]);
        }
    }
}

double TaskExtrudeParameters::getOffset() const
{
    return ui->offsetEdit->value().getValue();
}

double TaskExtrudeParameters::getOffset2() const
{
    return ui->offsetEdit2->value().getValue();
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

int TaskExtrudeParameters::getMode() const
{
    return ui->changeMode->currentIndex();
}

int TaskExtrudeParameters::getMode2() const
{
    return ui->changeMode2->currentIndex();
}

int TaskExtrudeParameters::getSidesMode() const
{
    return ui->sidesMode->currentIndex();
}

QString TaskExtrudeParameters::getFaceName(QLineEdit* lineEdit) const
{
    QVariant featureName = lineEdit->property("FeatureName");
    if (featureName.isValid()) {
        QString faceName = lineEdit->property("FaceName").toString();
        return getFaceReference(featureName.toString(), faceName);
    }

    return QStringLiteral("None");
}

void TaskExtrudeParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        QSignalBlocker length(ui->lengthEdit);
        QSignalBlocker length2(ui->lengthEdit2);
        QSignalBlocker offset(ui->offsetEdit);
        QSignalBlocker offset2(ui->offsetEdit2);
        QSignalBlocker taper(ui->taperEdit);
        QSignalBlocker taper2(ui->taperEdit2);
        QSignalBlocker xdir(ui->XDirectionEdit);
        QSignalBlocker ydir(ui->YDirectionEdit);
        QSignalBlocker zdir(ui->ZDirectionEdit);
        QSignalBlocker dir(ui->directionCB);
        QSignalBlocker face(ui->lineFaceName);
        QSignalBlocker face2(ui->lineFaceName2);
        QSignalBlocker mode(ui->changeMode);
        QSignalBlocker mode2(ui->changeMode2);
        QSignalBlocker sidesMode(ui->sidesMode);

        // Save all items
        QStringList items;
        for (int i = 0; i < ui->directionCB->count(); i++) {
            items << ui->directionCB->itemText(i);
        }

        // Translate direction items
        int index = ui->directionCB->currentIndex();
        ui->retranslateUi(proxy);

        // Keep custom items
        for (int i = 0; i < ui->directionCB->count(); i++) {
            items.pop_front();
        }
        ui->directionCB->addItems(items);
        ui->directionCB->setCurrentIndex(index);

        // Translate mode items
        translateModeList(ui->changeMode, ui->changeMode->currentIndex());
        translateModeList(ui->changeMode2, ui->changeMode2->currentIndex());
        translateSidesList(ui->sidesMode->currentIndex());

        translateFaceName(ui->lineFaceName);
        translateFaceName(ui->lineFaceName2);
    }
}

void TaskExtrudeParameters::saveHistory()
{
    // save the user values to history
    ui->lengthEdit->pushToHistory();
    ui->lengthEdit2->pushToHistory();
    ui->offsetEdit->pushToHistory();
    ui->offsetEdit2->pushToHistory();
    ui->taperEdit->pushToHistory();
    ui->taperEdit2->pushToHistory();
}

void TaskExtrudeParameters::applyParameters()
{
    auto obj = getObject();

    QString facename = QStringLiteral("None");
    QString facename2 = QStringLiteral("None");
    if (static_cast<Mode>(getMode()) == Mode::ToFace) {
        facename = getFaceName(ui->lineFaceName);
    }
    if (static_cast<Mode>(getMode2()) == Mode::ToFace) {
        facename2 = getFaceName(ui->lineFaceName2);
    }

    ui->lengthEdit->apply();
    ui->lengthEdit2->apply();
    ui->taperEdit->apply();
    ui->taperEdit2->apply();
    FCMD_OBJ_CMD(obj, "UseCustomVector = " << (getCustom() ? 1 : 0));
    FCMD_OBJ_CMD(obj,
                 "Direction = (" << getXDirection() << ", " << getYDirection() << ", "
                                 << getZDirection() << ")");
    FCMD_OBJ_CMD(obj, "ReferenceAxis = " << getReferenceAxis());
    FCMD_OBJ_CMD(obj, "AlongSketchNormal = " << (getAlongSketchNormal() ? 1 : 0));
    FCMD_OBJ_CMD(obj, "SideType = " << getSidesMode());
    FCMD_OBJ_CMD(obj, "Type = " << getMode());
    FCMD_OBJ_CMD(obj, "Type2 = " << getMode2());
    FCMD_OBJ_CMD(obj, "UpToFace = " << facename.toLatin1().data());
    FCMD_OBJ_CMD(obj, "UpToFace2 = " << facename2.toLatin1().data());
    FCMD_OBJ_CMD(obj, "Reversed = " << (getReversed() ? 1 : 0));
    FCMD_OBJ_CMD(obj, "Offset = " << getOffset());
    FCMD_OBJ_CMD(obj, "Offset2 = " << getOffset2());
}

void TaskExtrudeParameters::onSidesModeChanged(int index)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    switch (static_cast<SidesMode>(index)) {
        case SidesMode::OneSide:
            extrude->SideType.setValue("One side");
            updateUI(Side::First);
            break;
        case SidesMode::TwoSides:
            extrude->SideType.setValue("Two sides");
            updateUI(Side::Second);
            break;
        case SidesMode::Symmetric:
            extrude->SideType.setValue("Symmetric");
            updateUI(Side::First);
            break;
    }

    recomputeFeature();
}

void TaskExtrudeParameters::updateUI(Side)
{
    // implement in sub-class
}

void TaskExtrudeParameters::translateModeList(QComboBox*, int)
{
    // implement in sub-class
}

void TaskExtrudeParameters::translateSidesList(int index)
{
    ui->sidesMode->clear();
    ui->sidesMode->addItem(tr("One sided"));
    ui->sidesMode->addItem(tr("Two sided"));
    ui->sidesMode->addItem(tr("Symmetric"));
    ui->sidesMode->setCurrentIndex(index);
}

void TaskExtrudeParameters::handleLineFaceNameClick(QLineEdit* lineEdit)
{
    lineEdit->setPlaceholderText(tr("Click on a face in the model"));
}

void TaskExtrudeParameters::handleLineFaceNameNo(QLineEdit* lineEdit)
{
    lineEdit->setPlaceholderText(tr("No face selected"));
}

TaskDlgExtrudeParameters::TaskDlgExtrudeParameters(PartDesignGui::ViewProviderExtrude* vp)
    : TaskDlgSketchBasedParameters(vp)
{}

bool TaskDlgExtrudeParameters::accept()
{
    getTaskParameters()->setSelectionMode(TaskExtrudeParameters::None);

    return TaskDlgSketchBasedParameters::accept();
}

bool TaskDlgExtrudeParameters::reject()
{
    getTaskParameters()->setSelectionMode(TaskExtrudeParameters::None);

    return TaskDlgSketchBasedParameters::reject();
}

#include "moc_TaskExtrudeParameters.cpp"


