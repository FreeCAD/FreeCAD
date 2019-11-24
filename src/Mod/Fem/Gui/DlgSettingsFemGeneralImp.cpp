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
#include "DlgSettingsFemGeneralImp.h"
#include <Gui/PrefWidgets.h>

using namespace FemGui;

DlgSettingsFemGeneralImp::DlgSettingsFemGeneralImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
}

DlgSettingsFemGeneralImp::~DlgSettingsFemGeneralImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsFemGeneralImp::saveSettings()
{

    cb_analysis_group_meshing->onSave();

    cb_restore_result_dialog->onSave();
    cb_keep_results_on_rerun->onSave();
    cb_hide_constraint->onSave();

    cb_wd_temp->onSave();
    cb_wd_beside->onSave();
    cb_wd_custom->onSave();
    le_wd_custom->onSave();
    cb_overwrite_solver_working_directory->onSave();
}

void DlgSettingsFemGeneralImp::loadSettings()
{

    cb_analysis_group_meshing->onRestore();

    cb_restore_result_dialog->onRestore();
    cb_keep_results_on_rerun->onRestore();
    cb_hide_constraint->onRestore();

    cb_wd_temp->onRestore();
    cb_wd_beside->onRestore();
    cb_wd_custom->onRestore();
    le_wd_custom->onRestore();
    cb_overwrite_solver_working_directory->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemGeneralImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsFemGeneralImp.cpp"
