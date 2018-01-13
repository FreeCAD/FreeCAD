/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemGeneralImp.cpp                 *
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
#include "DlgSettingsFemCcxImp.h"
#include <Gui/PrefWidgets.h>

using namespace FemGui;

DlgSettingsFemCcxImp::DlgSettingsFemCcxImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
}

DlgSettingsFemCcxImp::~DlgSettingsFemCcxImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsFemCcxImp::saveSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Fem/Ccx");
    hGrp->SetInt("Solver", cmb_solver->currentIndex());
    hGrp->SetInt("AnalysisType", cb_analysis_type->currentIndex());

    sb_ccx_numcpu->onSave();         //Number of CPUs
    cmb_solver->onSave();
    cb_ccx_non_lin_geom->onSave();
    cb_use_iterations_param->onSave();

    cb_static->onSave();
    sb_ccx_max_iterations->onSave(); //Max number of iterations
    dsb_ccx_initial_time_step->onSave(); //Initial time step
    dsb_ccx_analysis_time->onSave(); //Analysis time

    cb_analysis_type->onSave();
    cb_BeamShellOutput->onSave();   //Beam shell output 3d or 2d 
    sb_eigenmode_number->onSave();
    dsb_eigenmode_high_limit->onSave();
    dsb_eigenmode_low_limit->onSave();

    cb_int_editor->onSave();
    fc_ext_editor->onSave();
    cb_ccx_binary_std->onSave();
    fc_ccx_binary_path->onSave();
    cb_split_inp_writer->onSave();
}

void DlgSettingsFemCcxImp::loadSettings()
{
    sb_ccx_numcpu->onRestore();         //Number of CPUs
    cmb_solver->onRestore();
    cb_ccx_non_lin_geom->onRestore();
    cb_use_iterations_param->onRestore();

    cb_static->onRestore();
    sb_ccx_max_iterations->onRestore(); //Max number of iterations
    dsb_ccx_initial_time_step->onRestore(); //Initial time step
    dsb_ccx_analysis_time->onRestore(); //Analysis time

    cb_analysis_type->onRestore();
    cb_BeamShellOutput->onRestore(); //Beam shell output 3d or 2d 
    sb_eigenmode_number->onRestore();
    dsb_eigenmode_high_limit->onRestore();
    dsb_eigenmode_low_limit->onRestore();

    cb_int_editor->onRestore();
    fc_ext_editor->onRestore();
    cb_ccx_binary_std->onRestore();
    fc_ccx_binary_path->onRestore();
    cb_split_inp_writer->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Fem/Ccx");
    int index =  hGrp->GetInt("Solver", 0);
    if (index > -1) cmb_solver->setCurrentIndex(index);
    index =  hGrp->GetInt("AnalysisType", 0);
    if (index > -1) cb_analysis_type->setCurrentIndex(index);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemCcxImp::changeEvent(QEvent *e)
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

#include "moc_DlgSettingsFemCcxImp.cpp"
