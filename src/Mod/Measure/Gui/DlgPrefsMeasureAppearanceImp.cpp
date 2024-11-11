/**************************************************************************
 *   Copyright (c) 2023 Wanderer Fan <wandererfan@gmail.com>               *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include "DlgPrefsMeasureAppearanceImp.h"
#include "ui_DlgPrefsMeasureAppearanceImp.h"

using namespace MeasureGui;

DlgPrefsMeasureAppearanceImp::DlgPrefsMeasureAppearanceImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgPrefsMeasureAppearanceImp)
{
    ui->setupUi(this);
}

DlgPrefsMeasureAppearanceImp::~DlgPrefsMeasureAppearanceImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsMeasureAppearanceImp::saveSettings()
{
    ui->sbFontSize->onSave();
    ui->cbText->onSave();
    ui->cbLine->onSave();
    ui->cbBackground->onSave();
}

void DlgPrefsMeasureAppearanceImp::loadSettings()
{
    ui->sbFontSize->onRestore();
    ui->cbText->onRestore();
    ui->cbBackground->onRestore();
    ui->cbLine->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsMeasureAppearanceImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include <Mod/Measure/Gui/moc_DlgPrefsMeasureAppearanceImp.cpp>
