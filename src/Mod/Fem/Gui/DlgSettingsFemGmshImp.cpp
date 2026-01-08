/***************************************************************************
 *   Copyright (c) 2016 FreeCAD Developers                                 *
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
#include <QThread>


#include <App/Application.h>
#include "DlgSettingsFemGmshImp.h"
#include "ui_DlgSettingsFemGmsh.h"


using namespace FemGui;

DlgSettingsFemGmshImp::DlgSettingsFemGmshImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemGmshImp)
{
    ui->setupUi(this);

    connect(
        ui->fc_gmsh_binary_path,
        &Gui::PrefFileChooser::fileNameSelected,
        this,
        &DlgSettingsFemGmshImp::onfileNameSelected
    );
}

DlgSettingsFemGmshImp::~DlgSettingsFemGmshImp() = default;

void DlgSettingsFemGmshImp::saveSettings()
{
    ui->fc_gmsh_binary_path->onSave();
    ui->cb_log_verbosity->onSave();
    ui->sb_threads->onSave();
}

void DlgSettingsFemGmshImp::loadSettings()
{
    ui->fc_gmsh_binary_path->onRestore();

    // determine number of CPU threads

    ParameterGrp::handle hGrp = ui->sb_threads->getWindowParameter();
    ui->sb_threads->setValue(hGrp->GetInt(ui->sb_threads->entryName(), QThread::idealThreadCount()));

    populateLogVerbosity();
    ui->cb_log_verbosity->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemGmshImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        populateLogVerbosity();
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsFemGmshImp::onfileNameSelected(const QString& fileName)
{
    if (!fileName.isEmpty() && QStandardPaths::findExecutable(fileName).isEmpty()) {
        QMessageBox::critical(this, tr("Gmsh"), tr("Executable '%1' not found").arg(fileName));
    }
}

void DlgSettingsFemGmshImp::populateLogVerbosity()
{
    std::list<std::pair<std::string, int>> mapValues = {
        {QT_TR_NOOP("Silent"), 0},
        {QT_TR_NOOP("Errors"), 1},
        {QT_TR_NOOP("Warnings"), 2},
        {QT_TR_NOOP("Direct"), 3},
        {QT_TR_NOOP("Information"), 4},
        {QT_TR_NOOP("Status"), 5},
        {QT_TR_NOOP("Debug"), 99}
    };

    ui->cb_log_verbosity->clear();
    for (const auto& val : mapValues) {
        ui->cb_log_verbosity->addItem(tr(val.first.c_str()), QByteArray::number(val.second));
    }

    // set default index
    auto hGrp = ui->cb_log_verbosity->getWindowParameter();
    std::string current = hGrp->GetASCII(ui->cb_log_verbosity->entryName(), "3");
    int index = ui->cb_log_verbosity->findData(QByteArray::fromStdString(current));
    ui->cb_log_verbosity->setCurrentIndex(index);
}

#include "moc_DlgSettingsFemGmshImp.cpp"
