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
#include <Mod/PartDesign/App/FeatureExtrude.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>

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
    // Get the feature data
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    Base::Quantity l = extrude->Length.getQuantityValue();
    Base::Quantity l2 = extrude->Length2.getQuantityValue();
    Base::Quantity off = extrude->Offset.getQuantityValue();
    Base::Quantity off2 = extrude->Offset2.getQuantityValue();
    Base::Quantity taper = extrude->TaperAngle.getQuantityValue();
    Base::Quantity taper2 = extrude->TaperAngle2.getQuantityValue();

    bool alongNormal = extrude->AlongSketchNormal.getValue();
    bool useCustom = extrude->UseCustomVector.getValue();

    double xs = extrude->Direction.getValue().x;
    double ys = extrude->Direction.getValue().y;
    double zs = extrude->Direction.getValue().z;

    bool reversed = extrude->Reversed.getValue();

    int index = extrude->Type.getValue();  // must extract value here, clear() kills it!
    int index2 = extrude->Type2.getValue();  // must extract value here, clear() kills it!
    int index3 = extrude->SideType.getValue();  // must extract value here, clear() kills it!

    App::DocumentObject* obj1 = extrude->UpToFace.getValue();
    App::DocumentObject* obj2 = extrude->UpToFace2.getValue();
    std::vector<std::string> subStrings1 = extrude->UpToFace.getSubValues();
    std::vector<std::string> subStrings2 = extrude->UpToFace2.getSubValues();
    std::string upToFace1, upToFace2;
    int faceId1 = -1;
    if (obj1 && !subStrings1.empty()) {
        upToFace1 = subStrings1.front();
        if (upToFace1.compare(0, 4, "Face") == 0) {
            faceId1 = std::atoi(&upToFace1[4]);
        }
    }
    int faceId2 = -1;
    if (obj2 && !subStrings2.empty()) {
        upToFace2 = subStrings2.front();
        if (upToFace2.compare(0, 4, "Face") == 0) {
            faceId2 = std::atoi(&upToFace2[4]);
        }
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
    ui->offsetEdit2->setValue(off2);
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
    ui->offsetEdit2->bind(extrude->Offset2);
    ui->taperEdit->bind(extrude->TaperAngle);
    ui->taperEdit2->bind(extrude->TaperAngle2);
    ui->XDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, std::string("Direction.x")));
    ui->YDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, std::string("Direction.y")));
    ui->ZDirectionEdit->bind(App::ObjectIdentifier::parse(extrude, std::string("Direction.z")));

    // According to bug #0000521 the reversed option
    // shouldn't be de-activated if the pad has a support face
    ui->checkBoxReversed->setChecked(reversed);

    // Set object labels
    if (obj1 && PartDesign::Feature::isDatum(obj1)) {
        ui->lineFaceName->setText(QString::fromUtf8(obj1->Label.getValue()));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj1->getNameInDocument()));
    }
    else if (obj1 && faceId1 >= 0) {
        ui->lineFaceName->setText(
            QStringLiteral("%1:%2%3").arg(QString::fromUtf8(obj1->Label.getValue()),
                                               tr("Face"),
                                               QString::number(faceId1)));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj1->getNameInDocument()));
    }
    else {
        ui->lineFaceName->clear();
        ui->lineFaceName->setProperty("FeatureName", QVariant());
    }

    // Set object labels side2
    if (obj2 && PartDesign::Feature::isDatum(obj2)) {
        ui->lineFaceName2->setText(QString::fromUtf8(obj2->Label.getValue()));
        ui->lineFaceName2->setProperty("FeatureName", QByteArray(obj2->getNameInDocument()));
    }
    else if (obj2 && faceId2 >= 0) {
        ui->lineFaceName2->setText(
            QStringLiteral("%1:%2%3").arg(QString::fromUtf8(obj2->Label.getValue()),
                                               tr("Face"),
                                               QString::number(faceId2)));
        ui->lineFaceName2->setProperty("FeatureName", QByteArray(obj2->getNameInDocument()));
    }
    else {
        ui->lineFaceName2->clear();
        ui->lineFaceName2->setProperty("FeatureName", QVariant());
    }

    ui->lineFaceName->setProperty("FaceName", QByteArray(upToFace1.c_str()));
    ui->lineFaceName2->setProperty("FaceName", QByteArray(upToFace2.c_str()));

    updateShapeName(ui->lineShapeName, extrude->UpToShape);
    updateShapeName(ui->lineShapeName2, extrude->UpToShape2);
    updateShapeFaces(ui->listWidgetReferences, extrude->UpToShape);
    updateShapeFaces(ui->listWidgetReferences2, extrude->UpToShape2);

    translateModeList(ui->changeMode, index);
    translateModeList(ui->changeMode2, index2);
    translateSidesList(index3);

    unselectShapeFaceAction = new QAction(tr("Remove"), this);
    {
        auto& rcCmdMgr = Gui::Application::Instance->commandManager();
        auto shortcut = rcCmdMgr.getCommandByName("Std_Delete")->getShortcut();
        unselectShapeFaceAction->setShortcut(QKeySequence(shortcut));
    }
    // display shortcut behind the context menu entry
    unselectShapeFaceAction->setShortcutVisibleInContextMenu(true);

    ui->listWidgetReferences->addAction(unselectShapeFaceAction);
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->checkBoxAllFaces->setChecked(ui->listWidgetReferences->count() == 0);

    unselectShapeFaceAction2 = new QAction(tr("Remove"), this);
    {
        auto& rcCmdMgr = Gui::Application::Instance->commandManager();
        auto shortcut = rcCmdMgr.getCommandByName("Std_Delete")->getShortcut();
        unselectShapeFaceAction2->setShortcut(QKeySequence(shortcut));
    }
    // display shortcut behind the context menu entry
    unselectShapeFaceAction2->setShortcutVisibleInContextMenu(true);

    ui->listWidgetReferences2->addAction(unselectShapeFaceAction2);
    ui->listWidgetReferences2->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->checkBoxAllFaces2->setChecked(ui->listWidgetReferences2->count() == 0);

    connectSlots();

    this->propReferenceAxis = &(extrude->ReferenceAxis);

    // Due to signals attached after changes took took into effect we should update the UI now.
    updateUI(Sides::First);
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

    // clang-format off
    connect(ui->lengthEdit, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this, &TaskExtrudeParameters::onLengthChanged);
    connect(ui->lengthEdit2, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this, &TaskExtrudeParameters::onLength2Changed);
    connect(ui->offsetEdit, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this, &TaskExtrudeParameters::onOffsetChanged);
    connect(ui->offsetEdit2, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this, &TaskExtrudeParameters::onOffset2Changed);
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
    connect(ui->checkBoxReversed, &QCheckBox::toggled,
            this, &TaskExtrudeParameters::onReversedChanged);
    connect(ui->checkBoxAllFaces, &QCheckBox::toggled,
            this, &TaskExtrudeParameters::onAllFacesToggled);
    connect(ui->checkBoxAllFaces2, &QCheckBox::toggled,
            this, &TaskExtrudeParameters::onAllFaces2Toggled);
    connect(ui->sidesMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskExtrudeParameters::onSidesModeChanged);
    connect(ui->changeMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskExtrudeParameters::onModeChanged);
    connect(ui->changeMode2, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskExtrudeParameters::onMode2Changed);
    connect(ui->buttonFace, &QToolButton::toggled,
            this, &TaskExtrudeParameters::onSelectFaceToggle);
    connect(ui->buttonFace2, &QToolButton::toggled,
            this, &TaskExtrudeParameters::onSelectFace2Toggle);
    connect(ui->buttonShape, &QToolButton::toggled,
            this, &TaskExtrudeParameters::onSelectShapeToggle);
    connect(ui->buttonShape2, &QToolButton::toggled,
            this, &TaskExtrudeParameters::onSelectShape2Toggle);
    connect(ui->lineFaceName, &QLineEdit::textEdited,
            this, &TaskExtrudeParameters::onFaceName);
    connect(ui->lineFaceName2, &QLineEdit::textEdited,
            this, &TaskExtrudeParameters::onFaceName2);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskExtrudeParameters::onUpdateView);
    connect(ui->buttonShapeFace, &QToolButton::toggled,
            this, &TaskExtrudeParameters::onSelectShapeFacesToggle);
    connect(ui->buttonShapeFace2, &QToolButton::toggled,
            this, &TaskExtrudeParameters::onSelectShapeFaces2Toggle);
    connect(unselectShapeFaceAction, &QAction::triggered,
            this, &TaskExtrudeParameters::onUnselectShapeFacesTrigger);
    connect(unselectShapeFaceAction2, &QAction::triggered,
            this, &TaskExtrudeParameters::onUnselectShapeFaces2Trigger);
    // clang-format on
}

void TaskExtrudeParameters::onSelectShapeFacesToggle(bool checked)
{
    selectShapeFacesToggle(checked, SelectShapeFaces, ui->buttonShapeFace);
}

void TaskExtrudeParameters::onSelectShapeFaces2Toggle(bool checked)
{
    selectShapeFacesToggle(checked, SelectShapeFaces2, ui->buttonShapeFace2);
}

void TaskExtrudeParameters::selectShapeFacesToggle(bool checked, SelectionMode mode, QToolButton* btn)
{
    if (checked) {
        setSelectionMode(mode);
        btn->setText(tr("Preview"));
    }
    else {
        setSelectionMode(None);
        btn->setText(tr("Select faces"));
    }
}

void PartDesignGui::TaskExtrudeParameters::onUnselectShapeFacesTrigger()
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    unselectShapeFacesTrigger(ui->listWidgetReferences, extrude->UpToShape);
}

void PartDesignGui::TaskExtrudeParameters::onUnselectShapeFaces2Trigger()
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    unselectShapeFacesTrigger(ui->listWidgetReferences2, extrude->UpToShape2);
}

void PartDesignGui::TaskExtrudeParameters::unselectShapeFacesTrigger(QListWidget* list,
                                                                      App::PropertyLinkSubList& prop)
{
    auto selected = list->selectedItems();
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto faces = getShapeFaces(prop);


    faces.erase(std::remove_if(faces.begin(), faces.end(), [selected](const std::string& face) {
        for (auto& item : selected) {
            if (item->text().toStdString() == face) {
                return true;
            }
        }

        return false;
    }));

    prop.setValue(prop.getValue(), faces);

    updateShapeFaces(list, prop);
}

void TaskExtrudeParameters::setSelectionMode(SelectionMode mode)
{
    if (selectionMode == mode) {
        return;
    }

    ui->buttonShapeFace->setChecked(mode == SelectShapeFaces);
    ui->buttonShapeFace2->setChecked(mode == SelectShapeFaces2);
    ui->buttonFace->setChecked(mode == SelectFace);
    ui->buttonFace2->setChecked(mode == SelectFace2);
    ui->buttonShape->setChecked(mode == SelectShape);
    ui->buttonShape2->setChecked(mode == SelectShape2);

    selectionMode = mode;

    auto extrude = getObject<PartDesign::FeatureExtrude>();

    switch (mode) {
        case SelectShape:
        case SelectShape2:
            onSelectReference(AllowSelection::WHOLE);
            Gui::Selection().addSelectionGate(
                new SelectionFilterGate("SELECT Part::Feature COUNT 1"));
            break;
        case SelectFace:
        case SelectFace2:
            onSelectReference(AllowSelection::FACE);
            break;
        case SelectShapeFaces:
            onSelectReference(AllowSelection::FACE);
            getViewObject<ViewProviderExtrude>()->highlightShapeFaces(getShapeFaces(extrude->UpToShape));
            break;
        case SelectShapeFaces2:
            onSelectReference(AllowSelection::FACE);
            getViewObject<ViewProviderExtrude>()->highlightShapeFaces(getShapeFaces(extrude->UpToShape2));
            break;
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
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        auto extrude = getObject<PartDesign::FeatureExtrude>();
        switch (selectionMode) {
            case SelectShape:
                selectedShape(msg,
                              ui->lineShapeName,
                              ui->listWidgetReferences,
                              ui->checkBoxAllFaces,
                              extrude->UpToShape);
                break;
            case SelectShape2:
                selectedShape(msg,
                              ui->lineShapeName2,
                              ui->listWidgetReferences2,
                              ui->checkBoxAllFaces2,
                              extrude->UpToShape2);
                break;

            case SelectShapeFaces:
                selectedShapeFace(msg, ui->listWidgetReferences, extrude->UpToShape);
                break;

            case SelectShapeFaces2:
                selectedShapeFace(msg, ui->listWidgetReferences2, extrude->UpToShape2);
                break;

            case SelectFace:
                selectedFace(msg, ui->lineFaceName, ui->buttonFace, extrude->UpToFace);
                break;

            case SelectFace2:
                selectedFace(msg, ui->lineFaceName2, ui->buttonFace2, extrude->UpToFace2);
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
            clearFaceName(ui->lineFaceName);
        }
        else if (selectionMode == SelectFace2) {
            clearFaceName(ui->lineFaceName2);
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
                                              QListWidget* list,
                                              App::PropertyLinkSubList& prop)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto document = extrude->getDocument();

    if (strcmp(msg.pDocName, document->getName()) != 0) {
        return;
    }

    auto base = static_cast<Part::Feature*>(prop.getValue());
    if (!base){
        base = static_cast<Part::Feature*>(extrude);
    }
    else if (strcmp(msg.pObjectName, base->getNameInDocument()) != 0) {
        return;
    }

    std::vector<std::string> faces = getShapeFaces(prop);
    const std::string subName(msg.pSubName);

    if (subName.empty()) {
        return;
    }

    if (const auto positionInList = std::ranges::find(faces, subName);
        positionInList != faces.end()) {  // it's in the list
        faces.erase(positionInList);  // remove it.
    }
    else {
        faces.push_back(subName);  // not yet in the list so add it.
    }

    prop.setValue(base, faces);

    updateShapeFaces(list, prop);

    tryRecomputeFeature();
}

void PartDesignGui::TaskExtrudeParameters::selectedFace(const Gui::SelectionChanges& msg,
                                                        QLineEdit* lineEdit,
                                                        QToolButton* btn,
                                                        App::PropertyLinkSub& prop)
{
    QString refText = onAddSelection(msg, prop);

    if (refText.length() > 0) {
        QSignalBlocker block(lineEdit);

        lineEdit->setText(refText);
        lineEdit->setProperty("FeatureName", QByteArray(msg.pObjectName));
        lineEdit->setProperty("FaceName", QByteArray(msg.pSubName));

        // Turn off reference selection mode
        btn->setChecked(false);
    }
    else {
        clearFaceName(lineEdit);
    }

    setSelectionMode(None);
}

void PartDesignGui::TaskExtrudeParameters::selectedShape(const Gui::SelectionChanges& msg,
                                                         QLineEdit* lineEdit,
                                                         QListWidget* list,
                                                         QCheckBox* box,
                                                         App::PropertyLinkSubList& prop)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto document = extrude->getDocument();

    if (strcmp(msg.pDocName, document->getName()) != 0) {
        return;
    }

    Gui::Selection().clearSelection();

    auto ref = document->getObject(msg.pObjectName);

    prop.setValue(ref);

    box->setChecked(true);

    setSelectionMode(None);

    updateShapeName(lineEdit, prop);
    updateShapeFaces(list, prop);

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

    if (selectionMode == SelectShapeFaces || selectionMode == SelectShapeFaces2) {
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

void TaskExtrudeParameters::onLengthChanged(double len)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->Length.setValue(len);
        tryRecomputeFeature();
    }
}

void TaskExtrudeParameters::onLength2Changed(double len)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->Length2.setValue(len);
        tryRecomputeFeature();
    }
}

void TaskExtrudeParameters::onOffsetChanged(double len)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->Offset.setValue(len);
        tryRecomputeFeature();
    }
}

void TaskExtrudeParameters::onOffset2Changed(double len)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->Offset2.setValue(len);
        tryRecomputeFeature();
    }
}

void TaskExtrudeParameters::onTaperChanged(double angle)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->TaperAngle.setValue(angle);
        tryRecomputeFeature();
    }
}

void TaskExtrudeParameters::onTaper2Changed(double angle)
{
    if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
        extrude->TaperAngle2.setValue(angle);
        tryRecomputeFeature();
    }
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
        addAxisToCombo(nullptr, std::string(), tr("Select reference..."));

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

void TaskExtrudeParameters::setCheckboxes(Type type, Sides side)
{
    SidesMode sidesMode = static_cast<SidesMode>(ui->sidesMode->currentIndex());
    Mode mode = static_cast<Mode>(ui->changeMode->currentIndex());
    Mode mode2 = static_cast<Mode>(ui->changeMode2->currentIndex());

    // disable/hide everything unless we are sure we don't need it
    // exception: the direction parameters are in any case visible
    bool isLengthEditVisible = false;
    bool isOffsetEditVisible = false;
    bool isOffsetEditEnabled = true;
    bool isTaperEditVisible = false;
    bool isFaceEditVisible = false;
    bool isShapeEditVisible = false;

    bool isSide2Visible = false;
    bool isLengthEdit2Visible = false;
    bool isOffsetEdit2Visible = false;
    bool isOffsetEdit2Enabled = true;
    bool isTaperEdit2Visible = false;
    bool isFaceEdit2Visible = false;
    bool isShapeEdit2Visible = false;

    bool isReversedEnabled = true;

    if (sidesMode == SidesMode::Symmetric && mode == Mode::Dimension) {
        isReversedEnabled = false; // only case where reverse is not useful.
    }

    // Side 1 : Handled for all cases of sidesMode:
    if (mode == Mode::Dimension) {
        isLengthEditVisible = true;
        ui->lengthEdit->selectNumber();
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        isTaperEditVisible = true;
    }
    else if (mode == Mode::ThroughAll && type == Type::Pocket) {
        isOffsetEditVisible = true;
        isOffsetEditEnabled = false;  // offset may have some meaning for through all but it doesn't work
        // Note 2025-06-05: This is because through all is actually using a huge Length.
        // Unlike 'To last' of pad.
        isTaperEditVisible = true;
    }
    else if (mode == Mode::ToLast && type == Type::Pad) {
        isOffsetEditVisible = true;
    }
    else if (mode == Mode::ToFirst) {
        isOffsetEditVisible = true;
    }
    else if (mode == Mode::ToFace) {
        isOffsetEditVisible = true;
        isFaceEditVisible = true;

        if (side == Sides::First) {
            QMetaObject::invokeMethod(ui->lineFaceName, "setFocus", Qt::QueuedConnection);
            // Go into reference selection mode if no face has been selected yet
            if (ui->lineFaceName->property("FeatureName").isNull()) {
                ui->buttonFace->setChecked(true);
            }
        }
    }
    else if (mode == Mode::ToShape) {
        isShapeEditVisible = true;

        if (side == Sides::First) {
            if (!ui->checkBoxAllFaces->isChecked()) {
                ui->buttonShapeFace->setChecked(true);
            }
        }
    }

    // Side 2 : shown only in this case.
    if (sidesMode == SidesMode::TwoSides) {
        isSide2Visible = true;

        if (mode2 == Mode::Dimension) {
            isLengthEdit2Visible = true;
            isTaperEdit2Visible = true;
        }
        else if (mode2 == Mode::ThroughAll && type == Type::Pocket) {
            isOffsetEdit2Visible = true;
            isOffsetEdit2Enabled = false;
            isTaperEdit2Visible = true;
        }
        else if (mode2 == Mode::ToLast && type == Type::Pad) {
            isOffsetEdit2Visible = true;
        }
        else if (mode2 == Mode::ToFirst) {
            isOffsetEdit2Visible = true;
        }
        else if (mode2 == Mode::ToFace) {
            isOffsetEdit2Visible = true;
            isFaceEdit2Visible = true;

            if (side == Sides::Second) {
                QMetaObject::invokeMethod(ui->lineFaceName2, "setFocus", Qt::QueuedConnection);
                // Go into reference selection mode if no face has been selected yet
                if (ui->lineFaceName2->property("FeatureName").isNull()) {
                    ui->buttonFace2->setChecked(true);
                }
            }
        }
        else if (mode2 == Mode::ToShape) {
            isShapeEdit2Visible = true;

            if (side == Sides::Second) {
                if (!ui->checkBoxAllFaces2->isChecked()) {
                    ui->buttonShapeFace2->setChecked(true);
                }
            }
        }
    }

    ui->checkBoxAlongDirection->setVisible(isLengthEditVisible || isLengthEdit2Visible);
    ui->checkBoxReversed->setEnabled(isReversedEnabled);

    // Side 1
    ui->lengthEdit->setVisible(isLengthEditVisible);
    ui->lengthEdit->setEnabled(isLengthEditVisible);
    ui->labelLength->setVisible(isLengthEditVisible);

    ui->offsetEdit->setVisible(isOffsetEditVisible);
    ui->offsetEdit->setEnabled(isOffsetEditVisible && isOffsetEditEnabled);
    ui->labelOffset->setVisible(isOffsetEditVisible);

    ui->taperEdit->setVisible(isTaperEditVisible);
    ui->taperEdit->setEnabled(isTaperEditVisible);
    ui->labelTaperAngle->setVisible(isTaperEditVisible);

    ui->buttonFace->setVisible(isFaceEditVisible);
    ui->lineFaceName->setVisible(isFaceEditVisible);
    if (!isFaceEditVisible) {
        ui->buttonFace->setChecked(false);
    }

    ui->upToShapeList->setVisible(isShapeEditVisible);

    // Side 2
    ui->side1Label->setVisible(isSide2Visible);
    ui->side2Label->setVisible(isSide2Visible);
    ui->line1->setVisible(isSide2Visible);
    ui->line2->setVisible(isSide2Visible);
    ui->typeLabel2->setVisible(isSide2Visible);
    ui->changeMode2->setVisible(isSide2Visible);


    ui->lengthEdit2->setVisible(isLengthEdit2Visible);
    ui->lengthEdit2->setEnabled(isLengthEdit2Visible);
    ui->labelLength2->setVisible(isLengthEdit2Visible);

    ui->offsetEdit2->setVisible(isOffsetEdit2Visible);
    ui->offsetEdit2->setEnabled(isOffsetEdit2Visible && isOffsetEdit2Enabled);
    ui->labelOffset2->setVisible(isOffsetEdit2Visible);

    ui->taperEdit2->setVisible(isTaperEdit2Visible);
    ui->taperEdit2->setEnabled(isTaperEdit2Visible);
    ui->labelTaperAngle2->setVisible(isTaperEdit2Visible);

    ui->buttonFace2->setVisible(isFaceEdit2Visible);
    ui->lineFaceName2->setVisible(isFaceEdit2Visible);
    if (!isFaceEdit2Visible) {
        ui->buttonFace2->setChecked(false);
    }

    ui->upToShapeList2->setVisible(isShapeEdit2Visible);
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

void PartDesignGui::TaskExtrudeParameters::onAllFacesToggled(bool on)
{
    ui->upToShapeFaces->setVisible(!on);
    ui->buttonShapeFace->setChecked(false);

    if (on) {
        if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
            extrude->UpToShape.setValue(extrude->UpToShape.getValue());

            updateShapeFaces(ui->listWidgetReferences, extrude->UpToShape);

            tryRecomputeFeature();
        }
    }
}

void PartDesignGui::TaskExtrudeParameters::onAllFaces2Toggled(bool on)
{
    ui->upToShapeFaces2->setVisible(!on);
    ui->buttonShapeFace2->setChecked(false);

    if (on) {
        if (auto extrude = getObject<PartDesign::FeatureExtrude>()) {
            extrude->UpToShape2.setValue(extrude->UpToShape2.getValue());

            updateShapeFaces(ui->listWidgetReferences2, extrude->UpToShape2);

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

void TaskExtrudeParameters::onSelectFaceToggle(const bool checked)
{
    selectFaceToggle(checked, ui->lineFaceName, SelectFace);
}

void TaskExtrudeParameters::onSelectFace2Toggle(const bool checked)
{
    selectFaceToggle(checked, ui->lineFaceName2, SelectFace2);
}

void TaskExtrudeParameters::selectFaceToggle(const bool checked, QLineEdit* lineEdit, SelectionMode mode)
{
    if (!checked) {
        handleLineFaceNameNo(lineEdit);
    }
    else {
        handleLineFaceNameClick(lineEdit);  // sets placeholder text
        setSelectionMode(mode);
    }
}

void PartDesignGui::TaskExtrudeParameters::onSelectShapeToggle(const bool checked)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    selectShapeToggle(checked, ui->lineShapeName, extrude->UpToShape, SelectShape);
}

void PartDesignGui::TaskExtrudeParameters::onSelectShape2Toggle(const bool checked)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    selectShapeToggle(checked, ui->lineShapeName2, extrude->UpToShape2, SelectShape2);
}

void PartDesignGui::TaskExtrudeParameters::selectShapeToggle(const bool checked,
                                                             QLineEdit* lineEdit,
                                                             App::PropertyLinkSubList& prop,
                                                             SelectionMode mode)
{
    if (checked) {
        setSelectionMode(mode);

        lineEdit->setText({});
        lineEdit->setPlaceholderText(tr("Click on a shape in the model"));
    }
    else {
        setSelectionMode(None);
        updateShapeName(lineEdit, prop);
    }
}

void TaskExtrudeParameters::onFaceName(const QString& text)
{
    changeFaceName(ui->lineFaceName, text);
}

void TaskExtrudeParameters::onFaceName2(const QString& text)
{
    changeFaceName(ui->lineFaceName2, text);
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

void TaskExtrudeParameters::onModeChanged(int)
{
    // implement in sub-class
}

void TaskExtrudeParameters::onMode2Changed(int)
{
    // implement in sub-class
}

void TaskExtrudeParameters::onSidesModeChanged(int index)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    switch (static_cast<SidesMode>(index)) {
        case SidesMode::OneSide:
            extrude->SideType.setValue("1 side");
            updateUI(Sides::First);
            break;
        case SidesMode::TwoSides:
            extrude->SideType.setValue("2 sides");
            updateUI(Sides::Second);
            break;
        case SidesMode::Symmetric:
            extrude->SideType.setValue("Symmetric");
            updateUI(Sides::First);
            break;
    }

    recomputeFeature();
}

void TaskExtrudeParameters::updateUI(Sides side)
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
    ui->sidesMode->addItem(tr("1 side"));
    ui->sidesMode->addItem(tr("2 sides"));
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
