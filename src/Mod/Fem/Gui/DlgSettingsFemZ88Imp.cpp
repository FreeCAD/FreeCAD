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

#include <Gui/Application.h>

#include "DlgSettingsFemZ88Imp.h"
#include "ui_DlgSettingsFemZ88.h"


using namespace FemGui;

DlgSettingsFemZ88Imp::DlgSettingsFemZ88Imp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemZ88Imp)
{
    ui->setupUi(this);
}

DlgSettingsFemZ88Imp::~DlgSettingsFemZ88Imp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsFemZ88Imp::saveSettings()
{
    ui->cb_z88_binary_std->onSave();
    ui->fc_z88_binary_path->onSave();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Z88");
    hGrp->SetInt("Solver", ui->cmb_solver->currentIndex());
    ui->cmb_solver->onSave();
    hGrp->SetInt("MaxGS", ui->sb_Z88_MaxGS->value());
    ui->sb_Z88_MaxGS->onSave();
    hGrp->SetInt("MaxKOI", ui->sb_Z88_MaxKOI->value());
    ui->sb_Z88_MaxKOI->onSave();
}

void DlgSettingsFemZ88Imp::loadSettings()
{
    ui->cb_z88_binary_std->onRestore();
    ui->fc_z88_binary_path->onRestore();
    ui->cmb_solver->onRestore();
    ui->sb_Z88_MaxGS->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Z88");
    int index = hGrp->GetInt("Solver", 0);
    if (index > -1)
        ui->cmb_solver->setCurrentIndex(index);
    int places = hGrp->GetInt("MaxGS", 100000000);
    if (places > -1)
        ui->sb_Z88_MaxGS->setValue(places);
    places = hGrp->GetInt("MaxKOI", 2800000);
    if (places > -1)
        ui->sb_Z88_MaxKOI->setValue(places);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemZ88Imp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsFemZ88Imp.cpp"
