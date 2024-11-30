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

#include "PreCompiled.h"

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
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/InOutVtk");
    hGrp->SetInt("ImportObject", ui->comboBoxVtkImportObject->currentIndex());

    ui->comboBoxVtkImportObject->onSave();
}

void DlgSettingsFemInOutVtkImp::loadSettings()
{
    ui->comboBoxVtkImportObject->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/InOutVtk");
    int index = hGrp->GetInt("ImportObject", 0);
    // 0 is standard on first initialize, 0 .. vtk res obj, 1 .. FEM mesh obj, 2 .. FreeCAD res obj
    if (index > -1) {
        ui->comboBoxVtkImportObject->setCurrentIndex(index);
    }
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
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsFemInOutVtkImp.cpp"
