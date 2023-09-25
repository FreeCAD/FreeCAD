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
#ifndef _PreComp_
#include <QMessageBox>
#include <QThread>
#endif

#include <App/Application.h>

#include "DlgSettingsFemCcxImp.h"
#include "ui_DlgSettingsFemCcx.h"


using namespace FemGui;

DlgSettingsFemCcxImp::DlgSettingsFemCcxImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemCcxImp)
{
    ui->setupUi(this);
    // set ranges
    ui->dsb_ccx_analysis_time->setMaximum(FLOAT_MAX);
    ui->dsb_ccx_initial_time_step->setMaximum(FLOAT_MAX);
    // determine number of CPU cores
    int processor_count = QThread::idealThreadCount();
    ui->sb_ccx_numcpu->setMaximum(processor_count);

    connect(ui->fc_ccx_binary_path,
            &Gui::PrefFileChooser::fileNameChanged,
            this,
            &DlgSettingsFemCcxImp::onfileNameChanged);
}

DlgSettingsFemCcxImp::~DlgSettingsFemCcxImp() = default;

void DlgSettingsFemCcxImp::saveSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Ccx");
    hGrp->SetInt("Solver", ui->cmb_solver->currentIndex());
    hGrp->SetInt("AnalysisType", ui->cb_analysis_type->currentIndex());

    ui->sb_ccx_numcpu->onSave();  // Number of CPUs
    ui->cmb_solver->onSave();
    ui->cb_ccx_non_lin_geom->onSave();
    ui->cb_use_iterations_param->onSave();

    ui->cb_static->onSave();
    ui->sb_ccx_max_iterations->onSave();      // Max number of iterations
    ui->dsb_ccx_initial_time_step->onSave();  // Initial time step
    ui->dsb_ccx_analysis_time->onSave();      // Analysis time

    ui->cb_analysis_type->onSave();
    ui->cb_BeamShellOutput->onSave();  // Beam shell output 3d or 2d
    ui->sb_eigenmode_number->onSave();
    ui->dsb_eigenmode_high_limit->onSave();
    ui->dsb_eigenmode_low_limit->onSave();

    ui->cb_int_editor->onSave();
    ui->fc_ext_editor->onSave();
    ui->cb_ccx_binary_std->onSave();
    ui->fc_ccx_binary_path->onSave();
    ui->cb_split_inp_writer->onSave();
}

void DlgSettingsFemCcxImp::loadSettings()
{
    ui->sb_ccx_numcpu->onRestore();  // Number of CPUs
    ui->cmb_solver->onRestore();
    ui->cb_ccx_non_lin_geom->onRestore();
    ui->cb_use_iterations_param->onRestore();

    ui->cb_static->onRestore();
    ui->sb_ccx_max_iterations->onRestore();      // Max number of iterations
    ui->dsb_ccx_initial_time_step->onRestore();  // Initial time step
    ui->dsb_ccx_analysis_time->onRestore();      // Analysis time

    ui->cb_analysis_type->onRestore();
    ui->cb_BeamShellOutput->onRestore();  // Beam shell output 3d or 2d
    ui->sb_eigenmode_number->onRestore();
    ui->dsb_eigenmode_high_limit->onRestore();
    ui->dsb_eigenmode_low_limit->onRestore();

    ui->cb_int_editor->onRestore();
    ui->fc_ext_editor->onRestore();
    ui->cb_ccx_binary_std->onRestore();
    ui->fc_ccx_binary_path->onRestore();
    ui->cb_split_inp_writer->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Ccx");
    int index = hGrp->GetInt("Solver", 0);
    if (index > -1) {
        ui->cmb_solver->setCurrentIndex(index);
    }
    index = hGrp->GetInt("AnalysisType", 0);
    if (index > -1) {
        ui->cb_analysis_type->setCurrentIndex(index);
    }
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemCcxImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        int c_index = ui->cb_analysis_type->currentIndex();
        ui->retranslateUi(this);
        ui->cb_analysis_type->setCurrentIndex(c_index);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsFemCcxImp::onfileNameChanged(QString FileName)
{
    if (!QFileInfo::exists(FileName)) {
        QMessageBox::critical(this,
                              tr("File does not exist"),
                              tr("The specified executable\n'%1'\n does not exist!\n"
                                 "Specify another file please.")
                                  .arg(FileName));
    }
}

#include "moc_DlgSettingsFemCcxImp.cpp"
