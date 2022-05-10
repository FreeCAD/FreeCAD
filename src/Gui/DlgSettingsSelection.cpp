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

DlgSettingsSelection::~DlgSettingsSelection()
{
}

void DlgSettingsSelection::saveSettings()
{
    auto handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    handle->SetBool("SyncView", ui->checkBoxAutoSwitch->isChecked());
    handle->SetBool("SyncSelection", ui->checkBoxAutoExpand->isChecked());
    handle->SetBool("PreSelection", ui->checkBoxPreselect->isChecked());
    handle->SetBool("RecordSelection", ui->checkBoxRecord->isChecked());
    handle->SetBool("CheckBoxesSelection", ui->checkBoxSelectionCheckBoxes->isChecked());
}

void DlgSettingsSelection::loadSettings()
{
    auto handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    ui->checkBoxAutoSwitch->setChecked(handle->GetBool("SyncView", true));
    ui->checkBoxAutoExpand->setChecked(handle->GetBool("SyncSelection", true));
    ui->checkBoxPreselect->setChecked(handle->GetBool("PreSelection", true));
    ui->checkBoxRecord->setChecked(handle->GetBool("RecordSelection", true));
    ui->checkBoxSelectionCheckBoxes->setChecked(handle->GetBool("CheckBoxesSelection"));
}

void DlgSettingsSelection::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsSelection.cpp"

