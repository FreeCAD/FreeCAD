/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
 *   Based on src/Mod/Raytracing/Gui/DlgSettingsRayImp.cpp                 *
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

#include "Gui/Application.h"
#include "DlgSettingsFemImp.h"
#include <Gui/PrefWidgets.h>

using namespace FemGui;

DlgSettingsFemImp::DlgSettingsFemImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
}

DlgSettingsFemImp::~DlgSettingsFemImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsFemImp::saveSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Fem");
    hGrp->SetInt("AnalysisType", cb_analysis_type->currentIndex());

    fc_ccx_working_directory->onSave();
    cb_int_editor->onSave();
    fc_ext_editor->onSave();
    fc_ccx_binary->onSave();
    cb_analysis_type->onSave();
    sb_eigenmode_number->onSave();
    dsb_eigenmode_high_limit->onSave();
    dsb_eigenmode_low_limit->onSave();
    cb_use_built_in_materials->onSave();
    cb_use_mat_from_config_dir->onSave();
    cb_use_mat_from_custom_dir->onSave();
    fc_custom_mat_dir->onSave();
    cb_restore_result_dialog->onSave();
}

void DlgSettingsFemImp::loadSettings()
{
    fc_ccx_working_directory->onRestore();
    cb_int_editor->onRestore();
    fc_ext_editor->onRestore();
    fc_ccx_binary->onRestore();
    cb_analysis_type->onRestore();
    sb_eigenmode_number->onRestore();
    dsb_eigenmode_high_limit->onRestore();
    dsb_eigenmode_low_limit->onRestore();
    cb_use_built_in_materials->onRestore();
    cb_use_mat_from_config_dir->onRestore();
    cb_use_mat_from_custom_dir->onRestore();
    fc_custom_mat_dir->onRestore();
    cb_restore_result_dialog->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Fem");
    int index =  hGrp->GetInt("AnalysisType", 0);
    if (index > -1) cb_analysis_type->setCurrentIndex(index);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int c_index = cb_analysis_type->currentIndex();
        retranslateUi(this);
        cb_analysis_type->setCurrentIndex(c_index);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsFemImp.cpp"
