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
#include <Base/Interpreter.h>
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

TaskThreadParameters::TaskThreadParameters(ViewProviderDressUp* DressUpView, QWidget* parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
    , observer(new Observer(this, getObject<PartDesign::Thread>()))
    , ui(new Ui_TaskThreadParameters)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    QMetaObject::connectSlotsByName(this);

    PartDesign::Thread* pcThread = DressUpView->getObject<PartDesign::Thread>();

    setLinkSubText(ui->lateralFaceEdit, pcThread->LateralFace);
    setLinkSubText(ui->startEdit, pcThread->StartPlane);
    setLinkSubText(ui->upToGeometryEdit, pcThread->UpToGeometry);

    std::vector<std::string> strings = pcThread->Base.getSubValues();

    QMetaObject::connectSlotsByName(this);

    setThreadSelectionMode(SideFaceSel);

    ui->endTypeCombo->setCurrentIndex(pcThread->DepthType.getValue());
    bool isDimension = std::string(pcThread->DepthType.getValueAsString()) == "Dimension";
    bool isUpToGeometry = std::string(pcThread->DepthType.getValueAsString()) == "UpToGeometry";

    ui->labelDepth->setHidden(!isDimension);
    ui->Depth->setHidden(!isDimension);
    ui->upToGeometryWidget->setHidden(!isUpToGeometry);
    ui->Depth->setValue(pcThread->Depth.getValue());

    ui->standardCombo->clear();
    std::vector<std::string> cursorStandard = pcThread->ThreadTypeName.getEnumVector();
    for (const auto& it : cursorStandard) {
        ui->standardCombo->addItem(tr(it.c_str()));
    }
    ui->standardCombo->setCurrentIndex(pcThread->ThreadType.getValue());

    ui->diameterCombo->clear();
    std::vector<std::string> cursorDiameter = pcThread->ThreadSize.getEnumVector();
    for (const auto& it : cursorDiameter) {
        ui->diameterCombo->addItem(tr(it.c_str()));
    }
    ui->diameterCombo->setCurrentIndex(pcThread->ThreadSize.getValue());

    ui->classCombo->clear();
    std::vector<std::string> cursorClass = pcThread->ThreadClass.getEnumVector();
    for (const auto& it : cursorClass) {
        ui->classCombo->addItem(tr(it.c_str()));
    }
    ui->classCombo->setCurrentIndex(pcThread->ThreadClass.getValue());

    ui->pitchCombo->clear();
    std::vector<std::string> cursorPitch = pcThread->ThreadSizePitch.getEnumVector();
    for (const auto& it : cursorPitch) {
        ui->pitchCombo->addItem(tr(it.c_str()));
    }
    ui->pitchCombo->setCurrentIndex(pcThread->ThreadSizePitch.getValue());

    ui->directionCombo->clear();
    std::vector<std::string> cursorDirection = pcThread->ThreadDirection.getEnumVector();
    for (const auto& it : cursorDirection) {
        ui->directionCombo->addItem(tr(it.c_str()));
    }
    ui->directionCombo->setCurrentIndex(pcThread->ThreadDirection.getValue());


    ui->customClearanceCheck->setChecked(pcThread->UseCustomThreadClearance.getValue());
    ui->customClearanceField->setEnabled(ui->customClearanceCheck->isChecked());
    ui->classCombo->setEnabled(!ui->customClearanceCheck->isChecked());
    ui->customClearanceField->setValue(pcThread->CustomThreadClearance.getValue());

    bool isModeled = pcThread->ModelThread.getValue();
    ui->modelledThreadRadio->setChecked(isModeled);

    ui->designationEdit->setReadOnly(true);
    ui->designationEdit->setText(pcThread->ThreadDesignation.getValue());

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
        &TaskThreadParameters::CustomClearanceCheckValuesChanged
    );

    connect(
        ui->customClearanceField,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &TaskThreadParameters::customThreadClearanceChanged
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

void TaskThreadParameters::apply()
{
    auto pcThread = getObject<PartDesign::Thread>();
    if (pcThread->LateralFace.getSubValues().empty()) {
        Base::Console().warning(tr("Empty thread created!\n").toStdString().c_str());
    }
}

void TaskThreadParameters::onRefDeleted()
{
    /*TODO*/
}

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
            auto pcThread = getObject<PartDesign::Thread>();
            std::vector<std::string> planes;
            App::DocumentObject* selObj = nullptr;
            getReferencedSelection(pcThread, msg, selObj, planes);
            if (!selObj) {
                return;
            }
            setupTransaction();
            pcThread->LateralFace.setValue(selObj, planes);
            recomputeFeature();
            setLinkSubText(ui->lateralFaceEdit, pcThread->LateralFace);
            ui->selectLateralFace->setChecked(false);
            Gui::Selection().clearSelection();
            break;
        }

        case StartFaceSel: {
            auto pcThread = getObject<PartDesign::Thread>();
            std::vector<std::string> planes;
            App::DocumentObject* selObj = nullptr;
            getReferencedSelection(pcThread, msg, selObj, planes);
            if (!selObj) {
                return;
            }
            setupTransaction();
            pcThread->StartPlane.setValue(selObj, planes);
            recomputeFeature();
            setLinkSubText(ui->startEdit, pcThread->StartPlane);
            ui->selectStart->setChecked(false);
            Gui::Selection().clearSelection();
            break;
        }

        case UpToGeometrySel: {
            auto pcThread = getObject<PartDesign::Thread>();
            std::vector<std::string> planes;
            App::DocumentObject* selObj = nullptr;
            getReferencedSelection(pcThread, msg, selObj, planes);
            for (const auto& p : planes) {
            }

            if (!selObj) {
                return;
            }
            setupTransaction();
            pcThread->UpToGeometry.setValue(selObj, planes);
            recomputeFeature();
            setLinkSubText(ui->upToGeometryEdit, pcThread->UpToGeometry);
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

    auto pcThread = getObject<PartDesign::Thread>();
    if (!pcThread) {
        return;
    }

    QString ThreadClassString = ui->classCombo->currentText();

    pcThread->ThreadType.setValue(index);

    int threadClassIndex = ui->classCombo->findText(ThreadClassString, Qt::MatchContains);
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
    thread->DepthType.setValue(index);

    bool isDimension = std::string(thread->DepthType.getValueAsString()) == "Dimension";
    bool isUpToGeometry = std::string(thread->DepthType.getValueAsString()) == "UpToGeometry";
    ui->labelDepth->setHidden(!isDimension);
    ui->Depth->setHidden(!isDimension);
    ui->upToGeometryWidget->setHidden(!isUpToGeometry);
    recomputeFeature();
}

void TaskThreadParameters::depthChanged(double value)
{
    if (auto thread = getObject<PartDesign::Thread>()) {
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
        thread->ThreadDirection.setValue(index);
        recomputeFeature();
    }
}

void TaskThreadParameters::CustomClearanceCheckValuesChanged()
{
    if (auto thread = getObject<PartDesign::Thread>()) {
        thread->UseCustomThreadClearance.setValue(ui->customClearanceCheck->isChecked());
        ui->customClearanceField->setEnabled(ui->customClearanceCheck->isChecked());
        ui->classCombo->setEnabled(!ui->customClearanceCheck->isChecked());

        recomputeFeature();
    }
}

void TaskThreadParameters::threadModelChanged()
{
    if (auto thread = getObject<PartDesign::Thread>()) {
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
        thread->CustomThreadClearance.setValue(value);
        recomputeFeature();
    }
}

void TaskThreadParameters::changedObject(const App::Document&, const App::Property& Prop)
{
    auto thread = getObject<PartDesign::Thread>();
    if (!thread) {
        return;  // happens when aborting the command
    }
    bool ro = Prop.isReadOnly();

    Base::Console().log("Parameter %s was updated\n", Prop.getName());

    auto updateCheckable = [&](QCheckBox* widget, bool value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setChecked(value);
        widget->setDisabled(ro);
    };

    auto updateRadio = [&](QRadioButton* widget, bool value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setChecked(value);
        widget->setDisabled(ro);
    };

    auto updateComboBox = [&](QComboBox* widget, int value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setCurrentIndex(value);
        widget->setDisabled(ro);
    };

    if (&Prop == &thread->ThreadType) {
        ui->standardCombo->setEnabled(true);
        updateComboBox(ui->standardCombo, thread->ThreadType.getValue());

        // Thread type also updates related properties
        auto updateComboBoxItems = [&](QComboBox* widget, const auto& values, int selected) {
            QSignalBlocker blocker(widget);
            widget->clear();
            for (const auto& it : values) {
                widget->addItem(QString::fromStdString(it));
            }
            widget->setCurrentIndex(selected);
        };

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
    else if (&Prop == &thread->ThreadSize) {
        updateComboBox(ui->diameterCombo, thread->ThreadSize.getValue());
        // Thread size also updates related properties
        auto updateComboBoxItems = [&](QComboBox* widget, const auto& values, int selected) {
            QSignalBlocker blocker(widget);
            widget->clear();
            for (const auto& it : values) {
                widget->addItem(QString::fromStdString(it));
            }
            widget->setCurrentIndex(selected);
        };

        updateComboBoxItems(
            ui->pitchCombo,
            thread->ThreadSizePitch.getEnumVector(),
            thread->ThreadSizePitch.getValue()
        );

        ui->designationEdit->setText(thread->ThreadDesignation.getValue());
    }
    else if (&Prop == &thread->ThreadSizePitch) {

        ui->designationEdit->setText(thread->ThreadDesignation.getValue());
    }
    else if (&Prop == &thread->DepthType) {
        ui->endTypeCombo->setEnabled(true);
        updateComboBox(ui->endTypeCombo, thread->DepthType.getValue());
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


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgThreadParameters::TaskDlgThreadParameters(ViewProviderThread* DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter = new TaskThreadParameters(DressUpView);

    Content.push_back(parameter);

    Content.push_back(preview);
}

TaskDlgThreadParameters::~TaskDlgThreadParameters() = default;

bool TaskDlgThreadParameters::accept()
{
    parameter->apply();
    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskThreadParameters.cpp"

TaskThreadParameters::Observer::Observer(TaskThreadParameters* _owner, PartDesign::Thread* _thread)
    : DocumentObserver(_thread->getDocument())
    , owner(_owner)
    , thread(_thread)
{}

void TaskThreadParameters::Observer::slotChangedObject(
    const App::DocumentObject& Obj,
    const App::Property& Prop
)
{
    if (&Obj == thread) {
        Base::Console().log("Parameter %s was updated with a new value\n", Prop.getName());
        if (Obj.getDocument()) {
            owner->changedObject(*Obj.getDocument(), Prop);
        }
    }
}
