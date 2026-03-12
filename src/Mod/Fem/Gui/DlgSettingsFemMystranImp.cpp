// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2021 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemGeneralImp.cpp                 *
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

#include "DlgSettingsFemMystranImp.h"
#include "ui_DlgSettingsFemMystran.h"


using namespace FemGui;

DlgSettingsFemMystranImp::DlgSettingsFemMystranImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemMystranImp)
{
    ui->setupUi(this);

    connect(
        ui->fc_mystran_binary_path,
        &Gui::PrefFileChooser::fileNameSelected,
        this,
        &DlgSettingsFemMystranImp::onfileNameSelected
    );
}

DlgSettingsFemMystranImp::~DlgSettingsFemMystranImp() = default;

void DlgSettingsFemMystranImp::saveSettings()
{
    ui->fc_mystran_binary_path->onSave();
    ui->cb_mystran_write_comments->onSave();
}

void DlgSettingsFemMystranImp::loadSettings()
{
    ui->fc_mystran_binary_path->onRestore();
    ui->cb_mystran_write_comments->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemMystranImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsFemMystranImp::onfileNameSelected(const QString& fileName)
{
    if (!fileName.isEmpty() && QStandardPaths::findExecutable(fileName).isEmpty()) {
        QMessageBox::critical(this, tr("Mystran"), tr("Executable '%1' not found").arg(fileName));
    }
}

#include "moc_DlgSettingsFemMystranImp.cpp"
