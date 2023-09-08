/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "DlgSettingsPythonConsole.h"
#include "ui_DlgSettingsPythonConsole.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsPythonConsole */

DlgSettingsPythonConsole::DlgSettingsPythonConsole(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgSettingsPythonConsole)
{
    ui->setupUi(this);
}

DlgSettingsPythonConsole::~DlgSettingsPythonConsole() = default;

void DlgSettingsPythonConsole::saveSettings()
{
    ui->PythonWordWrap->onSave();
    ui->PythonBlockCursor->onSave();
    ui->PythonSaveHistory->onSave();
    ui->ProfilerInterval->onSave();
}

void DlgSettingsPythonConsole::loadSettings()
{
    ui->PythonWordWrap->onRestore();
    ui->PythonBlockCursor->onRestore();
    ui->PythonSaveHistory->onRestore();
    ui->ProfilerInterval->onRestore();
}

void DlgSettingsPythonConsole::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(event);
}

#include "moc_DlgSettingsPythonConsole.cpp"
