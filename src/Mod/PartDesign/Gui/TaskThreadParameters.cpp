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

TaskThreadParameters::TaskThreadParameters(ViewProviderDressUp* DressUpView, QWidget* parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
    , ui(new Ui_TaskThreadParameters)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    QMetaObject::connectSlotsByName(this);  // TODO: what this does?

    PartDesign::Thread* pcThread = DressUpView->getObject<PartDesign::Thread>();

    setUpUI(pcThread);

    std::vector<std::string> strings = pcThread->Base.getSubValues();
    // for (const auto& string : strings) {
    // ui->listWidgetReferences->addItem(QString::fromStdString(string));
    // }

    QMetaObject::connectSlotsByName(this);

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

    // diameter
    // ui->ThreadSize->clear();
    // std::vector<std::string> cursor = pcHole->ThreadSize.getEnumVector();
    // for (const auto& it : cursor) {
    //     ui->ThreadSize->addItem(tr(it.c_str()));
    // }
    // ui->ThreadSize->setCurrentIndex(pcHole->ThreadSize.getValue());

    // class
    // ui->ThreadClass->clear();
    // cursor = pcHole->ThreadClass.getEnumVector();
    // for (const auto& it : cursor) {
    //     ui->ThreadClass->addItem(tr(it.c_str()));
    // }

    // pitch
    // i think it depends on the class

    connect(
        ui->standardCombo,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskThreadParameters::threadTypeChanged
    );

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
    Base::Console().message("CHANGING THREAD TYPE\n");
    if (index < 0) {
        return;
    }

    auto pcThread = getObject<PartDesign::Thread>();
    if (!pcThread) {
        return;
    }

    // now set the new type, this will reset the comboboxes to item 0
    pcThread->ThreadType.setValue(index);

    // TODO: check a lot of new type consequences

    recomputeFeature();
    Base::Console().message("THREAD TYPE CHANGED\n");
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
