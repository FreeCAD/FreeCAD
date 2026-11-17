// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021 Werner Mayer
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


#include <App/Application.h>

#include "DlgSettingsSelection.h"
#include "ui_DlgSettingsSelection.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsSelection */

DlgSettingsSelection::DlgSettingsSelection(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsSelection)
{
    ui->setupUi(this);
}

DlgSettingsSelection::~DlgSettingsSelection() = default;

void DlgSettingsSelection::saveSettings()
{
    ui->checkBoxPreselection->onSave();
    ui->checkBoxSelection->onSave();
    ui->HighlightColor->onSave();
    ui->SelectionColor->onSave();
    ui->spinPickRadius->onSave();
    ui->checkBoxAutoSwitch->onSave();
    ui->checkBoxAutoExpand->onSave();
    ui->checkBoxPreselect->onSave();
    ui->checkBoxRecord->onSave();
    ui->checkBoxSelectionCheckBoxes->onSave();
}

void DlgSettingsSelection::loadSettings()
{
    ui->checkBoxPreselection->onRestore();
    ui->checkBoxSelection->onRestore();
    ui->HighlightColor->onRestore();
    ui->SelectionColor->onRestore();
    ui->spinPickRadius->onRestore();
    ui->checkBoxAutoSwitch->onRestore();
    ui->checkBoxAutoExpand->onRestore();
    ui->checkBoxPreselect->onRestore();
    ui->checkBoxRecord->onRestore();
    ui->checkBoxSelectionCheckBoxes->onRestore();
}

void DlgSettingsSelection::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsSelection.cpp"
