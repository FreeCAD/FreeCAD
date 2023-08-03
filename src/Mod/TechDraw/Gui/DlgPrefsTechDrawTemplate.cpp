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

#include "DlgPrefsTechDrawTemplate.h"
#include "ui_DlgPrefsTechDrawTemplate.h"


using namespace TechDrawGui;

DlgPrefsTechDrawTemplate::DlgPrefsTechDrawTemplate(QWidget* parent) :
    PreferencePage(parent),
    ui(new Ui_DlgPrefsTechDrawTemplate)
{
    ui->setupUi(this);

    connect(ui->button_add, &QPushButton::released, this, &DlgPrefsTechDrawTemplate::addRow);
    connect(ui->button_remove, &QPushButton::released, this, &DlgPrefsTechDrawTemplate::removeRow);
}

DlgPrefsTechDrawTemplate::~DlgPrefsTechDrawTemplate()
{

}

void DlgPrefsTechDrawTemplate::addRow()
{
    int location = ui->pathsTable->rowCount();
    ui->pathsTable->insertRow(location);
}

void DlgPrefsTechDrawTemplate::removeRow()
{
    if(ui->pathsTable->selectedRanges().isEmpty()) {
        return;  // No selection
    }

    QTableWidgetSelectionRange selection = ui->pathsTable->selectedRanges().at(0);
    for (int row = selection.bottomRow(); row >= selection.topRow(); row--) {
        ui->pathsTable->removeRow(row);
    }
}

void DlgPrefsTechDrawTemplate::saveSettings()
{
    ui->pathsTable->onSave();
}

void DlgPrefsTechDrawTemplate::loadSettings()
{
    ui->pathsTable->onRestore();
}

void DlgPrefsTechDrawTemplate::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
}
