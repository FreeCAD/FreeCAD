// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemCcxImp.cpp                     *
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

#include <QMessageBox>
#include <QStandardPaths>

#include "DlgSettingsFemElmerImp.h"
#include "ui_DlgSettingsFemElmer.h"


using namespace FemGui;

DlgSettingsFemElmerImp::DlgSettingsFemElmerImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemElmerImp)
{
    ui->setupUi(this);

    connect(
        ui->fc_grid_binary_path,
        &Gui::PrefFileChooser::fileNameSelected,
        this,
        &DlgSettingsFemElmerImp::onfileNameSelected
    );
    connect(
        ui->fc_elmer_binary_path,
        &Gui::PrefFileChooser::fileNameSelected,
        this,
        &DlgSettingsFemElmerImp::onfileNameSelected
    );
}

DlgSettingsFemElmerImp::~DlgSettingsFemElmerImp() = default;

void DlgSettingsFemElmerImp::saveSettings()
{
    ui->fc_elmer_binary_path->onSave();
    ui->fc_grid_binary_path->onSave();

    ui->sb_num_tasks->onSave();
    ui->sb_threads_per_task->onSave();

    ui->ckb_binary_format->onSave();
    ui->ckb_geom_id->onSave();
}

void DlgSettingsFemElmerImp::loadSettings()
{
    ui->fc_elmer_binary_path->onRestore();
    ui->fc_grid_binary_path->onRestore();

    ui->sb_num_tasks->onRestore();
    ui->sb_threads_per_task->onRestore();

    ui->ckb_binary_format->onRestore();
    ui->ckb_geom_id->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemElmerImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsFemElmerImp::onfileNameSelected(const QString& fileName)
{
    if (!fileName.isEmpty() && QStandardPaths::findExecutable(fileName).isEmpty()) {
        QMessageBox::critical(this, tr("Elmer"), tr("Executable '%1' not found").arg(fileName));
    }
}

#include "moc_DlgSettingsFemElmerImp.cpp"
