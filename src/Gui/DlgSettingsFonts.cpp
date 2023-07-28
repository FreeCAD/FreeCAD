/***************************************************************************
 *   Copyright (c) 2022 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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
# include <QDialog>
#endif

#include <App/FontPaths.h>

#include "DlgSettingsFonts.h"
#include "ui_DlgSettingsFonts.h"
#include "ui_DlgSettingsFontsSystem.h"


using namespace Gui::Dialog;

DlgSettingsFonts::DlgSettingsFonts(QWidget* parent) :
    PreferencePage(parent),
    ui(new Ui_DlgSettingsFonts)
{
    ui->setupUi(this);

    connect(ui->button_add, &QPushButton::released, this, &DlgSettingsFonts::addRow);
    connect(ui->button_remove, &QPushButton::released, this, &DlgSettingsFonts::removeRow);
    connect(ui->button_systemPaths, &QPushButton::released, this, &DlgSettingsFonts::showSystemPaths);
}

DlgSettingsFonts::~DlgSettingsFonts()
{

}

void DlgSettingsFonts::addRow()
{
    int location = ui->pathsTable->rowCount();
    ui->pathsTable->insertRow(location);
}

void DlgSettingsFonts::removeRow()
{
    if(ui->pathsTable->selectedRanges().isEmpty()) {
        return;  // No selection
    }

    QTableWidgetSelectionRange selection = ui->pathsTable->selectedRanges().at(0);
    for (int row = selection.bottomRow(); row >= selection.topRow(); row--) {
        ui->pathsTable->removeRow(row);
    }
}

void DlgSettingsFonts::saveSettings()
{
    ui->pathsTable->onSave();
}

void DlgSettingsFonts::loadSettings()
{
    ui->pathsTable->onRestore();
}

void DlgSettingsFonts::showSystemPaths()
{
    QDialog* systemPathsDialog = new QDialog(this);
    Ui_DlgSettingsFontsSystem widget;
    widget.setupUi(systemPathsDialog);

    // Load the system paths into the table
    int row = 0;
    QStringList systemPaths = App::FontPaths::getSystemPaths();
    for(const QString& systemPath : systemPaths) {
        QTableWidgetItem* cell = new QTableWidgetItem(systemPath);
        cell->setFlags(Qt::ItemIsEnabled);
        cell->setToolTip(QString::fromUtf8("System paths cannot be edited"));  // Needs translation
        widget.pathsTable->insertRow(row);
        widget.pathsTable->setItem(row, 0, cell);
        ++row;
    }

    systemPathsDialog->setAttribute(Qt::WA_DeleteOnClose);
    systemPathsDialog->show();
}

void DlgSettingsFonts::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
}
