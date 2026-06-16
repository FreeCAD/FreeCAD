// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QWidget>
#include <QEvent>
#include <QLineEdit>

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>

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

    QMetaObject::connectSlotsByName(this); //TODO: what this does?

    PartDesign::Thread* pcThread = DressUpView->getObject<PartDesign::Thread>();

    setUpUI(pcThread);

    std::vector<std::string> strings = pcThread->Base.getSubValues();
    // for (const auto& string : strings) {
        // ui->listWidgetReferences->addItem(QString::fromStdString(string));
    // }

    QMetaObject::connectSlotsByName(this);

    // setupGizmos(DressUpView);

    if (strings.size() == 0) {
        setSelectionMode(refSel);
    }
    else {
        hideOnError();
    }
}

void TaskThreadParameters::setUpUI(PartDesign::Thread* pcThread)
{
    
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

}

void TaskThreadParameters::onRefDeleted()
{
    
}

void TaskThreadParameters::setButtons(const PartDesignGui::TaskDressUpParameters::selectionModes mode)
{
    Q_UNUSED(mode);
}

void TaskThreadParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // executed when the user selected something in the CAD object
    // adds/deletes the selection accordingly
    Base::Console().message("SELECTIONCHANGED\n");

    // 1. Tratamento quando algo é selecionado no CAD
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        Base::Console().message("ta na hora campeao\n");
        if (selectionMode == refSel) {
            referenceQLineEditSelected(msg, ui->lateralFaceEdit);
        }
    }
 
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
