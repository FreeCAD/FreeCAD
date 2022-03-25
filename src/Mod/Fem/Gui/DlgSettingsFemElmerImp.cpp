/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemCcxImp.cpp                     *
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

#include "DlgSettingsFemElmerImp.h"
#include "ui_DlgSettingsFemElmer.h"


using namespace FemGui;

DlgSettingsFemElmerImp::DlgSettingsFemElmerImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemElmerImp)
{
    ui->setupUi(this);
}

DlgSettingsFemElmerImp::~DlgSettingsFemElmerImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsFemElmerImp::saveSettings()
{
    ui->cb_elmer_binary_std->onSave();
    ui->fc_elmer_binary_path->onSave();

    ui->cb_grid_binary_std->onSave();
    ui->fc_grid_binary_path->onSave();
}

void DlgSettingsFemElmerImp::loadSettings()
{
    ui->cb_elmer_binary_std->onRestore();
    ui->fc_elmer_binary_path->onRestore();

    ui->cb_grid_binary_std->onRestore();
    ui->fc_grid_binary_path->onRestore();
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

#include "moc_DlgSettingsFemElmerImp.cpp"
