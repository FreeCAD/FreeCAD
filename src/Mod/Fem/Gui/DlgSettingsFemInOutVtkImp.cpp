/**************************************************************************
 *   Copyright (c) 2018 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemExportAbaqusCcxImp.cpp         *
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


#include <Gui/Application.h>

#include "DlgSettingsFemInOutVtkImp.h"
#include "ui_DlgSettingsFemInOutVtk.h"


using namespace FemGui;

DlgSettingsFemInOutVtkImp::DlgSettingsFemInOutVtkImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemInOutVtk)
{
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsFemInOutVtkImp::~DlgSettingsFemInOutVtkImp() = default;

void DlgSettingsFemInOutVtkImp::saveSettings()
{
    ui->comboBoxVtkImportObject->onSave();
    ui->cb_export_level->onSave();
}

void DlgSettingsFemInOutVtkImp::loadSettings()
{
    ui->comboBoxVtkImportObject->onRestore();

    populateExportLevel();
    ui->cb_export_level->onSave();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemInOutVtkImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        int c_index = ui->comboBoxVtkImportObject->currentIndex();
        ui->retranslateUi(this);
        ui->comboBoxVtkImportObject->setCurrentIndex(c_index);
        populateExportLevel();
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsFemInOutVtkImp::populateExportLevel() const
{
    std::list<std::string> values = {QT_TR_NOOP("All"), QT_TR_NOOP("Highest")};

    ui->cb_export_level->clear();
    for (const auto& val : values) {
        ui->cb_export_level->addItem(tr(val.c_str()), QByteArray::fromStdString(val));
    }

    // set default index
    auto hGrp = ui->cb_export_level->getWindowParameter();
    std::string current = hGrp->GetASCII(ui->cb_export_level->entryName(), "Highest");
    int index = ui->cb_export_level->findData(QByteArray::fromStdString(current));
    ui->cb_export_level->setCurrentIndex(index);
}

#include "moc_DlgSettingsFemInOutVtkImp.cpp"
