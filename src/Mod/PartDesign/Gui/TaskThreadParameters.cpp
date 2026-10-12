// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Caio Venâncio <caio.venancio784@gmail.com>          *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include <QWidget>
#include <QEvent>
#include <QLineEdit>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>
#include <Mod/PartDesign/App/FeatureThread.h>

#include "ui_TaskThreadParameters.h"
#include "TaskThreadParameters.h"
#include "TaskDressUpParameters.h"

using namespace PartDesignGui;
using namespace Gui;
namespace sp = std::placeholders;

TaskThreadParameters::TaskThreadParameters(ViewProviderDressUp* threadView, QWidget* parent)
    : TaskDressUpParameters(threadView, true, true, parent)
    , ui(new Ui_TaskThreadParameters)
    , observer(new Observer(this, getObject<PartDesign::Thread>()))
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    QMetaObject::connectSlotsByName(this);

    const PartDesign::Thread* thread = threadView->getObject<PartDesign::Thread>();

    setLinkSubText(ui->lateralFaceEdit, thread->LateralFace);
    setLinkSubText(ui->startEdit, thread->StartPlane);
    setLinkSubText(ui->upToGeometryEdit, thread->UpToGeometry);

    const std::vector<std::string> strings = thread->Base.getSubValues();

    setThreadSelectionMode(SideFaceSel);

    ui->endTypeCombo->setCurrentIndex(thread->DepthType.getValue());
    const bool isDimension = std::string(thread->DepthType.getValueAsString()) == "Dimension";
    const bool isUpToGeometry = std::string(thread->DepthType.getValueAsString()) == "UpToGeometry";

    ui->labelDepth->setHidden(!isDimension);
    ui->Depth->setHidden(!isDimension);
    ui->upToGeometryWidget->setHidden(!isUpToGeometry);
    ui->Depth->setValue(thread->Depth.getValue());

    const auto addComboBoxItems = [this](QComboBox* widget, const auto& values) {
        for (const auto& value : values) {
            widget->addItem(tr(value.c_str()));
        }
    };

    ui->standardCombo->clear();
    const std::vector<std::string> cursorStandard = thread->ThreadTypeName.getEnumVector();
    addComboBoxItems(ui->standardCombo, cursorStandard);
    ui->standardCombo->setCurrentIndex(thread->ThreadType.getValue());

    ui->diameterCombo->clear();
    const std::vector<std::string> cursorDiameter = thread->ThreadSize.getEnumVector();
    addComboBoxItems(ui->diameterCombo, cursorDiameter);
    ui->diameterCombo->setCurrentIndex(thread->ThreadSize.getValue());

    ui->classCombo->clear();
    const std::vector<std::string> cursorClass = thread->ThreadClass.getEnumVector();
    addComboBoxItems(ui->classCombo, cursorClass);
    ui->classCombo->setCurrentIndex(thread->ThreadClass.getValue());

    ui->pitchCombo->clear();
    const std::vector<std::string> cursorPitch = thread->ThreadSizePitch.getEnumVector();
    addComboBoxItems(ui->pitchCombo, cursorPitch);
    ui->pitchCombo->setCurrentIndex(thread->ThreadSizePitch.getValue());

    ui->directionCombo->clear();
    const std::vector<std::string> cursorDirection = thread->ThreadDirection.getEnumVector();
    addComboBoxItems(ui->directionCombo, cursorDirection);
    ui->directionCombo->setCurrentIndex(thread->ThreadDirection.getValue());


    ui->customClearanceCheck->setChecked(thread->UseCustomThreadClearance.getValue());
    ui->customClearanceField->setEnabled(ui->customClearanceCheck->isChecked());
    ui->classCombo->setEnabled(!ui->customClearanceCheck->isChecked());
    ui->customClearanceField->setValue(thread->CustomThreadClearance.getValue());

    ui->taperedCheck->setChecked(thread->Tapered.getValue());
    ui->taperedAngleField->setEnabled(thread->Tapered.getValue() && !thread->TaperedAngle.isReadOnly());
    ui->taperedAngleField->setValue(thread->TaperedAngle.getValue());

    const bool isModeled = thread->ModelThread.getValue();
    ui->modelledThreadRadio->setChecked(isModeled);

    ui->designationEdit->setReadOnly(true);
    ui->designationEdit->setText(thread->ThreadDesignation.getValue());

    connect(
        ui->standardCombo,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskThreadParameters::threadTypeChanged
    );

    connect(
        ui->Depth,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &TaskThreadParameters::depthChanged
    );

    connect(
        ui->diameterCombo,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskThreadParameters::threadSizeChanged
    );

    connect(
        ui->pitchCombo,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskThreadParameters::threadSizePitchChanged
    );

    connect(
        ui->classCombo,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskThreadParameters::threadClassChanged
    );

    connect(
        ui->directionCombo,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskThreadParameters::threadDirectionChanged
    );

    connect(ui->selectLateralFace, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            ui->selectLateralFace->setChecked(false);
            setThreadSelectionMode(SideFaceSel);
        }
        else if (currentSelectionMode == SideFaceSel) {
            setThreadSelectionMode(None);
        }
    });

    connect(ui->selectStart, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            ui->selectStart->setChecked(false);
            setThreadSelectionMode(StartFaceSel);
        }
        else if (currentSelectionMode == StartFaceSel) {
            setThreadSelectionMode(None);
        }
    });

    connect(ui->selectUpToGeometry, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            ui->selectUpToGeometry->setChecked(false);
            setThreadSelectionMode(UpToGeometrySel);
        }
        else if (currentSelectionMode == UpToGeometrySel) {
            setThreadSelectionMode(None);
        }
    });


    connect(
        ui->endTypeCombo,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskThreadParameters::depthTypeChanged
    );

    connect(
        ui->customClearanceCheck,
        &QCheckBox::toggled,
        this,
        &TaskThreadParameters::useCustomThreadClearanceChanged
    );

    connect(ui->taperedCheck, &QCheckBox::toggled, this, &TaskThreadParameters::taperedCheckChanged);

    connect(
        ui->customClearanceField,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &TaskThreadParameters::customThreadClearanceChanged
    );

    connect(
        ui->taperedAngleField,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &TaskThreadParameters::taperedAngleChanged
    );

    connect(
        ui->cosmeticThreadRadio,
        &QRadioButton::clicked,
        this,
        &TaskThreadParameters::threadModelChanged
    );

    connect(
        ui->modelledThreadRadio,
        &QRadioButton::clicked,
        this,
        &TaskThreadParameters::threadModelChanged
    );

    // NOLINTBEGIN
    connectPropChanged = App::GetApplication().signalChangePropertyEditor.connect(
        std::bind(&TaskThreadParameters::changedObject, this, sp::_1, sp::_2)
    );
    // NOLINTEND

    if (strings.size() == 0) {
        setThreadSelectionGate();
    }
    else {
        hideOnError();
    }
}

TaskThreadParameters::~TaskThreadParameters()
{
    try {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
    }
    catch (const std::exception&) {
    }
}

void TaskThreadParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

bool TaskThreadParameters::getModelThread() const
{
    return ui->modelledThreadRadio->isChecked();
}

bool TaskThreadParameters::getCosmeticThreaded() const
{
    return ui->cosmeticThreadRadio->isChecked();
}

void TaskThreadParameters::apply()
{
    auto thread = getObject<PartDesign::Thread>();
    if (thread->LateralFace.getSubValues().empty()) {
        Base::Console().warning(tr("Empty thread created!\n").toStdString().c_str());
    }
}

void TaskThreadParameters::onRefDeleted()
{}

void TaskThreadParameters::setButtons(const PartDesignGui::TaskDressUpParameters::selectionModes mode)
{
    Q_UNUSED(mode);
}

void TaskThreadParameters::setThreadSelectionMode(threadSelectionModes mode)
{
    currentSelectionMode = mode;
}

void TaskThreadParameters::setLinkSubText(QLineEdit* edit, const App::PropertyLinkSub& prop)
{
    const auto& subs = prop.getSubValues();

    if (!subs.empty() && !subs.front().empty()) {
        edit->setText(QString::fromStdString(subs.front()));
    }
    else if (auto obj = prop.getValue()) {
        edit->setText(QString::fromUtf8(obj->Label.getValue()));
    }
    else {
        edit->setText(QString::fromUtf8("No selection"));
    }
}

void TaskThreadParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }

    switch (currentSelectionMode) {
        case SideFaceSel: {
            auto thread = getObject<PartDesign::Thread>();
            std::vector<std::string> planes;
            App::DocumentObject* selectedObject = nullptr;
            getReferencedSelection(thread, msg, selectedObject, planes);
            if (!selectedObject) {
                return;
            }
            setupTransaction();
            thread->LateralFace.setValue(selectedObject, planes);
            recomputeFeature();
            setLinkSubText(ui->lateralFaceEdit, thread->LateralFace);
            ui->selectLateralFace->setChecked(false);
            Gui::Selection().clearSelection();
            break;
        }

        case StartFaceSel: {
            auto thread = getObject<PartDesign::Thread>();
            std::vector<std::string> planes;
            App::DocumentObject* selectedObject = nullptr;
            getReferencedSelection(thread, msg, selectedObject, planes);
            if (!selectedObject) {
                return;
            }
            setupTransaction();
            thread->StartPlane.setValue(selectedObject, planes);
            recomputeFeature();
            setLinkSubText(ui->startEdit, thread->StartPlane);
            ui->selectStart->setChecked(false);
            Gui::Selection().clearSelection();
            break;
        }

        case UpToGeometrySel: {
            auto thread = getObject<PartDesign::Thread>();
            std::vector<std::string> planes;
            App::DocumentObject* selectedObject = nullptr;
            getReferencedSelection(thread, msg, selectedObject, planes);
            if (!selectedObject) {
                return;
            }
            setupTransaction();
            thread->UpToGeometry.setValue(selectedObject, planes);
            recomputeFeature();
            setLinkSubText(ui->upToGeometryEdit, thread->UpToGeometry);
            ui->selectUpToGeometry->setChecked(false);
            Gui::Selection().clearSelection();
            break;
        }

        default:
            break;
    }
}


void TaskThreadParameters::threadTypeChanged(int index)
{
    if (index < 0) {
        return;
    }

    auto thread = getObject<PartDesign::Thread>();
    if (!thread) {
        return;
    }

    const QString threadClassText = ui->classCombo->currentText();

    setupTransaction();
    thread->ThreadType.setValue(index);

    const int threadClassIndex = ui->classCombo->findText(threadClassText, Qt::MatchContains);
    if (threadClassIndex > -1) {
        ui->classCombo->setCurrentIndex(threadClassIndex);
    }

    recomputeFeature();
}

void TaskThreadParameters::threadSizeChanged(int index)
{
    if (index < 0) {
        return;
    }

    auto thread = getObject<PartDesign::Thread>();
    if (thread) {
        setupTransaction();
        thread->ThreadSize.setValue(index);
        recomputeFeature();
    }
}

void TaskThreadParameters::threadSizePitchChanged(int index)
{
    if (index < 0) {
        return;
    }

    auto thread = getObject<PartDesign::Thread>();
    if (thread) {
        setupTransaction();
        thread->ThreadSizePitch.setValue(index);
        recomputeFeature();
    }
}

void TaskThreadParameters::depthTypeChanged(int index)
{
    auto thread = getObject<PartDesign::Thread>();
    if (!thread) {
        return;
    }
    setupTransaction();
    thread->DepthType.setValue(index);

    const bool isDimension = std::string(thread->DepthType.getValueAsString()) == "Dimension";
    const bool isUpToGeometry = std::string(thread->DepthType.getValueAsString()) == "UpToGeometry";
    ui->labelDepth->setHidden(!isDimension);
    ui->Depth->setHidden(!isDimension);
    ui->upToGeometryWidget->setHidden(!isUpToGeometry);
    recomputeFeature();
}

void TaskThreadParameters::depthChanged(double value)
{
    if (auto thread = getObject<PartDesign::Thread>()) {
        setupTransaction();
        thread->Depth.setValue(value);
        recomputeFeature();
    }
}

void TaskThreadParameters::threadClassChanged(int index)
{
    if (index < 0) {
        return;
    }

    if (auto thread = getObject<PartDesign::Thread>()) {
        setupTransaction();
        thread->ThreadClass.setValue(index);
        recomputeFeature();
    }
}

void TaskThreadParameters::threadDirectionChanged(int index)
{
    if (index < 0) {
        return;
    }

    if (auto thread = getObject<PartDesign::Thread>()) {
        setupTransaction();
        thread->ThreadDirection.setValue(index);
        recomputeFeature();
    }
}

void TaskThreadParameters::useCustomThreadClearanceChanged()
{
    if (auto thread = getObject<PartDesign::Thread>()) {
        setupTransaction();
        thread->UseCustomThreadClearance.setValue(ui->customClearanceCheck->isChecked());
        ui->customClearanceField->setEnabled(ui->customClearanceCheck->isChecked());
        ui->classCombo->setEnabled(!ui->customClearanceCheck->isChecked());

        recomputeFeature();
    }
}

void TaskThreadParameters::taperedCheckChanged()
{
    if (auto thread = getObject<PartDesign::Thread>()) {
        setupTransaction();
        thread->Tapered.setValue(ui->taperedCheck->isChecked());
        ui->taperedAngleField->setEnabled(ui->taperedCheck->isChecked());
        recomputeFeature();
    }
}

void TaskThreadParameters::threadModelChanged()
{
    if (auto thread = getObject<PartDesign::Thread>()) {
        setupTransaction();
        if (sender() == ui->cosmeticThreadRadio) {
            thread->CosmeticThread.setValue(true);
            thread->ModelThread.setValue(false);
        }
        else {
            thread->CosmeticThread.setValue(false);
            thread->ModelThread.setValue(true);
        }
        recomputeFeature();
    }
}

void TaskThreadParameters::customThreadClearanceChanged(double value)
{
    if (auto thread = getObject<PartDesign::Thread>()) {
        setupTransaction();
        thread->CustomThreadClearance.setValue(value);
        recomputeFeature();
    }
}

void TaskThreadParameters::taperedAngleChanged(double value)
{
    if (auto thread = getObject<PartDesign::Thread>()) {
        setupTransaction();
        thread->TaperedAngle.setValue(value);
        recomputeFeature();
    }
}


void TaskThreadParameters::changedObject(const App::Document&, const App::Property& property)
{
    auto thread = getObject<PartDesign::Thread>();
    if (!thread) {
        return;
    }
    const bool readOnly = property.isReadOnly();

    const auto updateCheckable = [&](QCheckBox* widget, bool value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setChecked(value);
        widget->setDisabled(readOnly);
    };

    const auto updateRadio = [&](QRadioButton* widget, bool value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setChecked(value);
        widget->setDisabled(readOnly);
    };

    const auto updateComboBox = [&](QComboBox* widget, int value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setCurrentIndex(value);
        widget->setDisabled(readOnly);
    };

    const auto updateSpinBox = [&](Gui::PrefQuantitySpinBox* widget, double value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setValue(value);
        widget->setDisabled(readOnly);
    };

    const auto updateComboBoxItems = [](QComboBox* widget, const auto& values, int selected) {
        QSignalBlocker blocker(widget);
        widget->clear();
        for (const auto& value : values) {
            widget->addItem(QString::fromStdString(value));
        }
        widget->setCurrentIndex(selected);
    };

    if (&property == &thread->ThreadType) {
        ui->standardCombo->setEnabled(true);
        updateComboBox(ui->standardCombo, thread->ThreadType.getValue());

        updateComboBoxItems(
            ui->diameterCombo,
            thread->ThreadSize.getEnumVector(),
            thread->ThreadSize.getValue()
        );

        std::vector<std::string> translatedClassTypes;
        for (const auto& it : thread->ThreadClass.getEnumVector()) {
            translatedClassTypes.push_back(tr(it.c_str()).toStdString());
        }
        updateComboBoxItems(ui->classCombo, translatedClassTypes, thread->ThreadClass.getValue());

        ui->designationEdit->setText(thread->ThreadDesignation.getValue());
    }
    else if (&property == &thread->ThreadClass) {
        QSignalBlocker blocker(ui->classCombo);
        ui->classCombo->clear();
        for (const auto& value : thread->ThreadClass.getEnumVector()) {
            ui->classCombo->addItem(tr(value.c_str()));
        }
        ui->classCombo->setCurrentIndex(thread->ThreadClass.getValue());
    }
    else if (&property == &thread->ThreadSize) {
        updateComboBox(ui->diameterCombo, thread->ThreadSize.getValue());
        updateComboBoxItems(
            ui->pitchCombo,
            thread->ThreadSizePitch.getEnumVector(),
            thread->ThreadSizePitch.getValue()
        );

        ui->designationEdit->setText(thread->ThreadDesignation.getValue());
    }
    else if (&property == &thread->ThreadSizePitch) {
        updateComboBox(ui->pitchCombo, thread->ThreadSizePitch.getValue());
        ui->designationEdit->setText(thread->ThreadDesignation.getValue());
    }
    else if (&property == &thread->ThreadDesignation) {
        ui->designationEdit->setText(thread->ThreadDesignation.getValue());
    }
    else if (&property == &thread->LateralFace) {
        setLinkSubText(ui->lateralFaceEdit, thread->LateralFace);
    }
    else if (&property == &thread->StartPlane) {
        setLinkSubText(ui->startEdit, thread->StartPlane);
    }
    else if (&property == &thread->UpToGeometry) {
        setLinkSubText(ui->upToGeometryEdit, thread->UpToGeometry);
    }
    else if (&property == &thread->Depth) {
        updateSpinBox(ui->Depth, thread->Depth.getValue());
    }
    else if (&property == &thread->ModelThread || &property == &thread->CosmeticThread) {
        updateRadio(ui->modelledThreadRadio, thread->ModelThread.getValue());
        updateRadio(ui->cosmeticThreadRadio, !thread->ModelThread.getValue());
    }
    else if (&property == &thread->DepthType) {
        ui->endTypeCombo->setEnabled(true);
        updateComboBox(ui->endTypeCombo, thread->DepthType.getValue());
        ui->labelDepth->setHidden(std::string(thread->DepthType.getValueAsString()) != "Dimension");
        ui->Depth->setHidden(std::string(thread->DepthType.getValueAsString()) != "Dimension");
        ui->upToGeometryWidget->setHidden(
            std::string(thread->DepthType.getValueAsString()) != "UpToGeometry"
        );
    }
    else if (&property == &thread->Tapered) {
        updateCheckable(ui->taperedCheck, thread->Tapered.getValue());
        ui->taperedAngleField->setEnabled(
            thread->Tapered.getValue() && !thread->TaperedAngle.isReadOnly()
        );
    }
    else if (&property == &thread->TaperedAngle) {
        updateSpinBox(ui->taperedAngleField, thread->TaperedAngle.getValue());
        ui->taperedAngleField->setEnabled(thread->Tapered.getValue() && !readOnly);
    }
}

void TaskThreadParameters::setThreadSelectionGate()
{
    if (selectionMode == none) {
        Gui::Selection().rmvSelectionGate();
        return;
    }

    AllowSelectionFlags allow;
    allow.setFlag(AllowSelection::EDGE, allowEdges);
    allow.setFlag(AllowSelection::FACE, allowFaces);
    allow.setFlag(AllowSelection::POINT, true);

    Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), allow));
}


TaskDlgThreadParameters::TaskDlgThreadParameters(ViewProviderThread* threadView)
    : TaskDlgDressUpParameters(threadView)
{
    parameter = new TaskThreadParameters(threadView);

    Content.push_back(parameter);

    Content.push_back(preview);
}

TaskDlgThreadParameters::~TaskDlgThreadParameters() = default;

bool TaskDlgThreadParameters::accept()
{
    parameter->apply();
    return TaskDlgDressUpParameters::accept();
}

bool TaskDlgThreadParameters::reject()
{
    auto* thread = getObject<PartDesign::Thread>();
    App::Document* document = thread ? thread->getDocument() : nullptr;
    const std::string threadName = thread && thread->getNameInDocument()
        ? thread->getNameInDocument()
        : "";

    if (auto* threadView = getViewObject<ViewProviderThread>()) {
        threadView->restoreBaseVisibility();
        threadView->clearThreadTextures();
    }

    const bool result = TaskDlgDressUpParameters::reject();

    auto* restoredThread = document && !threadName.empty()
        ? dynamic_cast<PartDesign::Thread*>(document->getObject(threadName.c_str()))
        : nullptr;
    if (restoredThread) {
        if (auto* restoredView = dynamic_cast<ViewProviderThread*>(
                Gui::Application::Instance->getViewProvider(restoredThread)
            )) {
            restoredView->updateOverlay();
        }
    }

    return result;
}

TaskThreadParameters::Observer::Observer(
    TaskThreadParameters* taskPanel,
    const PartDesign::Thread* threadObject
)
    : DocumentObserver(threadObject->getDocument())
    , owner(taskPanel)
    , thread(threadObject)
{}

void TaskThreadParameters::Observer::slotChangedObject(
    const App::DocumentObject& object,
    const App::Property& property
)
{
    if (&object == thread) {
        if (object.getDocument()) {
            owner->changedObject(*object.getDocument(), property);
        }
    }
}

#include "moc_TaskThreadParameters.cpp"
