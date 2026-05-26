// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QWidget>
#include <QEvent>
#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>

#include "Ui_TaskThreadParameters.h"
#include "TaskThreadParameters.h"

using namespace PartDesignGui;
using namespace Gui;

TaskThreadParameters::TaskThreadParameters(ViewProviderDressUp* DressUpView, QWidget* parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
    , ui(std::make_unique<Ui_TaskThreadParameters>())
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    QMetaObject::connectSlotsByName(this);
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
{}

void TaskThreadParameters::onRefDeleted()
{}

void TaskThreadParameters::setButtons(const PartDesignGui::TaskDressUpParameters::selectionModes mode)
{
    Q_UNUSED(mode);
}

void TaskThreadParameters::onSelectionChanged(const Gui::SelectionChanges& change)
{
    Q_UNUSED(change);
}


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
