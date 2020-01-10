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


#include "PreCompiled.h"

#include "Gui/Application.h"
#include "DlgSettingsFemZ88Imp.h"
#include "ui_DlgSettingsFemZ88.h"
#include <Gui/PrefWidgets.h>

using namespace FemGui;

DlgSettingsFemZ88Imp::DlgSettingsFemZ88Imp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgSettingsFemZ88Imp)
{
    ui->setupUi(this);
}

DlgSettingsFemZ88Imp::~DlgSettingsFemZ88Imp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsFemZ88Imp::saveSettings()
{
    ui->cb_z88_binary_std->onSave();
    ui->fc_z88_binary_path->onSave();
}

void DlgSettingsFemZ88Imp::loadSettings()
{
    ui->cb_z88_binary_std->onRestore();
    ui->fc_z88_binary_path->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemZ88Imp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsFemZ88Imp.cpp"
