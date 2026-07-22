// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QWidget>
#include <QEvent>
#include <QLineEdit>

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>

#include <Mod/PartDesign/Gui/ReferenceSelection.h>
#include <Mod/PartDesign/App/FeatureThread.h>

#include "TaskDressUpParameters.h"

#include "ui_TaskThreadParameters.h"
#include "TaskThreadParameters.h"

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

    QMetaObject::connectSlotsByName(this);  // TODO: what this does?

    PartDesign::Thread* pcThread = DressUpView->getObject<PartDesign::Thread>();

    // Reloading Lateral Face
    const std::vector<std::string>& subNamesLateral = pcThread->LateralFace.getSubValues();
    if (!subNamesLateral.empty()) {
        ui->lateralFaceEdit->setText(QString::fromStdString(subNamesLateral.front()));
    }
    const std::vector<std::string>& subNamesStart = pcThread->StartPlane.getSubValues();
    if (!subNamesStart.empty()) {
        ui->startEdit->setText(QString::fromStdString(subNamesStart.front()));
    }

    setUpUI(pcThread);

    std::vector<std::string> strings = pcThread->Base.getSubValues();
    // for (const auto& string : strings) {
    // ui->listWidgetReferences->addItem(QString::fromStdString(string));
    // }


    QMetaObject::connectSlotsByName(this);

    setThreadSelectionMode(SideFaceSel);

    ui->endTypeCombo->setCurrentIndex(pcThread->DepthType.getValue());
    
    // TODO: change hardcoded for enum loop
    // TODO: change ui->standardCombo for a better name
    ui->standardCombo->addItem(tr("None"), QByteArray("None"));
    ui->standardCombo->addItem(tr("ISO metric regular"), QByteArray("ISO"));
    ui->standardCombo->addItem(tr("ISO metric fine"), QByteArray("ISO"));
    ui->standardCombo->addItem(tr("UTS coarse"), QByteArray("UTS"));
    ui->standardCombo->addItem(tr("UTS fine"), QByteArray("UTS"));
    ui->standardCombo->addItem(tr("UTS extra fine"), QByteArray("UTS"));
    ui->standardCombo->addItem(tr("ANSI pipes"), QByteArray("UTS"));
    ui->standardCombo->addItem(tr("ISO/BSP pipes"), QByteArray("ISO"));
    ui->standardCombo->addItem(tr("BSW whitworth"), QByteArray("Other"));
    ui->standardCombo->addItem(tr("BSF whitworth fine"), QByteArray("Other"));
    ui->standardCombo->addItem(tr("ISO tyre valves"), QByteArray("Other"));
    ui->standardCombo->setCurrentIndex(pcThread->ThreadType.getValue());
    
    // diameter
    ui->diameterCombo->clear();
    std::vector<std::string> cursorDiameter = pcThread->ThreadSize.getEnumVector();
    for (const auto& it : cursorDiameter) {
        ui->diameterCombo->addItem(tr(it.c_str()));
    }
    ui->diameterCombo->setCurrentIndex(pcThread->ThreadSize.getValue());
    
    // class
    ui->classCombo->clear();
    std::vector<std::string> cursorClass = pcThread->ThreadClass.getEnumVector();
    for (const auto& it : cursorClass) {
            ui->classCombo->addItem(tr(it.c_str()));
    }
    ui->classCombo->setCurrentIndex(pcThread->ThreadClass.getValue());
        
    // pitch
    ui->pitchCombo->clear();
    std::vector<std::string> cursorPitch = pcThread->ThreadSizePitch.getEnumVector();
    for (const auto& it : cursorPitch) {
        ui->pitchCombo->addItem(tr(it.c_str()));
    }
    ui->pitchCombo->setCurrentIndex(pcThread->ThreadSizePitch.getValue());

    // thread direction
    ui->directionCombo->clear();
    std::vector<std::string> cursorDirection = pcThread->ThreadDirection.getEnumVector();
    for (const auto& it : cursorDirection) {
        ui->directionCombo->addItem(tr(it.c_str()));
    }
    ui->directionCombo->setCurrentIndex(pcThread->ThreadDirection.getValue());

    ui->customClearanceCheck->setChecked(pcThread->UseCustomThreadClearance.getValue());

    bool isModeled = pcThread->ModelThread.getValue();
    ui->modelledThreadRadio->setChecked(isModeled);

    connect(ui->standardCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskThreadParameters::threadTypeChanged);

    connect(ui->diameterCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskThreadParameters::threadSizeChanged);
    
    connect(ui->pitchCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskThreadParameters::threadSizePitchChanged);
    
    connect(ui->classCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskThreadParameters::threadClassChanged);
    
    connect(ui->directionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskThreadParameters::threadDirectionChanged);

    // TODO: verificar se essa sentença ainda serve e tirar daqui
    connect(ui->lateralFaceEdit, &QLineEdit::textChanged, this, &TaskThreadParameters::QLineEditSelected);

    connect(ui->selectLateralFace, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            Base::Console().message("então é lateral face\n");
            // ui->selectLateralFace->setChecked(false); // Desliga o outro
            setThreadSelectionMode(SideFaceSel);
        }
        else if (currentSelectionMode == SideFaceSel) {
            Base::Console().message("não é mais lateral face\n");
            setThreadSelectionMode(None);
        }
    });

    connect(ui->selectStart, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            Base::Console().message("então é start face\n");
            // ui->selectStart->setChecked(false); // Desliga o outro
            setThreadSelectionMode(StartFaceSel);
        }
        else if (currentSelectionMode == StartFaceSel) {
            Base::Console().message("não é mais start face\n");
            // Base::Console().message("então é start face\n");
            setThreadSelectionMode(None);
        }
    });

    connect(ui->endTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskThreadParameters::depthChanged);

    connect(ui->customClearanceCheck, &QCheckBox::toggled, this, &TaskThreadParameters::CustomClearanceCheckValuesChanged);

    connect(ui->cosmeticThreadRadio, &QRadioButton::clicked, this, &TaskThreadParameters::threadModelChanged);

    connect(ui->modelledThreadRadio, &QRadioButton::clicked, this, &TaskThreadParameters::threadModelChanged);

    // NOLINTBEGIN
    connectPropChanged = App::GetApplication().signalChangePropertyEditor.connect(
        std::bind(&TaskThreadParameters::changedObject, this, sp::_1, sp::_2)
    );
    // NOLINTEND

    if (connectPropChanged.connected()) {
        Base::Console().message("✅ signalChangePropertyEditor connected successfully!\n");
    } else {
        Base::Console().error("❌ Failed to connect signalChangePropertyEditor!\n");
    }

    // setupGizmos(DressUpView);

    if (strings.size() == 0) {
        setSelectionMode(refSel);
    }
    else {
        hideOnError();
    }
}

void TaskThreadParameters::setUpUI(PartDesign::Thread* pcThread)
{}

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
{}

void TaskThreadParameters::onRefDeleted()
{}

void TaskThreadParameters::setButtons(const PartDesignGui::TaskDressUpParameters::selectionModes mode)
{
    Q_UNUSED(mode);
}

// void TaskThreadParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
// {
//     // executed when the user selected something in the CAD object
//     // adds/deletes the selection accordingly
//     Base::Console().message("SELECTIONCHANGED\n");

//     // 1. Tratamento quando algo é selecionado no CAD
//     if (msg.Type == Gui::SelectionChanges::AddSelection) {
//         Base::Console().message("ta na hora campeao\n");
//         if (selectionMode == refSel) {
//             referenceQLineEditSelected(msg, ui->lateralFaceEdit);
//         }
//     }

// }

void TaskThreadParameters::setThreadSelectionMode(threadSelectionModes mode)
{
    Base::Console().message("works!\n");
    currentSelectionMode = mode;

    // Atualiza o estado visual dos botões
    // ui->selectLateralFace->setChecked(mode == SideFaceSel);
    // ui->selectStart->setChecked(mode == StartFaceSel);

    // Configura os Gates de Seleção do FreeCAD se necessário
    // (ex: restringir para que apenas FACES possam ser clicadas)
    // if (mode != None) {
    // Gui::Selection().addSelectionGate(new Gui::SelectionGateFilter("HasSubElement:Face"));
    // } else {
    // Gui::Selection().rmvSelectionGate();
    // }
}

void TaskThreadParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    Base::Console().message("SELECTIONCHANGED\n");

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

            // ui->lateralFaceEdit->setText(QString::fromStdString(msg.SubName));
            referenceQLineEditSelected(msg, ui->lateralFaceEdit);
            // Opcional: Se for seleção única, desmarca o botão após escolher
            ui->selectLateralFace->setChecked(false);
            break;
        }

        case StartFaceSel: {
            // ui->startEdit->setText(QString::fromStdString(msg.SubName));
            auto pcThread = getObject<PartDesign::Thread>();
            std::vector<std::string> planes;
            App::DocumentObject* selObj = nullptr;
            getReferencedSelection(pcThread, msg, selObj, planes);
            if (!selObj) {
                return;
            }
            setupTransaction();
            pcThread->StartPlane.setValue(selObj, planes);

            referenceQLineEditSelected(msg, ui->startEdit);
            ui->selectStart->setChecked(false);
            break;
        }

        default:
            break;
    }
}

void TaskThreadParameters::QLineEditSelected(const QString& text)
{
    Base::Console().message("SINAL RECEBIDO DE QLINEEDIT");
}

void TaskThreadParameters::threadTypeChanged(int index)
{
    Base::Console().message("ESTOU mudando TYPE\n");
    if (index < 0) {
        return;
    }
    
    auto pcThread = getObject<PartDesign::Thread>();
    if (!pcThread) {
        return;
    }
    
    // store the current class
    QString ThreadClassString = ui->classCombo->currentText();

    // now set the new type, this will reset the comboboxes to item 0
    pcThread->ThreadType.setValue(index);
    Base::Console().message("MUDEI THREAD TYPE\n");

    // TODO: check a lot of new type consequences

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

        // apply the recompute result to the widgets
        // ui->HoleCutCustomValues->setDisabled(hole->HoleCutCustomValues.isReadOnly());
        // ui->HoleCutCustomValues->setChecked(hole->HoleCutCustomValues.getValue());
    }

    // pitch
    // ui->pitchCombo->clear();
    // std::vector<std::string> cursorPitch = thread->ThreadSizePitch.getEnumVector();
    // for (const auto& it : cursorPitch) {
    //     ui->pitchCombo->addItem(tr(it.c_str()));
    // }
    // ui->pitchCombo->setCurrentIndex(thread->ThreadSizePitch.getValue());
}

void TaskThreadParameters::threadSizePitchChanged(int index){
    if (index < 0) {
        return;
    }

    auto thread = getObject<PartDesign::Thread>();
    if (thread) {
        thread->ThreadSizePitch.setValue(index);
        recomputeFeature();

        // apply the recompute result to the widgets
        // ui->HoleCutCustomValues->setDisabled(hole->HoleCutCustomValues.isReadOnly());
        // ui->HoleCutCustomValues->setChecked(hole->HoleCutCustomValues.getValue());
    }
}

void TaskThreadParameters::depthChanged(int index)
{
    auto thread = getObject<PartDesign::Thread>();
    if (!thread) {
        return;
    }
    thread->DepthType.setValue(index);
    recomputeFeature();
    // enabling must be handled after recompute
    // bool DepthisDimension = (std::string(hole->DepthType.getValueAsString()) == "Dimension");
    // ui->DrillPointAngled->setEnabled(DepthisDimension);
    // ui->DrillPointAngle->setEnabled(DepthisDimension);
    // ui->DrillForDepth->setEnabled(DepthisDimension);
    // setCutDiagram();

    // setGizmoPositions();
}

void TaskThreadParameters::threadClassChanged(int index){
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

void TaskThreadParameters::CustomClearanceCheckValuesChanged(){
    if (auto thread = getObject<PartDesign::Thread>()) {
        thread->UseCustomThreadClearance.setValue(ui->customClearanceCheck->isChecked());

        if (ui->customClearanceCheck->isChecked()) {
            // ui->HoleCutDiameter->setEnabled(true);
            // ui->HoleCutDepth->setEnabled(true);
            // if (!hole->HoleCutCountersinkAngle.isReadOnly()) {
                // ui->HoleCutCountersinkAngle->setEnabled(true);
            // }
        }
        else {
            // ui->HoleCutDiameter->setEnabled(false);
            // ui->HoleCutDepth->setEnabled(false);
            // ui->HoleCutCountersinkAngle->setEnabled(false);
        }

        recomputeFeature();
    }
}

void TaskThreadParameters::threadModelChanged()
{
    if (auto thread = getObject<PartDesign::Thread>()) {
        if (sender() == ui->cosmeticThreadRadio) {
            // hole->ThreadDirection.setValue(0L);
            thread->CosmeticThread.setValue(true);
            thread->ModelThread.setValue(false);
        }
        else {
            thread->CosmeticThread.setValue(false);
            thread->ModelThread.setValue(true);
            // hole->ThreadDirection.setValue(1L);
        }
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

    // auto updateSpinBox = [&](Gui::PrefQuantitySpinBox* widget, double value) {
    //     [[maybe_unused]] QSignalBlocker blocker(widget);
    //     widget->setValue(value);
    //     widget->setDisabled(ro);
    // };

    // if (&Prop == &hole->Threaded || &Prop == &hole->CosmeticThread) {
    //     updateHoleTypeCombo();
    // }
    // else if (&Prop == &hole->ModelThread) {
    //     updateHoleTypeCombo();
    //     updateCheckable(ui->ModelThread, hole->ModelThread.getValue());
    // }
    // else 
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

        // std::vector<std::string> translatedCutTypes;
        // for (const auto& it : hole->HoleCutType.getEnumVector()) {
        //     translatedCutTypes.push_back(tr(it.c_str()).toStdString());
        // }
        // updateComboBoxItems(ui->HoleCutType, translatedCutTypes, hole->HoleCutType.getValue());

        std::vector<std::string> translatedClassTypes;
        for (const auto& it : thread->ThreadClass.getEnumVector()) {
            translatedClassTypes.push_back(tr(it.c_str()).toStdString());
        }
        updateComboBoxItems(ui->classCombo, translatedClassTypes, thread->ThreadClass.getValue());
    }
    else if (&Prop == &thread->ThreadSize) {
        // ui->ThreadSize->setEnabled(true);
        // updateComboBox(ui->standardCombo, thread->ThreadSize.getValue());

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
    } else if (&Prop == &thread->DepthType) {
        ui->endTypeCombo->setEnabled(true);
        updateComboBox(ui->endTypeCombo, thread->DepthType.getValue());
    }
    // else if (&Prop == &hole->ThreadClass) {
    //     ui->ThreadClass->setEnabled(true);
    //     updateComboBox(ui->ThreadClass, hole->ThreadClass.getValue());
    // }
    // else if (&Prop == &hole->ThreadFit) {
    //     ui->ThreadFit->setEnabled(true);
    //     updateComboBox(ui->ThreadFit, hole->ThreadFit.getValue());
    // }
    // else if (&Prop == &hole->Diameter) {
    //     ui->Diameter->setEnabled(true);
    //     updateSpinBox(ui->Diameter, hole->Diameter.getValue());
    //     updateHoleCutLimits(hole);
    // }
    // else if (&Prop == &hole->ThreadDirection) {
    //     ui->directionRightHand->setEnabled(true);
    //     ui->directionLeftHand->setEnabled(true);

    //     std::string direction(hole->ThreadDirection.getValueAsString());
    //     updateRadio(ui->directionRightHand, direction == "Right");
    //     updateRadio(ui->directionLeftHand, direction == "Left");
    // }
    // else if (&Prop == &hole->HoleCutType) {
    //     ui->HoleCutType->setEnabled(true);
    //     updateComboBox(ui->HoleCutType, hole->HoleCutType.getValue());
    // }
    // else if (&Prop == &hole->HoleCutDiameter) {
    //     ui->HoleCutDiameter->setEnabled(true);
    //     updateSpinBox(ui->HoleCutDiameter, hole->HoleCutDiameter.getValue());
    // }
    // else if (&Prop == &hole->HoleCutDepth) {
    //     ui->HoleCutDepth->setEnabled(true);
    //     updateSpinBox(ui->HoleCutDepth, hole->HoleCutDepth.getValue());
    // }
    // else if (&Prop == &hole->HoleCutCountersinkAngle) {
    //     ui->HoleCutCountersinkAngle->setEnabled(true);
    //     updateSpinBox(ui->HoleCutCountersinkAngle, hole->HoleCutCountersinkAngle.getValue());
    // }
    // else if (&Prop == &hole->DepthType) {
    //     ui->DepthType->setEnabled(true);
    //     updateComboBox(ui->DepthType, hole->DepthType.getValue());
    // }
    // else if (&Prop == &hole->Depth) {
    //     ui->Depth->setEnabled(true);
    //     updateSpinBox(ui->Depth, hole->Depth.getValue());
    // }
    // else if (&Prop == &hole->DrillPoint) {
    //     ui->DrillPointAngled->setEnabled(true);
    //     updateCheckable(
    //         ui->DrillPointAngled,
    //         hole->DrillPoint.getValueAsString() == std::string("Angled")
    //     );
    // }
    // else if (&Prop == &hole->DrillPointAngle) {
    //     ui->DrillPointAngle->setEnabled(true);
    //     updateSpinBox(ui->DrillPointAngle, hole->DrillPointAngle.getValue());
    // }
    // else if (&Prop == &hole->DrillForDepth) {
    //     ui->DrillForDepth->setEnabled(true);
    //     updateCheckable(ui->DrillForDepth, hole->DrillForDepth.getValue());
    // }
    // else if (&Prop == &hole->Tapered) {
    //     ui->Tapered->setEnabled(true);
    //     updateCheckable(ui->Tapered, hole->Tapered.getValue());
    // }
    // else if (&Prop == &hole->TaperedAngle) {
    //     ui->TaperedAngle->setEnabled(true);
    //     updateSpinBox(ui->TaperedAngle, hole->TaperedAngle.getValue());
    // }
    // else if (&Prop == &hole->UseCustomThreadClearance) {
    //     ui->UseCustomThreadClearance->setEnabled(true);
    //     updateCheckable(ui->UseCustomThreadClearance, hole->UseCustomThreadClearance.getValue());
    // }
    // else if (&Prop == &hole->CustomThreadClearance) {
    //     ui->CustomThreadClearance->setEnabled(true);
    //     updateSpinBox(ui->CustomThreadClearance, hole->CustomThreadClearance.getValue());
    // }
    // else if (&Prop == &hole->ThreadDepthType) {
    //     ui->ThreadDepthType->setEnabled(true);
    //     updateComboBox(ui->ThreadDepthType, hole->ThreadDepthType.getValue());
    // }
    // else if (&Prop == &hole->ThreadDepth) {
    //     ui->ThreadDepth->setEnabled(true);
    //     updateSpinBox(ui->ThreadDepth, hole->ThreadDepth.getValue());
    // }
    // else if (&Prop == &hole->BaseProfileType) {
    //     ui->BaseProfileType->setEnabled(true);
    //     updateComboBox(
    //         ui->BaseProfileType,
    //         PartDesign::Hole::baseProfileOption_bitmaskToIdx(hole->BaseProfileType.getValue())
    //     );
    // }
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
