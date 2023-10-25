/***************************************************************************
 *   Copyright (c) 2017 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemCcx.cpp                        *
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

#include "DlgSettingsFemExportAbaqusImp.h"
#include "ui_DlgSettingsFemExportAbaqus.h"


using namespace FemGui;

DlgSettingsFemExportAbaqusImp::DlgSettingsFemExportAbaqusImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemExportAbaqus)
{
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsFemExportAbaqusImp::~DlgSettingsFemExportAbaqusImp() = default;

void DlgSettingsFemExportAbaqusImp::saveSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Abaqus");
    hGrp->SetInt("AbaqusElementChoice", ui->comboBoxElemChoiceParam->currentIndex());

    ui->comboBoxElemChoiceParam->onSave();
    ui->checkBoxWriteGroups->onSave();
}

void DlgSettingsFemExportAbaqusImp::loadSettings()
{
    ui->comboBoxElemChoiceParam->onRestore();
    ui->checkBoxWriteGroups->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Abaqus");
    int index = hGrp->GetInt("AbaqusElementChoice", 0);
    if (index > -1) {
        ui->comboBoxElemChoiceParam->setCurrentIndex(index);
    }
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemExportAbaqusImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        int c_index = ui->comboBoxElemChoiceParam->currentIndex();
        ui->retranslateUi(this);
        ui->comboBoxElemChoiceParam->setCurrentIndex(c_index);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsFemExportAbaqusImp.cpp"
