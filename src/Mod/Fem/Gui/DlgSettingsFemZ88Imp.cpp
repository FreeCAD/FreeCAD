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

#include <QMessageBox>
#include <QStandardPaths>

#include <Gui/Application.h>

#include "DlgSettingsFemZ88Imp.h"
#include "ui_DlgSettingsFemZ88.h"


using namespace FemGui;

DlgSettingsFemZ88Imp::DlgSettingsFemZ88Imp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemZ88Imp)
{
    ui->setupUi(this);

    connect(
        ui->fc_z88_binary_path,
        &Gui::PrefFileChooser::fileNameSelected,
        this,
        &DlgSettingsFemZ88Imp::onfileNameSelected
    );
}

DlgSettingsFemZ88Imp::~DlgSettingsFemZ88Imp() = default;

void DlgSettingsFemZ88Imp::saveSettings()
{
    ui->fc_z88_binary_path->onSave();
    ui->cmb_solver->onSave();
    ui->sb_Z88_MaxGS->onSave();
    ui->sb_Z88_MaxKOI->onSave();
}

void DlgSettingsFemZ88Imp::loadSettings()
{
    ui->fc_z88_binary_path->onRestore();
    ui->sb_Z88_MaxGS->onRestore();
    ui->sb_Z88_MaxKOI->onRestore();

    populateSolverType();
    ui->cmb_solver->onRestore();
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

void DlgSettingsFemZ88Imp::populateSolverType()
{
    std::list<std::pair<std::string, std::string>> mapValues = {
        {QT_TR_NOOP("Succesive over-relaxation (SOR)"), "sorcg"},
        {QT_TR_NOOP("Shifted incomplete Cholesky (SIC)"), "siccg"},
        {QT_TR_NOOP("Simple Cholesky"), "choly"},
    };

    ui->cmb_solver->clear();
    for (const auto& val : mapValues) {
        ui->cmb_solver->addItem(tr(val.first.c_str()), QByteArray::fromStdString(val.second));
    }

    // set default index
    auto hGrp = ui->cmb_solver->getWindowParameter();
    std::string current = hGrp->GetASCII(ui->cmb_solver->entryName(), "sorcg");
    int index = ui->cmb_solver->findData(QByteArray::fromStdString(current));
    ui->cmb_solver->setCurrentIndex(index);
}

void DlgSettingsFemZ88Imp::onfileNameSelected(const QString& fileName)
{
    if (!fileName.isEmpty() && QStandardPaths::findExecutable(fileName).isEmpty()) {
        QMessageBox::critical(this, tr("Z88"), tr("Executable '%1' not found").arg(fileName));
    }
}

#include "moc_DlgSettingsFemZ88Imp.cpp"
