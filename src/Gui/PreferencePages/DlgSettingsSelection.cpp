/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/Application.h>

#include "DlgSettingsSelection.h"
#include "ui_DlgSettingsSelection.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsSelection */

DlgSettingsSelection::DlgSettingsSelection(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsSelection)
{
    ui->setupUi(this);
}

DlgSettingsSelection::~DlgSettingsSelection() = default;

void DlgSettingsSelection::saveSettings()
{
    ui->checkBoxPreselection->onSave();
    ui->checkBoxSelection->onSave();
    ui->HighlightColor->onSave();
    ui->SelectionColor->onSave();
    ui->spinPickRadius->onSave();
    ui->checkBoxAutoSwitch->onSave();
    ui->checkBoxAutoExpand->onSave();
    ui->checkBoxPreselect->onSave();
    ui->checkBoxRecord->onSave();
    ui->checkBoxSelectionCheckBoxes->onSave();
}

void DlgSettingsSelection::loadSettings()
{
    ui->checkBoxPreselection->onRestore();
    ui->checkBoxSelection->onRestore();
    ui->HighlightColor->onRestore();
    ui->SelectionColor->onRestore();
    ui->spinPickRadius->onRestore();
    ui->checkBoxAutoSwitch->onRestore();
    ui->checkBoxAutoExpand->onRestore();
    ui->checkBoxPreselect->onRestore();
    ui->checkBoxRecord->onRestore();
    ui->checkBoxSelectionCheckBoxes->onRestore();
}

void DlgSettingsSelection::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsSelection.cpp"
