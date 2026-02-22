// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
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
#include <QThread>


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
    ui->dsb_ccx_time_period->setMaximum(std::numeric_limits<float>::max());
    ui->dsb_ccx_initial_time_increment->setMaximum(std::numeric_limits<float>::max());

    connect(
        ui->fc_ccx_binary_path,
        &Gui::PrefFileChooser::fileNameSelected,
        this,
        &DlgSettingsFemCcxImp::onfileNameSelected
    );
}

DlgSettingsFemCcxImp::~DlgSettingsFemCcxImp() = default;

void DlgSettingsFemCcxImp::saveSettings()
{
    ui->sb_ccx_numcpu->onSave();  // Number of CPUs
    ui->cmb_solver->onSave();
    ui->cb_ccx_non_lin_geom->onSave();
    ui->cb_use_iterations_param->onSave();

    ui->cb_static->onSave();
    ui->sb_ccx_max_increments->onSave();           // Max number of increments
    ui->dsb_ccx_initial_time_increment->onSave();  // Initial time increment
    ui->dsb_ccx_time_period->onSave();             // Step time period
    ui->dsb_ccx_minimum_time_increment->onSave();  // Minimum time increment
    ui->dsb_ccx_maximum_time_increment->onSave();  // Maximum time increment
    ui->ckb_pipeline_result->onSave();
    ui->ckb_result_format->onSave();

    ui->cb_analysis_type->onSave();
    ui->cb_BeamShellOutput->onSave();  // Beam shell output 3d or 2d
    ui->sb_eigenmode_number->onSave();
    ui->dsb_eigenmode_high_limit->onSave();
    ui->dsb_eigenmode_low_limit->onSave();

    ui->cb_int_editor->onSave();
    ui->fc_ext_editor->onSave();
    ui->fc_ccx_binary_path->onSave();
    ui->cb_split_inp_writer->onSave();
}

void DlgSettingsFemCcxImp::loadSettings()
{
    ui->cmb_solver->onRestore();
    ui->cb_ccx_non_lin_geom->onRestore();
    ui->cb_use_iterations_param->onRestore();

    ui->cb_static->onRestore();
    ui->sb_ccx_max_increments->onRestore();           // Max number of increments
    ui->dsb_ccx_initial_time_increment->onRestore();  // Initial time increment
    ui->dsb_ccx_time_period->onRestore();             // Step time period
    ui->dsb_ccx_minimum_time_increment->onRestore();  // Minimum time increment
    ui->dsb_ccx_maximum_time_increment->onRestore();  // Maximum time increment
    ui->ckb_pipeline_result->onRestore();
    ui->ckb_result_format->onRestore();

    ui->cb_analysis_type->onRestore();
    ui->cb_BeamShellOutput->onRestore();  // Beam shell output 3d or 2d
    ui->sb_eigenmode_number->onRestore();
    ui->dsb_eigenmode_high_limit->onRestore();
    ui->dsb_eigenmode_low_limit->onRestore();

    ui->cb_int_editor->onRestore();
    ui->fc_ext_editor->onRestore();
    ui->fc_ccx_binary_path->onRestore();
    ui->cb_split_inp_writer->onRestore();

    // determine number of CPU threads
    ui->sb_ccx_numcpu->setValue(QThread::idealThreadCount());
    ui->sb_ccx_numcpu->onRestore();  // Number of CPUs
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

void DlgSettingsFemCcxImp::onfileNameSelected(const QString& fileName)
{
    if (!fileName.isEmpty() && QStandardPaths::findExecutable(fileName).isEmpty()) {
        QMessageBox::critical(this, tr("CalculiX"), tr("Executable '%1' not found").arg(fileName));
    }
}

#include "moc_DlgSettingsFemCcxImp.cpp"
