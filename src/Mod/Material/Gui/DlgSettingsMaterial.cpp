/***************************************************************************
 *   Copyright (c) 2018 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemElmerImp.cpp                   *
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

#include "DlgSettingsMaterial.h"
#include "ui_DlgSettingsMaterial.h"


using namespace MatGui;

DlgSettingsMaterial::DlgSettingsMaterial(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsMaterial)
{
    ui->setupUi(this);
}

DlgSettingsMaterial::~DlgSettingsMaterial()
{
    // no need to delete child widgets, Qt does it all for us
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