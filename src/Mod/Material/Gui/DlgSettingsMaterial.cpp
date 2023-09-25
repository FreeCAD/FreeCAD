/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include "DlgSettingsMaterial.h"
#include "ui_DlgSettingsMaterial.h"


using namespace MatGui;

DlgSettingsMaterial::DlgSettingsMaterial(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsMaterial)
{
    ui->setupUi(this);
}

void DlgSettingsMaterial::saveSettings()
{
    ui->cb_use_built_in_materials->onSave();
    ui->cb_use_mat_from_workbenches->onSave();
    ui->cb_use_mat_from_config_dir->onSave();
    ui->cb_use_mat_from_custom_dir->onSave();
    ui->fc_custom_mat_dir->onSave();
    ui->cb_delete_duplicates->onSave();
    ui->cb_sort_by_resources->onSave();

    // Temporary for testing
    ui->cb_legacy_editor->onSave();
}

void DlgSettingsMaterial::loadSettings()
{
    ui->cb_use_built_in_materials->onRestore();
    ui->cb_use_mat_from_workbenches->onRestore();
    ui->cb_use_mat_from_config_dir->onRestore();
    ui->cb_use_mat_from_custom_dir->onRestore();
    ui->fc_custom_mat_dir->onRestore();
    ui->cb_delete_duplicates->onRestore();
    ui->cb_sort_by_resources->onRestore();

    // Temporary for testing
    ui->cb_legacy_editor->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsMaterial::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsMaterial.cpp"
