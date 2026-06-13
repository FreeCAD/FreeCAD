// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Max Wilfinger
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include <QAction>
#include <QListWidget>

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <Mod/PartDesign/App/FeatureDefeaturing.h>

#include "ui_TaskDefeaturingParameters.h"
#include "TaskDefeaturingParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDefeaturingParameters */

TaskDefeaturingParameters::TaskDefeaturingParameters(ViewProviderDressUp* DressUpView, QWidget* parent)
    : TaskDressUpParameters(DressUpView, false, true, parent)
    , ui(new Ui_TaskDefeaturingParameters)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    PartDesign::Defeaturing* pcDefeaturing = DressUpView->getObject<PartDesign::Defeaturing>();

    std::vector<std::string> strings = pcDefeaturing->Base.getSubValues();
    for (const auto& string : strings) {
        ui->listWidgetReferences->addItem(QString::fromStdString(string));
    }

    // clang-format off
    connect(ui->buttonRefSel, &QToolButton::toggled,
            this, &TaskDefeaturingParameters::onButtonRefSel);

    createDeleteAction(ui->listWidgetReferences);
    connect(deleteAction, &QAction::triggered, this, &TaskDefeaturingParameters::onRefDeleted);

    connect(ui->listWidgetReferences, &QListWidget::currentItemChanged,
            this, &TaskDefeaturingParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemClicked,
            this, &TaskDefeaturingParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemDoubleClicked,
            this, &TaskDefeaturingParameters::doubleClicked);
    // clang-format on

    setSelectionMode(refSel);
    hideOnError();
}

void TaskDefeaturingParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionMode == refSel) {
            referenceSelected(msg, ui->listWidgetReferences);
        }
    }
}

void TaskDefeaturingParameters::setButtons(const selectionModes mode)
{
    ui->buttonRefSel->setChecked(mode == refSel);
    ui->buttonRefSel->setText(mode == refSel ? stopSelectionLabel() : startSelectionLabel());
}

void TaskDefeaturingParameters::onRefDeleted()
{
    TaskDressUpParameters::deleteRef(ui->listWidgetReferences);
}

TaskDefeaturingParameters::~TaskDefeaturingParameters()
{
    try {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
    }
    catch (const Py::Exception&) {
        Base::PyException e;
        e.reportException();
    }
}

void TaskDefeaturingParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskDefeaturingParameters::apply()
{
    if (ui->listWidgetReferences->count() == 0) {
        std::string text = tr("Empty defeaturing created!").toStdString();
        Base::Console().warning("%s\n", text.c_str());
    }
}

//**************************************************************************
// TaskDialog
//**************************************************************************

TaskDlgDefeaturingParameters::TaskDlgDefeaturingParameters(ViewProviderDefeaturing* DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter = new TaskDefeaturingParameters(DressUpView);

    Content.push_back(parameter);
    Content.push_back(preview);
}

TaskDlgDefeaturingParameters::~TaskDlgDefeaturingParameters() = default;

bool TaskDlgDefeaturingParameters::accept()
{
    auto obj = getObject();
    if (!obj->isError()) {
        getViewObject()->showPreviousFeature(false);
    }

    parameter->apply();

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskDefeaturingParameters.cpp"
